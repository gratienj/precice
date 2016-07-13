#ifndef PRECICE_IO_TESTS_EXPORTVRMLTEST_HPP_
#define PRECICE_IO_TESTS_EXPORTVRMLTEST_HPP_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"
#include <string>

namespace precice {
namespace io {
namespace tests {

/**
 * @brief Provides tests for call precice::io::ExportVRML.
 */
class ExportVRMLTest : public tarch::tests::TestCase
{
public:

  /**
   * @brief Constructor.
   */
  ExportVRMLTest();

  /**
   * @brief Destructor, empty.
   */
  virtual ~ExportVRMLTest() {}

  /**
   * @brief Empty.
   */
  virtual void setUp() {}

  /**
   * @brief Calls all test methods.
   */
  virtual void run();

private:

  // @brief Logging device.
  static tarch::logging::Log _log;

  /**
   * @brief Tests exporting the drift ratchet mesh.
   */
  void testExportDriftRatchet();

  /**
   * @brief Tests exporting a cuboid mesh.
   */
  void testExportCuboid();
};

}}} // namespace precice, io, tests

#endif // PRECICE_IO_TESTS_EXPORTVRMLTEST_HPP_
