#ifndef PRECICE_NO_MPI
#include "testing/Testing.hpp"

#include "precice/SolverInterface.hpp"
#include "precice/config/Configuration.hpp"
#include "precice/impl/DataContext.hpp"
#include "precice/impl/MeshContext.hpp"
#include "precice/impl/Participant.hpp"
#include "precice/impl/SolverInterfaceImpl.hpp"
#include "utils/MasterSlave.hpp"
#include "utils/Parallel.hpp"

using namespace precice;

struct SerialTestFixture : testing::SlimConfigurator {

  std::string _pathToTests;

  void reset()
  {
    mesh::Data::resetDataCount();
    impl::Participant::resetParticipantCount();
    utils::MasterSlave::reset();
  }

  SerialTestFixture()
  {
    _pathToTests = testing::getPathToSources() + "/precice/tests/";
    reset();
  }
};

BOOST_AUTO_TEST_SUITE(PreciceTests)
BOOST_FIXTURE_TEST_SUITE(Serial, SerialTestFixture)

/// Test reading of a full features coupling configuration file.
BOOST_AUTO_TEST_CASE(TestConfiguration)
{
  std::string filename = _pathToTests + "/configuration.xml";
  // Test configuration for accessor "Peano"
  SolverInterface interfacePeano("Peano", 0, 1);
  slimConfigure(interfacePeano, filename);

  BOOST_TEST(impl(interfacePeano)._participants.size() == 2);
  BOOST_TEST(interfacePeano.getDimensions() == 2);

  impl::PtrParticipant peano = impl(interfacePeano)._participants[0];
  BOOST_TEST(peano);
  BOOST_TEST(peano->getName() == "Peano");
  BOOST_TEST(peano->getID() == 0);

  std::vector<impl::MeshContext *> meshContexts = peano->_meshContexts;
  BOOST_TEST(meshContexts.size() == 2);
  BOOST_TEST(peano->_usedMeshContexts.size() == 2);

  BOOST_TEST(meshContexts[0]->mesh->getName() == std::string("PeanoNodes"));
  BOOST_TEST(meshContexts[1]->mesh->getName() == std::string("ComsolNodes"));

  // Test configuration for accessor "Comsol"
  SolverInterface interfaceComsol("Comsol", 0, 1);
  slimConfigure(interfaceComsol, filename);
  BOOST_TEST(impl(interfaceComsol)._participants.size() == 2);
  BOOST_TEST(interfaceComsol.getDimensions() == 2);

  impl::PtrParticipant comsol = impl(interfaceComsol)._participants[1];
  BOOST_TEST(comsol);
  BOOST_TEST(comsol->getName() == "Comsol");
  BOOST_TEST(comsol->getID() == 1);

  meshContexts = comsol->_meshContexts;
  BOOST_TEST(meshContexts.size() == 2);
  BOOST_TEST(meshContexts[0] == static_cast<void *>(nullptr));
  BOOST_TEST(meshContexts[1]->mesh->getName() == std::string("ComsolNodes"));
  BOOST_TEST(comsol->_usedMeshContexts.size() == 1);
}

/// Test to run simple "do nothing" coupling between two solvers.
BOOST_AUTO_TEST_CASE(TestExplicit,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  std::vector<std::string> configs;
  configs.resize(3);
  configs[0] = _pathToTests + "explicit-mpi-single.xml";
  configs[1] = _pathToTests + "explicit-mpi.xml";
  configs[2] = _pathToTests + "explicit-sockets.xml";

  for (std::string configurationFileName : configs) {

    reset();

    BOOST_TEST_MESSAGE("Config: " << configurationFileName);

    std::string solverName;
    int         timesteps = 0;
    double      time      = 0.0;
    if (utils::Parallel::getProcessRank() == 0) {
      solverName = "SolverOne";
    } else {
      BOOST_TEST(utils::Parallel::getProcessRank() == 1);
      solverName = "SolverTwo";
    }

    SolverInterface couplingInterface(solverName, 0, 1);
    slimConfigure(couplingInterface, configurationFileName);

    //was necessary to replace pre-defined geometries
    if (solverName == "SolverOne" && couplingInterface.hasMesh("MeshOne")) {
      int meshID = couplingInterface.getMeshID("MeshOne");
      couplingInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
      couplingInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());
    }
    if (solverName == "SolverTwo" && couplingInterface.hasMesh("Test-Square")) {
      int meshID = couplingInterface.getMeshID("Test-Square");
      couplingInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
      couplingInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());
    }

    BOOST_TEST(couplingInterface.getDimensions() == 3);
    double dt = couplingInterface.initialize();
    while (couplingInterface.isCouplingOngoing()) {
      time += dt;
      dt = couplingInterface.advance(dt);
      timesteps++;
    }
    couplingInterface.finalize();

    BOOST_TEST(time == 10.0);
    BOOST_TEST(timesteps == 10);
  }
}

/// Test to run a simple "do nothing" coupling with subcycling solvers.
BOOST_AUTO_TEST_CASE(testExplicitWithSubcycling,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  if (utils::Parallel::getProcessRank() == 0) {
    SolverInterface precice("SolverOne", 0, 1);
    slimConfigure(precice, _pathToTests + "explicit-mpi-single.xml");

    double maxDt     = precice.initialize();
    int    timestep  = 0;
    double dt        = maxDt / 2.0; // Timestep length desired by solver
    double currentDt = dt;          // Timestep length used by solver
    while (precice.isCouplingOngoing()) {
      maxDt     = precice.advance(currentDt);
      currentDt = dt > maxDt ? maxDt : dt;
      timestep++;
    }
    precice.finalize();
    BOOST_TEST(timestep == 20);
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface precice("SolverTwo", 0, 1);
    slimConfigure(precice, _pathToTests + "explicit-mpi-single.xml");
    int meshID = precice.getMeshID("Test-Square");
    precice.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    precice.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());
    double maxDt     = precice.initialize();
    int    timestep  = 0;
    double dt        = maxDt / 3.0; // Timestep length desired by solver
    double currentDt = dt;          // Timestep length used by solver
    while (precice.isCouplingOngoing()) {
      maxDt     = precice.advance(currentDt);
      currentDt = dt > maxDt ? maxDt : dt;
      timestep++;
    }
    precice.finalize();
    BOOST_TEST(timestep == 30);
  }
}

