#include "MultiCouplingScheme.hpp"
#include "acceleration/Acceleration.hpp"
#include "m2n/M2N.hpp"
#include "m2n/SharedPointer.hpp"
#include "math/math.hpp"
#include "mesh/Mesh.hpp"
#include "utils/EigenHelperFunctions.hpp"
#include "utils/Helpers.hpp"
#include "utils/MasterSlave.hpp"

namespace precice {
namespace cplscheme {

MultiCouplingScheme::MultiCouplingScheme(
    double                        maxTime,
    int                           maxTimeWindows,
    double                        timeWindowSize,
    int                           validDigits,
    const std::string &           localParticipant,
    std::vector<m2n::PtrM2N>      m2ns,
    constants::TimesteppingMethod dtMethod,
    int                           maxIterations)
    : BaseCouplingScheme(maxTime, maxTimeWindows, timeWindowSize, validDigits, "neverFirstParticipant",
                         localParticipant, localParticipant, m2n::PtrM2N(), maxIterations, Implicit, dtMethod),
      _communications(m2ns)
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");
  PRECICE_ASSERT(not doesFirstStep(), "MultiCouplingScheme never does the first step, because it is never the first participant");
  for (size_t i = 0; i < _communications.size(); ++i) {
    DataMap receiveMap;
    DataMap sendMap;
    _receiveDataVector.push_back(receiveMap);
    _sendDataVector.push_back(sendMap);
  }
}

void MultiCouplingScheme::checkConfiguration()
{
  if (isImplicitCouplingScheme()) {
    bool hasAnySendData = std::any_of(_sendDataVector.cbegin(), _sendDataVector.cend(), [](DataMap sendData) { return not sendData.empty(); });
    PRECICE_CHECK(hasAnySendData, "No send data configured. Use explicit scheme for one-way coupling.");
    if (not doesFirstStep() && getAcceleration()) {
      PRECICE_CHECK(getAcceleration()->getDataIDs().size() >= 3,
                    "For multi coupling, the number of coupling data vectors has to be at least 3, not: "
                        << getAcceleration()->getDataIDs().size());
    }
  }
}

void MultiCouplingScheme::initializeImplementation()
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");

  for (DataMap &sendData : _sendDataVector) {
    determineInitialSend(sendData);
  }
  for (DataMap &receiveData : _receiveDataVector) {
    determineInitialReceive(receiveData);
  }
}

void MultiCouplingScheme::exchangeInitialData()
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");

  if (receivesInitializedData()) {
    receiveData();
    // second participant has to save values for extrapolation
    for (DataMap &receiveData : _receiveDataVector) {
      updateOldValues(receiveData);
    }
  }
  if (sendsInitializedData()) {
    for (DataMap &sendData : _sendDataVector) {
      updateOldValues(sendData);
    }
    sendData();
  }
}

std::pair<bool, bool> MultiCouplingScheme::exchangeDataAndAccelerate()
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");
  // @todo implement MultiCouplingScheme for explicit coupling

  PRECICE_DEBUG("Computed full length of iteration");

  receiveData();

  PRECICE_DEBUG("Perform acceleration (only second participant)...");
  std::pair<bool, bool> convergenceInformation = accelerate();
  bool convergence = convergenceInformation.first;

  for (m2n::PtrM2N m2n : _communications) {
    PRECICE_ASSERT(not getIsCoarseModelOptimizationActive());
    sendConvergence(m2n, convergence);
  }

  sendData();

  // TODO: Using a hard-coded "true" here looks strange.
  return std::pair<bool, bool> (convergence, true);
}

void MultiCouplingScheme::mergeData()
{
  PRECICE_TRACE();
  PRECICE_ASSERT(_allData.empty(), "This function should only be called once.");
  PRECICE_ASSERT(_sendDataVector.size() == _receiveDataVector.size());
  for (size_t i = 0; i < _sendDataVector.size(); i++) {
    _allData.insert(_sendDataVector[i].begin(), _sendDataVector[i].end());
    _allData.insert(_receiveDataVector[i].begin(), _receiveDataVector[i].end());
  }
}

void MultiCouplingScheme::addDataToSend(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          initialize,
    int           index)
{
  int id = data->getID();
  if (!utils::contained(id, _sendDataVector[index])) {
    PtrCouplingData     ptrCplData(new CouplingData(&(data->values()), mesh, initialize, data->getDimensions()));
    DataMap::value_type pair = std::make_pair(id, ptrCplData);
    _sendDataVector[index].insert(pair);
  } else {
    PRECICE_ERROR("Data \"" << data->getName()
                            << "\" of mesh \"" << mesh->getName() << "\" cannot be "
                            << "added twice for sending.");
  }
}

void MultiCouplingScheme::addDataToReceive(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          initialize,
    int           index)
{
  int id = data->getID();
  if (!utils::contained(id, _receiveDataVector[index])) {
    PtrCouplingData     ptrCplData(new CouplingData(&(data->values()), mesh, initialize, data->getDimensions()));
    DataMap::value_type pair = std::make_pair(id, ptrCplData);
    _receiveDataVector[index].insert(pair);
  } else {
    PRECICE_ERROR("Data \"" << data->getName()
                            << "\" of mesh \"" << mesh->getName() << "\" cannot be "
                            << "added twice for receiving.");
  }
}

void MultiCouplingScheme::sendData()
{
  PRECICE_TRACE();

  for (size_t i = 0; i < _communications.size(); i++) {
    PRECICE_ASSERT(_communications[i].get() != nullptr);
    PRECICE_ASSERT(_communications[i]->isConnected());

    for (DataMap::value_type &pair : _sendDataVector[i]) {
      int size = pair.second->values->size();
      if (size > 0) {
        _communications[i]->send(pair.second->values->data(), size, pair.second->mesh->getID(), pair.second->dimension);
      }
    }
  }
}

void MultiCouplingScheme::receiveData()
{
  PRECICE_TRACE();

  for (size_t i = 0; i < _communications.size(); i++) {
    PRECICE_ASSERT(_communications[i].get() != nullptr);
    PRECICE_ASSERT(_communications[i]->isConnected());

    for (DataMap::value_type &pair : _receiveDataVector[i]) {
      int size = pair.second->values->size();
      if (size > 0) {
        _communications[i]->receive(pair.second->values->data(), size, pair.second->mesh->getID(), pair.second->dimension);
      }
    }
  }
  setHasDataBeenExchanged(true);
}

CouplingData *MultiCouplingScheme::getData(
    int dataID)
{
  PRECICE_TRACE(dataID);
  DataMap::iterator iter = _allData.find(dataID);
  if (iter != _allData.end()) {
    return &(*(iter->second));
  }
  return nullptr;
}

void MultiCouplingScheme::assignDataToConvergenceMeasure(ConvergenceMeasure *convergenceMeasure, int dataID)
{
  convergenceMeasure->couplingData = getData(dataID);
  PRECICE_ASSERT(convergenceMeasure->couplingData != nullptr);
}

} // namespace cplscheme
} // namespace precice
