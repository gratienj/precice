#ifndef PRECICE_ITESTS_COUPLINGINTERFACETESTREMOTE_HPP_
#define PRECICE_ITESTS_COUPLINGINTERFACETESTREMOTE_HPP_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"
#include <string>
#include "precice/impl/SharedPointer.hpp"
#include "precice/SolverInterface.hpp"

namespace precice {
namespace tests {

class SolverInterfaceTestRemote : public tarch::tests::TestCase
{
public:

  /**
   * @brief Constructor.
   */
  SolverInterfaceTestRemote();

  /**
   * @brief Destructor.
   */
  virtual ~SolverInterfaceTestRemote();

  /**
   * @brief Retrieves path to test directory.
   */
  virtual void setUp();

  /**
   * @brief Runs all tests.
   */
  virtual void run();

private:

  // @brief Logging device.
  static tarch::logging::Log _log;

  // @brief Path to this directory.
  std::string _pathToTests;

  /**
   * @brief As SolverInterface::configure(), but without changing logging config.
   */
  void configureSolverInterface (
    const std::string& configFilename,
    SolverInterface&   interface );

  /**
   * @brief Runs the solver interface in geometry mode with server.
   */
  void testGeometryMode();

  /**
   * @brief Two processes run the solver interface in geo-mode with server.
   */
  void testGeometryModeParallel();

  /**
   * @brief As testGeometryModeParallel() but with a stationary mapping.
   */
  void testGeometryModeParallelStationaryMapping();

  /**
   * @brief Runs two solver interfaces in coupling mode, one using a server.
   */
  void testCouplingModeWithOneServer();

  /**
   * @brief Two solvers in coupling mode, one in parallel using a server.
   */
  void testCouplingModeParallelWithOneServer();
};

}} // namespace precice, tests

#endif /* PRECICE_ITESTS_COUPLINGINTERFACETESTREMOTE_HPP_ */