/// One solver uses incremental position set, read/write methods.
BOOST_AUTO_TEST_CASE(testExplicitWithDataExchange,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  double counter = 0.0;
  using Eigen::Vector3d;

  if (utils::Parallel::getProcessRank() == 0) {
    SolverInterface cplInterface("SolverOne", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-mpi-single.xml");

    int meshOneID = cplInterface.getMeshID("MeshOne");
    /* int squareID = */ cplInterface.getMeshID("Test-Square");
    int forcesID     = cplInterface.getDataID("Forces", meshOneID);
    int velocitiesID = cplInterface.getDataID("Velocities", meshOneID);
    int indices[8];
    int i = 0;

    //need one vertex to start
    Vector3d vertex = Vector3d::Zero();
    cplInterface.setMeshVertex(meshOneID, vertex.data());
    double maxDt = cplInterface.initialize();

    const auto &vertices = impl(cplInterface).mesh("Test-Square").vertices();
    while (cplInterface.isCouplingOngoing()) {
      impl(cplInterface).resetMesh(meshOneID);
      i = 0;
      for (auto &vertex : vertices) {
        int index = cplInterface.setMeshVertex(meshOneID, vertex.getCoords().data());
        BOOST_TEST(index == vertex.getID());
        indices[i] = index;
        i++;
      }
      //for (VertexIterator it = vertices.begin(); it != vertices.end(); it++) {
      for (auto &vertex : vertices) {
        Vector3d force(Vector3d::Constant(counter) + vertex.getCoords());
        cplInterface.writeVectorData(forcesID, vertex.getID(), force.data());
      }
      maxDt = cplInterface.advance(maxDt);
      if (cplInterface.isCouplingOngoing()) {
        i = 0;
        for (auto &vertex : vertices) {
          Vector3d vel   = Vector3d::Zero();
          int      index = indices[i];
          i++;
          cplInterface.readVectorData(velocitiesID, index, vel.data());
          BOOST_TEST(vel == Vector3d::Constant(counter) + vertex.getCoords());
        }
        counter += 1.0;
      }
    }
    cplInterface.finalize();
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface cplInterface("SolverTwo", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-mpi-single.xml");

    int meshID = cplInterface.getMeshID("Test-Square");
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 1.0, 0.0).data());
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 1.0, 0.0).data());
    int    forcesID     = cplInterface.getDataID("Forces", meshID);
    int    velocitiesID = cplInterface.getDataID("Velocities", meshID);
    double maxDt        = cplInterface.initialize();
    auto & vertices     = impl(cplInterface).mesh("Test-Square").vertices();
    // SolverTwo does not start the coupled simulation and has, hence,
    // already received the first data to be validated.
    for (auto &vertex : vertices) {
      Vector3d force = Vector3d::Zero();
      cplInterface.readVectorData(forcesID, vertex.getID(), force.data());
      BOOST_TEST(force == Vector3d::Constant(counter) + vertex.getCoords());
    }
    counter += 1.0;

    while (cplInterface.isCouplingOngoing()) {
      for (auto &vertex : vertices) {
        Vector3d vel(Vector3d::Constant(counter - 1.0) + vertex.getCoords());
        cplInterface.writeVectorData(velocitiesID, vertex.getID(), vel.data());
      }
      maxDt = cplInterface.advance(maxDt);
      if (cplInterface.isCouplingOngoing()) {
        for (auto &vertex : vertices) {
          Vector3d force = Vector3d::Zero();
          cplInterface.readVectorData(forcesID, vertex.getID(), force.data());
          BOOST_TEST(force == Vector3d::Constant(counter) + vertex.getCoords());
        }
        counter += 1.0;
      }
    }
    cplInterface.finalize();
  }
}

/**
 * @brief The second solver initializes the data of the first.
 *
 * A mapping is employed for the second solver, i.e., at the end of
 * initializeData(), the mapping needs to be invoked.
 */
