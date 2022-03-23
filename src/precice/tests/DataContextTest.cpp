#include <Eigen/Core>
#include <string>
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/Vertex.hpp"
#include "precice/impl/ReadDataContext.hpp"
#include "precice/impl/WriteDataContext.hpp"
#include "testing/DataContextFixture.hpp"
#include "testing/TestContext.hpp"
#include "testing/Testing.hpp"

using namespace precice;
using namespace precice::impl;

BOOST_AUTO_TEST_SUITE(PreciceTests)

BOOST_AUTO_TEST_SUITE(DataContextTests)

BOOST_AUTO_TEST_CASE(testDataContextWriteMapping)
{
  PRECICE_TEST(1_rank);

  testing::DataContextFixture fixture;

  // Create mesh object for from mesh
  int           dimensions  = 3;
  mesh::PtrMesh ptrFromMesh = std::make_shared<mesh::Mesh>("ParticipantMesh", dimensions, testing::nextMeshID());
  mesh::PtrData ptrFromData = ptrFromMesh->createData("MappedData", dimensions, 0_dataID);

  ptrFromMesh->createVertex(Eigen::Vector3d(0.0, 0.0, 0.0));
  ptrFromMesh->createVertex(Eigen::Vector3d(1.0, 0.0, 0.0));
  ptrFromMesh->createVertex(Eigen::Vector3d(0.0, 0.0, 1.0));

  // Create mesh object for from mesh
  mesh::PtrMesh ptrToMesh = std::make_shared<mesh::Mesh>("OtherMesh", dimensions, testing::nextMeshID());
  mesh::PtrData ptrToData = ptrToMesh->createData("MappedData", dimensions, 1_dataID);

  ptrToMesh->createVertex(Eigen::Vector3d(0.0, 0.1, 0.0));
  ptrToMesh->createVertex(Eigen::Vector3d(1.0, 0.1, 0.0));
  ptrToMesh->createVertex(Eigen::Vector3d(0.0, 0.1, 1.0));

  MeshContext toMeshContext(dimensions);
  toMeshContext.mesh = ptrToMesh;

  WriteDataContext dataContext(ptrFromData, ptrFromMesh);

  MappingContext mappingContext;
  mappingContext.fromMeshID = ptrFromMesh->getID();
  mappingContext.toMeshID   = ptrToMesh->getID();

  BOOST_TEST(ptrToData->getID() != ptrFromData->getID());
  BOOST_TEST(ptrToMesh->getID() != ptrFromMesh->getID());

  BOOST_TEST(!fixture.hasMapping(dataContext));
  BOOST_TEST(fixture.getProvidedDataID(dataContext) == ptrFromData->getID());
  BOOST_TEST(dataContext.getMeshID() == ptrFromMesh->getID());

  dataContext.configureMapping(mappingContext, toMeshContext);

  // mapping is configured. Write mapping, therefore _providedData == _fromData
  BOOST_TEST(fixture.hasMapping(dataContext));
  BOOST_TEST(fixture.getFromDataID(dataContext) == ptrFromData->getID());
  BOOST_TEST(fixture.getToDataID(dataContext) == ptrToData->getID());
  BOOST_TEST(fixture.getProvidedDataID(dataContext) != ptrToData->getID());
  BOOST_TEST(fixture.getProvidedDataID(dataContext) == ptrFromData->getID());
  BOOST_TEST(dataContext.getMeshID() != ptrToMesh->getID());
  BOOST_TEST(dataContext.getMeshID() == ptrFromMesh->getID());
  BOOST_TEST(fixture.hasWriteMapping(dataContext));
  BOOST_TEST(!fixture.hasReadMapping(dataContext));
  BOOST_TEST(fixture.mappingContext(dataContext).fromMeshID == mappingContext.fromMeshID);
  BOOST_TEST(fixture.mappingContext(dataContext).toMeshID == mappingContext.toMeshID);
  BOOST_TEST(fixture.mappingContext(dataContext).hasMappedData == mappingContext.hasMappedData);
  BOOST_TEST(fixture.mappingContext(dataContext).mapping == mappingContext.mapping);
  BOOST_TEST(fixture.mappingContext(dataContext).timing == mappingContext.timing);
}

