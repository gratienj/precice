#include "Edge.hpp"

namespace precice {
namespace mesh {

Edge:: Edge
(
  Vertex& vertexOne,
  Vertex& vertexTwo,
  int     id )
:
  _vertices( {&vertexOne, &vertexTwo} ),
  _id ( id ),
  _normal ( Eigen::VectorXd::Constant(vertexOne.getDimensions(), 0.0) ),
  _center ( Eigen::VectorXd::Constant(vertexOne.getDimensions(), 0.0) )
{
  assertion ( vertexOne.getDimensions() == vertexTwo.getDimensions(),
              vertexOne.getDimensions(), vertexTwo.getDimensions() );
}

void Edge:: setEnclosingRadius
(
  double radius )
{
  _enclosingRadius = radius;
}

int Edge:: getID () const
{
  return _id;
}

const Eigen::VectorXd& Edge::getCenter () const
{
  return _center;
}

double Edge:: getEnclosingRadius () const
{
  return _enclosingRadius;
}

std::ostream& operator<<(std::ostream& stream, const Edge& edge){
    stream << "LINESTRING (";
    for (int i = 0; i < 2; i++){
        stream << edge.vertex(i).getCoords().transpose();
        if (i < 1)
            stream << ", ";
    }
    return stream << ")";
}

bool Edge::operator==(const Edge& other) const
{
    return math::equals(_normal, other._normal) &&
        std::is_permutation(_vertices.begin(), _vertices.end(), other._vertices.begin(),
                [](const Vertex* a, const Vertex* b){return *a == *b;});
}
bool Edge::operator!=(const Edge& other) const
{
  return !(*this == other);
}
}} // namespace precice, mesh
