#pragma once

#include <string>
#include <vector>

#include <precice/Participant.hpp>
#include "testing/QuickTest.hpp"
#include "testing/TestContext.hpp"
#include "testing/Testing.hpp"

namespace precice::tests::remesh::parallelImplicit {

using testing::QuickTest;
using testing::operator""_mesh;
using testing::operator""_data;

namespace noop {

inline void runResetInput(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.advance();

      std::vector<double> expected1{0.11, 0.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.advance();

      std::vector<double> expected1{1.11, 1.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}

inline void runResetOutput(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance();
      std::vector<double> expected1{0.11, 0.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance();
      std::vector<double> expected1{1.11, 1.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}

inline void runResetBoth(testing::TestContext &context)
{
  constexpr double y = 0.0;
  Participant      p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance();

      std::vector<double> expected1{0.11, 0.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance();

      std::vector<double> expected1{1.11, 1.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}
} // namespace noop

namespace changemapping {

inline void runResetInput(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .resetMesh()
          .setVertices({1.0, y})
          .write({0.11})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.advance();

      std::vector<double> expected1{0.11, 0.11};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.advance();

      std::vector<double> expected1{1.11, 1.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}

inline void runResetOutput(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({1.0, y})
          .advance();
      std::vector<double> expected1{0.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({2.0, y})
          .advance();
      std::vector<double> expected1{1.11};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}

inline void runResetBoth(testing::TestContext &context)
{
  constexpr double y = 0.0;
  Participant      p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .resetMesh()
          .setVertices({-1.0, y, 0, y})
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .resetMesh()
          .setVertices({3.0, y, 4.0, y})
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance();

      std::vector<double> expected1{0.12, 0.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance();

      std::vector<double> expected1{1.11, 1.11};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}
} // namespace changemapping

namespace changepartition {

inline void runOverlapBoth(testing::TestContext &context)
{
  constexpr double y = 0.0;
  Participant      p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .resetMesh()
          .setVertices({0.0, y, 1.0, y, 2.0, y})
          .write({0.11, 0.12, 0.13})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .resetMesh()
          .setVertices({3.0, y})
          .write({1.11})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({0.0, y})
          .advance();

      std::vector<double> expected1{0.11};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({1.0, y, 2.0, y, 3.0, y})
          .advance();

      std::vector<double> expected1{0.12, 0.13, 1.11};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}

inline void runSwapOutputs(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance();
      std::vector<double> expected1{1.11, 1.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance();
      std::vector<double> expected1{0.11, 0.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}

inline void runScatterOutputs(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .advance()
          .write({0.11, 0.12})
          .advance()
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_data)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .advance()
          .write({1.11, 1.12})
          .advance()
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({0.0, y, 1.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{0.01, 0.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({0.0, y, 2.0, y})
          .advance();
      std::vector<double> expected1{0.11, 1.11};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    } else {
      auto qt = QuickTest(p, "MB"_mesh, "D"_data)
                    .setVertices({2.0, y, 3.0, y})
                    .initialize()
                    .advance();
      std::vector<double> expected0{1.01, 1.02};
      BOOST_TEST(qt.read() == expected0, boost::test_tools::per_element());
      qt.resetMesh()
          .setVertices({1.0, y, 3.0, y})
          .advance();
      std::vector<double> expected1{0.12, 1.12};
      BOOST_TEST(qt.read() == expected1, boost::test_tools::per_element());
      qt.finalize();
    }
  }
}
} // namespace changepartition
} // namespace precice::tests::remesh::parallelImplicit
