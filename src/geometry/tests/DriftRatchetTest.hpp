#ifndef PRECICE_GEOMETRY_TESTS_DRIFTRATCHETTEST_HPP_
#define PRECICE_GEOMETRY_TESTS_DRIFTRATCHETTEST_HPP_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"

namespace precice {
namespace geometry {
namespace tests {


class DriftRatchetTest : public tarch::tests::TestCase
{
public:

   static tarch::logging::Log _log;

   DriftRatchetTest ();

   virtual ~DriftRatchetTest () {};

   /**
    * @brief Empty.
    */
   virtual void setUp () {}

   virtual void run ();
};

}}} // namespace precice, geometry, tests

#endif // PRECICE_GEOMETRY_TESTS_DRIFTRATCHETTEST_HPP_
