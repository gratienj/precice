#ifndef PRECICE_NO_MPI

#include <Eigen/Core>
#include <algorithm>
#include <deque>
#include <fstream>
#include <istream>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "action/RecorderAction.hpp"
#include "logging/LogMacros.hpp"
#include "math/constants.hpp"
#include "math/geometry.hpp"
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/SharedPointer.hpp"
#include "mesh/Utils.hpp"
#include "mesh/Vertex.hpp"
#include "precice/SolverInterface.hpp"
#include "precice/impl/MeshContext.hpp"
#include "precice/impl/Participant.hpp"
#include "precice/impl/SharedPointer.hpp"
#include "precice/impl/SolverInterfaceImpl.hpp"
#include "precice/types.hpp"
#include "testing/TestContext.hpp"
#include "testing/Testing.hpp"

using namespace precice;
using precice::testing::TestContext;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(MappingNearestNeighborGradient)

// Bidirectional test : Read: Vector & NN - Write: Scalar & NNG (Parallel coupling)
BOOST_AUTO_TEST_CASE(GradientTestBidirectionalReadScalar)
{

  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));
  using Eigen::Vector3d;

  SolverInterface cplInterface(context.name, context.config(), 0, 1);
  if (context.isNamed("SolverOne")) {
    int      meshOneID = cplInterface.getMeshID("MeshOne");
    Vector3d vec1      = Vector3d::Constant(0.1);
    cplInterface.setMeshVertex(meshOneID, vec1.data());
    int dataAID = cplInterface.getDataID("DataOne", meshOneID);
    int dataBID = cplInterface.getDataID("DataTwo", meshOneID);

    double valueDataB = 0.0;
    double maxDt      = cplInterface.initialize();
    cplInterface.readScalarData(dataBID, 0, valueDataB);
    BOOST_TEST(valueDataB == 1.0);

    while (cplInterface.isCouplingOngoing()) {

      cplInterface.writeScalarData(dataAID, 0, 3.0);
      Vector3d valueGradDataA(1.0, 2.0, 3.0);
      BOOST_TEST(cplInterface.isGradientDataRequired(dataAID));
      cplInterface.writeScalarGradientData(dataAID, 0, valueGradDataA.data());

      maxDt = cplInterface.advance(maxDt);

      cplInterface.readScalarData(dataBID, 0, valueDataB);
      BOOST_TEST(valueDataB == 1.5);
    }
    cplInterface.finalize();

  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    int      meshTwoID = cplInterface.getMeshID("MeshTwo");
    Vector3d vec2      = Vector3d::Constant(0.0);
    cplInterface.setMeshVertex(meshTwoID, vec2.data());

    int dataAID = cplInterface.getDataID("DataOne", meshTwoID);
    int dataBID = cplInterface.getDataID("DataTwo", meshTwoID);
    BOOST_TEST(cplInterface.isGradientDataRequired(dataBID) == false);
    double valueDataB = 1.0;
    cplInterface.writeScalarData(dataBID, 0, valueDataB);

    //tell preCICE that data has been written and call initialize
    cplInterface.markActionFulfilled(precice::constants::actionWriteInitialData());
    double maxDt = cplInterface.initialize();

    while (cplInterface.isCouplingOngoing()) {
      cplInterface.writeScalarData(dataBID, 0, 1.5);

      maxDt = cplInterface.advance(maxDt);

      double valueDataA;
      cplInterface.readScalarData(dataAID, 0, valueDataA);
      BOOST_TEST(valueDataA == 2.4);
    }
    cplInterface.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // PRECICE_NO_MPI
