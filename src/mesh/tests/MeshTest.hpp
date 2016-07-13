#ifndef PRECICE_MESH_TESTS_MESHTEST_HPP_
#define PRECICE_MESH_TESTS_MESHTEST_HPP_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"

namespace precice {
namespace mesh {
namespace tests {

/**
 * @brief Provides tests for class Mesh.
 */
class MeshTest : public tarch::tests::TestCase
{
public:

   /**
    * @brief Constructor.
    */
   MeshTest ();

   /**
    * @brief Destructor.
    */
   virtual ~MeshTest () {}

   /**
    * @brief Setup for tests, empty.
    */
   virtual void setUp () {}

   /**
    * @brief Calls all test methods.
    */
   virtual void run ();

private:

   // @brief Logging device.
   static tarch::logging::Log _log;

   /**
    * @brief Tests creation of a simple mesh with vertices, edges, and triangle.
    */
   void testBasicSetup ();

   /**
    * @brief Tests assigning geometry sub-IDs to the mesh elements.
    */
   void testSubIDs ();

   /**
    * @brief Tests method Mesh::computeState().
    */
   void testComputeState ();

   void testBoundingBoxCOG();

   /**
    * @brief Demonstrates the capabilities of class Mesh.
    */
   void testDemonstration ();

   /**
    * @brief Tests method Mesh::computeDistribution
    */
   void testDistribution();
};

}}} // namespace precice, mesh, tests

#endif // PRECICE_MESH_TESTS_MESHTEST_HPP_
