#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/Participant.hpp>
#include <vector>
#include "testing/TestContext.hpp"

using namespace precice;
using precice::testing::TestContext;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(GeomultiscaleMapping)
BOOST_AUTO_TEST_CASE(RadialGeoMultiscale)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));
  using Eigen::Vector3d;

  Participant cplInterface(context.name, context.config(), 0, 1);
  if (context.isNamed("SolverOne")) {
    Vector3d            coordOneA{0.0, 0.0, 0.0};
    Vector3d            coordOneB{0.0, 0.0, 1.0};
    Vector3d            coordOneC{0.0, 0.0, 2.0};
    std::vector<double> values;
    const unsigned int  nCoords = 3;
    for (unsigned int i = 0; i < nCoords; ++i) {
      values.emplace_back(std::pow(i + 1, 2));
    }
    auto             meshOneID = "MeshOne";
    std::vector<int> ids;
    ids.emplace_back(cplInterface.setMeshVertex(meshOneID, coordOneA));
    ids.emplace_back(cplInterface.setMeshVertex(meshOneID, coordOneB));
    ids.emplace_back(cplInterface.setMeshVertex(meshOneID, coordOneC));

    auto dataAID = "DataOne";

    cplInterface.initialize();
    double maxDt = cplInterface.getMaxTimeStepSize();
    BOOST_TEST(cplInterface.isCouplingOngoing());

    cplInterface.writeData(meshOneID, dataAID, ids, values);
    cplInterface.advance(maxDt);

    cplInterface.finalize();

    std::cout << "This is done 1 \n";

  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    auto meshTwoID = "MeshTwo";

    Vector3d coordTwoA{0.5, 0.0, 0.0};
    Vector3d coordTwoB{0.5, 0.0, 1.0};
    Vector3d coordTwoC{0.5, 0.0, 2.0};

    // Setup receiving mesh.
    int idA = cplInterface.setMeshVertex(meshTwoID, coordTwoA);
    int idB = cplInterface.setMeshVertex(meshTwoID, coordTwoB);
    int idC = cplInterface.setMeshVertex(meshTwoID, coordTwoC);

    auto dataAID = "DataOne";
    BOOST_REQUIRE(cplInterface.requiresInitialData());
    BOOST_TEST(cplInterface.requiresGradientDataFor(meshTwoID, dataAID) == false);

    cplInterface.initialize();
    double maxDt = cplInterface.getMaxTimeStepSize();

    double values[3];
    int    ids[] = {idA, idB, idC};

    cplInterface.readData(meshTwoID, dataAID, ids, maxDt, values);
    cplInterface.advance(maxDt);

    BOOST_TEST(values[0] == 1);
    BOOST_TEST(values[1] == 4);
    BOOST_TEST(values[2] == 9);

    cplInterface.finalize();

    std::cout << "And this is done 2 \n";
  }
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // GeomultiscaleMapping

#endif // PRECICE_NO_MPI