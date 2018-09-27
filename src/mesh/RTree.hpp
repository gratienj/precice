#pragma once

#include <boost/geometry.hpp>
#include <map>
#include <memory>
#include "mesh/Mesh.hpp"
#include "mesh/impl/RTreeAdapter.hpp"
#include "mesh/Triangle.hpp"
#include "mesh/Quad.hpp"

// Forward declaration to friend the boost test struct
namespace MeshTests
{
namespace RTree
{
struct CacheClearing;
}
} // namespace MeshTests

namespace precice
{
namespace mesh
{

/// The enumeration of various primitive types.
enum class Primitive {
  Vertex,
  Edge,
  Triangle,
  Quad
};

/// The type traits to return the enum value of a primitive type.
template <class T>
struct as_primitive_enum {
};
template <>
struct as_primitive_enum<mesh::Vertex> {
  static constexpr Primitive value = Primitive::Vertex;
};
template <>
struct as_primitive_enum<mesh::Edge> {
  static constexpr Primitive value = Primitive::Edge;
};
template <>
struct as_primitive_enum<mesh::Triangle> {
  static constexpr Primitive value = Primitive::Triangle;
};
template <>
struct as_primitive_enum<mesh::Quad> {
  static constexpr Primitive value = Primitive::Quad;
};

/// Binds an Index and Primitive into a type
struct PrimitiveIndex {
  Primitive type;
  size_t    index;

  /// Standard equality test
  bool operator==(const PrimitiveIndex &other)
  {
    return type == other.type && index == other.index;
  };
};

/// The axis aligned bounding box based on the Vertex Type
using AABB = boost::geometry::model::box<Eigen::VectorXd>;

/** Holding a reference to a Mesh it is a functor for packing
 *  PrimitiveIndex into AABBs.
 */
class AABBGenerator
{
public:
  /// Constructs a generator for a given mesh
  explicit AABBGenerator(const mesh::Mesh &mesh)
      : mesh_(mesh){};

  /** constructs a AABB of a PrimitiveIndex.
   *
   * This operator takes a PrimitiveIndex and fetches the actual primitive from the mesh.
   * It then constructs an AABB using boost::geometry::return_envelope() and returns it.
   *
   * \param pi the index to build the AABB for
   * \returns a AABB for the primitive
   */
  AABB operator()(const PrimitiveIndex &pi) const
  {
    switch (pi.type) {
    case (Primitive::Vertex):
      return boost::geometry::return_envelope<AABB>(mesh_.vertices()[pi.index]);
    case (Primitive::Edge):
      return boost::geometry::return_envelope<AABB>(mesh_.edges()[pi.index]);
    case (Primitive::Triangle):
      return boost::geometry::return_envelope<AABB>(mesh_.triangles()[pi.index]);
    case (Primitive::Quad):
      return boost::geometry::return_envelope<AABB>(mesh_.quads()[pi.index]);
    }
  }

private:
  /// The mesh used look the primitives up
  const mesh::Mesh &mesh_;
};

/// The rtree capable of indexing primitives of an entire Mesh
using PrimitiveRTree = boost::geometry::index::rtree<std::pair<AABB, PrimitiveIndex>, boost::geometry::index::rstar<16>>;

/// The shared_ptr convenience type for PrimitiveRTree
using PtrPrimitiveRTree = std::shared_ptr<PrimitiveRTree>;

/** indexes a container of primitives into an rtree.
 * 
 * The algorithm indexes every primitive of the given container using the passed generator.
 * It inserts its result and the PrimitiveIndex into the rtree as a std::pair.
 *
 * \param[inout] rtree the rtree to index into
 * \param gen the Generator generating something to index rtree::value_type.
 * \param conti the Container to index
 */
template <typename Container, typename Generator = AABBGenerator>
void indexPrimitive(PrimitiveRTree &rtree, const Generator& gen, const Container &conti)
{
  using ValueType = typename std::remove_reference<typename std::remove_cv<typename Container::value_type>::type>::type;
  for (size_t i = 0; i < conti.size(); ++i) {
    PrimitiveIndex index{as_primitive_enum<ValueType>::value, i};
    rtree.insert(std::make_pair(gen(index), index));
  }
}

/** Indexes a given mesh and returns a PrimitiveRTree holding the index
 *
 * This indexes the vertices, edges, triangles, and quads of a given Mesh and retrurns the index tree
 *
 * \param mesh the mesh to index
 *
 * \return the tree containing the indexed primitives descibed above
 */
inline PrimitiveRTree indexMesh(const Mesh &mesh)
{
  AABBGenerator  gen{mesh};
  PrimitiveRTree tree;
  indexPrimitive(tree, gen, mesh.vertices());
  indexPrimitive(tree, gen, mesh.edges());
  indexPrimitive(tree, gen, mesh.triangles());
  indexPrimitive(tree, gen, mesh.quads());
  return tree;
}

class rtree
{
public:
  using VertexIndexGetter = impl::PtrVectorIndexable<Mesh::VertexContainer>;
  using RTreeParameters   = boost::geometry::index::rstar<16>;
  using VertexRTree       = boost::geometry::index::rtree<Mesh::VertexContainer::container::size_type,
                                                    RTreeParameters,
                                                    VertexIndexGetter>;
  using PtrRTree          = std::shared_ptr<VertexRTree>;

  /// Returns the pointer to boost::geometry::rtree for the given mesh vertices
  /*
   * Creates and fills the tree, if it wasn't requested before, otherwise it returns the cached tree.
   */
  static PtrRTree getVertexRTree(PtrMesh mesh);

  /// Returns the pointer to boost::geometry::rtree for the given mesh primitives
  /*
   * Creates and fills the tree, if it wasn't requested before, otherwise it returns the cached tree.
   */
  static PtrPrimitiveRTree getPrimitiveRTree(PtrMesh mesh);

  /// Only clear the trees of that specific mesh
  static void clear(Mesh &mesh);

  friend struct MeshTests::RTree::CacheClearing;

private:
  static std::map<int, PtrPrimitiveRTree> primitive_trees_; ///< Cache for the primitive trees
  static std::map<int, PtrRTree>         trees; ///< Cache for the vertex trees
};

using Box3d = boost::geometry::model::box<boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian>>;

/// Returns a boost::geometry box that encloses a sphere of given radius around a middle point
Box3d getEnclosingBox(Vertex const &middlePoint, double sphereRadius);

} // namespace mesh
} // namespace precice
