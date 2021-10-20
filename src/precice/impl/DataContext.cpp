#include "precice/impl/DataContext.hpp"
#include <memory>
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"

namespace precice {
namespace impl {

DataContext::DataContext(mesh::PtrData data, mesh::PtrMesh mesh)
{
  PRECICE_ASSERT(data);
  _providedData = data;
  PRECICE_ASSERT(mesh);
  _mesh = mesh;
}

mesh::PtrData DataContext::providedData()
{
  PRECICE_ASSERT(_providedData);
  return _providedData;
}

mesh::PtrData DataContext::toData()
{
  PRECICE_ASSERT(_toData);
  return _toData;
}

std::string DataContext::getDataName() const
{
  PRECICE_ASSERT(_providedData);
  return _providedData->getName();
}

int DataContext::getProvidedDataID() const
{
  PRECICE_ASSERT(_providedData);
  return _providedData->getID();
}

int DataContext::getFromDataID() const
{
  PRECICE_ASSERT(hasMapping());
  PRECICE_ASSERT(_fromData);
  return _fromData->getID();
}

void DataContext::resetProvidedData()
{
  _providedData->toZero();
}

void DataContext::resetToData()
{
  _toData->toZero();
}

int DataContext::getToDataID() const
{
  PRECICE_ASSERT(hasMapping());
  PRECICE_ASSERT(_toData);
  return _toData->getID();
}

std::string DataContext::getMeshName() const
{
  PRECICE_ASSERT(_mesh);
  return _mesh->getName();
}

int DataContext::getMeshID() const
{
  PRECICE_ASSERT(_mesh);
  return _mesh->getID();
}

void DataContext::setMapping(MappingContext mappingContext, mesh::PtrData fromData, mesh::PtrData toData)
{
  PRECICE_ASSERT(!hasMapping());
  PRECICE_ASSERT(fromData);
  PRECICE_ASSERT(toData);
  _mappingContext = mappingContext;
  PRECICE_ASSERT(fromData == _providedData || toData == _providedData, "Either fromData or toData has to equal provided data.");
  PRECICE_ASSERT(fromData->getName() == getDataName());
  _fromData = fromData;
  PRECICE_ASSERT(toData->getName() == getDataName());
  _toData = toData;
  PRECICE_ASSERT(_toData != _fromData);
}

void DataContext::configureForReadMapping(MappingContext mappingContext, MeshContext fromMeshContext)
{
  PRECICE_ASSERT(fromMeshContext.mesh->hasDataName(getDataName()));
  mesh::PtrData fromData = fromMeshContext.mesh->data(getDataName());
  PRECICE_ASSERT(fromData != _providedData);
  this->setMapping(mappingContext, fromData, _providedData);
  PRECICE_ASSERT(hasReadMapping());
}

void DataContext::configureForWriteMapping(MappingContext mappingContext, MeshContext toMeshContext)
{
  PRECICE_ASSERT(toMeshContext.mesh->hasDataName(getDataName()));
  mesh::PtrData toData = toMeshContext.mesh->data(getDataName());
  PRECICE_ASSERT(toData != _providedData);
  this->setMapping(mappingContext, _providedData, toData);
  PRECICE_ASSERT(hasWriteMapping());
}

bool DataContext::hasMapping() const
{
  return hasReadMapping() || hasWriteMapping();
}

bool DataContext::hasReadMapping() const
{
  return _toData == _providedData;
}

bool DataContext::hasWriteMapping() const
{
  return _fromData == _providedData;
}

const MappingContext DataContext::mappingContext() const
{
  PRECICE_ASSERT(hasMapping());
  return _mappingContext;
}

} // namespace impl
} // namespace precice