BOOST_AUTO_TEST_CASE(testExplicitWithDataInitialization,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  using Eigen::Vector3d;

  if (utils::Parallel::getProcessRank() == 0) {
    SolverInterface cplInterface("SolverOne", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-data-init.xml");

    int meshOneID = cplInterface.getMeshID("MeshOne");
    cplInterface.setMeshVertex(meshOneID, Vector3d(1.0, 2.0, 3.0).data());
    double maxDt      = cplInterface.initialize();
    int    dataAID    = cplInterface.getDataID("DataOne", meshOneID);
    int    dataBID    = cplInterface.getDataID("DataTwo", meshOneID);
    double valueDataB = 0.0;
    cplInterface.initializeData();
    cplInterface.readScalarData(dataBID, 0, valueDataB);
    BOOST_TEST(2.0 == valueDataB);
    while (cplInterface.isCouplingOngoing()) {
      Vector3d valueDataA(1.0, 1.0, 1.0);
      cplInterface.writeVectorData(dataAID, 0, valueDataA.data());
      maxDt = cplInterface.advance(maxDt);
      cplInterface.readScalarData(dataBID, 0, valueDataB);
      BOOST_TEST(2.5 == valueDataB);
    }
    cplInterface.finalize();
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface cplInterface("SolverTwo", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-data-init.xml");

    int      meshTwoID = cplInterface.getMeshID("MeshTwo");
    Vector3d pos       = Vector3d::Zero();
    cplInterface.setMeshVertex(meshTwoID, pos.data());
    double maxDt   = cplInterface.initialize();
    int    dataAID = cplInterface.getDataID("DataOne", meshTwoID);
    int    dataBID = cplInterface.getDataID("DataTwo", meshTwoID);
    cplInterface.writeScalarData(dataBID, 0, 2.0);
    //sagen dass daten jetzt geschrieben
    cplInterface.fulfilledAction(precice::constants::actionWriteInitialData());
    cplInterface.initializeData();
    Vector3d valueDataA;
    cplInterface.readVectorData(dataAID, 0, valueDataA.data());
    Vector3d expected(1.0, 1.0, 1.0);
    BOOST_TEST(valueDataA == expected);
    while (cplInterface.isCouplingOngoing()) {
      cplInterface.writeScalarData(dataBID, 0, 2.5);
      maxDt = cplInterface.advance(maxDt);
      cplInterface.readVectorData(dataAID, 0, valueDataA.data());
      BOOST_TEST(valueDataA == expected);
    }
    cplInterface.finalize();
  }
}

/// One solver uses block set/get/read/write methods.
BOOST_AUTO_TEST_CASE(testExplicitWithBlockDataExchange,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  double counter = 0.0;
  using Eigen::Vector3d;

  if (utils::Parallel::getProcessRank() == 0) {
    SolverInterface cplInterface("SolverOne", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-mpi-single-non-inc.xml");

    int             meshOneID      = cplInterface.getMeshID("MeshOne");
    double          maxDt          = cplInterface.initialize();
    int             forcesID       = cplInterface.getDataID("Forces", meshOneID);
    int             pressuresID    = cplInterface.getDataID("Pressures", meshOneID);
    int             velocitiesID   = cplInterface.getDataID("Velocities", meshOneID);
    int             temperaturesID = cplInterface.getDataID("Temperatures", meshOneID);
    auto &          vertices       = impl(cplInterface).mesh("Test-Square").vertices();
    int             size           = vertices.size();
    Eigen::VectorXd writePositions(size * 3);
    Eigen::VectorXd getWritePositions(size * 3);
    Eigen::VectorXd forces(size * 3);
    Eigen::VectorXd pressures(size);
    Eigen::VectorXi writeIDs(size);
    Eigen::VectorXi getWriteIDs(size);
    Eigen::VectorXd readPositions(size * 3);
    Eigen::VectorXd getReadPositions(size * 3);
    Eigen::VectorXd velocities(size * 3);
    Eigen::VectorXd temperatures(size);
    Eigen::VectorXd expectedVelocities(size * 3);
    Eigen::VectorXd expectedTemperatures(size);
    Eigen::VectorXi readIDs(size);
    Eigen::VectorXi getReadIDs(size);

    while (cplInterface.isCouplingOngoing()) {
      impl(cplInterface).resetMesh(meshOneID);
      for (auto &vertex : vertices) {
        for (int dim = 0; dim < 3; dim++) {
          writePositions[vertex.getID() * 3 + dim] = vertex.getCoords()[dim];
        }
      }
      cplInterface.setMeshVertices(meshOneID, size, writePositions.data(),
                                   writeIDs.data());
      for (auto &vertex : vertices) {
        // Vector3d force ( Vector3D(counter) + wrap<3,double>(vertex.getCoords()) );
        Vector3d force(Vector3d::Constant(counter) + vertex.getCoords());
        for (int dim = 0; dim < 3; dim++)
          forces[vertex.getID() * 3 + dim] = force[dim];
        pressures[vertex.getID()] = counter + vertex.getCoords()[0];
      }
      cplInterface.writeBlockVectorData(forcesID, size, writeIDs.data(), forces.data());
      cplInterface.writeBlockScalarData(pressuresID, size, writeIDs.data(), pressures.data());

      cplInterface.getMeshVertices(meshOneID, size, writeIDs.data(),
                                   getWritePositions.data());
      BOOST_TEST(writePositions == getWritePositions);

      cplInterface.getMeshVertexIDsFromPositions(meshOneID, size, writePositions.data(),
                                                 getWriteIDs.data());
      BOOST_TEST(writeIDs == getWriteIDs);
      //cplInterface.mapWrittenData(meshID);
      maxDt = cplInterface.advance(maxDt);
      if (cplInterface.isCouplingOngoing()) {
        for (auto &vertex : vertices) {
          for (int dim = 0; dim < 3; dim++) {
            int index                 = vertex.getID() * 3 + dim;
            readPositions[index]      = vertex.getCoords()[dim];
            expectedVelocities[index] = counter + vertex.getCoords()[dim];
          }
          expectedTemperatures[vertex.getID()] = counter + vertex.getCoords()[0];
        }
        impl(cplInterface).resetMesh(meshOneID);
        cplInterface.setMeshVertices(meshOneID, size, readPositions.data(), readIDs.data());
        cplInterface.mapReadDataTo(meshOneID);
        cplInterface.readBlockVectorData(velocitiesID, size, readIDs.data(),
                                         velocities.data());
        cplInterface.readBlockScalarData(temperaturesID, size, readIDs.data(),
                                         temperatures.data());
        BOOST_TEST(velocities == expectedVelocities);
        BOOST_TEST(temperatures == expectedTemperatures);

        counter += 1.0;
      }
    }
    cplInterface.finalize();
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface cplInterface("SolverTwo", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-mpi-single-non-inc.xml");

    int squareID       = cplInterface.getMeshID("Test-Square");
    int forcesID       = cplInterface.getDataID("Forces", squareID);
    int pressuresID    = cplInterface.getDataID("Pressures", squareID);
    int velocitiesID   = cplInterface.getDataID("Velocities", squareID);
    int temperaturesID = cplInterface.getDataID("Temperatures", squareID);
    int meshID         = cplInterface.getMeshID("Test-Square");
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 1.0, 0.0).data());
    cplInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 1.0, 0.0).data());
    double      maxDt    = cplInterface.initialize();
    const auto &vertices = impl(cplInterface).mesh("Test-Square").vertices();
    // SolverTwo does not start the coupled simulation and has, hence,
    // already received the first data to be validated.
    for (auto &vertex : vertices) {
      Vector3d force    = Vector3d::Zero();
      double   pressure = 0.0;
      cplInterface.readVectorData(forcesID, vertex.getID(), force.data());
      cplInterface.readScalarData(pressuresID, vertex.getID(), pressure);
      BOOST_TEST(force == Vector3d::Constant(counter) + vertex.getCoords());
      BOOST_TEST(pressure == counter + vertex.getCoords()[0]);
    }
    counter += 1.0;

    while (cplInterface.isCouplingOngoing()) {
      for (auto &vertex : vertices) {
        Vector3d vel(Vector3d::Constant(counter - 1.0) + vertex.getCoords());
        cplInterface.writeVectorData(velocitiesID, vertex.getID(), vel.data());
        double temperature = counter - 1.0 + vertex.getCoords()[0];
        cplInterface.writeScalarData(temperaturesID, vertex.getID(), temperature);
      }
      maxDt = cplInterface.advance(maxDt);
      if (cplInterface.isCouplingOngoing()) {
        for (auto &vertex : vertices) {
          Vector3d force    = Vector3d::Zero();
          double   pressure = 0.0;
          cplInterface.readVectorData(forcesID, vertex.getID(), force.data());
          cplInterface.readScalarData(pressuresID, vertex.getID(), pressure);
          BOOST_TEST(force == Vector3d::Constant(counter) + vertex.getCoords());
          BOOST_TEST(pressure == counter + vertex.getCoords()[0]);
        }
        counter += 1.0;
      }
    }
    cplInterface.finalize();
  }
}

/**
  * @brief Runs a coupled simulation where one solver supplies a geometry.
  *
  * SolverOne only reads the displacements of the geometry and checks whether
  * they are equals to the coordinates of SolverTwo. SolverTwo creates and
  * displaces the coordinates.
  */
BOOST_AUTO_TEST_CASE(testExplicitWithSolverGeometry,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  int    timesteps = 0;
  double time      = 0;

  if (utils::Parallel::getProcessRank() == 0) {

    SolverInterface couplingInterface("SolverOne", 0, 1);
    slimConfigure(couplingInterface, _pathToTests + "explicit-solvergeometry.xml");

    //was necessary to replace pre-defined geometries
    int meshID = couplingInterface.getMeshID("MeshOne");
    couplingInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    couplingInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());

    BOOST_TEST(couplingInterface.getDimensions() == 3);
    double dt = couplingInterface.initialize();
    while (couplingInterface.isCouplingOngoing()) {
      time += dt;
      dt = couplingInterface.advance(dt);
      timesteps++;
    }
    couplingInterface.finalize();
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface cplInterface("SolverTwo", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-solvergeometry.xml");

    BOOST_TEST(cplInterface.getDimensions() == 3);
    int meshID = cplInterface.getMeshID("SolverGeometry");
    int i0     = cplInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());
    int i1     = cplInterface.setMeshVertex(meshID, Eigen::Vector3d(1.0, 0.0, 0.0).data());
    int i2     = cplInterface.setMeshVertex(meshID, Eigen::Vector3d(0.0, 1.0, 0.0).data());
    int e0     = cplInterface.setMeshEdge(meshID, i0, i1);
    int e1     = cplInterface.setMeshEdge(meshID, i1, i2);
    int e2     = cplInterface.setMeshEdge(meshID, i2, i0);
    cplInterface.setMeshTriangle(meshID, e0, e1, e2);
    double dt = cplInterface.initialize();

    int size = cplInterface.getMeshVertexSize(meshID);
    BOOST_TEST(size == 3);

    while (cplInterface.isCouplingOngoing()) {
      time += dt;
      dt = cplInterface.advance(dt);
      timesteps++;
    }
    cplInterface.finalize();
  }
}

/**
 * @brief Runs a coupled sim. with data scaling applied.
 *
 * SolverOne writes vector data on a cube geometry. The data values are defined
 * and stay constant over the coupling cycles. SolverTwo has a scaling of the
 * values activated and reads the scaled values.
 */
