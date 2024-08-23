#ifndef PRECICE_NO_MPI

#include "precice/Participant.hpp"
#include "testing/QuickTest.hpp"
#include "testing/Testing.hpp"

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Remeshing)
BOOST_AUTO_TEST_SUITE(Parallel)
BOOST_AUTO_TEST_SUITE(Implicit)
BOOST_AUTO_TEST_SUITE(ChangedMapping)
BOOST_AUTO_TEST_CASE(RemeshInputSerial)
{
  PRECICE_TEST("A"_on(1_rank), "B"_on(1_rank));
  using namespace precice::testing;
  constexpr double y = 0.0;

  precice::Participant participant{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    QuickTest(participant, "MA"_mesh, "D"_data)
        .setVertices({0.0, y, 1.0, y})
        .initialize()
        .write({0.01, 0.02})
        .advance()
        .resetMesh()
        .setVertices({0.0, y, 0.5, y, 1.0, y})
        .write({0.11, 0.12, 0.13})
        .advance()
        .finalize();
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    auto qt = QuickTest(participant, "MB"_mesh, "D"_data)
                  .setVertices({0.0, y, 1.0, y})
                  .initialize()
                  .advance();
    std::vector<double> expected0{0.01, 0.02};
    BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
    qt.advance();

    std::vector<double> expected1{0.11, 0.13};
    BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
    qt.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // ChangedMapping
BOOST_AUTO_TEST_SUITE_END() // Implicit
BOOST_AUTO_TEST_SUITE_END() // Parallel
BOOST_AUTO_TEST_SUITE_END() // Remeshing
BOOST_AUTO_TEST_SUITE_END() // Integration

#endif // PRECICE_NO_MPI
