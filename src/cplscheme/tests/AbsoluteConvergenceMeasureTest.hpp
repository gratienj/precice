#ifndef PRECICE_CPLSCHEME_TESTS_ABSOLUTECONVERGENCEMEASURETEST_HPP_
#define PRECICE_CPLSCHEME_TESTS_ABSOLUTECONVERGENCEMEASURETEST_HPP_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"

namespace precice {
namespace cplscheme {
namespace tests {

class AbsoluteConvergenceMeasureTest : public tarch::tests::TestCase
{
public:

  AbsoluteConvergenceMeasureTest ();

  virtual ~AbsoluteConvergenceMeasureTest () {};

  /**
   * @brief Empty.
   */
  virtual void setUp () {}

  virtual void run ();

private:

  static tarch::logging::Log _log;

  //   void testMeasureVectorData ();

  void testMeasureData ();

  //   void testMeasureIntegerData ();
};

}}} // namespace precice, cplscheme, tests

#endif // PRECICE_CPLSCHEME_TESTS_ABSOLUTECONVERGENCEMEASURETEST_HPP_