BOOST_AUTO_TEST_CASE(testExplicitWithDataScaling,
                     *testing::Deleted() * testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  double dt;
  if (utils::Parallel::getProcessRank() == 0) { // SolverOne part
    SolverInterface cplInterface("SolverOne", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-datascaling.xml");

    BOOST_TEST(cplInterface.getDimensions() == 2);

    int                 meshID    = cplInterface.getMeshID("Test-Square");
    std::vector<double> positions = {0.0, 0.0, 0.1, 0.0, 0.2, 0.0, 0.3, 0.0, 0.4, 0.0};
    std::vector<int>    ids       = {0, 0, 0, 0, 0};
    cplInterface.setMeshVertices(meshID, 5, positions.data(), ids.data());
    for (int i = 0; i < 4; i++)
      cplInterface.setMeshEdge(meshID, ids[i], ids[i + 1]);

    dt = cplInterface.initialize();

    int velocitiesID = cplInterface.getDataID("Velocities", meshID);
    while (cplInterface.isCouplingOngoing()) {
      for (size_t i = 0; i < impl(cplInterface).mesh("Test-Square").vertices().size(); ++i) {
        Eigen::Vector2d data = Eigen::Vector2d::Constant(i);
        cplInterface.writeVectorData(velocitiesID, i, data.data());
      }
      dt = cplInterface.advance(dt);
    }
    cplInterface.finalize();

  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface cplInterface("SolverTwo", 0, 1);
    slimConfigure(cplInterface, _pathToTests + "explicit-datascaling.xml");

    BOOST_TEST(cplInterface.getDimensions() == 2);
    dt               = cplInterface.initialize();
    int meshID       = cplInterface.getMeshID("Test-Square");
    int velocitiesID = cplInterface.getDataID("Velocities", meshID);
    while (cplInterface.isCouplingOngoing()) {
      const auto size = impl(cplInterface).mesh("Test-Square").vertices().size();
      for (size_t i = 0; i < size; ++i) {
        Eigen::Vector2d readData;
        cplInterface.readVectorData(velocitiesID, i, readData.data());
        Eigen::Vector2d expectedData = Eigen::Vector2d::Constant(i * 10.0);
        BOOST_TEST(readData == expectedData, boost::test_tools::tolerance(5e-13));
      }
      dt = cplInterface.advance(dt);
    }
    cplInterface.finalize();
  }
}

/// Test simple coupled simulation with coupling iterations.
BOOST_AUTO_TEST_CASE(testImplicit,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  double state              = 0.0;
  double checkpoint         = 0.0;
  int    iterationCount     = 0;
  double initialStateChange = 5.0;
  double stateChange        = initialStateChange;
  int    computedTimesteps  = 0;
  using namespace precice::constants;

  if (utils::Parallel::getProcessRank() == 0) {
    SolverInterface couplingInterface("SolverOne", 0, 1);
    slimConfigure(couplingInterface, _pathToTests + "implicit.xml");

    int    meshID = couplingInterface.getMeshID("Square");
    double pos[3];
    // Set mesh positions
    pos[0] = 0.0;
    pos[1] = 0.0;
    pos[2] = 0.0;
    couplingInterface.setMeshVertex(meshID, pos);
    pos[0] = 1.0;
    pos[1] = 0.0;
    pos[2] = 0.0;
    couplingInterface.setMeshVertex(meshID, pos);
    pos[0] = 1.0;
    pos[1] = 1.0;
    pos[2] = 0.0;
    couplingInterface.setMeshVertex(meshID, pos);
    pos[0] = 0.0;
    pos[1] = 1.0;
    pos[2] = 0.0;
    couplingInterface.setMeshVertex(meshID, pos);

    double maxDt = couplingInterface.initialize();
    while (couplingInterface.isCouplingOngoing()) {
      if (couplingInterface.isActionRequired(actionWriteIterationCheckpoint())) {
        couplingInterface.fulfilledAction(actionWriteIterationCheckpoint());
        checkpoint     = state;
        iterationCount = 1;
      }
      if (couplingInterface.isActionRequired(actionReadIterationCheckpoint())) {
        couplingInterface.fulfilledAction(actionReadIterationCheckpoint());
        state = checkpoint;
      }
      iterationCount++;
      stateChange = initialStateChange / (double) iterationCount;
      state += stateChange;
      maxDt = couplingInterface.advance(maxDt);
      if (couplingInterface.isTimestepComplete()) {
        computedTimesteps++;
      }
    }
    couplingInterface.finalize();
    BOOST_TEST(computedTimesteps == 4);
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface couplingInterface("SolverTwo", 0, 1);
    slimConfigure(couplingInterface, _pathToTests + "implicit.xml");

    double maxDt = couplingInterface.initialize();
    while (couplingInterface.isCouplingOngoing()) {
      if (couplingInterface.isActionRequired(actionWriteIterationCheckpoint())) {
        couplingInterface.fulfilledAction(actionWriteIterationCheckpoint());
        checkpoint     = state;
        iterationCount = 1;
      }
      if (couplingInterface.isActionRequired(actionReadIterationCheckpoint())) {
        couplingInterface.fulfilledAction(actionReadIterationCheckpoint());
        state = checkpoint;
        iterationCount++;
      }
      stateChange = initialStateChange / (double) iterationCount;
      state += stateChange;
      maxDt = couplingInterface.advance(maxDt);
      if (couplingInterface.isTimestepComplete()) {
        computedTimesteps++;
      }
    }
    couplingInterface.finalize();
    BOOST_TEST(computedTimesteps == 4);
  }
}

/// Tests stationary mapping with solver provided meshes.
BOOST_AUTO_TEST_CASE(testStationaryMappingWithSolverMesh,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  std::string config2D = _pathToTests + "mapping-without-geo-2D.xml";
  std::string config3D = _pathToTests + "mapping-without-geo-3D.xml";
  int         rank     = utils::Parallel::getProcessRank();
  BOOST_TEST(((rank == 0) || (rank == 1)), rank);
  std::string solverName  = rank == 0 ? "SolverA" : "SolverB";
  std::string meshForcesA = "MeshForcesA";
  std::string meshDisplA  = "MeshDisplacementsA";
  std::string meshForcesB = "MeshForcesB";
  std::string meshDisplB  = "MeshDisplacementsB";
  std::string dataForces  = "Forces";
  std::string dataDispl   = "Displacements";
  using testing::equals;

  for (int dim : {2, 3}) {
    // @todo this should normally happen in finalize and should not be necessary
    mesh::Data::resetDataCount();
    impl::Participant::resetParticipantCount();
    SolverInterface interface(solverName, 0, 1);
    slimConfigure(interface, (dim == 2 ? config2D : config3D));
    BOOST_TEST(interface.getDimensions() == dim);

    std::vector<Eigen::VectorXd> positions;
    Eigen::VectorXd              position(dim);
    if (dim == 2) {
      position << 0.0, 0.0;
      positions.push_back(position);
      position << 1.0, 0.0;
      positions.push_back(position);
      position << 1.0, 1.0;
      positions.push_back(position);
      position << 0.0, 1.0;
      positions.push_back(position);
    } else {
      position << 0.0, 0.0, 0.0;
      positions.push_back(position);
      position << 1.0, 0.0, 0.0;
      positions.push_back(position);
      position << 1.0, 1.0, 0.0;
      positions.push_back(position);
      position << 0.0, 1.0, 1.0;
      positions.push_back(position);
      position << 0.0, 0.0, 1.0;
      positions.push_back(position);
    }
    size_t size = positions.size();

    if (rank == 0) {
      int meshForcesID = interface.getMeshID(meshForcesA);
      int meshDisplID  = interface.getMeshID(meshDisplA);
      int dataForcesID = interface.getDataID(dataForces, meshForcesID);
      int dataDisplID  = interface.getDataID(dataDispl, meshDisplID);

      // Set solver mesh positions for reading and writing data with mappings
      for (size_t i = 0; i < size; i++) {
        position = positions[i].array() + 0.1;
        interface.setMeshVertex(meshForcesID, position.data());
        position = positions[i].array() + 0.6;
        interface.setMeshVertex(meshDisplID, position.data());
      }
      double maxDt = interface.initialize();

      BOOST_TEST(interface.isWriteDataRequired(maxDt));
      BOOST_TEST(not interface.isReadDataAvailable());
      Eigen::VectorXd force = Eigen::VectorXd::Constant(dim, 1);
      Eigen::VectorXd displ = Eigen::VectorXd::Constant(dim, 0);
      for (size_t i = 0; i < size; i++) {
        interface.writeVectorData(dataForcesID, i, force.data());
      }
      interface.mapWriteDataFrom(meshForcesID);
      maxDt = interface.advance(maxDt);
      interface.mapReadDataTo(meshDisplID);

      BOOST_TEST(interface.isWriteDataRequired(maxDt));
      BOOST_TEST(interface.isReadDataAvailable());
      force.array() += 1.0;
      for (size_t i = 0; i < size; i++) {
        interface.readVectorData(dataDisplID, i, displ.data());
        BOOST_TEST(displ[0] == positions[i][0] + 0.1);
        interface.writeVectorData(dataForcesID, i, force.data());
      }
      interface.mapWriteDataFrom(meshForcesID);
      maxDt = interface.advance(maxDt);
      interface.mapReadDataTo(meshDisplID);

      BOOST_TEST(interface.isWriteDataRequired(maxDt));
      BOOST_TEST(interface.isReadDataAvailable());
      for (size_t i = 0; i < size; i++) {
        interface.readVectorData(dataDisplID, i, displ.data());
        BOOST_TEST(displ[0] == 2.0 * (positions[i][0] + 0.1));
      }
      interface.finalize();
    } else {
      BOOST_TEST(rank == 1, rank);
      int meshForcesID = interface.getMeshID(meshForcesB);
      int meshDisplID  = interface.getMeshID(meshDisplB);
      int dataForcesID = interface.getDataID(dataForces, meshForcesID);
      int dataDisplID  = interface.getDataID(dataDispl, meshDisplID);

      // Set solver mesh positions provided to SolverA for data mapping
      for (size_t i = 0; i < size; i++) {
        interface.setMeshVertex(meshForcesID, positions[i].data());
        position = positions[i].array() + 0.5;
        interface.setMeshVertex(meshDisplID, position.data());
      }
      double maxDt = interface.initialize();

      BOOST_TEST(interface.isWriteDataRequired(maxDt));
      BOOST_TEST(interface.isReadDataAvailable());
      Eigen::VectorXd force      = Eigen::VectorXd::Zero(dim);
      Eigen::VectorXd totalForce = Eigen::VectorXd::Zero(dim);
      Eigen::VectorXd displ      = Eigen::VectorXd::Zero(dim);
      for (size_t i = 0; i < size; i++) {
        interface.readVectorData(dataForcesID, i, force.data());
        totalForce += force;
        displ.setConstant(positions[i][0]);
        interface.writeVectorData(dataDisplID, i, displ.data());
      }
      Eigen::VectorXd expected = Eigen::VectorXd::Constant(dim, size);
      BOOST_TEST(equals(totalForce, expected));
      maxDt = interface.advance(maxDt);

      BOOST_TEST(interface.isWriteDataRequired(maxDt));
      BOOST_TEST(interface.isReadDataAvailable());
      totalForce.setConstant(0);
      for (size_t i = 0; i < positions.size(); i++) {
        interface.readVectorData(dataForcesID, i, force.data());
        totalForce += force;
        displ.setConstant(2.0 * positions[i][0]);
        interface.writeVectorData(dataDisplID, i, displ.data());
      }
      expected.setConstant(2.0 * (double) size);
      BOOST_TEST(equals(totalForce, expected));
      maxDt = interface.advance(maxDt);

      BOOST_TEST(interface.isWriteDataRequired(maxDt));
      BOOST_TEST(not interface.isReadDataAvailable()); //second participant has no new data after last advance
      for (size_t i = 0; i < size; i++) {
        interface.readVectorData(dataForcesID, i, force.data());
      }
      interface.finalize();
    }
  }
}

/**
 * @brief Buggy simulation setup of FSI coupling between Flite and Calculix.
 *
 * Bug: after first call of advance by Flite the mapped forces are value NaN.
 *
 * Some information about the coupling:
 * - explicit coupling scheme
 * - Flite (incompressible Navier-Stokes) starts simulation
 * - Mapping is done on Flite side with RBF
 */
BOOST_AUTO_TEST_CASE(testBug,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  using Eigen::Vector3d;
  std::string configName = _pathToTests + "bug.xml";

  int                   slices = 5;
  std::vector<Vector3d> coords;
  for (int i = 0; i < slices; i++) {
    double z = (double) i * 1.0;
    coords.push_back(Vector3d(1.0, 0.0, z));
    coords.push_back(Vector3d(0.0, 1.0, z));
    coords.push_back(Vector3d(-1.0, 0.0, z));
    coords.push_back(Vector3d(0.0, -1.0, z));
  }

  int rank = utils::Parallel::getProcessRank();
  BOOST_TEST(((rank == 0) || (rank == 1)), rank);
  std::string solverName = rank == 0 ? "Flite" : "Calculix";
  if (solverName == std::string("Flite")) {
    SolverInterface precice("Flite", 0, 1);
    slimConfigure(precice, configName);

    int meshID             = precice.getMeshID("FliteNodes");
    int forcesID           = precice.getDataID("Forces", meshID);
    int displacementsID    = precice.getDataID("Displacements", meshID);
    int oldDisplacementsID = precice.getDataID("OldDisplacements", meshID);
    BOOST_TEST(precice.getDimensions() == 3);
    for (Vector3d &coord : coords) {
      precice.setMeshVertex(meshID, coord.data());
    }
    double maxDt = precice.initialize();
    double dt    = 1.0e-5 / 15.0; // Flite took 15 subcycling steps
    while (precice.isCouplingOngoing()) {
      dt = dt < maxDt ? dt : maxDt;
      for (int i = 0; i < (int) coords.size(); i++) {
        double force[3] = {1.0, 2.0, 3.0};
        precice.writeVectorData(forcesID, i, force);
      }
      maxDt = precice.advance(dt);
      precice.mapReadDataTo(meshID);
      for (int i = 0; i < (int) coords.size(); i++) {
        double displacement[3];
        double oldDisplacement[3];
        precice.readVectorData(displacementsID, i, displacement);
        precice.readVectorData(oldDisplacementsID, i, oldDisplacement);
      }
    }
    precice.finalize();
  } else {
    BOOST_TEST(solverName == std::string("Calculix"), solverName);
    SolverInterface precice("Calculix", 0, 1);
    slimConfigure(precice, configName);

    int meshID = precice.getMeshID("CalculixNodes");
    for (Vector3d &coord : coords) {
      precice.setMeshVertex(meshID, coord.data());
    }
    for (int i = 0; i < slices - 1; i++) {
      // Build cylinder/channel geometry
      precice.setMeshTriangleWithEdges(meshID, i * 4, (i * 4) + 1, (i + 1) * 4);
      precice.setMeshTriangleWithEdges(meshID, (i + 1) * 4, (i * 4) + 1, ((i + 1) * 4) + 1);
      precice.setMeshTriangleWithEdges(meshID, i * 4 + 1, (i * 4) + 2, (i + 1) * 4 + 1);
      precice.setMeshTriangleWithEdges(meshID, (i + 1) * 4 + 1, (i * 4) + 2, ((i + 1) * 4) + 2);
      precice.setMeshTriangleWithEdges(meshID, i * 4 + 2, (i * 4) + 3, (i + 1) * 4 + 2);
      precice.setMeshTriangleWithEdges(meshID, (i + 1) * 4 + 2, (i * 4) + 3, ((i + 1) * 4) + 3);
      precice.setMeshTriangleWithEdges(meshID, i * 4 + 3, (i * 4), (i + 1) * 4 + 3);
      precice.setMeshTriangleWithEdges(meshID, (i + 1) * 4 + 3, i * 4, (i + 1) * 4);
    }
    double dt = precice.initialize();
    while (precice.isCouplingOngoing()) {
      precice.advance(dt);
    }
    precice.finalize();
  }
}

/**
 * @brief Three solvers are coupled in a fork S2 <-> S1 <-> S3.
 *
 * Both couplings are explicit, solver 1 provides the mesh to the other two
 * solvers.
 */
BOOST_AUTO_TEST_CASE(testThreeSolvers,
                     *testing::MinRanks(3) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1, 2})))
{
  if (utils::Parallel::getCommunicatorSize() != 3)
    return;

  int                      numberOfTests = 5;
  std::vector<std::string> configs;
  configs.resize(5);
  configs[0] = _pathToTests + "three-solver-explicit-explicit.xml";
  configs[1] = _pathToTests + "three-solver-implicit-implicit.xml";
  configs[2] = _pathToTests + "three-solver-implicit-explicit.xml";
  configs[3] = _pathToTests + "three-solver-explicit-implicit.xml";
  configs[4] = _pathToTests + "three-solver-parallel.xml";

  std::vector<std::vector<int>> expectedCallsOfAdvance;
  expectedCallsOfAdvance.resize(5);
  expectedCallsOfAdvance[0] = {10, 10, 10};
  expectedCallsOfAdvance[1] = {30, 30, 20};
  expectedCallsOfAdvance[2] = {30, 30, 10};
  expectedCallsOfAdvance[3] = {30, 10, 30};
  expectedCallsOfAdvance[4] = {30, 30, 10};

  for (int k = 0; k < numberOfTests; k++) {
    reset();

    int rank = utils::Parallel::getProcessRank();
    BOOST_TEST(((rank == 0) || (rank == 1) || (rank == 2)), rank);

    std::string writeIterCheckpoint(constants::actionWriteIterationCheckpoint());
    std::string readIterCheckpoint(constants::actionReadIterationCheckpoint());
    std::string writeInitData(constants::actionWriteInitialData());

    std::string solverName;
    if (rank == 0)
      solverName = std::string("SolverOne");
    else if (rank == 1)
      solverName = std::string("SolverTwo");
    else
      solverName = std::string("SolverThree");
    int callsOfAdvance = 0;

    if (solverName == std::string("SolverOne")) {
      SolverInterface precice(solverName, 0, 1);
      slimConfigure(precice, configs[k]);

      int meshAID = precice.getMeshID("MeshA");
      int meshBID = precice.getMeshID("MeshB");
      precice.setMeshVertex(meshAID, Eigen::Vector2d(0, 0).data());
      precice.setMeshVertex(meshBID, Eigen::Vector2d(1, 1).data());
      double dt = precice.initialize();

      if (precice.isActionRequired(writeInitData)) {
        precice.fulfilledAction(writeInitData);
      }
      precice.initializeData();

      while (precice.isCouplingOngoing()) {
        if (precice.isActionRequired(writeIterCheckpoint)) {
          precice.fulfilledAction(writeIterCheckpoint);
        }
        dt = precice.advance(dt);
        if (precice.isActionRequired(readIterCheckpoint)) {
          precice.fulfilledAction(readIterCheckpoint);
        }
        callsOfAdvance++;
      }
      precice.finalize();
      BOOST_TEST(callsOfAdvance == expectedCallsOfAdvance[k][0]);
    } else if (solverName == std::string("SolverTwo")) {
      SolverInterface precice(solverName, 0, 1);
      slimConfigure(precice, configs[k]);

      int meshID = precice.getMeshID("MeshC");
      precice.setMeshVertex(meshID, Eigen::Vector2d(0, 0).data());
      double dt = precice.initialize();

      if (precice.isActionRequired(writeInitData)) {
        precice.fulfilledAction(writeInitData);
      }
      precice.initializeData();

      while (precice.isCouplingOngoing()) {
        if (precice.isActionRequired(writeIterCheckpoint)) {
          precice.fulfilledAction(writeIterCheckpoint);
        }
        dt = precice.advance(dt);
        if (precice.isActionRequired(readIterCheckpoint)) {
          precice.fulfilledAction(readIterCheckpoint);
        }
        callsOfAdvance++;
      }
      precice.finalize();
      BOOST_TEST(callsOfAdvance == expectedCallsOfAdvance[k][1]);
    } else {
      BOOST_TEST(solverName == std::string("SolverThree"), solverName);
      SolverInterface precice(solverName, 0, 1);
      slimConfigure(precice, configs[k]);

      int meshID = precice.getMeshID("MeshD");
      precice.setMeshVertex(meshID, Eigen::Vector2d(0, 0).data());
      double dt = precice.initialize();

      if (precice.isActionRequired(writeInitData)) {
        precice.fulfilledAction(writeInitData);
      }
      precice.initializeData();

      while (precice.isCouplingOngoing()) {
        if (precice.isActionRequired(writeIterCheckpoint)) {
          precice.fulfilledAction(writeIterCheckpoint);
        }
        dt = precice.advance(dt);
        if (precice.isActionRequired(readIterCheckpoint)) {
          precice.fulfilledAction(readIterCheckpoint);
        }
        callsOfAdvance++;
      }
      precice.finalize();
      BOOST_TEST(callsOfAdvance == expectedCallsOfAdvance[k][2]);
    }
  }
}

