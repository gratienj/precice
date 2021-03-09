#include "Index.hpp"
#include <boost/range/irange.hpp>
#include "FindClosest.hpp"
#include "impl/Indexer.hpp"
#include "logging/LogMacros.hpp"
#include "utils/Event.hpp"

namespace precice {
extern bool syncMode;
namespace query {

precice::logging::Logger Index::_log{"query::Index"};

namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

struct Index::IndexImpl {
  impl::MeshIndices indices;
};

Index::Index(const mesh::PtrMesh &mesh)
    : _mesh(mesh)
{
  _pimpl = std::make_unique<IndexImpl>(IndexImpl{});
}

Index::~Index()
{
}

VertexMatch Index::getClosestVertex(const Eigen::VectorXd &sourceCoord)
{
  PRECICE_TRACE();
  // Add tree to the local cache
  if (not _pimpl->indices.vertexRTree) {
    precice::utils::Event event("query.index.getVertexIndexTree." + _mesh->getName(), precice::syncMode);
    _pimpl->indices.vertexRTree = impl::Indexer::instance()->getVertexRTree(_mesh);
    event.stop();
  }

  VertexMatch match;
  _pimpl->indices.vertexRTree->query(bgi::nearest(sourceCoord, 1), boost::make_function_output_iterator([&](size_t matchID) {
                                       match = VertexMatch(bg::distance(sourceCoord, _mesh->vertices()[matchID]), matchID);
                                     }));
  return match;
}

std::vector<EdgeMatch> Index::getClosestEdges(const Eigen::VectorXd &sourceCoord, int n)
{
  PRECICE_TRACE();
  // Add tree to the local cache
  if (not _pimpl->indices.edgeRTree) {
    precice::utils::Event event("query.index.getEdgeIndexTree." + _mesh->getName(), precice::syncMode);
    _pimpl->indices.edgeRTree = impl::Indexer::instance()->getEdgeRTree(_mesh);
    event.stop();
  }

  std::vector<EdgeMatch> matches;
  _pimpl->indices.edgeRTree->query(bgi::nearest(sourceCoord, n), boost::make_function_output_iterator([&](size_t matchID) {
                                     matches.emplace_back(bg::distance(sourceCoord, _mesh->edges()[matchID]), matchID);
                                   }));
  std::sort(matches.begin(), matches.end());
  return matches;
}

std::vector<TriangleMatch> Index::getClosestTriangles(const Eigen::VectorXd &sourceCoord, int n)
{
  PRECICE_TRACE();
  // Add tree to the local cache
  if (not _pimpl->indices.triangleRTree) {
    precice::utils::Event event("query.index.getTriangleIndexTree." + _mesh->getName(), precice::syncMode);
    _pimpl->indices.triangleRTree = impl::Indexer::instance()->getTriangleRTree(_mesh);
    event.stop();
  }

  std::vector<TriangleMatch> matches;
  _pimpl->indices.triangleRTree->query(bgi::nearest(sourceCoord, n),
                                       boost::make_function_output_iterator([&](impl::TriangleTraits::IndexType const &match) {
                                         matches.emplace_back(bg::distance(sourceCoord, _mesh->triangles()[match.second]), match.second);
                                       }));
  std::sort(matches.begin(), matches.end());
  return matches;
}

VertexMatch Index::getClosestVertex(const mesh::Vertex &sourceVertex)
{
  return getClosestVertex(sourceVertex.getCoords());
}

std::vector<EdgeMatch> Index::getClosestEdges(const mesh::Vertex &sourceVertex, int n)
{
  return getClosestEdges(sourceVertex.getCoords(), n);
}

std::vector<TriangleMatch> Index::getClosestTriangles(const mesh::Vertex &sourceVertex, int n)
{
  return getClosestTriangles(sourceVertex.getCoords(), n);
}

std::vector<size_t> Index::getVerticesInsideBox(const mesh::Vertex &centerVertex, double radius)
{
  PRECICE_TRACE();
  // Add tree to the local cache
  if (_pimpl->indices.vertexRTree == nullptr) {
    precice::utils::Event event("query.index.getVertexIndexTree." + _mesh->getName(), precice::syncMode);
    _pimpl->indices.vertexRTree = impl::Indexer::instance()->getVertexRTree(_mesh);
    event.stop();
  }

  // Prepare boost::geometry box
  auto &          coords = centerVertex.getCoords();
  query::RTreeBox searchBox{coords.array() - radius, coords.array() + radius};

  std::vector<size_t> matches;
  _pimpl->indices.vertexRTree->query(bgi::intersects(searchBox) and bg::index::satisfies([&](size_t const i) { return bg::distance(centerVertex, _mesh->vertices()[i]) <= radius; }),
                                     std::back_inserter(matches));
  return matches;
}

std::vector<size_t> Index::getVerticesInsideBox(const mesh::BoundingBox &bb)
{
  PRECICE_TRACE();
  // Add tree to the local cache
  if (not _pimpl->indices.vertexRTree) {
    precice::utils::Event event("query.index.getVertexIndexTree." + _mesh->getName(), precice::syncMode);
    _pimpl->indices.vertexRTree = impl::Indexer::instance()->getVertexRTree(_mesh);
    event.stop();
  }
  std::vector<size_t> matches;
  _pimpl->indices.vertexRTree->query(bgi::intersects(query::RTreeBox{bb.minCorner(), bb.maxCorner()}), std::back_inserter(matches));
  return matches;
}

std::pair<InterpolationElements, double> Index::findNearestProjection(const mesh::Vertex &sourceVertex, int n)
{
  if (_mesh->getDimensions() == 2) {
    return findEdgeProjection(sourceVertex, n);
  } else {
    return findTriangleProjection(sourceVertex, n);
  }
}

std::pair<InterpolationElements, double> Index::findVertexProjection(const mesh::Vertex &sourceVertex)
{
  auto match = getClosestVertex(sourceVertex.getCoords());
  return std::pair<InterpolationElements, double>(generateInterpolationElements(sourceVertex, _mesh->vertices()[match.index]), match.distance);
}

std::pair<InterpolationElements, double> Index::findEdgeProjection(const mesh::Vertex &sourceVertex, int n)
{
  auto matchedEdges = getClosestEdges(sourceVertex.getCoords(), n);
  for (const auto &match : matchedEdges) {
    auto weights = query::generateInterpolationElements(sourceVertex, _mesh->edges()[match.index]);
    if (std::all_of(weights.begin(), weights.end(), [](query::InterpolationElement const &elem) { return elem.weight >= 0.0; })) {
      return std::pair<InterpolationElements, double>(weights, match.distance);
    }
  }
  // Could not find edge projection element, fall back to vertex projection
  return findVertexProjection(sourceVertex);
}

std::pair<InterpolationElements, double> Index::findTriangleProjection(const mesh::Vertex &sourceVertex, int n)
{
  auto matchedTriangles = getClosestTriangles(sourceVertex.getCoords(), n);
  for (const auto &match : matchedTriangles) {
    auto weights = query::generateInterpolationElements(sourceVertex, _mesh->triangles()[match.index]);
    if (std::all_of(weights.begin(), weights.end(), [](query::InterpolationElement const &elem) { return elem.weight >= 0.0; })) {
      return std::pair<InterpolationElements, double>(weights, match.distance);
    }
  }
  // Could not triangle find projection element, fall back to edge projection
  return findEdgeProjection(sourceVertex, n);
}

void clearCache()
{
  impl::Indexer::instance()->clearCache();
}

void clearCache(int meshID)
{
  impl::Indexer::instance()->clearCache(meshID);
}

void clearCache(mesh::Mesh &mesh)
{
  impl::Indexer::instance()->clearCache(mesh.getID());
}

} // namespace query
} // namespace precice