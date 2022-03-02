#include "Participant.hpp"
#include <algorithm>
#include <ostream>
#include <utility>

#include "MappingContext.hpp"
#include "MeshContext.hpp"
#include "WatchIntegral.hpp"
#include "WatchPoint.hpp"
#include "action/Action.hpp"
#include "logging/LogMacros.hpp"
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/config/DataConfiguration.hpp"
#include "mesh/config/MeshConfiguration.hpp"
#include "precice/impl/SharedPointer.hpp"
#include "precice/types.hpp"
#include "utils/ManageUniqueIDs.hpp"
#include "utils/assertion.hpp"

namespace precice {
namespace impl {

Participant::Participant(
    std::string                 name,
    mesh::PtrMeshConfiguration &meshConfig)
    : _name(std::move(name)),
      _meshContexts(meshConfig->meshes().size(), nullptr)
{
}

Participant::~Participant()
{
  for (MeshContext *context : _usedMeshContexts) {
    delete context;
  }
  _usedMeshContexts.clear();
  _readMappingContexts.deleteElements();
  _writeMappingContexts.deleteElements();
}

/// Configuration interface

void Participant::addAction(action::PtrAction &&action)
{
  auto &context = meshContext(action->getMesh()->getID());
  context.require(action->getMeshRequirement());
  _actions.push_back(std::move(action));
}

void Participant::setUseMaster(bool useMaster)
{
  _useMaster = useMaster;
}

void Participant::addWatchPoint(
    const PtrWatchPoint &watchPoint)
{
  _watchPoints.push_back(watchPoint);
}

void Participant::addWatchIntegral(
    const PtrWatchIntegral &watchIntegral)
{
  _watchIntegrals.push_back(watchIntegral);
}

void Participant::useMesh(const mesh::PtrMesh &                         mesh,
                          const Eigen::VectorXd &                       localOffset,
                          bool                                          remote,
                          const std::string &                           fromParticipant,
                          double                                        safetyFactor,
                          bool                                          provideMesh,
                          partition::ReceivedPartition::GeometricFilter geoFilter,
                          const bool                                    allowDirectAccess)
{
  PRECICE_TRACE(_name, mesh->getName(), mesh->getID());
  checkDuplicatedUse(mesh);
  PRECICE_ASSERT(mesh->getID() < (int) _meshContexts.size());
  auto context         = new MeshContext(mesh->getDimensions());
  context->mesh        = mesh;
  context->localOffset = localOffset;
  PRECICE_ASSERT(mesh->getDimensions() == context->localOffset.size(),
                 mesh->getDimensions(), context->localOffset.size());
  context->receiveMeshFrom   = fromParticipant;
  context->safetyFactor      = safetyFactor;
  context->provideMesh       = provideMesh;
  context->geoFilter         = geoFilter;
  context->allowDirectAccess = allowDirectAccess;

  _meshContexts[mesh->getID()] = context;

  _usedMeshContexts.push_back(context);

  PRECICE_CHECK(fromParticipant.empty() || (!provideMesh),
                "Participant \"{}\" cannot receive and provide mesh \"{}\" at the same time. "
                "Please remove all but one of the \"from\" and \"provide\" attributes in the <use-mesh name=\"{}\"/> node of {}.",
                _name, mesh->getName(), mesh->getName(), _name);
}

void Participant::addWriteData(
    const mesh::PtrData &data,
    const mesh::PtrMesh &mesh)
{
  checkDuplicatedData(data, mesh->getName());
  _writeDataContexts.emplace(data->getID(), WriteDataContext(data, mesh));
}

void Participant::addReadData(
    const mesh::PtrData &data,
    const mesh::PtrMesh &mesh)
{
  checkDuplicatedData(data, mesh->getName());
  _readDataContexts.emplace(data->getID(), ReadDataContext(data, mesh));
}

void Participant::addReadMappingContext(
    MappingContext *mappingContext)
{
  _readMappingContexts.push_back(mappingContext);
}

void Participant::addWriteMappingContext(
    MappingContext *mappingContext)
{
  _writeMappingContexts.push_back(mappingContext);
}

// Data queries
const ReadDataContext &Participant::readDataContext(DataID dataID) const
{
  auto it = _readDataContexts.find(dataID);
  PRECICE_CHECK(it != _readDataContexts.end(), "DataID does not exist.")
  return it->second;
}

ReadDataContext &Participant::readDataContext(DataID dataID)
{
  auto it = _readDataContexts.find(dataID);
  PRECICE_CHECK(it != _readDataContexts.end(), "DataID does not exist.")
  return it->second;
}

const WriteDataContext &Participant::writeDataContext(DataID dataID) const
{
  auto it = _writeDataContexts.find(dataID);
  PRECICE_CHECK(it != _writeDataContexts.end(), "DataID does not exist.")
  return it->second;
}

WriteDataContext &Participant::writeDataContext(DataID dataID)
{
  auto it = _writeDataContexts.find(dataID);
  PRECICE_CHECK(it != _writeDataContexts.end(), "DataID does not exist.")
  return it->second;
}

bool Participant::hasData(DataID dataID) const
{
  return std::any_of(
      _meshContexts.begin(), _meshContexts.end(),
      [dataID](const auto mcptr) {
        if (!mcptr) {
          return false;
        }
        const auto &meshData = mcptr->mesh->data();
        return std::any_of(meshData.begin(), meshData.end(), [dataID](const auto &dptr) {
          return dptr->getID() == dataID;
        });
      });
}

bool Participant::isDataUsed(const std::string &dataName, MeshID meshID) const
{
  const auto &meshData = meshContext(meshID).mesh->data();
  const auto  match    = std::find_if(meshData.begin(), meshData.end(), [&dataName](auto &dptr) { return dptr->getName() == dataName; });
  return match != meshData.end();
}

bool Participant::isDataRead(DataID dataID) const
{
  return _readDataContexts.count(dataID) > 0;
}

bool Participant::isDataWrite(DataID dataID) const
{
  return _writeDataContexts.count(dataID) > 0;
}

int Participant::getUsedDataID(const std::string &dataName, MeshID meshID) const
{
  const auto &dptr = usedMeshContext(meshID).mesh->data(dataName);
  PRECICE_ASSERT(dptr != nullptr);
  return dptr->getID();
}

std::string Participant::getDataName(DataID dataID) const
{
  for (const MeshContext *mcptr : _meshContexts) {
    if (!mcptr) {
      continue;
    }
    for (const auto &dptr : mcptr->mesh->data()) {
      if (dptr->getID() == dataID) {
        return dptr->getName();
      }
    }
  }
  PRECICE_UNREACHABLE("The dataID {} is invalid.", dataID);
}

/// Mesh queries

const MeshContext &Participant::meshContext(MeshID meshID) const
{
  PRECICE_ASSERT((meshID >= 0) && (meshID < (int) _meshContexts.size()));
  PRECICE_ASSERT(_meshContexts[meshID] != nullptr);
  return *_meshContexts[meshID];
}

MeshContext &Participant::meshContext(MeshID meshID)
{
  PRECICE_TRACE(meshID, _meshContexts.size());
  PRECICE_ASSERT((meshID >= 0) && (meshID < (int) _meshContexts.size()),
                 meshID, _meshContexts.size());
  PRECICE_ASSERT(_meshContexts[meshID] != nullptr);
  return *_meshContexts[meshID];
}

const std::vector<MeshContext *> &Participant::usedMeshContexts() const
{
  return _usedMeshContexts;
}

std::vector<MeshContext *> &Participant::usedMeshContexts()
{
  return _usedMeshContexts;
}

MeshContext &Participant::usedMeshContext(MeshID meshID)
{
  auto pos = std::find_if(_usedMeshContexts.begin(), _usedMeshContexts.end(),
                          [meshID](MeshContext const *context) {
                            return context->mesh->getID() == meshID;
                          });
  PRECICE_ASSERT(pos != _usedMeshContexts.end());
  return **pos;
}

MeshContext const &Participant::usedMeshContext(MeshID meshID) const
{
  auto pos = std::find_if(_usedMeshContexts.begin(), _usedMeshContexts.end(),
                          [meshID](MeshContext const *context) {
                            return context->mesh->getID() == meshID;
                          });
  PRECICE_ASSERT(pos != _usedMeshContexts.end());
  return **pos;
}

MeshContext &Participant::usedMeshContext(const std::string &name)
{
  auto pos = std::find_if(_usedMeshContexts.begin(), _usedMeshContexts.end(),
                          [&name](MeshContext const *context) {
                            return context->mesh->getName() == name;
                          });
  PRECICE_ASSERT(pos != _usedMeshContexts.end());
  return **pos;
}

MeshContext const &Participant::usedMeshContext(const std::string &name) const
{
  auto pos = std::find_if(_usedMeshContexts.begin(), _usedMeshContexts.end(),
                          [&name](MeshContext const *context) {
                            return context->mesh->getName() == name;
                          });
  PRECICE_ASSERT(pos != _usedMeshContexts.end());
  return **pos;
}

bool Participant::hasMesh(MeshID meshID) const
{
  return meshID < static_cast<int>(_meshContexts.size()) && _meshContexts.at(meshID) != nullptr;
}

bool Participant::hasMesh(const std::string &meshName) const
{
  return std::any_of(
      _meshContexts.begin(), _meshContexts.end(),
      [&meshName](const MeshContext *mcptr) {
        return mcptr && meshName == mcptr->mesh->getName();
      });
}

bool Participant::isMeshUsed(MeshID meshID) const
{
  return std::any_of(
      _usedMeshContexts.begin(), _usedMeshContexts.end(),
      [meshID](const MeshContext *mcptr) {
        return mcptr->mesh->getID() == meshID;
      });
}

bool Participant::isMeshUsed(const std::string &meshName) const
{
  return std::any_of(
      _usedMeshContexts.begin(), _usedMeshContexts.end(),
      [&meshName](const MeshContext *mcptr) {
        return mcptr->mesh->getName() == meshName;
      });
}

bool Participant::isMeshProvided(MeshID meshID) const
{
  PRECICE_ASSERT((meshID >= 0) && (meshID < (int) _meshContexts.size()));
  auto context = _meshContexts[meshID];
  return (context != nullptr) && context->provideMesh;
}

int Participant::getUsedMeshID(const std::string &meshName) const
{
  return usedMeshContext(meshName).mesh->getID();
}

bool Participant::isDirectAccessAllowed(const int meshID) const
{
  PRECICE_ASSERT((meshID >= 0) && (meshID < (int) _meshContexts.size()));
  auto context = _meshContexts[meshID];
  return context->allowDirectAccess;
}

std::string Participant::getMeshName(MeshID meshID) const
{
  return meshContext(meshID).mesh->getName();
}

std::string Participant::getMeshNameFromData(DataID dataID) const
{
  for (const MeshContext *mcptr : _meshContexts) {
    for (const auto &dptr : mcptr->mesh->data()) {
      if (dptr->getID() == dataID) {
        return mcptr->mesh->getName();
      }
    }
  }
  PRECICE_UNREACHABLE("The dataID {} is invalid.", dataID);
}

// Other queries

const utils::ptr_vector<MappingContext> &Participant::readMappingContexts() const
{
  return _readMappingContexts;
}

const utils::ptr_vector<MappingContext> &Participant::writeMappingContexts() const
{
  return _writeMappingContexts;
}

std::vector<action::PtrAction> &Participant::actions()
{
  return _actions;
}

const std::vector<action::PtrAction> &Participant::actions() const
{
  return _actions;
}

void Participant::addExportContext(
    const io::ExportContext &exportContext)
{
  _exportContexts.push_back(exportContext);
}

const std::vector<io::ExportContext> &Participant::exportContexts() const
{
  return _exportContexts;
}

std::vector<PtrWatchPoint> &Participant::watchPoints()
{
  return _watchPoints;
}

std::vector<PtrWatchIntegral> &Participant::watchIntegrals()
{
  return _watchIntegrals;
}

bool Participant::useMaster() const
{
  return _useMaster;
}

const std::string &Participant::getName() const
{
  return _name;
}

// private

void Participant::checkDuplicatedUse(const mesh::PtrMesh &mesh)
{
  PRECICE_ASSERT((int) _meshContexts.size() > mesh->getID());
  PRECICE_CHECK(_meshContexts[mesh->getID()] == nullptr,
                "Mesh \"{} cannot be used twice by participant {}. "
                "Please remove one of the use-mesh nodes with name=\"{}\"./>",
                mesh->getName(), _name, mesh->getName());
}

void Participant::checkDuplicatedData(const mesh::PtrData &data, const std::string &meshName)
{
  PRECICE_CHECK(!isDataWrite(data->getID()) && !isDataRead(data->getID()),
                "Participant \"{}\" can read/write data \"{}\" from/to mesh \"{}\" only once. "
                "Please remove any duplicate instances of write-data/read-data nodes.",
                _name, meshName, data->getName());
}

} // namespace impl
} // namespace precice
