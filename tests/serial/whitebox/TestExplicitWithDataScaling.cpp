#ifndef PRECICE_NO_MPI

#include "precice/impl/SolverInterfaceImpl.hpp"
#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(Whitebox)
/**
 * @brief Runs a coupled sim. with data scaling applied.
 *
 * SolverOne writes vector data on a cube geometry. The data values are defined
 * and stay constant over the coupling cycles. SolverTwo has a scaling of the
 * values activated and reads the scaled values.
 */
BOOST_AUTO_TEST_CASE(TestExplicitWithDataScaling)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));

  SolverInterface cplInterface(context.name, context.config(), 0, 1);
  BOOST_TEST(cplInterface.getDimensions() == 2);

  std::vector<double> positions = {0.0, 0.0, 0.0, 0.1, 0.1, 0.1, 0.1, 0.0};
  std::vector<int>    ids       = {0, 0, 0, 0};

  if (context.isNamed("SolverOne")) {
    MeshID meshID = cplInterface.getMeshID("Test-Square-One");
    cplInterface.setMeshVertices(meshID, 4, positions.data(), ids.data());
    for (int i = 0; i < 4; i++)
      cplInterface.setMeshEdge(meshID, ids.at(i), ids.at((i + 1) % 4));

    double dt = cplInterface.initialize();

    int velocitiesID = cplInterface.getDataID("Velocities", meshID);
    while (cplInterface.isCouplingOngoing()) {
      for (size_t i = 0; i < testing::WhiteboxAccessor::impl(cplInterface).mesh("Test-Square-One").vertices().size(); ++i) {
        Eigen::Vector2d data = Eigen::Vector2d::Constant(i);
        cplInterface.writeVectorData(velocitiesID, i, data.data());
      }
      dt = cplInterface.advance(dt);
    }
    cplInterface.finalize();
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    MeshID meshID = cplInterface.getMeshID("Test-Square-Two");
    cplInterface.setMeshVertices(meshID, 4, positions.data(), ids.data());
    for (int i = 0; i < 4; i++)
      cplInterface.setMeshEdge(meshID, ids.at(i), ids.at((i + 1) % 4));

    double dt = cplInterface.initialize();

    int velocitiesID = cplInterface.getDataID("Velocities", meshID);
    while (cplInterface.isCouplingOngoing()) {
      const auto size = testing::WhiteboxAccessor::impl(cplInterface).mesh("Test-Square-Two").vertices().size();
      for (size_t i = 0; i < size; ++i) {
        Eigen::Vector2d readData;
        cplInterface.readVectorData(velocitiesID, i, readData.data());
        Eigen::Vector2d expectedData = Eigen::Vector2d::Constant(i * 10.0);
        BOOST_TEST(readData(0) == expectedData(0));
        BOOST_TEST(readData(1) == expectedData(1));
      }
      dt = cplInterface.advance(dt);
    }
    cplInterface.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // Whitebox

#endif // PRECICE_NO_MPI
