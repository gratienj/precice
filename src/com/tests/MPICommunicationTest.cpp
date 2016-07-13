#include "MPICommunicationTest.hpp"
#include "com/MPIPortsCommunication.hpp"
#include "utils/Parallel.hpp"
#include "utils/Dimensions.hpp"
#include "utils/Globals.hpp"

#include "tarch/tests/TestCaseFactory.h"

using precice::utils::Parallel;

registerTest(precice::com::tests::MPICommunicationTest);

namespace precice {
namespace com {
namespace tests {

tarch::logging::Log MPICommunicationTest::_log(
    "precice::com::tests::MPICommunicationTest");

MPICommunicationTest::MPICommunicationTest()
    : TestCase("precice::com::tests::MPICommunicationTest") {
}

void
MPICommunicationTest::run() {
# ifndef PRECICE_NO_MPI

  preciceTrace("run");

  Parallel::synchronizeProcesses();

  if (Parallel::getCommunicatorSize() > 1) {
    auto communicator = Parallel::getRestrictedCommunicator({0, 1});

    if (Parallel::getProcessRank() < 2) {
      Parallel::setGlobalCommunicator(communicator);
      testMethod(testSendAndReceiveString);
      testMethod(testSendAndReceiveVector);
      testMethod(testSendAndReceiveInteger);
      Parallel::setGlobalCommunicator(Parallel::getCommunicatorWorld());
    }
  }
# endif
}

#ifndef PRECICE_NO_MPI

void
MPICommunicationTest::testSendAndReceiveString() {
  preciceTrace("testSendAndReceiveString()");
  utils::Parallel::synchronizeProcesses();
  MPIPortsCommunication com;
  if (utils::Parallel::getProcessRank() == 0) {
    com.acceptConnection("A", "R", 0, 1);
    std::string msg("testOne");
    com.send(msg, 0);
    com.receive(msg, 0);
    validate(msg == std::string("testTwo"));

  } else if (utils::Parallel::getProcessRank() == 1) {
    com.requestConnection("A", "R", 0, 1);
    std::string msg;
    com.receive(msg, 0);
    validate(msg == std::string("testOne"));
    msg = "testTwo";
    com.send(msg, 0);
  }
  utils::Parallel::clearGroups();
}

void
MPICommunicationTest::testSendAndReceiveVector() {
  preciceTrace("testSendAndReceiveVector()");
  utils::Parallel::synchronizeProcesses();

  MPIPortsCommunication com;
  if (utils::Parallel::getProcessRank() == 0) {
    com.acceptConnection("A", "R", 0, 1);
    utils::DynVector msg(3, 1.0);
    com.send(tarch::la::raw(msg), msg.size(), 0);
    com.receive(tarch::la::raw(msg), msg.size(), 0);
    validate(tarch::la::equals(msg, utils::Vector3D(2.0)));
  } else if (utils::Parallel::getProcessRank() == 1) {
    com.requestConnection("A", "R", 0, 1);
    utils::DynVector msg(3, 0.0);
    com.receive(tarch::la::raw(msg), msg.size(), 0);
    validate(tarch::la::equals(msg, utils::Vector3D(1.0)));
    msg = utils::Vector3D(2.0);
    com.send(tarch::la::raw(msg), msg.size(), 0);
  }
  utils::Parallel::clearGroups();
}

void
MPICommunicationTest::testSendAndReceiveInteger() {
  preciceTrace("testSendAndReceiveInteger()");
  utils::Parallel::synchronizeProcesses();

  MPIPortsCommunication com;
  if (utils::Parallel::getProcessRank() == 0) {
    com.acceptConnection("A", "R", 0, 1);
    int msg = 1;
    com.send(msg, 0);
    com.receive(msg, 0);
    validate(msg == 2);
  } else if (utils::Parallel::getProcessRank() == 1) {
    com.requestConnection("A", "R", 0, 1);
    int msg = 0;
    com.receive(msg, 0);
    validate(msg == 1);
    msg = 2;
    com.send(msg, 0);
  }
  utils::Parallel::clearGroups();
}


#endif // not PRECICE_NO_MPI
}
}
} // namespace precice, com, tests
