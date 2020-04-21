#include "BaseCouplingScheme.hpp"
#include <Eigen/Core>
#include <limits>
#include <sstream>
#include "acceleration/Acceleration.hpp"
#include "com/Communication.hpp"
#include "com/SharedPointer.hpp"
#include "impl/ConvergenceMeasure.hpp"
#include "io/TXTReader.hpp"
#include "io/TXTWriter.hpp"
#include "m2n/M2N.hpp"
#include "m2n/SharedPointer.hpp"
#include "math/math.hpp"
#include "mesh/Mesh.hpp"
#include "utils/EigenHelperFunctions.hpp"
#include "utils/Helpers.hpp"
#include "utils/MasterSlave.hpp"

namespace precice {
namespace cplscheme {

BaseCouplingScheme::BaseCouplingScheme(
    double                        maxTime,
    int                           maxTimeWindows,
    double                        timeWindowSize,
    int                           validDigits,
    const std::string &           firstParticipant,
    const std::string &           secondParticipant,
    const std::string &           localParticipant,
    m2n::PtrM2N                   m2n,
    int                           maxIterations,
    CouplingMode                  cplMode,
    constants::TimesteppingMethod dtMethod)
    : _m2n(m2n),
      _couplingMode(cplMode),
      _maxTime(maxTime),
      _maxTimeWindows(maxTimeWindows),
      _timeWindows(1),
      _timeWindowSize(timeWindowSize),
      _maxIterations(maxIterations),
      _iterations(1),
      _totalIterations(1),
      _validDigits(validDigits),
      _firstParticipant(firstParticipant),
      _secondParticipant(secondParticipant),
      _localParticipant(localParticipant),
      _eps(std::pow(10.0, -1 * validDigits))
{
  PRECICE_ASSERT(not((maxTime != UNDEFINED_TIME) && (maxTime < 0.0)),
                "Maximum time has to be larger than zero.");
  PRECICE_ASSERT(not((maxTimeWindows != UNDEFINED_TIME_WINDOWS) && (maxTimeWindows < 0)),
                "Maximum number of time windows has to be larger than zero.");
  PRECICE_ASSERT(not((timeWindowSize != UNDEFINED_TIME_WINDOW_SIZE) && (timeWindowSize < 0.0)),
                "Time window size has to be larger than zero.");
  PRECICE_ASSERT((_validDigits >= 1) && (_validDigits < 17),
                "Valid digits of time window size has to be between 1 and 16.");
  PRECICE_ASSERT(_firstParticipant != _secondParticipant,
                "First participant and second participant must have different names.");
  if (dtMethod == constants::FIXED_TIME_WINDOW_SIZE) {
    PRECICE_ASSERT(hasTimeWindowSize(),
                  "Time window size has to be given when the fixed time window size method is used."); 
  }
  if (localParticipant == _firstParticipant) {
    _doesFirstStep = true;
  } else if (localParticipant == _secondParticipant) {
    _doesFirstStep = false;
  } else {
    PRECICE_ERROR("Name of local participant \""
                  << localParticipant << "\" does not match any "
                  << "participant specified for the coupling scheme.");
  }

  PRECICE_ASSERT((maxIterations > 0) || (maxIterations == -1),
                 "Maximal iteration limit has to be larger than zero.");

  if (isExplicitCouplingScheme()) {
    PRECICE_ASSERT(maxIterations == -1);
  } else {
    PRECICE_ASSERT(maxIterations >= 1);
  }
}

void BaseCouplingScheme::setTimeWindowSize(double timeWindowSize)
{
  _timeWindowSize = timeWindowSize;
}

void BaseCouplingScheme::addDataToSend(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          requiresInitialization)
{
  PRECICE_TRACE();
  int id = data->getID();
  if (!utils::contained(id, _sendData)) {
    PtrCouplingData     ptrCplData(new CouplingData(&(data->values()), mesh, requiresInitialization, data->getDimensions()));
    DataMap::value_type pair = std::make_pair(id, ptrCplData);
    _sendData.insert(pair);
  } else {
    PRECICE_ERROR("Data \"" << data->getName() << "\" cannot be added twice for sending.");
  }
}

void BaseCouplingScheme::addDataToReceive(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          requiresInitialization)
{
  PRECICE_TRACE();
  int id = data->getID();
  if (!utils::contained(id, _receiveData)) {
    PtrCouplingData     ptrCplData(new CouplingData(&(data->values()), mesh, requiresInitialization, data->getDimensions()));
    DataMap::value_type pair = std::make_pair(id, ptrCplData);
    _receiveData.insert(pair);
  } else {
    PRECICE_ERROR("Data \"" << data->getName() << "\" cannot be added twice for receiving.");
  }
}

std::vector<int> BaseCouplingScheme::sendData()
{
  PRECICE_TRACE();
  std::vector<int> sentDataIDs;
  PRECICE_ASSERT(_m2n.get() != nullptr);
  PRECICE_ASSERT(_m2n->isConnected());
  for (const DataMap::value_type &pair : _sendData) {
    int size = pair.second->values->size();
    _m2n->send(pair.second->values->data(), size, pair.second->mesh->getID(), pair.second->dimension);
    sentDataIDs.push_back(pair.first);
  }
  PRECICE_DEBUG("Number of sent data sets = " << sentDataIDs.size());
  return sentDataIDs;
}

std::vector<int> BaseCouplingScheme::receiveData()
{
  PRECICE_TRACE();
  std::vector<int> receivedDataIDs;
  PRECICE_ASSERT(_m2n.get() != nullptr);
  PRECICE_ASSERT(_m2n->isConnected());
  for (DataMap::value_type &pair : _receiveData) {
    int size = pair.second->values->size();
    _m2n->receive(pair.second->values->data(), size, pair.second->mesh->getID(), pair.second->dimension);
    receivedDataIDs.push_back(pair.first);
  }
  PRECICE_DEBUG("Number of received data sets = " << receivedDataIDs.size());
  _hasDataBeenExchanged = true;
  return receivedDataIDs;
}

CouplingData *BaseCouplingScheme::getSendData(
    int dataID)
{
  PRECICE_TRACE(dataID);
  DataMap::iterator iter = _sendData.find(dataID);
  if (iter != _sendData.end()) {
    return &(*(iter->second));
  }
  return nullptr;
}

CouplingData *BaseCouplingScheme::getReceiveData(
    int dataID)
{
  PRECICE_TRACE(dataID);
  DataMap::iterator iter = _receiveData.find(dataID);
  if (iter != _receiveData.end()) {
    return &(*(iter->second));
  }
  return nullptr;
}

void BaseCouplingScheme::finalize()
{
  PRECICE_TRACE();
  checkCompletenessRequiredActions();
  PRECICE_ASSERT(_isInitialized, "Called finalize() before initialize().");
}

void BaseCouplingScheme::initialize(double startTime, int startTimeWindow)
{
  // Initialize uses the template method pattern (https://en.wikipedia.org/wiki/Template_method_pattern).
  PRECICE_TRACE(startTime, startTimeWindow);
  PRECICE_ASSERT(not isInitialized());
  PRECICE_ASSERT(math::greaterEquals(startTime, 0.0), startTime);
  PRECICE_ASSERT(startTimeWindow >= 0, startTimeWindow);
  _time          = startTime;
  _timeWindows   = startTimeWindow;

  if (isImplicitCouplingScheme()) {
    if (not doesFirstStep()) {
      PRECICE_ASSERT(not _convergenceMeasures.empty(), "Implicit scheme must have at least one convergence measure.");
      mergeData();                             // merge send and receive data for all pp calls
      setupConvergenceMeasures();              // needs _couplingData configured
      setupDataMatrices(getAccelerationData()); // Reserve memory and initialize data with zero
      if (getAcceleration()) {
        getAcceleration()->initialize(getAccelerationData()); // Reserve memory, initialize
      }
    }
    requireAction(constants::actionWriteIterationCheckpoint());
    initializeTXTWriters();
  }

  initializeImplementation();

  if (sendsInitializedData()) {
    requireAction(constants::actionWriteInitialData());
  }

  _isInitialized = true;
}

void BaseCouplingScheme::initializeData()
{
  // InitializeData uses the template method pattern (https://en.wikipedia.org/wiki/Template_method_pattern).
  PRECICE_CHECK(_isInitialized, "initializeData() can be called after initialize() only.");
  PRECICE_CHECK(not _initializeDataHasBeenCalled, "initializeData() can only be called once.");
  _initializeDataHasBeenCalled = true;
  PRECICE_TRACE("initializeData()");

  if (not _sendsInitializedData && not _receivesInitializedData) {
    PRECICE_INFO("initializeData is skipped since no data has to be initialized.");
    return;
  }

  PRECICE_DEBUG("Initializing Data ...");

  PRECICE_CHECK(not(_sendsInitializedData && isActionRequired(constants::actionWriteInitialData())),
                "InitialData has to be written to preCICE before calling initializeData().");

  _hasDataBeenExchanged = false;

  exchangeInitialData();
}

void BaseCouplingScheme::advance()
{
  PRECICE_TRACE(_timeWindows, _time);
  checkCompletenessRequiredActions();
  PRECICE_ASSERT(_isInitialized, "Before calling advance() coupling scheme has to be initialized via initialize().");
  PRECICE_CHECK((not _receivesInitializedData && not _sendsInitializedData) || (_initializeDataHasBeenCalled),
                "initializeData() needs to be called before advance if data has to be initialized.");
  _hasDataBeenExchanged = false;
  _isTimeWindowComplete = false;

#ifndef NDEBUG
  for (const DataMap::value_type &pair : getReceiveData()) {
    Eigen::VectorXd &  values = *pair.second->values;
    int                max    = values.size();
    std::ostringstream stream;
    for (int i = 0; (i < max) && (i < 10); i++) {
      stream << values[i] << " ";
    }
    PRECICE_DEBUG("Begin advance, first New Values: " << stream.str());
  }
#endif
  PRECICE_ASSERT(_couplingMode != Undefined);

  if (reachedEndOfTimeWindow()) {

    _timeWindows += 1;  // increment window counter. If not converged, will be decremented again later.

    bool convergence = exchangeDataAndAccelerate();

    if(isImplicitCouplingScheme()) {  // check convergence
      if (not convergence) {  // repeat window
        PRECICE_DEBUG("No convergence achieved");
        requireAction(constants::actionReadIterationCheckpoint());
        // The computed time window part equals the time window size, since the
        // time window remainder is zero. Subtract the time window size and do another
        // coupling iteration.
        PRECICE_ASSERT(math::greater(_computedTimeWindowPart, 0.0));
        _time = _time - _computedTimeWindowPart;
        _timeWindows -= 1;
      } else {  // write output, prepare for next window
        PRECICE_DEBUG("Convergence achieved");
        advanceTXTWriters();
        PRECICE_INFO("Time window completed");
        _isTimeWindowComplete = true;
        if (isCouplingOngoing()) {
          PRECICE_DEBUG("Setting require create checkpoint");
          requireAction(constants::actionWriteIterationCheckpoint());
        }
      }
      //update iterations
      _totalIterations++;
      if (not convergence) {
        _iterations++;
      } else {
        _iterations = 1;
      }
      _hasDataBeenExchanged = true;
    } else {
      PRECICE_INFO("Time window completed");
      _isTimeWindowComplete = true;
    }
    _computedTimeWindowPart = 0.0; // reset window
  }
}

void BaseCouplingScheme::setExtrapolationOrder(
    int order)
{
  PRECICE_CHECK((order == 0) || (order == 1) || (order == 2),
                "Extrapolation order has to be  0, 1, or 2.");
  _extrapolationOrder = order;
}

void BaseCouplingScheme::updateOldValues(DataMap &dataMap)
{
  if (isImplicitCouplingScheme() && _extrapolationOrder > 0) {
    for (DataMap::value_type &pair : dataMap) {
      if (pair.second->oldValues.cols() == 0)
        PRECICE_ASSERT(isExplicitCouplingScheme());
      break;
      pair.second->oldValues.col(0) = *pair.second->values;
      // For extrapolation, treat the initial value as old time windows value
      utils::shiftSetFirst(pair.second->oldValues, *pair.second->values);
    }
  }
}


// @todo extrapolation of data should only be done for the fine cplData -> then copied to the coarse cplData
void BaseCouplingScheme::extrapolateData(DataMap &data)
{
  if (_extrapolationOrder > 0) {
    PRECICE_TRACE(_timeWindows);
    if ((_extrapolationOrder == 1) || getTimeWindows() == 2) { //timesteps is increased before extrapolate is called
      PRECICE_INFO("Performing first order extrapolation");
      for (DataMap::value_type &pair : data) {
        PRECICE_DEBUG("Extrapolate data: " << pair.first);
        PRECICE_ASSERT(pair.second->oldValues.cols() > 1);
        Eigen::VectorXd &values       = *pair.second->values;
        pair.second->oldValues.col(0) = values;  // = x^t
        values *= 2.0;                           // = 2*x^t
        values -= pair.second->oldValues.col(1); // = 2*x^t - x^(t-1)
        utils::shiftSetFirst(pair.second->oldValues, values);
      }
    } else if (_extrapolationOrder == 2) {
      PRECICE_INFO("Performing second order extrapolation");
      for (DataMap::value_type &pair : data) {
        PRECICE_ASSERT(pair.second->oldValues.cols() > 2);
        Eigen::VectorXd &values     = *pair.second->values;
        auto             valuesOld1 = pair.second->oldValues.col(1);
        auto             valuesOld2 = pair.second->oldValues.col(2);

        pair.second->oldValues.col(0) = values; // = x^t
        values *= 2.5;                          // = 2.5 x^t
        values -= valuesOld1 * 2.0;             // = 2.5x^t - 2x^(t-1)
        values += valuesOld2 * 0.5;             // = 2.5x^t - 2x^(t-1) + 0.5x^(t-2)
        utils::shiftSetFirst(pair.second->oldValues, values);
      }
    } else {
      PRECICE_ERROR("Called extrapolation with order != 1,2.");
    }
  }
}

bool BaseCouplingScheme::hasTimeWindowSize() const
{
  return not math::equals(_timeWindowSize, UNDEFINED_TIME_WINDOW_SIZE);
}

double BaseCouplingScheme::getTimeWindowSize() const
{
  PRECICE_ASSERT(not math::equals(_timeWindowSize, UNDEFINED_TIME_WINDOW_SIZE));
  return _timeWindowSize;
}

void BaseCouplingScheme::addComputedTime(
    double timeToAdd)
{
  PRECICE_TRACE(timeToAdd, _time);
  PRECICE_ASSERT(isCouplingOngoing(), "Invalid call of addComputedTime() after simulation end.");

  // add time interval that has been computed in the solver to get the correct time remainder
  _computedTimeWindowPart += timeToAdd;
  _time += timeToAdd;

  // Check validness
  bool valid = math::greaterEquals(getThisTimeWindowRemainder(), 0.0, _eps);
  PRECICE_CHECK(valid, "The computed timestep length of "
                           << timeToAdd << " exceeds the maximum timestep limit of "
                           << _timeWindowSize - _computedTimeWindowPart + timeToAdd
                           << " for this time window.");
}

bool BaseCouplingScheme::willDataBeExchanged(
    double lastSolverTimestepLength) const
{
  PRECICE_TRACE(lastSolverTimestepLength);
  double remainder = getThisTimeWindowRemainder() - lastSolverTimestepLength;
  return not math::greater(remainder, 0.0, _eps);
}

bool BaseCouplingScheme::hasDataBeenExchanged() const
{
  return _hasDataBeenExchanged;
}

void BaseCouplingScheme::setHasDataBeenExchanged(
    bool hasDataBeenExchanged)
{
  _hasDataBeenExchanged = hasDataBeenExchanged;
}

double BaseCouplingScheme::getTime() const
{
  return _time;
}

int BaseCouplingScheme::getTimeWindows() const
{
  return _timeWindows;
}

std::vector<std::string> BaseCouplingScheme::getCouplingPartners() const
{
  std::vector<std::string> partnerNames;
  // Add non-local participant
  if (doesFirstStep()) {
    partnerNames.push_back(_secondParticipant);
  } else {
    partnerNames.push_back(_firstParticipant);
  }
  return partnerNames;
}

double BaseCouplingScheme::getThisTimeWindowRemainder() const
{
  PRECICE_TRACE();
  double remainder = 0.0;
  if (not math::equals(_timeWindowSize, UNDEFINED_TIME_WINDOW_SIZE)) {
    remainder = _timeWindowSize - _computedTimeWindowPart;
  }
  PRECICE_DEBUG("return " << remainder);
  return remainder;
}

double BaseCouplingScheme::getNextTimestepMaxLength() const
{
  if (math::equals(_timeWindowSize, UNDEFINED_TIME_WINDOW_SIZE)) {
    if (math::equals(_maxTime, UNDEFINED_TIME)) {
      return std::numeric_limits<double>::max();
    } else {
      return _maxTime - _time;
    }
  }
  return _timeWindowSize - _computedTimeWindowPart;
}

bool BaseCouplingScheme::isCouplingOngoing() const
{
  bool timeLeft      = math::greater(_maxTime, _time, _eps) || math::equals(_maxTime, UNDEFINED_TIME);
  bool timestepsLeft = (_maxTimeWindows >= _timeWindows) || (_maxTimeWindows == UNDEFINED_TIME_WINDOWS);
  return timeLeft && timestepsLeft;
}

bool BaseCouplingScheme::isTimeWindowComplete() const
{
  return _isTimeWindowComplete;
}

bool BaseCouplingScheme::isActionRequired(
    const std::string &actionName) const
{
  return _actions.count(actionName) > 0;
}

void BaseCouplingScheme::markActionFulfilled(
    const std::string &actionName)
{
  _actions.erase(actionName);
}

void BaseCouplingScheme::requireAction(
    const std::string &actionName)
{
  _actions.insert(actionName);
}

std::string BaseCouplingScheme::printCouplingState() const
{
  std::ostringstream os;
  os << "it " << _iterations; //_iterations;
  if (_maxIterations != -1) {
    os << " of " << _maxIterations;
  }
  os << " | " << printBasicState(_timeWindows, _time) << " | " << printActionsState();
  return os.str();
}

std::string BaseCouplingScheme::printBasicState(
    int    timeWindows,
    double time) const
{
  std::ostringstream os;
  os << "dt# " << timeWindows;
  if (_maxTimeWindows != UNDEFINED_TIME_WINDOWS) {
    os << " of " << _maxTimeWindows;
  }
  os << " | t " << time;
  if (_maxTime != UNDEFINED_TIME) {
    os << " of " << _maxTime;
  }
  if (_timeWindowSize != UNDEFINED_TIME_WINDOW_SIZE) {
    os << " | dt " << _timeWindowSize;
  }
  if ((_timeWindowSize != UNDEFINED_TIME_WINDOW_SIZE) || (_maxTime != UNDEFINED_TIME)) {
    os << " | max dt " << getNextTimestepMaxLength();
  }
  os << " | ongoing ";
  isCouplingOngoing() ? os << "yes" : os << "no";
  os << " | dt complete ";
  _isTimeWindowComplete ? os << "yes" : os << "no";
  return os.str();
}

std::string BaseCouplingScheme::printActionsState() const
{
  std::ostringstream os;
  for (const std::string &actionName : _actions) {
    os << actionName << " | ";
  }
  return os.str();
}

void BaseCouplingScheme::checkCompletenessRequiredActions()
{
  PRECICE_TRACE();
  if (not _actions.empty()) {
    std::ostringstream stream;
    for (const std::string &action : _actions) {
      if (not stream.str().empty()) {
        stream << ", ";
      }
      stream << action;
    }
    PRECICE_ERROR("Unfulfilled required actions: " << stream.str() << ".");
  }
}

void BaseCouplingScheme::setupDataMatrices(DataMap &data)
{
  PRECICE_TRACE();
  PRECICE_DEBUG("Data size: " << data.size());
  // Reserve storage for convergence measurement of send and receive data values
  for (ConvergenceMeasure &convMeasure : _convergenceMeasures) {
    PRECICE_ASSERT(convMeasure.couplingData != nullptr);
    if (convMeasure.couplingData->oldValues.cols() < 1) {
      utils::append(convMeasure.couplingData->oldValues,
                    (Eigen::MatrixXd) Eigen::MatrixXd::Zero(convMeasure.couplingData->values->size(), 1));
    }
  }
  // Reserve storage for extrapolation of data values
  if (_extrapolationOrder > 0) {
    for (DataMap::value_type &pair : data) {
      int cols = pair.second->oldValues.cols();
      PRECICE_DEBUG("Add cols: " << pair.first << ", cols: " << cols);
      PRECICE_ASSERT(cols <= 1, cols);
      utils::append(pair.second->oldValues,
                    (Eigen::MatrixXd) Eigen::MatrixXd::Zero(pair.second->values->size(), _extrapolationOrder + 1 - cols));
    }
  }
}

void BaseCouplingScheme::setAcceleration(
    acceleration::PtrAcceleration acceleration)
{
  PRECICE_ASSERT(acceleration.get() != nullptr);
  _acceleration = acceleration;
}

void BaseCouplingScheme::setupConvergenceMeasures()
{
  PRECICE_TRACE();
  PRECICE_ASSERT(not doesFirstStep());
  PRECICE_CHECK(not _convergenceMeasures.empty(),
                "At least one convergence measure has to be defined for "
                    << "an implicit coupling scheme.");
  for (ConvergenceMeasure &convMeasure : _convergenceMeasures) {
    int dataID = convMeasure.data->getID();
    assignDataToConvergenceMeasure(&convMeasure, dataID);
  }
}

void BaseCouplingScheme::newConvergenceMeasurements()
{
  PRECICE_TRACE();
  for (ConvergenceMeasure &convMeasure : _convergenceMeasures) {
    PRECICE_ASSERT(convMeasure.measure.get() != nullptr);
    convMeasure.measure->newMeasurementSeries();
  }
}

void BaseCouplingScheme::addConvergenceMeasure(
    mesh::PtrData               data,
    bool                        suffices,
    impl::PtrConvergenceMeasure measure)
{
  ConvergenceMeasure convMeasure;
  convMeasure.data         = std::move(data);
  convMeasure.couplingData = nullptr;
  convMeasure.suffices     = suffices;
  convMeasure.measure      = std::move(measure);
  _convergenceMeasures.push_back(convMeasure);
  _firstResiduumNorm.push_back(0);
}

bool BaseCouplingScheme::measureConvergence()
{
  PRECICE_TRACE();
  PRECICE_ASSERT(not doesFirstStep());
  bool allConverged = true;
  bool oneSuffices  = false;
  PRECICE_ASSERT(_convergenceMeasures.size() > 0);
  if (not utils::MasterSlave::isSlave()) {
    _convergenceWriter->writeData("TimeWindow", _timeWindows - 1);
    _convergenceWriter->writeData("Iteration", _iterations);
  }
  for (size_t i = 0; i < _convergenceMeasures.size(); i++) {
    ConvergenceMeasure &convMeasure = _convergenceMeasures[i];

    PRECICE_ASSERT(convMeasure.couplingData != nullptr);
    PRECICE_ASSERT(convMeasure.measure.get() != nullptr);
    const auto &oldValues = convMeasure.couplingData->oldValues.col(0);

    convMeasure.measure->measure(oldValues, *convMeasure.couplingData->values);

    if (not utils::MasterSlave::isSlave()) {
      std::stringstream sstm;
      sstm << "ResNorm(" << convMeasure.data->getName() << ")";
      _convergenceWriter->writeData(sstm.str(), convMeasure.measure->getNormResidual());
    }

    if (_iterations == 1)
      _firstResiduumNorm[i] = convMeasure.measure->getNormResidual();

    if (not convMeasure.measure->isConvergence()) {
      allConverged = false;
    } else if (convMeasure.suffices == true) {
      oneSuffices = true;
    }
    PRECICE_INFO(convMeasure.measure->printState());
  }

  if (allConverged) {
    PRECICE_INFO("All converged");
  } else if (oneSuffices) {
    PRECICE_INFO("Sufficient measure converged");
  }

  return allConverged || oneSuffices;
}

void BaseCouplingScheme::initializeTXTWriters()
{
  if (not utils::MasterSlave::isSlave()) {

    _iterationsWriter = std::make_shared<io::TXTTableWriter>("precice-" + _localParticipant + "-iterations.log");
    if (not doesFirstStep()) {
      _convergenceWriter = std::make_shared<io::TXTTableWriter>("precice-" + _localParticipant + "-convergence.log");
    }

    _iterationsWriter->addData("TimeWindow", io::TXTTableWriter::INT);
    _iterationsWriter->addData("TotalIterations", io::TXTTableWriter::INT);
    _iterationsWriter->addData("Iterations", io::TXTTableWriter::INT);
    _iterationsWriter->addData("Convergence", io::TXTTableWriter::INT);

    if (not doesFirstStep()) {
      _convergenceWriter->addData("TimeWindow", io::TXTTableWriter::INT);
      _convergenceWriter->addData("Iteration", io::TXTTableWriter::INT);
    }

    if (not doesFirstStep()) {
      for (ConvergenceMeasure &convMeasure : _convergenceMeasures) {
        std::stringstream sstm, sstm2;
        sstm << "AvgConvRate(" << convMeasure.data->getName() << ")";
        sstm2 << "ResNorm(" << convMeasure.data->getName() << ")";
        _iterationsWriter->addData(sstm.str(), io::TXTTableWriter::DOUBLE);
        _convergenceWriter->addData(sstm2.str(), io::TXTTableWriter::DOUBLE);
      }
      _iterationsWriter->addData("DeletedColumns", io::TXTTableWriter::INT);
    }
  }
}

void BaseCouplingScheme::advanceTXTWriters()
{
  if (not utils::MasterSlave::isSlave()) {

    _iterationsWriter->writeData("TimeWindow", _timeWindows - 1);
    _iterationsWriter->writeData("TotalIterations", _totalIterations);
    _iterationsWriter->writeData("Iterations", _iterations);
    int converged = _iterations < _maxIterations ? 1 : 0;
    _iterationsWriter->writeData("Convergence", converged);

    if (not doesFirstStep()) {
      int i = -1;
      for (ConvergenceMeasure &convMeasure : _convergenceMeasures) {
        i++;

        std::stringstream sstm;
        sstm << "AvgConvRate(" << convMeasure.data->getName() << ")";
        if (math::equals(_firstResiduumNorm[i], 0.)) {
          _iterationsWriter->writeData(sstm.str(), std::numeric_limits<double>::infinity());
        } else {
          double avgConvRate = _convergenceMeasures[i].measure->getNormResidual() / _firstResiduumNorm[i];
          _iterationsWriter->writeData(sstm.str(), std::pow(avgConvRate, 1. / (double) _iterations));
        }
      }
      _iterationsWriter->writeData("DeletedColumns", _deletedColumnsPPFiltering);
    }
  }
}

bool BaseCouplingScheme::reachedEndOfTimeWindow()
{
  return math::equals(getThisTimeWindowRemainder(), 0.0, _eps);
}

bool BaseCouplingScheme::maxIterationsReached()
{
  return _iterations == _maxIterations;
}

void BaseCouplingScheme::determineInitialSend(BaseCouplingScheme::DataMap &sendData) {
  if(anyDataRequiresInitialization(sendData)) {
    _sendsInitializedData = true;
  }
}

void BaseCouplingScheme::determineInitialReceive(BaseCouplingScheme::DataMap &receiveData) {
  if(anyDataRequiresInitialization(receiveData)) {
    _receivesInitializedData = true;
  }
}

bool BaseCouplingScheme::anyDataRequiresInitialization(BaseCouplingScheme::DataMap &dataMap) const
{
  /// @todo implement this function using https://en.cppreference.com/w/cpp/algorithm/all_any_none_of
  for (DataMap::value_type &pair : dataMap) {
    if (pair.second->requiresInitialization) {
      return true;
    }
  }
  return false;
}

bool BaseCouplingScheme::receiveConvergence()
{
  PRECICE_ASSERT(doesFirstStep(), "For convergence information the receiving participant is always the first one.");
  bool convergence;
  _m2n->receive(convergence);
  return convergence;
}

void BaseCouplingScheme::sendConvergence(bool convergence)
{
  PRECICE_ASSERT(not doesFirstStep(), "For convergence information the sending participant is never the first one.");
  _m2n->send(convergence);
}

bool BaseCouplingScheme::accelerate()
{
  PRECICE_DEBUG("measure convergence of the coupling iteration");
  bool convergence = measureConvergence();
  // Stop, when maximal iteration count (given in config) is reached
  if (maxIterationsReached())
    convergence = true;

  // coupling iteration converged for current time window. Advance in time.
  if (convergence) {
    if (getAcceleration()) {
      _deletedColumnsPPFiltering = getAcceleration()->getDeletedColumns();
      getAcceleration()->iterationsConverged(getAccelerationData());
    }
    newConvergenceMeasurements();
    // no convergence achieved for the coupling iteration within the current time window
  } else if (getAcceleration()) {
    getAcceleration()->performAcceleration(getAccelerationData());
  }

  // extrapolate new input data for the solver evaluation in time.
  if (convergence) {
    extrapolateData(getAccelerationData()); // Also stores data
  } else {                          // Store data for conv. measurement, acceleration, or extrapolation
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

  return convergence;
}
} // namespace cplscheme
} // namespace precice
