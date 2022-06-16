#pragma once

#include <array>
#include <iostream>
#include "precice/types.hpp"
#include "utils/assertion.hpp"

namespace precice {
namespace mesh {
class Vertex;
}
} // namespace precice

// ----------------------------------------------------------- CLASS DEFINITION

namespace precice {
namespace mesh {

/// Tetrahedron of a mesh, defined by 4 vertices
class Tetrahedron {
public:
  /// Constructor, the order of vertices doesn't matter.
  Tetrahedron(
      Vertex &      vertexOne,
      Vertex &      vertexTwo,
      Vertex &      vertexThree,
      Vertex &      vertexFour,
      TetrahedronID id);

  /// Returns dimensionalty of space the Tetrahedron is embedded in.
  int getDimensions() const;

  /**
   * @brief Returns tetrahedron vertex with index 0, 1, 2 or 3.
   */
  Vertex &vertex(int i);

  /**
   * @brief Returns const tetrahedron vertex with index 0, 1, 2 or 3.
   */
  const Vertex &vertex(int i) const;

  /// Returns a among Tetrahedrons globally unique ID.
  TetrahedronID getID() const;

  /// Returns the unsigned volume of the tetrahedron
  double getVolume() const;

  /// Returns the barycenter of the tetrahedron.
  const Eigen::VectorXd getCenter() const;

  /// Returns the radius of the sphere enclosing the tetrahedron.
  double getEnclosingRadius() const;

  /**
   * @brief Compares two Tetrahedrons for equality
   *
   * Two Tetrahedrons are equal if their vertices are the same, up to permutations
   */
  bool operator==(const Tetrahedron &other) const;

  /// Not equal, implemented in terms of equal.
  bool operator!=(const Tetrahedron &other) const;

private:
  /// Vertices defining the Tetrahedron.
  std::array<Vertex *, 4> _vertices;

  /// ID of the Tetrahedron.
  TetrahedronID _id;
};

// --------------------------------------------------------- HEADER DEFINITIONS

inline Vertex &Tetrahedron::vertex(int i)
{
  PRECICE_ASSERT((i >= 0) && (i < 4), i);
  return *_vertices[i];
}

inline const Vertex &Tetrahedron::vertex(int i) const
{
  PRECICE_ASSERT((i >= 0) && (i < 4), i);
  return *_vertices[i];
}

inline TetrahedronID Tetrahedron::getID() const
{
  return _id;
}

std::ostream &operator<<(std::ostream &os, const Tetrahedron &t);

} // namespace mesh
} // namespace precice
