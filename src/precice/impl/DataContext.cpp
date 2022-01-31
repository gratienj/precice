#include "precice/impl/DataContext.hpp"
#include <memory>

namespace precice {
namespace impl {

logging::Logger DataContext::_log{"impl::DataContext"};

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
  PRECICE_TRACE();
  PRECICE_ASSERT(hasMapping());
  PRECICE_ASSERT(_fromData);
  return _fromData->getID();
}

void DataContext::resetProvidedData()
{
  PRECICE_TRACE();
  _providedData->toZero();
}

void DataContext::resetToData()
{
  PRECICE_TRACE();
  _toData->toZero();
}

int DataContext::getToDataID() const
{
  PRECICE_TRACE();
  PRECICE_ASSERT(hasMapping());
  PRECICE_ASSERT(_toData);
  return _toData->getID();
}

int DataContext::getDataDimensions() const
{
  PRECICE_TRACE();
  PRECICE_ASSERT(_providedData);
  return _providedData->getDimensions();
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
  PRECICE_ASSERT(fromData == _providedData || toData == _providedData, "Either fromData or toData has to equal _providedData.");
  PRECICE_ASSERT(fromData->getName() == getDataName());
  _fromData = fromData;
  PRECICE_ASSERT(toData->getName() == getDataName());
  _toData = toData;
  PRECICE_ASSERT(_toData != _fromData);
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
