#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(MultipleMappings)
BOOST_AUTO_TEST_CASE(MultipleReadFromMappings)
{
  PRECICE_TEST("A"_on(1_rank), "B"_on(1_rank));

  using Eigen::Vector2d;
  using namespace precice::constants;

  precice::SolverInterface interface(context.name, context.config(), context.rank, context.size);
  Vector2d                 vertex{0.0, 0.0};

  if (context.isNamed("A")) {
    const precice::MeshID meshIDTop      = interface.getMeshID("MeshATop");
    const precice::MeshID meshIDBottom   = interface.getMeshID("MeshABottom");
    int                   vertexIDTop    = interface.setMeshVertex(meshIDTop, vertex.data());
    int                   vertexIDBottom = interface.setMeshVertex(meshIDBottom, vertex.data());
    int                   dataIDTop      = interface.getDataID("Pressure", meshIDTop);
    int                   dataIDBottom   = interface.getDataID("Pressure", meshIDBottom);

    double dt = interface.initialize();
    interface.advance(dt);
    double pressure = -1.0;
    interface.readScalarData(dataIDTop, vertexIDTop, pressure);
    BOOST_TEST(pressure == 1.0);
    pressure = -1.0;
    interface.readScalarData(dataIDBottom, vertexIDBottom, pressure);
    BOOST_TEST(pressure == 1.0);
    BOOST_TEST(not interface.isCouplingOngoing());
    interface.finalize();

  } else {
    BOOST_TEST(context.isNamed("B"));
    const precice::MeshID meshID   = interface.getMeshID("MeshB");
    int                   vertexID = interface.setMeshVertex(meshID, vertex.data());
    int                   dataID   = interface.getDataID("Pressure", meshID);

    double dt       = interface.initialize();
    double pressure = 1.0;
    interface.writeScalarData(dataID, vertexID, pressure);
    interface.advance(dt);
    BOOST_TEST(not interface.isCouplingOngoing());
    interface.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // MultipleMappings

#endif // PRECICE_NO_MPI
