#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(MultipleMappings)
BOOST_AUTO_TEST_CASE(MultipleReadToMappings)
{
  PRECICE_TEST("A"_on(1_rank), "B"_on(1_rank));

  using Eigen::Vector2d;

  precice::SolverInterface interface(context.name, context.config(), context.rank, context.size);
  Vector2d                 vertex{0.0, 0.0};

  if (context.isNamed("A")) {
    auto meshNameTop    = "MeshATop";
    auto meshNameBottom = "MeshABottom";
    int  vertexIDTop    = interface.setMeshVertex(meshNameTop, vertex.data());
    int  vertexIDBottom = interface.setMeshVertex(meshNameBottom, vertex.data());
    auto dataNameTop    = "DisplacementTop";
    auto dataNameBottom = "DisplacementBottom";

    interface.initialize();
    double displacementTop = 1.0;
    interface.writeScalarData(meshNameTop, dataNameTop, vertexIDTop, displacementTop);
    double displacementBottom = 2.0;
    interface.writeScalarData(meshNameBottom, dataNameBottom, vertexIDBottom, displacementBottom);
    double dt = interface.getMaxTimeStepSize();
    interface.advance(dt);
    BOOST_TEST(not interface.isCouplingOngoing());
    interface.finalize();

  } else {
    BOOST_TEST(context.isNamed("B"));
    auto meshName = "MeshB";
    int  vertexID = interface.setMeshVertex(meshName, vertex.data());
    auto bottomID = "DisplacementBottom";
    auto topID    = "DisplacementTop";

    interface.initialize();
    double dt = interface.getMaxTimeStepSize();
    interface.advance(dt);
    dt                        = interface.getMaxTimeStepSize();
    double displacementTop    = -1.0;
    double displacementBottom = -3.0;
    interface.readScalarData(meshName, topID, vertexID, dt, displacementTop);
    BOOST_TEST(displacementTop == 1.0);
    interface.readScalarData(meshName, bottomID, vertexID, dt, displacementBottom);
    BOOST_TEST(displacementBottom == 2.0);
    BOOST_TEST(not interface.isCouplingOngoing());
    interface.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // MultipleMappings

#endif // PRECICE_NO_MPI
