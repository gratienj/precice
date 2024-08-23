#ifndef PRECICE_NO_MPI

#include "testing/QuickTest.hpp"
#include "testing/Testing.hpp"

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Remeshing)
BOOST_AUTO_TEST_SUITE(Parallel)
BOOST_AUTO_TEST_SUITE(Explicit)
BOOST_AUTO_TEST_SUITE(ChangedMapping)
BOOST_AUTO_TEST_CASE(RemeshOutputSerial)
{
  using namespace precice::testing;
  PRECICE_TEST("A"_on(1_rank), "B"_on(1_rank));
  constexpr double y = 0.0;

  Participant participant{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    QuickTest(participant, "MA"_mesh, "D"_data)
        .setVertices({0.0, y, 1.0, y})
        .initialize()
        .write({0.01, 0.02})
        .advance()
        .write({0.11, 0.12})
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
    qt.resetMesh()
        .setVertices({0.0, y, 1.0, y, 2.0, y})
        .advance();
    std::vector<double> expected1{0.11, 0.12, 0.12};
    BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
    qt.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // ChangedMapping
BOOST_AUTO_TEST_SUITE_END() // Explicit
BOOST_AUTO_TEST_SUITE_END() // Parallel
BOOST_AUTO_TEST_SUITE_END() // Remeshing
BOOST_AUTO_TEST_SUITE_END() // Integration

#endif // PRECICE_NO_MPI
