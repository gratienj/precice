#include "precice/impl/DataContext.hpp"
#include "mesh/Data.hpp"

namespace precice {
namespace impl {

std::string DataContext::getName() const
{
  return fromData->getName();
}

} // namespace impl
} // namespace precice
