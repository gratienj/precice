#include "mesh/impl/RTree.hpp"
#include "mesh/impl/RTreeAdapter.hpp"

#include "mesh/RTree.hpp"

namespace precice {
namespace mesh {

namespace bg = boost::geometry;

// Initialize static member
std::map<int, rtree::MeshIndices> precice::mesh::rtree::_cached_trees;
std::map<int, PtrPrimitiveRTree> precice::mesh::rtree::_primitive_trees;

rtree::MeshIndices& rtree::cacheEntry(int meshID)
{
    auto result = _cached_trees.emplace(std::make_pair(meshID, rtree::MeshIndices{}));
    return result.first->second;
}


rtree::vertex_traits::Ptr rtree::getVertexRTree(const PtrMesh& mesh)
{
  assertion(mesh);
  auto& cache = cacheEntry(mesh->getID());
  if (cache.vertices) {
      return cache.vertices;
  }

  RTreeParameters params;
  vertex_traits::IndexGetter ind(mesh->vertices());
  auto tree = std::make_shared<vertex_traits::RTree>(params, ind);
  for (size_t i = 0; i < mesh->vertices().size(); ++i) {
      tree->insert(i);
  }

  cache.vertices = tree;
  return tree;
}


rtree::edge_traits::Ptr rtree::getEdgeRTree(const PtrMesh& mesh)
{
  assertion(mesh);
  auto& cache = cacheEntry(mesh->getID());
  if (cache.edges) {
      return cache.edges;
  }

  RTreeParameters params;
  edge_traits::IndexGetter ind(mesh->edges());
  auto tree = std::make_shared<edge_traits::RTree>(params, ind);
  for (size_t i = 0; i < mesh->edges().size(); ++i) {
      tree->insert(i);
  }

  cache.edges = tree;
  return tree;
}


rtree::triangle_traits::Ptr rtree::getTriangleRTree(const PtrMesh& mesh)
{
  assertion(mesh);
  auto& cache = cacheEntry(mesh->getID());
  if (cache.triangles) {
      return cache.triangles;
  }

  RTreeParameters params;
  triangle_traits::IndexGetter ind;
  auto tree = std::make_shared<triangle_traits::RTree>(params, ind);
  for (size_t i = 0; i < mesh->triangles().size(); ++i) {
      auto box = bg::return_envelope<RTreeBox>(mesh->triangles()[i]);
      tree->insert(std::make_pair(std::move(box) , i));
  }

  cache.triangles = tree;
  return tree;
}


PtrPrimitiveRTree rtree::getPrimitiveRTree(const PtrMesh& mesh)
{
  assertion(mesh, "Empty meshes are not allowed.");
  auto iter = _primitive_trees.find(mesh->getID());
  if (iter != _primitive_trees.end()) {
    return iter->second;
  }
  auto treeptr = std::make_shared<PrimitiveRTree>(indexMesh(*mesh));
  _primitive_trees.emplace(std::piecewise_construct,
          std::forward_as_tuple(mesh->getID()),
          std::forward_as_tuple(treeptr));
  return treeptr;
}


void rtree::clear(Mesh &mesh)
{
  _cached_trees.erase(mesh.getID());
  _primitive_trees.erase(mesh.getID());
}

void rtree::clear()
{
  _cached_trees.clear();
  _primitive_trees.clear();
}

Box3d getEnclosingBox(Vertex const & middlePoint, double sphereRadius)
{
  namespace bg = boost::geometry;
  auto & coords = middlePoint.getCoords();

  Box3d box;
  bg::set<bg::min_corner, 0>(box, bg::get<0>(coords) - sphereRadius);
  bg::set<bg::min_corner, 1>(box, bg::get<1>(coords) - sphereRadius);
  bg::set<bg::min_corner, 2>(box, bg::get<2>(coords) - sphereRadius);

  bg::set<bg::max_corner, 0>(box, bg::get<0>(coords) + sphereRadius);
  bg::set<bg::max_corner, 1>(box, bg::get<1>(coords) + sphereRadius);
  bg::set<bg::max_corner, 2>(box, bg::get<2>(coords) + sphereRadius);
  
  return box;
}


PrimitiveRTree indexMesh(const Mesh &mesh)
{
  using namespace impl;

  AABBGenerator  gen{mesh};
  PrimitiveRTree tree;
  indexPrimitive(tree, gen, mesh.vertices());
  indexPrimitive(tree, gen, mesh.edges());
  indexPrimitive(tree, gen, mesh.triangles());
  indexPrimitive(tree, gen, mesh.quads());
  return tree;
}

std::ostream &operator<<(std::ostream &out, Primitive val)
{
  switch (val) {
  case (Primitive::Vertex):
    out << "Vertex";
    break;
  case (Primitive::Edge):
    out << "Edge";
    break;
  case (Primitive::Triangle):
    out << "Triangle";
    break;
  case (Primitive::Quad):
    out << "Quad";
    break;
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, PrimitiveIndex val) {
    return out << val.type << ":" << val.index;
}

bool operator==(const PrimitiveIndex& lhs, const PrimitiveIndex& rhs)
{
    return lhs.type == rhs.type && lhs.index == rhs.index;
}

/// Standard non-equality test for PrimitiveIndex
bool operator!=(const PrimitiveIndex& lhs, const PrimitiveIndex& rhs)
{
    return !(lhs == rhs);
}

}}

