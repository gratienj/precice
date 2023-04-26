#ifndef PRECICE_NO_MPI

#include "helpers.hpp"
#include "testing/Testing.hpp"

#include "precice/SolverInterface.hpp"

/**
 * @brief helper function for a simple test with data initialization
 */
void testDataInitialization(precice::testing::TestContext context, std::string config)
{
  using Eigen::Vector3d;

  SolverInterface cplInterface(context.name, config, 0, 1);
  if (context.isNamed("SolverOne")) {
    auto     meshName = "MeshOne";
    Vector3d pos      = Vector3d::Zero();
    cplInterface.setMeshVertex(meshName, pos.data());
    auto   dataName   = "Data";
    double valueDataB = 0.0;
    cplInterface.initialize();
    double dt = cplInterface.getMaxTimeStepSize();
    cplInterface.readScalarData(meshName, dataName, 0, dt, valueDataB);
    BOOST_TEST(2.0 == valueDataB);
    cplInterface.finalize();
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    auto     meshName = "MeshTwo";
    Vector3d pos      = Vector3d::Zero();
    cplInterface.setMeshVertex(meshName, pos.data());

    //tell preCICE that data has been written
    BOOST_REQUIRE(cplInterface.requiresInitialData());

    auto dataName = "Data";
    cplInterface.writeScalarData(meshName, dataName, 0, 2.0);
    cplInterface.initialize();
    cplInterface.finalize();
  }
}

#endif
