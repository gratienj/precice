#include "cplscheme/CouplingData.hpp"

#include <utility>

#include "utils/EigenHelperFunctions.hpp"

namespace precice {
namespace cplscheme {

CouplingData::CouplingData(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          requiresInitialization)
    : requiresInitialization(requiresInitialization),
      _data(std::move(data)),
      _mesh(std::move(mesh))
{
  PRECICE_ASSERT(_data != nullptr);
  _previousIteration = Eigen::VectorXd::Zero(_data->values().size());
  PRECICE_ASSERT(_mesh != nullptr);
  PRECICE_ASSERT(_mesh.use_count() > 0);
  std::cout<<"COUPLING DATA : "<<_data->getName()<<" MESH : "<<_mesh->getName()<<std::endl ;
}

int CouplingData::getDimensions() const
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->getDimensions();
}

Eigen::VectorXd &CouplingData::values()
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->values();
}

const Eigen::VectorXd &CouplingData::values() const
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->values();
}

bool CouplingData::hasMeshFilter()
{
  return _mesh->hasMeshFilter();
}

bool CouplingData::hasDataMappingMeshFilter(int dataID)
{
  return _mesh->hasDataMappingMeshFilter(dataID);
}

bool CouplingData::meshFilterActivated()
{
  return _mesh->verticesFilterIsActivated();
}

std::vector<int> const &CouplingData::filterIds() const
{
  return _mesh->activatedVerticesIds();
}

std::vector<int> const &CouplingData::getDataMappingMeshFilterIds(int dataID) const
{
  return _mesh->getDataMappingMeshFilterIds(dataID);
}

void CouplingData::storeIteration()
{
  _previousIteration = this->values();
}

const Eigen::VectorXd CouplingData::previousIteration() const
{
  return _previousIteration;
}

int CouplingData::getMeshID()
{
  return _mesh->getID();
}

int CouplingData::getDataID()
{
  return _data->getID();
}

std::string CouplingData::getDataName()
{
  return _data->getName();
}

std::vector<int> CouplingData::getVertexOffsets()
{
  return _mesh->getVertexOffsets();
}

} // namespace cplscheme
} // namespace precice