BOOST_AUTO_TEST_CASE(testDataContextReadMapping)
{
  PRECICE_TEST(1_rank);

  testing::DataContextFixture fixture;

  // Create mesh object
  int           dimensions = 3;
  mesh::PtrMesh ptrToMesh  = std::make_shared<mesh::Mesh>("ParticipantMesh", dimensions, testing::nextMeshID());
  mesh::PtrData ptrToData  = ptrToMesh->createData("MappedData", dimensions, 0_dataID);

  ptrToMesh->createVertex(Eigen::Vector3d(0.0, 0.0, 0.0));
  ptrToMesh->createVertex(Eigen::Vector3d(1.0, 0.0, 0.0));
  ptrToMesh->createVertex(Eigen::Vector3d(0.0, 0.0, 1.0));

  // Create mesh object for from mesh
  mesh::PtrMesh ptrFromMesh = std::make_shared<mesh::Mesh>("OtherMesh", dimensions, testing::nextMeshID());
  mesh::PtrData ptrFromData = ptrFromMesh->createData("MappedData", dimensions, 1_dataID);

  ptrFromMesh->createVertex(Eigen::Vector3d(0.0, 0.1, 0.0));
  ptrFromMesh->createVertex(Eigen::Vector3d(1.0, 0.1, 0.0));
  ptrFromMesh->createVertex(Eigen::Vector3d(0.0, 0.1, 1.0));

  MeshContext fromMeshContext(dimensions);
  fromMeshContext.mesh = ptrFromMesh;

  ReadDataContext dataContext(ptrToData, ptrToMesh);

  MappingContext mappingContext;
  mappingContext.fromMeshID = ptrFromMesh->getID();
  mappingContext.toMeshID   = ptrToMesh->getID();

  BOOST_TEST(ptrToData->getID() != ptrFromData->getID());
  BOOST_TEST(ptrToMesh->getID() != ptrFromMesh->getID());

  BOOST_TEST(!fixture.hasMapping(dataContext));
  BOOST_TEST(fixture.getProvidedDataID(dataContext) == ptrToData->getID());
  BOOST_TEST(dataContext.getMeshID() == ptrToMesh->getID());

  dataContext.configureMapping(mappingContext, fromMeshContext);

  // mapping is configured. Write mapping, therefore _providedData == _toData
  BOOST_TEST(fixture.hasMapping(dataContext));
  BOOST_TEST(fixture.getFromDataID(dataContext) == ptrFromData->getID());
  BOOST_TEST(fixture.getToDataID(dataContext) == ptrToData->getID());
  BOOST_TEST(fixture.getProvidedDataID(dataContext) == ptrToData->getID());
  BOOST_TEST(fixture.getProvidedDataID(dataContext) != ptrFromData->getID());
  BOOST_TEST(dataContext.getMeshID() == ptrToMesh->getID());
  BOOST_TEST(dataContext.getMeshID() != ptrFromMesh->getID());
  BOOST_TEST(!fixture.hasWriteMapping(dataContext));
  BOOST_TEST(fixture.hasReadMapping(dataContext));
  BOOST_TEST(fixture.mappingContext(dataContext).fromMeshID == mappingContext.fromMeshID);
  BOOST_TEST(fixture.mappingContext(dataContext).toMeshID == mappingContext.toMeshID);
  BOOST_TEST(fixture.mappingContext(dataContext).hasMappedData == mappingContext.hasMappedData);
  BOOST_TEST(fixture.mappingContext(dataContext).mapping == mappingContext.mapping);
  BOOST_TEST(fixture.mappingContext(dataContext).timing == mappingContext.timing);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
