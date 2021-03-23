#include "NearestProjectionMapping.hpp"

#include <Eigen/Core>
#include <algorithm>
#include <deque>
#include <memory>
#include <ostream>
#include <unordered_set>
#include <utility>

#include "logging/LogMacros.hpp"
#include "mapping/Mapping.hpp"
#include "math/differences.hpp"
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/SharedPointer.hpp"
#include "mesh/Vertex.hpp"
#include "query/FindClosest.hpp"
#include "query/Index.hpp"
#include "utils/Event.hpp"
#include "utils/Statistics.hpp"
#include "utils/assertion.hpp"

namespace precice {
extern bool syncMode;

namespace mapping {

NearestProjectionMapping::NearestProjectionMapping(
    Constraint constraint,
    int        dimensions)
    : Mapping(constraint, dimensions)
{
  if (constraint == CONSISTENT) {
    setInputRequirement(Mapping::MeshRequirement::FULL);
    setOutputRequirement(Mapping::MeshRequirement::VERTEX);
  } else if (constraint == CONSERVATIVE) {
    setInputRequirement(Mapping::MeshRequirement::VERTEX);
    setOutputRequirement(Mapping::MeshRequirement::FULL);
  } else {
    PRECICE_ASSERT(constraint == SCALEDCONSISTENT, constraint);
    setInputRequirement(Mapping::MeshRequirement::FULL);
    setOutputRequirement(Mapping::MeshRequirement::FULL);
  }
}

namespace {
struct MatchType {
  double distance;
  int    index;

