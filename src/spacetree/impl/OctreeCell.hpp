#ifndef PRECICE_NEWSPACETREE_OCTREECELL_HPP_
#define PRECICE_NEWSPACETREE_OCTREECELL_HPP_

#include "spacetree/Spacetree.hpp"
#include "mesh/Group.hpp"
#include "utils/PointerVector.hpp"
#include "tarch/logging/Log.h"
#include "utils/Helpers.hpp"
#include "query/FindVoxelContent.hpp"

namespace precice {
  namespace spacetree {
    namespace tests {
      class SpacetreeTestScenarios;
    }
  }
}

// ------------------------------------------------------------ CLASS DEFINITION

namespace precice {
namespace spacetree {
namespace impl {

class OctreeCell
{
public:

  OctreeCell();

  ~OctreeCell();

  bool isLeaf() const
  {
    return _content != NULL;
  }

  mesh::Group& content()
  {
    assertion(_content != NULL);
    return *_content;
  }

  bool needsRefinement (
    const utils::DynVector& cellHalflengths,
    double                  refinementLimit );

  int getPosition() const
  {
    return _position;
  }

  void setPosition ( int position )
  {
    _position = position;
  }

  void refine (
    const utils::DynVector& cellCenter,
    const utils::DynVector& cellHalflengths );

  int getChildCount()
  {
    return _childs.size();
  }

  OctreeCell& child ( int index )
  {
    assertion ( (index >= 0) && (index < (int)_childs.size()), index, _childs.size() );
    return _childs[index];
  }

  int getChildIndex (
    const utils::DynVector& searchPoint,
    const utils::DynVector& cellCenter,
    const utils::DynVector& cellHalflengths );

  /**
   * @brief Computes and sets child center and halflengths from parent cell.
   */
  void getChildData (
    int                     childIndex,
    const utils::DynVector& cellCenter,
    const utils::DynVector& cellHalflengths,
    utils::DynVector&       childCenter,
    utils::DynVector&       childHalflengths );

  void accept (
    Spacetree::Visitor& visitor,
    const utils::DynVector& center,
    const utils::DynVector& halflengths );

  void clear();

private:

  static tarch::logging::Log _log;

  mesh::Group* _content;

  int _position;

  utils::ptr_vector<OctreeCell> _childs;
};

}}} // namespace precice, spacetree, impl

#endif /* PRECICE_NEWSPACETREE_OCTREECELL_HPP_ */
