#include "WatchPointTest.hpp"
#include "../impl/WatchPoint.hpp"
#include "mesh/Vertex.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/SharedPointer.hpp"
#include "geometry/Cuboid.hpp"
#include "utils/Parallel.hpp"
#include "utils/Globals.hpp"

#include "tarch/tests/TestCaseFactory.h"
registerTest(precice::tests::WatchPointTest)

namespace precice {
namespace tests {

tarch::logging::Log WatchPointTest:: _log ( "precice::tests::WatchPointTest" );

WatchPointTest:: WatchPointTest()
:
  tarch::tests::TestCase ( "tests::WachPointTest" )
{}

void WatchPointTest:: run ()
{
  PRECICE_MASTER_ONLY {
    preciceTrace ( "run()" );
    using namespace mesh;
    int dim = 2;
    using utils::DynVector;

    // Setup geometry
    std::string name ( "rectangle" );
    bool flipNormals = false;
    PtrMesh mesh ( new Mesh(name, dim, flipNormals) );
    DynVector offset(dim, 0.0);
    double discretizationWidth = 0.5;
    DynVector sidelengths(dim, 1.0);
    geometry::Cuboid rectangleGeometry ( offset, discretizationWidth, sidelengths );
    PtrData doubleData = mesh->createData ( "DoubleData", 1 );
    PtrData vectorData = mesh->createData ( "VectorData", dim );
    auto& doubleValues = doubleData->values();
    auto& vectorValues = vectorData->values();
    rectangleGeometry.create ( *mesh );

    // Create watchpoints
    DynVector pointToWatch0(dim, 1.0);
    std::string filename0 ( "tests-WatchPointTest-output0.txt" );
    impl::WatchPoint watchpoint0 ( pointToWatch0, mesh, filename0 );
    DynVector pointToWatch1(utils::Vector2D(1.0, 0.5));
    std::string filename1 ( "tests-WatchPointTest-output1.txt" );
    impl::WatchPoint watchpoint1 ( pointToWatch1, mesh, filename1 );

    // Initialize
    watchpoint0.initialize();
    watchpoint1.initialize();

    // Write output
    watchpoint0.exportPointData(0.0);
    watchpoint1.exportPointData(0.0);

    // Change geometry and write output again
    for ( mesh::Vertex& vertex : mesh->vertices() ) {
      validate ( vectorValues.size() > vertex.getID() );
      doubleValues[vertex.getID()] = 1.0;
    }
    watchpoint0.exportPointData(1.0);
    watchpoint1.exportPointData(1.0);
  }
}

}} // namespace precice, tests