/// Four solvers are multi-coupled.
BOOST_AUTO_TEST_CASE(testMultiCoupling, *testing::OnSize(4))
{
  std::vector<Eigen::Vector2d> positions;
  Eigen::Vector2d              position;
  position << 0.0, 0.0;
  positions.push_back(position);
  position << 1.0, 0.0;
  positions.push_back(position);
  position << 1.0, 1.0;
  positions.push_back(position);
  position << 0.0, 1.0;
  positions.push_back(position);

  std::vector<Eigen::Vector2d> datas;
  Eigen::Vector2d              data;
  data << 1.0, 1.0;
  datas.push_back(data);
  data << 2.0, 2.0;
  datas.push_back(position);
  data << 3.0, 3.0;
  datas.push_back(data);
  data << 4.0, 5.0;
  datas.push_back(data);

  std::string writeIterCheckpoint(constants::actionWriteIterationCheckpoint());
  std::string readIterCheckpoint(constants::actionReadIterationCheckpoint());

  if (utils::Parallel::getProcessRank() < 3) {
    int meshID      = -1;
    int dataWriteID = -1;
    int dataReadID  = -1;

    std::string participant = "";

    if (utils::Parallel::getProcessRank() == 0) {
      participant = "SOLIDZ1";
    } else if (utils::Parallel::getProcessRank() == 1) {
      participant = "SOLIDZ2";
    } else if (utils::Parallel::getProcessRank() == 2) {
      participant = "SOLIDZ3";
    }

    SolverInterface precice(participant, 0, 1);
    slimConfigure(precice, _pathToTests + "/multi.xml");
    BOOST_TEST(precice.getDimensions() == 2);

    if (utils::Parallel::getProcessRank() == 0) {
      meshID      = precice.getMeshID("SOLIDZ_Mesh1");
      dataWriteID = precice.getDataID("Displacements1", meshID);
      dataReadID  = precice.getDataID("Forces1", meshID);
    } else if (utils::Parallel::getProcessRank() == 1) {
      meshID      = precice.getMeshID("SOLIDZ_Mesh2");
      dataWriteID = precice.getDataID("Displacements2", meshID);
      dataReadID  = precice.getDataID("Forces2", meshID);
    } else if (utils::Parallel::getProcessRank() == 2) {
      meshID      = precice.getMeshID("SOLIDZ_Mesh3");
      dataWriteID = precice.getDataID("Displacements3", meshID);
      dataReadID  = precice.getDataID("Forces3", meshID);
    }

    std::vector<int> vertexIDs;
    int              vertexID = -1;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID, positions[i].data());
      vertexIDs.push_back(vertexID);
    }

    precice.initialize();

    for (size_t i = 0; i < 4; i++) {
      precice.writeVectorData(dataWriteID, vertexIDs[i], datas[i].data());
    }

    if (precice.isActionRequired(writeIterCheckpoint)) {
      precice.fulfilledAction(writeIterCheckpoint);
    }
    precice.advance(0.0001);
    if (precice.isActionRequired(readIterCheckpoint)) {
      precice.fulfilledAction(readIterCheckpoint);
    }

    for (size_t i = 0; i < 4; i++) {
      precice.readVectorData(dataReadID, vertexIDs[i], datas[i].data());
    }

    BOOST_TEST(datas[0][0] == 1.00000000000000002082e-03);
    BOOST_TEST(datas[0][1] == 1.00000000000000002082e-03);
    BOOST_TEST(datas[1][0] == 0.00000000000000000000e+00);
    BOOST_TEST(datas[1][1] == 1.00000000000000002082e-03);
    BOOST_TEST(datas[2][0] == 3.00000000000000006245e-03);
    BOOST_TEST(datas[2][1] == 3.00000000000000006245e-03);
    BOOST_TEST(datas[3][0] == 4.00000000000000008327e-03);
    BOOST_TEST(datas[3][1] == 5.00000000000000010408e-03);

    precice.finalize();

  } else {
    BOOST_TEST(utils::Parallel::getProcessRank() == 3);
    SolverInterface precice("NASTIN", 0, 1);
    slimConfigure(precice, _pathToTests + "/multi.xml");
    BOOST_TEST(precice.getDimensions() == 2);
    int meshID1      = precice.getMeshID("NASTIN_Mesh1");
    int meshID2      = precice.getMeshID("NASTIN_Mesh2");
    int meshID3      = precice.getMeshID("NASTIN_Mesh3");
    int dataWriteID1 = precice.getDataID("Forces1", meshID1);
    int dataWriteID2 = precice.getDataID("Forces2", meshID2);
    int dataWriteID3 = precice.getDataID("Forces3", meshID3);

    std::vector<int> vertexIDs1;
    int              vertexID = -1;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID1, positions[i].data());
      vertexIDs1.push_back(vertexID);
    }
    std::vector<int> vertexIDs2;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID2, positions[i].data());
      vertexIDs2.push_back(vertexID);
    }
    std::vector<int> vertexIDs3;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID3, positions[i].data());
      vertexIDs3.push_back(vertexID);
    }

    precice.initialize();

    for (size_t i = 0; i < 4; i++) {
      precice.writeVectorData(dataWriteID1, vertexIDs1[i], datas[i].data());
      precice.writeVectorData(dataWriteID2, vertexIDs2[i], datas[i].data());
      precice.writeVectorData(dataWriteID3, vertexIDs3[i], datas[i].data());
    }

    if (precice.isActionRequired(writeIterCheckpoint)) {
      precice.fulfilledAction(writeIterCheckpoint);
    }
    precice.advance(0.0001);
    if (precice.isActionRequired(readIterCheckpoint)) {
      precice.fulfilledAction(readIterCheckpoint);
    }

    precice.finalize();
  }
}

