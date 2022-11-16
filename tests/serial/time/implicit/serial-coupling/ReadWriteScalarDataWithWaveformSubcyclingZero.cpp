#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(Time)
BOOST_AUTO_TEST_SUITE(Implicit)
BOOST_AUTO_TEST_SUITE(SerialCoupling)

/**
 * @brief Test to run a simple coupling with zeroth order waveform subcycling.
 *
 * Provides a dt argument to the read function, but since a zeroth order waveform is used the result should be identical to the case without waveform relaxation
 */
BOOST_AUTO_TEST_CASE(ReadWriteScalarDataWithWaveformSubcyclingZero)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));

  SolverInterface precice(context.name, context.config(), 0, 1);

  MeshID meshID;
  DataID writeDataID;
  DataID readDataID;

  typedef double (*DataFunction)(double);

  DataFunction dataOneFunction = [](double t) -> double {
    return (double) (2 + t);
  };
  DataFunction dataTwoFunction = [](double t) -> double {
    return (double) (10 + t);
  };
  DataFunction writeFunction;
  DataFunction readFunction;

  if (context.isNamed("SolverOne")) {
    meshID        = precice.getMeshID("MeshOne");
    writeDataID   = precice.getDataID("DataOne", meshID);
    writeFunction = dataOneFunction;
    readDataID    = precice.getDataID("DataTwo", meshID);
    readFunction  = dataTwoFunction;
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    meshID        = precice.getMeshID("MeshTwo");
    writeDataID   = precice.getDataID("DataTwo", meshID);
    writeFunction = dataTwoFunction;
    readDataID    = precice.getDataID("DataOne", meshID);
    readFunction  = dataOneFunction;
  }

  double   writeData, readData;
  VertexID vertexID = precice.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());

  int    nSubsteps = 4; // perform subcycling on solvers. 4 steps happen in each window.
  int    nWindows  = 5; // perform 5 windows.
  int    timestep  = 0;
  double time      = 0;
  int    timestepCheckpoint;
  double timeCheckpoint;
  int    iterations;

  if (precice.isActionRequired(precice::constants::actionWriteInitialData())) {
    writeData = writeFunction(time);
    precice.writeScalarData(writeDataID, vertexID, writeData);
    precice.markActionFulfilled(precice::constants::actionWriteInitialData());
  }

  double maxDt    = precice.initialize();
  double windowDt = maxDt;
  double dt       = windowDt / nSubsteps; // Timestep length desired by solver. E.g. 4 steps  with size 1/4
  dt += windowDt / nSubsteps / nSubsteps; // increase timestep such that we get a non-matching subcycling. E.g. 3 step with size 5/16 and 1 step with size 1/16.
  double currentDt = dt;                  // Timestep length used by solver

  while (precice.isCouplingOngoing()) {
    if (precice.isActionRequired(precice::constants::actionWriteIterationCheckpoint())) {
      timeCheckpoint     = time;
      timestepCheckpoint = timestep;
      iterations         = 0;
      precice.markActionFulfilled(precice::constants::actionWriteIterationCheckpoint());
    }
    double readTime;
    readTime = timeCheckpoint + windowDt;

    precice.readScalarData(readDataID, vertexID, currentDt, readData);
    if (context.isNamed("SolverOne") && iterations == 0) { // in the first iteration of each window, use data from previous window.
      BOOST_TEST(readData == readFunction(timeCheckpoint));
    } else { // in the following iterations, use data at the end of window.
      BOOST_TEST(readData == readFunction(readTime));
    }

    precice.readScalarData(readDataID, vertexID, currentDt / 2, readData);
    if (context.isNamed("SolverOne") && iterations == 0) { // in the first iteration of each window, use data from previous window.
      BOOST_TEST(readData == readFunction(timeCheckpoint));
    } else { // in the following iterations, use data at the end of window.
      BOOST_TEST(readData == readFunction(readTime));
    }

    // solve usually goes here. Dummy solve: Just sampling the writeFunction.
    time += currentDt;
    timestep++;
    writeData = writeFunction(time);
    precice.writeScalarData(writeDataID, vertexID, writeData);
    maxDt = precice.advance(currentDt);
    if (precice.isActionRequired(precice::constants::actionReadIterationCheckpoint())) {
      time     = timeCheckpoint;
      timestep = timestepCheckpoint;
      iterations++;
      precice.markActionFulfilled(precice::constants::actionReadIterationCheckpoint());
    }
    currentDt = dt > maxDt ? maxDt : dt;
  }

  precice.finalize();
  BOOST_TEST(timestep == nWindows * nSubsteps);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // Time
BOOST_AUTO_TEST_SUITE_END() // Explicit
BOOST_AUTO_TEST_SUITE_END() // SerialCoupling

#endif // PRECICE_NO_MPI
