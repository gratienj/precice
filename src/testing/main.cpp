// Setup boost test
// Disable the auto generation of main()
#define BOOST_TEST_NO_MAIN
// Specify the overall name of test framework
#define BOOST_TEST_MODULE "preCICE Tests"

#include <boost/test/tools/fpc_tolerance.hpp>
#include <boost/test/tree/test_case_counter.hpp>
#include <boost/test/tree/traverse.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>

#include "com/SharedPointer.hpp"
#include "logging/LogConfiguration.hpp"
#include "utils/IntraComm.hpp"
#include "utils/Parallel.hpp"

namespace precice {
extern bool syncMode;
} // namespace precice

int countEnabledTests()
{
  using namespace boost::unit_test;
  test_case_counter tcc;
  traverse_test_tree(framework::master_test_suite(), tcc, true);
  return tcc.p_count;
}

void setupTolerance()
{
  static constexpr double tolerance = 1e-9;

  boost::test_tools::fpc_tolerance<double>() = tolerance;
  boost::test_tools::fpc_tolerance<float>()  = tolerance;
}

/// Entry point for the boost test executable
int main(int argc, char *argv[])
{
  using namespace precice;

  precice::syncMode = false;
  utils::Parallel::initializeMPI(&argc, &argv);
  const auto rank = utils::Parallel::current()->rank();
  const auto size = utils::Parallel::current()->size();
  logging::setMPIRank(rank);

  if (size < 4 && argc < 2) {
    if (rank == 0) {
      std::cerr << "ERROR: The tests require at least 4 MPI processes. Please use \"mpirun -np 4 ./testprecice\" or \"ctest\" to run the full testsuite. \n";
    }
    return 2;
  }

  std::cout << "This test suite runs on rank " << rank << " of " << size << '\n';

  setupTolerance();
  int       retCode  = boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
  const int testsRan = countEnabledTests();

  // Override the return code if the secondary ranks have nothing to test
  if ((testsRan == 0) && (rank != 0)) {
    retCode = EXIT_SUCCESS;
  }

  utils::IntraComm::getCommunication() = nullptr;
  utils::Parallel::finalizeMPI();
  return retCode;
}
