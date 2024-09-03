#pragma once

#include <string>

#include <precice/Participant.hpp>
#include "testing/QuickTest.hpp"
#include "testing/TestContext.hpp"
#include "testing/Testing.hpp"

namespace precice::tests::remesh::parallelImplicit {

using testing::QuickTest;
using testing::operator""_mesh;
using testing::operator""_read;
using testing::operator""_write;

namespace noop {

inline void runResetFirst(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .advance()
          .expect({0.11, 0.12})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .advance()
          .expect({1.11, 1.12})
          .expect({0.00, 0.00})
          .finalize();
    }
  }
}

inline void runResetSecond(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance()
          .expect({0.11, 0.12})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance()
          .expect({1.11, 1.12})
          .finalize();
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
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance()
          .expect({0.11, 0.12})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance()
          .expect({1.11, 1.12})
          .finalize();
    }
  }
}
} // namespace noop

namespace changemapping {

inline void runResetFirst(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({1.0, y})
          .write({0.11})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .advance()
          .expect({0.11, 0.11})
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .advance()
          .expect({1.11, 1.12})
          .expect({0.00, 0.00})
          .finalize();
    }
  }
}

inline void runResetSecond(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({1.0, y})
          .advance()
          .expect({0.12})
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({2.0, y})
          .advance()
          .expect({1.11})
          .expect({0.00, 0.00})
          .finalize();
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
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({-1.0, y, 0, y})
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({3.0, y, 4.0, y})
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance()
          .expect({0.12, 0.12})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance()
          .expect({1.11, 1.11})
          .finalize();
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
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({0.0, y, 1.0, y, 2.0, y})
          .write({0.11, 0.12, 0.13})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .resetMesh()
          .setVertices({3.0, y})
          .write({1.11})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({0.0, y})
          .advance()
          .expect({0.11})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({1.0, y, 2.0, y, 3.0, y})
          .advance()
          .expect({0.12, 0.13, 1.11})
          .finalize();
    }
  }
}

inline void runSwapSecond(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({2.0, y, 3.0, y})
          .advance()
          .expect({1.11, 1.12})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({0.0, y, 1.0, y})
          .advance()
          .expect({0.11, 0.12})
          .finalize();
    }
  }
}

inline void runScatterSecond(testing::TestContext &context)
{
  constexpr double y = 0.0;

  Participant p{context.name, context.config(), context.rank, context.size};

  // A - Static Geometry
  if (context.isNamed("A")) {
    if (context.isPrimary()) {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .write({0.01, 0.02})
          .expect({0.00, 0.00})
          .advance()
          .write({0.11, 0.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    } else {
      QuickTest(p, "MA"_mesh, "D"_write)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .write({1.01, 1.02})
          .expect({0.00, 0.00})
          .advance()
          .write({1.11, 1.12})
          .expect({0.00, 0.00})
          .advance()
          .expect({0.00, 0.00})
          .finalize();
    }
  }
  // B - Adaptive Geometry
  if (context.isNamed("B")) {
    if (context.isPrimary()) {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({0.0, y, 1.0, y})
          .initialize()
          .advance()
          .expect({0.01, 0.02})
          .resetMesh()
          .setVertices({0.0, y, 2.0, y})
          .advance()
          .expect({0.11, 1.11})
          .finalize();
    } else {
      QuickTest(p, "MB"_mesh, "D"_read)
          .setVertices({2.0, y, 3.0, y})
          .initialize()
          .advance()
          .expect({1.01, 1.02})
          .resetMesh()
          .setVertices({1.0, y, 3.0, y})
          .advance()
          .expect({0.12, 1.12})
          .finalize();
    }
  }
}
} // namespace changepartition
} // namespace precice::tests::remesh::parallelImplicit
