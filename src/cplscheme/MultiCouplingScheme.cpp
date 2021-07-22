#include "MultiCouplingScheme.hpp"
#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>
#include "acceleration/Acceleration.hpp"
#include "acceleration/SharedPointer.hpp"
#include "cplscheme/BaseCouplingScheme.hpp"
#include "cplscheme/CouplingData.hpp"
#include "cplscheme/SharedPointer.hpp"
#include "logging/LogMacros.hpp"
#include "m2n/SharedPointer.hpp"
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "utils/Helpers.hpp"
#include "utils/MasterSlave.hpp"
#include "utils/assertion.hpp"

namespace precice {
namespace cplscheme {

MultiCouplingScheme::MultiCouplingScheme(
    double                             maxTime,
    int                                maxTimeWindows,
    double                             timeWindowSize,
    int                                validDigits,
    const std::string &                localParticipant,
    std::map<std::string, m2n::PtrM2N> m2ns,
    constants::TimesteppingMethod      dtMethod,
    const std::string &                controller,
    int                                maxIterations)
    : BaseCouplingScheme(maxTime, maxTimeWindows, timeWindowSize, validDigits, localParticipant, maxIterations, Implicit, dtMethod),
      _m2ns(std::move(m2ns)), _controller(controller), _isController(controller == localParticipant)
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");
  // Controller participant never does the first step, because it is never the first participant
  setDoesFirstStep(!_isController);

  PRECICE_DEBUG("MultiCoupling scheme is created for {}.", localParticipant);
}

std::vector<std::string> MultiCouplingScheme::getCouplingPartners() const
{
  std::vector<std::string> partnerNames;

  for (const auto &m2nPair : _m2ns) {
    partnerNames.emplace_back(m2nPair.first);
  }

  return partnerNames;
}

void MultiCouplingScheme::initializeImplementation()
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");

  PRECICE_DEBUG("MultiCouplingScheme is being initialized.");
  for (auto &sendExchange : _sendDataVector) {
    determineInitialSend(sendExchange.second);
  }
  for (auto &receiveExchange : _receiveDataVector) {
    determineInitialReceive(receiveExchange.second);
  }
  PRECICE_DEBUG("MultiCouplingScheme is initialized.");
}

void MultiCouplingScheme::exchangeInitialData()
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");

  if (_isController) {
    if (receivesInitializedData()) {
      for (auto &receiveExchange : _receiveDataVector) {
        receiveData(_m2ns[receiveExchange.first], receiveExchange.second);
      }
      checkDataHasBeenReceived();
    }
    if (sendsInitializedData()) {
      for (auto &sendExchange : _sendDataVector) {
        sendData(_m2ns[sendExchange.first], sendExchange.second);
      }
    }
  } else {
    if (sendsInitializedData()) {
      for (auto &sendExchange : _sendDataVector) {
        sendData(_m2ns[sendExchange.first], sendExchange.second);
      }
    }
    if (receivesInitializedData()) {
      for (auto &receiveExchange : _receiveDataVector) {
        receiveData(_m2ns[receiveExchange.first], receiveExchange.second);
      }
      checkDataHasBeenReceived();
    }
  }
  PRECICE_DEBUG("Initial data is exchanged in MultiCouplingScheme");
}

bool MultiCouplingScheme::exchangeDataAndAccelerate()
{
  PRECICE_ASSERT(isImplicitCouplingScheme(), "MultiCouplingScheme is always Implicit.");
  // @todo implement MultiCouplingScheme for explicit coupling

  PRECICE_DEBUG("Computed full length of iteration");

  bool convergence = true;

  if (_isController) {
    for (auto &receiveExchange : _receiveDataVector) {
      receiveData(_m2ns[receiveExchange.first], receiveExchange.second);
    }
    checkDataHasBeenReceived();

    convergence = doImplicitStep();
    for (const auto &m2nPair : _m2ns) {
      sendConvergence(m2nPair.second, convergence);
    }

    for (auto &sendExchange : _sendDataVector) {
      sendData(_m2ns[sendExchange.first], sendExchange.second);
    }
  } else {
    for (auto &sendExchange : _sendDataVector) {
      sendData(_m2ns[sendExchange.first], sendExchange.second);
    }

    convergence = receiveConvergence(_m2ns[_controller]);

    for (auto &receiveExchange : _receiveDataVector) {
      receiveData(_m2ns[receiveExchange.first], receiveExchange.second);
    }
    checkDataHasBeenReceived();
  }
  return convergence;
}

void MultiCouplingScheme::addDataToSend(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          initialize,
    std::string   to)
{
  int id = data->getID();
  PRECICE_DEBUG("Configuring send data to {}", to);
  PtrCouplingData     ptrCplData(new CouplingData(data, mesh, initialize));
  DataMap::value_type dataPair = std::make_pair(id, ptrCplData);
  _sendDataVector[to].insert(dataPair);
  if (!utils::contained(id, _allData)) {
    _allData.insert(dataPair);
  }
}

void MultiCouplingScheme::addDataToReceive(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          initialize,
    std::string   from)
{
  int id = data->getID();
  PRECICE_DEBUG("Configuring receive data from {}", from);
  PtrCouplingData     ptrCplData(new CouplingData(data, mesh, initialize));
  DataMap::value_type dataPair = std::make_pair(id, ptrCplData);
  _receiveDataVector[from].insert(dataPair);
  if (!utils::contained(id, _allData)) {
    _allData.insert(dataPair);
  }
}

} // namespace cplscheme
} // namespace precice
