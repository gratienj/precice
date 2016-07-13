#ifndef PRECICE_IO_TESTS_EXPORTANDREIMPORTVRMLTEST_HPP_
#define PRECICE_IO_TESTS_EXPORTANDREIMPORTVRMLTEST_HPP_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"
#include <string>

namespace precice {
namespace io {
namespace tests {


/**
 * @brief Test class to test repeated exporting and importing in VRML.
 */
class ExportAndReimportVRMLTest : public tarch::tests::TestCase
{
public:

  /**
   * @brief Constructor.
   */
  ExportAndReimportVRMLTest();

  /**
   * @brief Destructor.
   */
  virtual ~ExportAndReimportVRMLTest() {}

  /**
   * @brief Empty.
   */
  virtual void setUp() {}

  /**
   * @brief Runs all test methods.
   */
  virtual void run();

private:

  static tarch::logging::Log _log;

  void testInternallyCreatedGeometry();

  void testReimportDriftRatchet();
};

}}} // namespace precice, io, tests

#endif /* PRECICE_IO_TESTS_EXPORTANDREIMPORTVRMLTEST_HPP_ */
