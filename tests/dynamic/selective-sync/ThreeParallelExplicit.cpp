#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/precice.hpp>
#include "../../serial/three-solvers/helpers.hpp"

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Dynamic)
BOOST_AUTO_TEST_SUITE(SelectiveSync)
BOOST_AUTO_TEST_CASE(ThreeParallelExplicit)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank), "SolverThree"_on(1_rank));
  std::string      config = context.config();
  std::vector<int> expectedCallsOfAdvance{10, 10, 10};
  runTestThreeSolvers(config, expectedCallsOfAdvance, context);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // ThreeSolvers

#endif // PRECICE_NO_MPI
