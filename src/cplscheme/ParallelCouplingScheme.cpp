#include "ParallelCouplingScheme.hpp"
#include "acceleration/Acceleration.hpp"
#include "m2n/M2N.hpp"
#include "math/math.hpp"
#include "utils/EigenHelperFunctions.hpp"
#include "utils/MasterSlave.hpp"

namespace precice {
namespace cplscheme {

ParallelCouplingScheme::ParallelCouplingScheme(
    double                        maxTime,
    int                           maxTimeWindows,
    double                        timeWindowsSize,
    int                           validDigits,
    const std::string &           firstParticipant,
    const std::string &           secondParticipant,
    const std::string &           localParticipant,
    m2n::PtrM2N                   m2n,
    constants::TimesteppingMethod dtMethod,
    CouplingMode                  cplMode,
    int                           maxIterations)
    : BaseCouplingScheme(maxTime, maxTimeWindows, timeWindowsSize, validDigits, firstParticipant,
                         secondParticipant, localParticipant, m2n, maxIterations, dtMethod)
{
  _couplingMode = cplMode;
  // Coupling mode must be either Explicit or Implicit when using SerialCouplingScheme.
  PRECICE_ASSERT(_couplingMode != Undefined);
  if (_couplingMode == Explicit) {
    PRECICE_ASSERT(maxIterations == 1);
  }
}

void ParallelCouplingScheme::initialize(
    double startTime,
    int    startTimeWindow)
{
  PRECICE_TRACE(startTime, startTimeWindow);
  PRECICE_ASSERT(not isInitialized());
  PRECICE_ASSERT(math::greaterEquals(startTime, 0.0), startTime);
  PRECICE_ASSERT(startTimeWindow >= 0, startTimeWindow);
  setTime(startTime);
  setTimeWindows(startTimeWindow);
  if (_couplingMode == Implicit) {
    PRECICE_CHECK(not getSendData().empty(), "No send data configured! Use explicit scheme for one-way coupling.");
    if (not doesFirstStep()) {         // second participant
      setupConvergenceMeasures();      // needs _couplingData configured
      mergeData();                     // merge send and receive data for all pp calls
      setupDataMatrices(getAllData()); // Reserve memory and initialize data with zero
      if (getAcceleration().get() != nullptr) {
        getAcceleration()->initialize(getAllData()); // Reserve memory, initialize
      }
    }

    requireAction(constants::actionWriteIterationCheckpoint());
    initializeTXTWriters();
  }

  for (DataMap::value_type &pair : getSendData()) {
    if (pair.second->initialize) {
      setHasToSendInitData(true);
      break;
    }
  }
  for (DataMap::value_type &pair : getReceiveData()) {
    if (pair.second->initialize) {
      setHasToReceiveInitData(true);
      break;
    }
  }

  if (hasToSendInitData()) {
    requireAction(constants::actionWriteInitialData());
  }

  setIsInitialized(true);
}

void ParallelCouplingScheme::initializeData()
{
  PRECICE_TRACE("initializeData()");
  PRECICE_CHECK(isInitialized(), "initializeData() can be called after initialize() only!");

  if (not hasToSendInitData() && not hasToReceiveInitData()) {
    PRECICE_INFO("initializeData is skipped since no data has to be initialized");
    return;
  }

  PRECICE_CHECK(not(hasToSendInitData() && isActionRequired(constants::actionWriteInitialData())),
                "InitialData has to be written to preCICE before calling initializeData()");

  setHasDataBeenExchanged(false);

  // F: send, receive, S: receive, send
  if (doesFirstStep()) {
    if (hasToSendInitData()) {
      sendData(getM2N());
    }
    if (hasToReceiveInitData()) {
      receiveData(getM2N());
      setHasDataBeenExchanged(true);
    }
  }

  else { // second participant
    if (hasToReceiveInitData()) {
      receiveData(getM2N());
      setHasDataBeenExchanged(true);

      // second participant has to save values for extrapolation
      if (_couplingMode == Implicit) {
        for (DataMap::value_type &pair : getReceiveData()) {
          if (pair.second->oldValues.cols() == 0)
            break;
          pair.second->oldValues.col(0) = *pair.second->values;
          // For extrapolation, treat the initial value as old time windows value
          utils::shiftSetFirst(pair.second->oldValues, *pair.second->values);
        }
      }
    }
    if (hasToSendInitData()) {
      if (_couplingMode == Implicit) {
        for (DataMap::value_type &pair : getSendData()) {
          if (pair.second->oldValues.cols() == 0)
            break;
          pair.second->oldValues.col(0) = *pair.second->values;
          // For extrapolation, treat the initial value as old time windows value
          utils::shiftSetFirst(pair.second->oldValues, *pair.second->values);
        }
      }
      sendData(getM2N());
    }
  }

  // in order to check in advance if initializeData has been called (if necessary)
  setHasToSendInitData(false);
  setHasToReceiveInitData(false);
}

void ParallelCouplingScheme::advance()
{
  PRECICE_TRACE(getTimeWindows(), getTime());
  checkCompletenessRequiredActions();

  PRECICE_CHECK(!hasToReceiveInitData() && !hasToSendInitData(),
                "initializeData() needs to be called before advance if data has to be initialized!");

  setHasDataBeenExchanged(false);
  setIsTimeWindowComplete(false);

  if (math::equals(getThisTimeWindowRemainder(), 0.0, _eps)) {
    if (_couplingMode == Explicit) {
      explicitAdvance();
    } else if (_couplingMode == Implicit) {
      implicitAdvance();
    }
  } // subcycling complete
}

void ParallelCouplingScheme::explicitAdvance()
{
  setIsTimeWindowComplete(true);
  setTimeWindows(getTimeWindows() + 1);

  if (doesFirstStep()) {
    PRECICE_DEBUG("Sending data...");
    sendDt();
    sendData(getM2N());

    PRECICE_DEBUG("Receiving data...");
    receiveAndSetDt();
    receiveData(getM2N());
    setHasDataBeenExchanged(true);
  } else { //second participant
    PRECICE_DEBUG("Receiving data...");
    receiveAndSetDt();
    receiveData(getM2N());
    setHasDataBeenExchanged(true);

    PRECICE_DEBUG("Sending data...");
    sendDt();
    sendData(getM2N());
  }

  //both participants
  setComputedTimeWindowPart(0.0);
}

void ParallelCouplingScheme::implicitAdvance()
{
  PRECICE_DEBUG("Computed full length of iteration");
  bool convergence = false;
  if (doesFirstStep()) { //First participant
    sendData(getM2N());
    getM2N()->receive(convergence);
    if (convergence) {
      timeWindowCompleted();
    }
    receiveData(getM2N());
  } else { // second participant
    receiveData(getM2N());

    PRECICE_DEBUG("measure convergence.");
    convergence = measureConvergence();
    // Stop, when maximal iteration count (given in config) is reached
    if (maxIterationsReached())
      convergence = true;

    if (convergence) {
      if (getAcceleration().get() != nullptr) {
        _deletedColumnsPPFiltering = getAcceleration()->getDeletedColumns();
        getAcceleration()->iterationsConverged(getAllData());
      }
      newConvergenceMeasurements();
      timeWindowCompleted();
    } else if (getAcceleration().get() != nullptr) {
      getAcceleration()->performAcceleration(getAllData());
    }

    // extrapolate new input data for the solver evaluation in time.
    if (convergence && (getExtrapolationOrder() > 0)) {
      extrapolateData(getAllData()); // Also stores data
    } else {                         // Store data for conv. measurement, acceleration, or extrapolation
      for (DataMap::value_type &pair : getSendData()) {
        if (pair.second->oldValues.size() > 0) {
          pair.second->oldValues.col(0) = *pair.second->values;
        }
      }
      for (DataMap::value_type &pair : getReceiveData()) {
        if (pair.second->oldValues.size() > 0) {
          pair.second->oldValues.col(0) = *pair.second->values;
        }
      }
    }

    getM2N()->send(convergence);

    sendData(getM2N());
  }

  // both participants
  if (not convergence) {
    PRECICE_DEBUG("No convergence achieved");
    requireAction(constants::actionReadIterationCheckpoint());
  } else {
    PRECICE_DEBUG("Convergence achieved");
    advanceTXTWriters();
  }
  updateTimeAndIterations(convergence);
  setHasDataBeenExchanged(true);
  setComputedTimeWindowPart(0.0);
}

void ParallelCouplingScheme::mergeData()
{
  PRECICE_TRACE();
  PRECICE_ASSERT(!doesFirstStep(), "Only the second participant should do the acceleration.");
  PRECICE_ASSERT(_allData.empty(), "This function should only be called once.");
  _allData.insert(getSendData().begin(), getSendData().end());
  _allData.insert(getReceiveData().begin(), getReceiveData().end());
}

} // namespace cplscheme
} // namespace precice
