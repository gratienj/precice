#include "testing/Fixtures.hpp" // include that for the more advanced fixtures, e.g. to build up m2n com
#include "testing/Testing.hpp"
#include "utils/Parallel.hpp" // only required when using something from utils::Parallel

using namespace precice;

BOOST_AUTO_TEST_SUITE(TestingTests) // Use name of the module, e.g. subdirectory below src/, suffixed with Tests

BOOST_AUTO_TEST_SUITE(Examples) // If your file contains multiple tests, put them in a test suite

/// This test runs independently on all processors. Attention when doing so, the ranks may interfere
BOOST_AUTO_TEST_CASE(SingleProcessor)
{
  /* Do not use DEBUG, TRACE, INFO calls inside tests, if you need to log a message use
     BOOST_TEST_MESSAGE("I have done that " << whatIHaveDone);

     From Boost Test Documentation:
     "Messages generated by this tool do not appear in test log output with default value of the
     active log level threshold. For these messages to appear the active log level threshold has to
     be set to a value below or equal to "message"."

     In order to get this output to the terminal, use testprecice with --log-level=message.
  */

  BOOST_TEST(0 == 0); // Always use BOOST_TEST

  Eigen::Vector3d one(1, 2, 3);
  Eigen::Vector3d two(1, 2, 3);
  // Use testing::equals instead of math::equals when comparing in tests.
  // This gives you a report which coordinates fail to compare.
  BOOST_TEST(testing::equals(one, two));
}

/// Test with a modified numerical tolerance
BOOST_AUTO_TEST_CASE(NumericalTolerance,
                     *boost::unit_test::tolerance(1e-4))
{
  // Default tolerance is 1e-9, it can be changed for the entire case or even suite
  // using the decorator above
  BOOST_TEST(1.0 == 1.0001);

  // Or on a per test basis
  BOOST_TEST(1.0 == 1.01, boost::test_tools::tolerance(0.1));
}

/// Use testing::Deleted to unconditionally delete the test
BOOST_AUTO_TEST_CASE(Deleted,
                     *testing::Deleted())
{
  BOOST_TEST(false);
}

/// Test that requires 4 processors.
/*
 * If less than 4 procs are available, the test is deleted, if more are available, procs > 4 are deleted
 */
BOOST_AUTO_TEST_CASE(FourProcTests,
                     *testing::OnSize(4))
{
  // Don't copy over that line, it's for testing the example
  BOOST_TEST(utils::Parallel::getCommunicatorSize() == 4);
}

/// Test that runs on 2 processors.
/*
 * This case is trickier than with 4 procs, because we need to restrict the global communicator on all
 * ranks first, and then test if we execute at the correct ranks.
 */
BOOST_AUTO_TEST_CASE(TwoProcTests,
                     *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  // Put your test code here
}

#ifndef PRECICE_NO_MPI
/// Test that requires 4 processors and a master communication
/*
 * For some master tests, you might need a master communication. This example shows how to set one up.
 * Please note: Such tests always need to be excluded for compilation without MPI (PRECICE_NO_MPI).
 */
BOOST_AUTO_TEST_CASE(FourProcTestsWithMasterCommmunication,
                     *testing::OnSize(4) * boost::unit_test::fixture<testing::MasterComFixture>())
{
  // In this test you can use a master communication, here is an example how:
  BOOST_TEST(utils::MasterSlave::_communication->isConnected());
}

/// Tests that requires an m2n communication
/*
 * For some master tests, you might need an m2n communication (e.g. partition or cplscheme).
 * This example shows how to set up one. Don't use the M2N fixture as a decorator. Otherwise,
 * you cannot access the m2n.
 * Please note: Such tests always need to be excluded for compilation without MPI (PRECICE_NO_MPI).
 */
BOOST_FIXTURE_TEST_CASE(TwoProcTestsWithM2NCommunication, testing::M2NFixture,
                        *testing::MinRanks(2) * boost::unit_test::fixture<testing::MPICommRestrictFixture>(std::vector<int>({0, 1})))
{
  if (utils::Parallel::getCommunicatorSize() != 2)
    return;

  //This is how you can access the m2n communication
  BOOST_TEST(m2n->getMasterCommunication()->isConnected());
}

#endif // PRECICE_NO_MPI

/// Integration tests with two participants.
/*
 * For integration tests (tests that directly use the preCICE API), often, you need two participants
 * where each participant uses it own "global" communicator, i.e. each participant should not see that
 * he is part of a test. Then, you can use the SplitParticipantsFixture. This gives you two participants
 * with two ranks each (ranks 0,1 for both, not 0,1,2,3). For other settings (e.g. 3-1, or more than two
 * participants), you can easily copy functionality from this fixture. As now all internals, including
 * "utils::Parallel::getProcessRank()" are local, we cannot use this for distinguishing the participants.
 * As a solution, we have an ID "participantID". That's why you cannot use the SplitParticipantsFixture
 * fixture as a decorator, but only the following way:
 */
BOOST_FIXTURE_TEST_CASE(IntegrationTestsWithTwoParticipants, testing::SplitParticipantsFixture,
                        *testing::OnSize(4))
{
  if (participantID == 1) {

    // Put here your preCICE API code for the first participant

    // Don't copy over that line, it's for testing the example
    BOOST_TEST(utils::Parallel::getCommunicatorSize() == 2);
  } else {
    BOOST_TEST(participantID == 2);

    // Put here your preCICE API code for the second participant

    // Don't copy over that line, it's for testing the example
    BOOST_TEST(utils::Parallel::getCommunicatorSize() == 2);
  }
}

BOOST_AUTO_TEST_SUITE_END() // Examples
BOOST_AUTO_TEST_SUITE_END() // TestingTests