void testMappingNearestProjection(bool defineEdgesExplicitly, const std::string configFile)
{
  using Eigen::Vector3d;

  const double z = 0.3;

  // MeshOne
  Vector3d coordOneA{0.0, 0.0, z};
  Vector3d coordOneB{1.0, 0.0, z};
  Vector3d coordOneC{1.0, 1.0, z};
  Vector3d coordOneD{0.0, 1.0, z};
  double   valOneA = 1.0;
  double   valOneB = 3.0;
  double   valOneC = 5.0;
  double   valOneD = 7.0;

  // MeshTwo
  Vector3d coordTwoA{0.0, 0.0, z + 0.1};               // Maps to vertex A
  Vector3d coordTwoB{0.0, 0.5, z - 0.01};              // Maps to edge AD
  Vector3d coordTwoC{2.0 / 3.0, 1.0 / 3.0, z + 0.001}; // Maps to triangle ABC
  // This corresponds to the point C from mesh two on the triangle ABC on mesh one.
  Vector3d barycenterABC{0.3798734633239789, 0.24025307335204216, 0.3798734633239789};
  double   expectedValTwoA = 1.0;
  double   expectedValTwoB = 4.0;
  double   expectedValTwoC = Vector3d{valOneA, valOneB, valOneC}.dot(barycenterABC);

  if (utils::Parallel::getProcessRank() == 0) {
    SolverInterface cplInterface("SolverOne", 0, 1);
    // namespace is required because we are outside the fixture
    testing::SlimConfigurator::slimConfigure(cplInterface, configFile);
    const int meshOneID = cplInterface.getMeshID("MeshOne");

    // Setup mesh one.
    int idA = cplInterface.setMeshVertex(meshOneID, coordOneA.data());
    int idB = cplInterface.setMeshVertex(meshOneID, coordOneB.data());
    int idC = cplInterface.setMeshVertex(meshOneID, coordOneC.data());
    int idD = cplInterface.setMeshVertex(meshOneID, coordOneD.data());

    if (defineEdgesExplicitly) {

      int idAB = cplInterface.setMeshEdge(meshOneID, idA, idB);
      int idBC = cplInterface.setMeshEdge(meshOneID, idB, idC);
      int idCD = cplInterface.setMeshEdge(meshOneID, idC, idD);
      int idDA = cplInterface.setMeshEdge(meshOneID, idD, idA);
      int idCA = cplInterface.setMeshEdge(meshOneID, idC, idA);

      cplInterface.setMeshTriangle(meshOneID, idAB, idBC, idCA);
      cplInterface.setMeshTriangle(meshOneID, idCD, idDA, idCA);

    } else {
      cplInterface.setMeshTriangleWithEdges(meshOneID, idA, idB, idC);
      cplInterface.setMeshTriangleWithEdges(meshOneID, idC, idD, idA);
    }

    // Initialize, thus sending the mesh.
    double maxDt = cplInterface.initialize();
    BOOST_TEST(cplInterface.isCouplingOngoing(), "Sending participant should have to advance once!");

    // Write the data to be send.
    int dataAID = cplInterface.getDataID("DataOne", meshOneID);
    cplInterface.writeScalarData(dataAID, idA, valOneA);
    cplInterface.writeScalarData(dataAID, idB, valOneB);
    cplInterface.writeScalarData(dataAID, idC, valOneC);
    cplInterface.writeScalarData(dataAID, idD, valOneD);

    // Advance, thus send the data to the receiving partner.
    cplInterface.advance(maxDt);
    BOOST_TEST(!cplInterface.isCouplingOngoing(), "Sending participant should have to advance once!");
    cplInterface.finalize();
  } else if (utils::Parallel::getProcessRank() == 1) {
    SolverInterface cplInterface("SolverTwo", 0, 1);
    // namespace is required because we are outside the fixture
    testing::SlimConfigurator::slimConfigure(cplInterface, configFile);
    int meshTwoID = cplInterface.getMeshID("MeshTwo");

    // Setup receiving mesh.
    int idA = cplInterface.setMeshVertex(meshTwoID, coordTwoA.data());
    int idB = cplInterface.setMeshVertex(meshTwoID, coordTwoB.data());
    int idC = cplInterface.setMeshVertex(meshTwoID, coordTwoC.data());

    // Initialize, thus receive the data and map.
    double maxDt = cplInterface.initialize();
    BOOST_TEST(cplInterface.isCouplingOngoing(), "Receiving participant should have to advance once!");

    // Read the mapped data from the mesh.
    int    dataAID = cplInterface.getDataID("DataOne", meshTwoID);
    double valueA, valueB, valueC;
    cplInterface.readScalarData(dataAID, idA, valueA);
    cplInterface.readScalarData(dataAID, idB, valueB);
    cplInterface.readScalarData(dataAID, idC, valueC);

    BOOST_TEST(valueA == expectedValTwoA);
    BOOST_TEST(valueB == expectedValTwoB);
    BOOST_TEST(valueC == expectedValTwoC);

    // Verify that there is only one time step necessary.
    cplInterface.advance(maxDt);
    BOOST_TEST(!cplInterface.isCouplingOngoing(), "Receiving participant should have to advance once!");
    cplInterface.finalize();
  }
}

