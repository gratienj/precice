#include <boost/test/unit_test_log.hpp>
#ifndef PRECICE_NO_MPI

#include "helpers.hpp"
#include "testing/Testing.hpp"

#include "precice/SolverInterface.hpp"

/// Test to run simple "do nothing" coupling between two solvers.
void runTestExplicit(std::string const &configurationFileName, TestContext const &context)
{
  BOOST_TEST_MESSAGE("Config: " << configurationFileName);

  int    timesteps = 0;
  double time      = 0.0;

  SolverInterface couplingInterface(context.name, configurationFileName, 0, 1);

  // was necessary to replace pre-defined geometries
  if (context.isNamed("SolverOne")) {
    auto meshName = "MeshOne";
    BOOST_REQUIRE(couplingInterface.hasMesh(meshName));
    BOOST_REQUIRE(couplingInterface.getMeshDimensions(meshName) == 3);
    couplingInterface.setMeshVertex(meshName, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    couplingInterface.setMeshVertex(meshName, Eigen::Vector3d(1.0, 0.0, 0.0).data());
  }
  if (context.isNamed("SolverTwo")) {
    auto meshName = "Test-Square";
    BOOST_REQUIRE(couplingInterface.hasMesh(meshName));
    BOOST_REQUIRE(couplingInterface.getMeshDimensions(meshName) == 3);
    couplingInterface.setMeshVertex(meshName, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    couplingInterface.setMeshVertex(meshName, Eigen::Vector3d(1.0, 0.0, 0.0).data());
  }

  couplingInterface.initialize();
  double dt = couplingInterface.getMaxTimeStepSize();
  while (couplingInterface.isCouplingOngoing()) {
    time += dt;
    couplingInterface.advance(dt);
    dt = couplingInterface.getMaxTimeStepSize();
    timesteps++;
  }
  couplingInterface.finalize();

  BOOST_TEST(time == 10.0);
  BOOST_TEST(timesteps == 10);
}

#endif
