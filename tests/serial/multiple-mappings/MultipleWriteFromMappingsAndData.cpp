#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(MultipleMappings)
BOOST_AUTO_TEST_CASE(MultipleWriteFromMappingsAndData)
{
  PRECICE_TEST("A"_on(1_rank), "B"_on(1_rank));

  using Eigen::Vector2d;
  using namespace precice::constants;

  precice::SolverInterface interface(context.name, context.config(), context.rank, context.size);
  Vector2d                 vertex1{0.0, 0.0};
  Vector2d                 vertex2{2.0, 0.0};
  Vector2d                 vertex3{4.0, 0.0};

  if (context.isNamed("A")) {
    const precice::MeshID meshIDTop      = interface.getMeshID("MeshATop");
    const precice::MeshID meshIDBottom   = interface.getMeshID("MeshABottom");
    int                   vertexIDTop    = interface.setMeshVertex(meshIDTop, vertex1.data());
    int                   vertexIDBottom = interface.setMeshVertex(meshIDBottom, vertex3.data());
    int                   dataIDTopP     = interface.getDataID("Pressure", meshIDTop);
    int                   dataIDBottomP  = interface.getDataID("Pressure", meshIDBottom);
    int                   dataIDTopT     = interface.getDataID("Temperature", meshIDTop);
    int                   dataIDBottomT  = interface.getDataID("Temperature", meshIDBottom);

    double dt = interface.initialize();
    interface.advance(dt);
    double pressure    = -1.0;
    double temperature = -1.0;
    interface.readScalarData(dataIDTopP, vertexIDTop, pressure);
    interface.readScalarData(dataIDTopT, vertexIDTop, temperature);
    BOOST_TEST(pressure == 1.0);
    BOOST_TEST(temperature == 331);
    pressure    = -1.0;
    temperature = -1.0;
    interface.readScalarData(dataIDBottomP, vertexIDBottom, pressure);
    interface.readScalarData(dataIDBottomT, vertexIDBottom, temperature);
    BOOST_TEST(temperature == 273.15);
    BOOST_TEST(pressure == 5.0);
    BOOST_TEST(not interface.isCouplingOngoing());
    interface.finalize();

  } else {
    BOOST_TEST(context.isNamed("B"));
    const precice::MeshID meshID    = interface.getMeshID("MeshB");
    int                   vertexID1 = interface.setMeshVertex(meshID, vertex1.data());
    int                   vertexID2 = interface.setMeshVertex(meshID, vertex2.data());
    int                   vertexID3 = interface.setMeshVertex(meshID, vertex3.data());
    int                   dataIDP   = interface.getDataID("Pressure", meshID);
    int                   dataIDT   = interface.getDataID("Temperature", meshID);

    double dt          = interface.initialize();
    double pressure    = 1.0;
    double temperature = 331;
    interface.writeScalarData(dataIDP, vertexID1, pressure);
    interface.writeScalarData(dataIDT, vertexID1, temperature);
    pressure    = 4.0;
    temperature = 335;
    interface.writeScalarData(dataIDP, vertexID2, pressure);
    interface.writeScalarData(dataIDT, vertexID2, temperature);
    pressure    = 5.0;
    temperature = 273.15;
    interface.writeScalarData(dataIDP, vertexID3, pressure);
    interface.writeScalarData(dataIDT, vertexID3, temperature);
    interface.advance(dt);
    BOOST_TEST(not interface.isCouplingOngoing());
    interface.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // MultipleMappings

#endif // PRECICE_NO_MPI