/**
 * @brief Tests the Nearest Projection Mapping between two participants with explicit definition of edges
 *
 */
BOOST_AUTO_TEST_CASE(testMappingNearestProjectionExplicitEdges,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;
  bool              defineEdgesExplicitly = true;
  const std::string configFile            = _pathToTests + "mapping-nearest-projection.xml";
  testMappingNearestProjection(defineEdgesExplicitly, configFile);
}

/**
 * @brief Tests the Nearest Projection Mapping between two participants with explicit definition of edges
 *
 */
BOOST_AUTO_TEST_CASE(testMappingNearestProjectionImplicitEdges,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;
  bool              defineEdgesExplicitly = false;
  const std::string configFile            = _pathToTests + "mapping-nearest-projection.xml";
  testMappingNearestProjection(defineEdgesExplicitly, configFile);
}

/**
 * @brief Tests sending one mesh to multiple participants
 *
 */
BOOST_AUTO_TEST_CASE(testSendMeshToMultipleParticipants,
                     *testing::MinRanks(3) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1, 2})))
{
  if (utils::Parallel::getCommunicatorSize() != 3)
    return;

  const std::string configFile = _pathToTests + "send-mesh-to-multiple-participants.xml";
  std::string       solverName;
  std::string       meshName;

  Eigen::Vector2d vertex{0.0, 0.0};

  double value = 1.0;

  if (utils::Parallel::getProcessRank() == 0) {
    solverName = "SolverOne";
    meshName   = "MeshA";
  } else if (utils::Parallel::getProcessRank() == 1) {
    solverName = "SolverTwo";
    meshName   = "MeshB";
  } else if (utils::Parallel::getProcessRank() == 2) {
    solverName = "SolverThree";
    meshName   = "MeshC";
  }

  SolverInterface cplInterface(solverName, 0, 1);
  slimConfigure(cplInterface, configFile);

  const int meshID = cplInterface.getMeshID(meshName);

  int vertexID = cplInterface.setMeshVertex(meshID, vertex.data());

  double maxDt = cplInterface.initialize();

  int dataID = cplInterface.getDataID("Data", meshID);

  if (utils::Parallel::getProcessRank() == 0) {
    cplInterface.writeScalarData(dataID, vertexID, value);
  } else {
    double valueReceived = -1.0;
    cplInterface.readScalarData(dataID, vertexID, valueReceived);
    BOOST_TEST(valueReceived == value);
  }

  cplInterface.advance(maxDt);
  cplInterface.finalize();
}

