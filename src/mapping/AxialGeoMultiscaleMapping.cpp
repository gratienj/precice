#include "AxialGeoMultiscaleMapping.hpp"
#include "mesh/Mesh.hpp"

namespace precice {

namespace mapping {

AxialGeoMultiscaleMapping::AxialGeoMultiscaleMapping(
    Constraint     constraint,
    int            dimensions,
    MultiscaleType type,
    double         radius)
    : Mapping(constraint, dimensions),
      _type(type),
      _radius(radius)
{
  setInputRequirement(Mapping::MeshRequirement::VERTEX);
  setOutputRequirement(Mapping::MeshRequirement::VERTEX);

  //PRECICE_ASSERT(dimensions == 2 || dimensions == 3);
  //if (dimensions == 2) {
  //  _scaling = 1.5;
  //} else if (dimensions == 3) {
  //  _scaling = 2.0;
  //}
}

void AxialGeoMultiscaleMapping::computeMapping()
{
  PRECICE_TRACE(output()->vertices().size());

  PRECICE_ASSERT(input().get() != nullptr);
  PRECICE_ASSERT(output().get() != nullptr);

  if (getConstraint() == CONSISTENT) {
    PRECICE_DEBUG("Compute consistent mapping");
    if (_type == SPREAD) {
      PRECICE_CHECK(input()->vertices().size() == 1, "You can only define an axial geometric multiscale mapping of type spread from a mesh with exactly one vertex.");
    } else {
      PRECICE_ASSERT(_type == COLLECT);
      PRECICE_CHECK(output()->vertices().size() == 1, "You can only define an axial geometric multiscale mapping of type spread from a mesh with exactly one vertex.");
    }
  } else {
    PRECICE_ASSERT(getConstraint() == CONSERVATIVE, getConstraint());
    PRECICE_ASSERT(false, "Not yet implemented");
    PRECICE_DEBUG("Compute conservative mapping");
  }
  _hasComputedMapping = true;
}

void AxialGeoMultiscaleMapping::clear()
{
  PRECICE_TRACE();
  _hasComputedMapping = false;
}

void AxialGeoMultiscaleMapping::mapConservative(DataID inputDataID, DataID outputDataID)
{
  PRECICE_ASSERT(getConstraint() == CONSERVATIVE, getConstraint());
  PRECICE_ASSERT(false, "Not yet implemented");
  PRECICE_DEBUG("Map conservative");
}

void AxialGeoMultiscaleMapping::mapConsistent(DataID inputDataID, DataID outputDataID)
{
  PRECICE_TRACE(inputDataID, outputDataID);

  const Eigen::VectorXd &inputValues     = input()->data(inputDataID)->values();
  int                    valueDimensions = input()->data(inputDataID)->getDimensions();
  Eigen::VectorXd &      outputValues    = output()->data(outputDataID)->values();

  PRECICE_ASSERT(valueDimensions == output()->data(outputDataID)->getDimensions(),
                 valueDimensions, output()->data(outputDataID)->getDimensions());
  PRECICE_ASSERT(inputValues.size() / valueDimensions == static_cast<int>(input()->vertices().size()),
                 inputValues.size(), valueDimensions, input()->vertices().size());
  PRECICE_ASSERT(outputValues.size() / valueDimensions == static_cast<int>(output()->vertices().size()),
                 outputValues.size(), valueDimensions, output()->vertices().size());

  PRECICE_DEBUG("Map consistent");
  if (_type == SPREAD) {
    PRECICE_ASSERT(inputValues.size() == valueDimensions);
    PRECICE_ASSERT(input()->vertices().size() == 1);
    mesh::Vertex &v0      = input()->vertices()[0];
    size_t const  outSize = output()->vertices().size();
    for (size_t i = 0; i < outSize; i++) {
      Eigen::VectorXd difference(valueDimensions);
      difference = v0.getCoords();
      difference -= output()->vertices()[i].getCoords();
      double distance = difference.norm() / _radius;
      PRECICE_CHECK(distance <= 1.05, "Output mesh has vertices that do not coincide with the geometric multiscale interface defined by the input mesh. Ratio of vertex distance to radius is {}.", distance);
      for (int dim = 0; dim < valueDimensions; dim++) {
        outputValues((i * valueDimensions) + dim) = 2 * inputValues(dim) * (1 - distance * distance);
      }
    }
  } else {
    PRECICE_ASSERT(_type == COLLECT);
    PRECICE_ASSERT(output()->vertices().size() == 1);
    PRECICE_ASSERT(outputValues.size() == valueDimensions);
    for (int dim = 0; dim < valueDimensions; dim++) {
      outputValues(dim) = 0.0;
    }
    size_t const inSize = input()->vertices().size();
    for (size_t i = 0; i < inSize; i++) {
      for (int dim = 0; dim < valueDimensions; dim++) {
        outputValues(dim) += inputValues((i * valueDimensions) + dim) / inSize;
      }
    }
  }
}

void AxialGeoMultiscaleMapping::tagMeshFirstRound()
{
  PRECICE_TRACE();

  computeMapping();

  if (getConstraint() == CONSISTENT) {
    PRECICE_ASSERT(_type == SPREAD, "Not yet implemented");
    PRECICE_ASSERT(input()->vertices().size() == 1);

    input()->vertices()[0].tag();

  } else {
    PRECICE_ASSERT(getConstraint() == CONSERVATIVE, getConstraint());
    PRECICE_ASSERT(false, "Not yet implemented");
  }

  clear();
}

void AxialGeoMultiscaleMapping::tagMeshSecondRound()
{
  PRECICE_TRACE();
  // no operation needed here for the moment
}

} // namespace mapping
} // namespace precice
