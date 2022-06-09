#ifndef PRECICE_NO_MPI

#include "helpers.hpp"
#include "math/geometry.hpp"
#include "precice/SolverInterface.hpp"
#include "precice/impl/SolverInterfaceImpl.hpp"
#include "testing/Testing.hpp"

void testQuadMappingScaledConsistent(const std::string configFile, const TestContext &context)
{
  using Eigen::Vector3d;

  const double z = 0.3;

  // MeshOne
  Vector3d coordOneA{0.0, 0.0, z};
  Vector3d coordOneB{1.0, 0.0, z};
  Vector3d coordOneC{0.999999999, 1.0, z}; // Forces diagonal 0-2 to be shorter.
  Vector3d coordOneD{0.0, 1.0, z};
  double   valOneA = 1.0;
  double   valOneB = 3.0;
  double   valOneC = 5.0;
  double   valOneD = 7.0;

  // MeshTwo
  Vector3d coordTwoA{0.0, 0.0, z + 0.1};               // Maps to vertex A
  Vector3d coordTwoB{0.0, 0.5, z - 0.01};              // Maps to edge AD
  Vector3d coordTwoC{2.0 / 3.0, 1.0 / 3.0, z + 0.001}; // Maps to triangle ABC

  double expectedIntegral = precice::math::geometry::triangleArea(coordOneA, coordOneB, coordOneC) * (valOneA + valOneB + valOneC) / 3.0 +
                            precice::math::geometry::triangleArea(coordOneA, coordOneC, coordOneD) * (valOneA + valOneC + valOneD) / 3.0;

  if (context.isNamed("SolverOne")) {
    precice::SolverInterface interface("SolverOne", configFile, 0, 1);
    // namespace is required because we are outside the fixture
    const int meshOneID = interface.getMeshID("MeshOne");

    // Setup mesh one.
    int idA = interface.setMeshVertex(meshOneID, coordOneA.data());
    int idB = interface.setMeshVertex(meshOneID, coordOneB.data());
    int idC = interface.setMeshVertex(meshOneID, coordOneC.data());
    int idD = interface.setMeshVertex(meshOneID, coordOneD.data());

    int idAB = interface.setMeshEdge(meshOneID, idA, idB);
    int idBC = interface.setMeshEdge(meshOneID, idB, idC);
    int idCD = interface.setMeshEdge(meshOneID, idC, idD);
    int idDA = interface.setMeshEdge(meshOneID, idD, idA);

    interface.setMeshQuad(meshOneID, idAB, idBC, idCD, idDA);

    auto &mesh = testing::WhiteboxAccessor::impl(interface).mesh("MeshOne");
    BOOST_REQUIRE(mesh.vertices().size() == 4);
    BOOST_REQUIRE(mesh.edges().size() == 5);
    BOOST_REQUIRE(mesh.triangles().size() == 2);

    // Initialize, thus sending the mesh.
    double maxDt = interface.initialize();
    BOOST_TEST(interface.isCouplingOngoing(), "Sending participant should have to advance once!");

    // Write the data to be send.
    int dataAID = interface.getDataID("DataOne", meshOneID);
    interface.writeScalarData(dataAID, idA, valOneA);
    interface.writeScalarData(dataAID, idB, valOneB);
    interface.writeScalarData(dataAID, idC, valOneC);
    interface.writeScalarData(dataAID, idD, valOneD);

    // Advance, thus send the data to the receiving partner.
    interface.advance(maxDt);
    BOOST_TEST(!interface.isCouplingOngoing(), "Sending participant should have to advance once!");
    interface.finalize();
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    precice::SolverInterface interface("SolverTwo", configFile, 0, 1);
    // namespace is required because we are outside the fixture
    int meshTwoID = interface.getMeshID("MeshTwo");

    // Setup receiving mesh.
    int idA = interface.setMeshVertex(meshTwoID, coordTwoA.data());
    int idB = interface.setMeshVertex(meshTwoID, coordTwoB.data());
    int idC = interface.setMeshVertex(meshTwoID, coordTwoC.data());

    int idAB = interface.setMeshEdge(meshTwoID, idA, idB);
    int idBC = interface.setMeshEdge(meshTwoID, idB, idC);
    int idAC = interface.setMeshEdge(meshTwoID, idA, idC);

    interface.setMeshTriangle(meshTwoID, idAB, idBC, idAC);

    // Initialize, thus receive the data and map.
    double maxDt = interface.initialize();
    BOOST_TEST(interface.isCouplingOngoing(), "Receiving participant should have to advance once!");

    // Read the mapped data from the mesh.
    int    dataAID = interface.getDataID("DataOne", meshTwoID);
    double valueA, valueB, valueC;
    interface.readScalarData(dataAID, idA, valueA);
    interface.readScalarData(dataAID, idB, valueB);
    interface.readScalarData(dataAID, idC, valueC);

    double calculatedIntegral = precice::math::geometry::triangleArea(coordTwoA, coordTwoB, coordTwoC) * (valueA + valueB + valueC) / 3.0;
    BOOST_TEST(expectedIntegral == calculatedIntegral);

    // Verify that there is only one time step necessary.
    interface.advance(maxDt);
    BOOST_TEST(!interface.isCouplingOngoing(), "Receiving participant should have to advance once!");
    interface.finalize();
  }
}

#endif