/**
 * @brief Test to reproduce the problem of issue 383, https://github.com/precice/precice/issues/383
 *
 */
BOOST_AUTO_TEST_CASE(testPreconditionerBug,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  using Eigen::Vector2d;
  using namespace precice::constants;

  const std::string configFile = _pathToTests + "preconditioner-bug.xml";

  std::string participantName = utils::Parallel::getProcessRank() == 0 ? "SolverOne" : "SolverTwo";
  std::string meshName        = utils::Parallel::getProcessRank() == 0 ? "MeshOne" : "MeshTwo";

  SolverInterface cplInterface(participantName, 0, 1);
  slimConfigure(cplInterface, configFile);
  const int meshID = cplInterface.getMeshID(meshName);

  Vector2d vertex{0.0, 0.0};

  int vertexID = cplInterface.setMeshVertex(meshID, vertex.data());

  cplInterface.initialize();
  int numberOfAdvanceCalls = 0;

  while (cplInterface.isCouplingOngoing()) {
    if (cplInterface.isActionRequired(actionWriteIterationCheckpoint()))
      cplInterface.fulfilledAction(actionWriteIterationCheckpoint());
    if (cplInterface.isActionRequired(actionReadIterationCheckpoint()))
      cplInterface.fulfilledAction(actionReadIterationCheckpoint());

    if (utils::Parallel::getProcessRank() == 1) {
      int dataID = cplInterface.getDataID("DataOne", meshID);
      // to get convergence in first timestep (everything 0), but not in second timestep
      Vector2d value{0.0, 0.0 + numberOfAdvanceCalls};
      cplInterface.writeVectorData(dataID, vertexID, value.data());
    }
    cplInterface.advance(1.0);
    ++numberOfAdvanceCalls;
  }
  cplInterface.finalize();
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // PRECICE_NO_MPI