  MatchType() = default;
  MatchType(double d, int i)
      : distance(d), index(i){};
  constexpr bool operator<(MatchType const &other) const
  {
    return distance < other.distance;
  };
};
} // namespace

void NearestProjectionMapping::computeMapping()
{
  PRECICE_TRACE(input()->vertices().size(), output()->vertices().size());
  const std::string     baseEvent = "map.np.computeMapping.From" + input()->getName() + "To" + output()->getName();
  precice::utils::Event e(baseEvent, precice::syncMode);

  // Setup Direction of Mapping
  mesh::PtrMesh origins, searchSpace;
  if (hasConstraint(CONSERVATIVE)) {
    PRECICE_DEBUG("Compute conservative mapping");
    origins     = input();
    searchSpace = output();
  } else {
    PRECICE_DEBUG("Compute consistent mapping");
    origins     = output();
    searchSpace = input();
  }

  const auto &fVertices = origins->vertices();

  if (getDimensions() == 2) {
    if (!fVertices.empty() && searchSpace->edges().empty()) {
      PRECICE_WARN("2D Mesh \"" << searchSpace->getName() << "\" does not contain edges. Nearest projection mapping falls back to nearest neighbor mapping.");
    }
  } else {
    if (!fVertices.empty() && searchSpace->triangles().empty()) {
      PRECICE_WARN("3D Mesh \"" << searchSpace->getName() << "\" does not contain triangles. Nearest projection mapping will map to primitives of lower dimension.");
    }
  }

  // Amount of nearest elements to fetch for detailed comparison.
  // This safety margin results in a candidate set which forms the base for the
  // local nearest projection and counters the loss of detail due to bounding box generation.
  // @TODO Add a configuration option for this factor
  constexpr int nnearest = 4;

  query::Index                           indexTree(searchSpace);
  utils::statistics::DistanceAccumulator distanceStatistics;

  _weights.resize(fVertices.size());

  for (size_t i = 0; i < fVertices.size(); i++) {
    // Nearest projection element is edge for 2d if exists, if not, it is the nearest vertex
    // Nearest projection element is triangle for 3d if exists, if not the edge and at the worst case it is the nearest vertex
    auto interpolationElements = indexTree.findNearestProjection(fVertices[i], nnearest);
    _weights[i]                = std::move(interpolationElements.first);
    distanceStatistics(interpolationElements.second);
  }

  if (distanceStatistics.empty()) {
    PRECICE_INFO("Mapping distance not available due to empty partition.");
  } else {
    PRECICE_INFO("Mapping distance " << distanceStatistics);
  }

  _hasComputedMapping = true;
}

bool NearestProjectionMapping::hasComputedMapping() const
{
  return _hasComputedMapping;
}

void NearestProjectionMapping::clear()
{
  PRECICE_TRACE();
  _weights.clear();
  _hasComputedMapping = false;
}

void NearestProjectionMapping::map(
    int inputDataID,
    int outputDataID)
{
  PRECICE_TRACE(inputDataID, outputDataID);

  precice::utils::Event e("map.np.mapData.From" + input()->getName() + "To" + output()->getName(), precice::syncMode);

  mesh::PtrData          inData    = input()->data(inputDataID);
  mesh::PtrData          outData   = output()->data(outputDataID);
  const Eigen::VectorXd &inValues  = inData->values();
  Eigen::VectorXd &      outValues = outData->values();
  //assign(outValues) = 0.0;
  int dimensions = inData->getDimensions();
  PRECICE_ASSERT(dimensions == outData->getDimensions());

  if (hasConstraint(CONSERVATIVE)) {
    PRECICE_ASSERT(getConstraint() == CONSERVATIVE, getConstraint());
    PRECICE_DEBUG("Map conservative");
    PRECICE_ASSERT(_weights.size() == input()->vertices().size(),
                   _weights.size(), input()->vertices().size());
    for (size_t i = 0; i < input()->vertices().size(); i++) {
      size_t                 inOffset = i * dimensions;
      InterpolationElements &elems    = _weights[i];
      for (query::InterpolationElement &elem : elems) {
        size_t outOffset = (size_t) elem.element->getID() * dimensions;
        for (int dim = 0; dim < dimensions; dim++) {
          PRECICE_ASSERT(outOffset + dim < (size_t) outValues.size());
          PRECICE_ASSERT(inOffset + dim < (size_t) inValues.size());
          outValues(outOffset + dim) += elem.weight * inValues(inOffset + dim);
        }
      }
    }
  } else {
    PRECICE_DEBUG("Map consistent");
    PRECICE_ASSERT(_weights.size() == output()->vertices().size(),
                   _weights.size(), output()->vertices().size());
    for (size_t i = 0; i < output()->vertices().size(); i++) {
      InterpolationElements &elems     = _weights[i];
      size_t                 outOffset = i * dimensions;
      for (query::InterpolationElement &elem : elems) {
        size_t inOffset = (size_t) elem.element->getID() * dimensions;
        for (int dim = 0; dim < dimensions; dim++) {
          PRECICE_ASSERT(outOffset + dim < (size_t) outValues.size());
          PRECICE_ASSERT(inOffset + dim < (size_t) inValues.size());
          outValues(outOffset + dim) += elem.weight * inValues(inOffset + dim);
        }
      }
    }
    if (hasConstraint(SCALEDCONSISTENT)) {
      scaleConsistentMapping(inputDataID, outputDataID);
    }
  }
}

void NearestProjectionMapping::tagMeshFirstRound()
{
  PRECICE_TRACE();
  precice::utils::Event e("map.np.tagMeshFirstRound.From" + input()->getName() + "To" + output()->getName(), precice::syncMode);
  PRECICE_DEBUG("Compute Mapping for Tagging");

  computeMapping();
  PRECICE_DEBUG("Tagging First Round");

  // Determine the Mesh to Tag
  mesh::PtrMesh origins;
  if (hasConstraint(CONSERVATIVE)) {
    origins = output();
  } else {
    origins = input();
  }

  // Gather all vertices to be tagged in a first phase.
  // max_count is used to shortcut if all vertices have been tagged.
  std::unordered_set<mesh::Vertex const *> tagged;
  const std::size_t                        max_count = origins->vertices().size();

  for (const InterpolationElements &elems : _weights) {
    for (const query::InterpolationElement &elem : elems) {
      if (!math::equals(elem.weight, 0.0)) {
        tagged.insert(elem.element);
      }
    }
    // Shortcut if all vertices are tagged
    if (tagged.size() == max_count) {
      break;
    }
  }

  // Now tag all vertices to be tagged in the second phase.
  for (auto &v : origins->vertices()) {
    if (tagged.count(&v) == 1) {
      v.tag();
    }
  }
  PRECICE_DEBUG("First Round Tagged " << tagged.size() << "/" << max_count << " Vertices");

  clear();
}

void NearestProjectionMapping::tagMeshSecondRound()
{
  PRECICE_TRACE();
  // for NP mapping no operation needed here
}

} // namespace mapping
} // namespace precice
