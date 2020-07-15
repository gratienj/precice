#include "ActionConfiguration.hpp"
#include <algorithm>
#include <memory>
#include <ostream>
#include <stdexcept>
#include "action/ComputeCurvatureAction.hpp"
#include "action/PythonAction.hpp"
#include "action/ScaleByAreaAction.hpp"
#include "action/ScaleByDtAction.hpp"
#include "action/SummationAction.hpp"
#include "logging/LogMacros.hpp"
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/config/MeshConfiguration.hpp"
#include "utils/assertion.hpp"
#include "xml/ConfigParser.hpp"
#include "xml/XMLAttribute.hpp"

namespace precice {
namespace action {

ActionConfiguration::ActionConfiguration(
    xml::XMLTag &                     parent,
    const mesh::PtrMeshConfiguration &meshConfig)
    : NAME_DIVIDE_BY_AREA("divide-by-area"),
      NAME_MULTIPLY_BY_AREA("multiply-by-area"),
      NAME_SCALE_BY_COMPUTED_DT_RATIO("scale-by-computed-dt-ratio"),
      NAME_SCALE_BY_COMPUTED_DT_PART_RATIO("scale-by-computed-dt-part-ratio"),
      NAME_SCALE_BY_DT("scale-by-dt"),
      NAME_SUMMATION("summation"),
      NAME_COMPUTE_CURVATURE("compute-curvature"),
      NAME_PYTHON("python"),
      TAG_SOURCE_DATA("source-data"),
      TAG_TARGET_DATA("target-data"),
      TAG_CONVERGENCE_TOLERANCE("convergence-tolerance"),
      TAG_MAX_ITERATIONS("max-iterations"),
      TAG_MODULE_PATH("path"),
      TAG_MODULE_NAME("module"),
      VALUE_REGULAR_PRIOR("regular-prior"),
      VALUE_REGULAR_POST("regular-post"),
      VALUE_ON_EXCHANGE_PRIOR("on-exchange-prior"),
      VALUE_ON_EXCHANGE_POST("on-exchange-post"),
      VALUE_ON_TIME_WINDOW_COMPLETE_POST("on-time-window-complete-post"),
      _meshConfig(meshConfig)
{
  using namespace xml;
  std::string doc;
  XMLTag      tagSourceData(*this, TAG_SOURCE_DATA, XMLTag::OCCUR_ONCE);
  tagSourceData.setDocumentation("Single data to read from. ");
  XMLTag tagMultipleSourceData(*this, TAG_SOURCE_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
  tagMultipleSourceData.setDocumentation("Multiple data to read from.");
  XMLTag tagTargetData(*this, TAG_TARGET_DATA, XMLTag::OCCUR_ONCE);
  tagTargetData.setDocumentation("Data to read from and write to.");

  auto attrName = XMLAttribute<std::string>(ATTR_NAME).setDocumentation("Name of data.");
  tagSourceData.addAttribute(attrName);
  tagMultipleSourceData.addAttribute(attrName);
  tagTargetData.addAttribute(attrName);

  std::list<XMLTag>  tags;
  XMLTag::Occurrence occ = XMLTag::OCCUR_ARBITRARY;
  {
    XMLTag tag(*this, NAME_MULTIPLY_BY_AREA, occ, TAG);
    doc = "Multiplies data values with mesh area associated to vertex holding the value.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_DIVIDE_BY_AREA, occ, TAG);
    doc = "Divides data values by mesh area associated to vertex holding the value.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_SCALE_BY_COMPUTED_DT_RATIO, occ, TAG);
    doc = "Multiplies source data values by ratio of full dt / last computed dt,";
    doc += " and writes the result into target data.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagSourceData);
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_SCALE_BY_COMPUTED_DT_PART_RATIO, occ, TAG);
    doc = "Multiplies source data values by ratio of full dt / computed dt part,";
    doc += " and writes the result into target data.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagSourceData);
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_SCALE_BY_DT, occ, TAG);
    doc = "Multiplies source data values by last computed dt, and writes the ";
    doc += "result into target data.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagSourceData);
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_SUMMATION, occ, TAG);
    doc = "Sums up multiple source data values and writes the result into ";
    doc += "target data.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagMultipleSourceData);
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_COMPUTE_CURVATURE, occ, TAG);
    tag.setDocumentation("Computes curvature values at mesh vertices.");
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_PYTHON, occ, TAG);
    doc = "Calls Python script to execute action.";
    doc += " See preCICE file \"src/action/PythonAction.py\" for an overview.";
    tag.setDocumentation(doc);
    XMLTag tagModulePath(*this, TAG_MODULE_PATH, XMLTag::OCCUR_NOT_OR_ONCE);
    doc = "Directory path to Python module, i.e. script file.";
    doc = " If it doesn't occur, the current path is used";
    tagModulePath.setDocumentation(doc);
    XMLTag tagModule(*this, TAG_MODULE_NAME, XMLTag::OCCUR_ONCE);
    doc = "Name of Python module, i.e. Python script file without file ending. ";
    doc += "The module name has to differ from existing (library) modules, ";
    doc += "otherwise, the existing module will be loaded instead of the user script.";
    tagModule.setDocumentation(doc);
    tagModulePath.addAttribute(makeXMLAttribute(ATTR_NAME, ""));
    tagModule.addAttribute(attrName);
    tag.addSubtag(tagModulePath);
    tag.addSubtag(tagModule);
    XMLTag tagOptionalSourceData(*this, TAG_SOURCE_DATA, XMLTag::OCCUR_NOT_OR_ONCE);
    doc = "Source data to be read is handed to the Python module.";
    doc += " Can be omitted, if only a target data is needed.";
    tagOptionalSourceData.setDocumentation(doc);
    XMLTag tagOptionalTargetData(*this, TAG_TARGET_DATA, XMLTag::OCCUR_NOT_OR_ONCE);
    doc = "Target data to be read and written to is handed to the Python module.";
    doc = " Can be omitted, if only source data is needed.";
    tagOptionalTargetData.setDocumentation(doc);
    tagOptionalSourceData.addAttribute(attrName);
    tagOptionalTargetData.addAttribute(attrName);
    tag.addSubtag(tagOptionalSourceData);
    tag.addSubtag(tagOptionalTargetData);
    tags.push_back(tag);
  }

  auto attrTiming = XMLAttribute<std::string>(ATTR_TIMING)
                        .setDocumentation(
                            "Determines when (relative to advancing the coupling scheme) the action is executed.")
                        .setOptions({VALUE_REGULAR_PRIOR, VALUE_REGULAR_POST,
                                     VALUE_ON_EXCHANGE_PRIOR, VALUE_ON_EXCHANGE_POST,
                                     VALUE_ON_TIME_WINDOW_COMPLETE_POST});

  auto attrMesh = XMLAttribute<std::string>(ATTR_MESH)
                      .setDocumentation("Determines mesh used in action.");
  for (XMLTag &tag : tags) {
    tag.addAttribute(attrTiming);
    tag.addAttribute(attrMesh);
    parent.addSubtag(tag);
  }
}

void ActionConfiguration::xmlTagCallback(
    const xml::ConfigurationContext &context,
    xml::XMLTag &                    callingTag)
{
  PRECICE_TRACE(callingTag.getName());
  if (callingTag.getNamespace() == TAG) {
    _configuredAction        = ConfiguredAction();
    _configuredAction.type   = callingTag.getName();
    _configuredAction.timing = callingTag.getStringAttributeValue(ATTR_TIMING);
    _configuredAction.mesh   = callingTag.getStringAttributeValue(ATTR_MESH);
    //addSubtags ( callingTag, _configured.type );
  } else if (callingTag.getName() == TAG_SOURCE_DATA) {
    _configuredAction.sourceDataVector.push_back(callingTag.getStringAttributeValue(ATTR_NAME));
  } else if (callingTag.getName() == TAG_TARGET_DATA) {
    _configuredAction.targetData = callingTag.getStringAttributeValue(ATTR_NAME);
  } else if (callingTag.getName() == TAG_CONVERGENCE_TOLERANCE) {
    _configuredAction.convergenceTolerance =
        callingTag.getDoubleAttributeValue(ATTR_VALUE);
  } else if (callingTag.getName() == TAG_MAX_ITERATIONS) {
    _configuredAction.maxIterations = callingTag.getIntAttributeValue(ATTR_VALUE);
  } else if (callingTag.getName() == TAG_MODULE_PATH) {
    _configuredAction.path = callingTag.getStringAttributeValue(ATTR_NAME);
  } else if (callingTag.getName() == TAG_MODULE_NAME) {
    _configuredAction.module = callingTag.getStringAttributeValue(ATTR_NAME);
  }
}

void ActionConfiguration::xmlEndTagCallback(
    const xml::ConfigurationContext &context,
    xml::XMLTag &                    callingTag)
{
  if (callingTag.getNamespace() == TAG) {
    createAction();
  }
}

int ActionConfiguration::getUsedMeshID() const
{
  for (mesh::PtrMesh mesh : _meshConfig->meshes()) {
    if (mesh->getName() == _configuredAction.mesh) {
      return mesh->getID();
    }
  }
  PRECICE_ERROR("No mesh ID found!");
  return -1; // To please compiler
}

void ActionConfiguration::createAction()
{
  PRECICE_TRACE();

  PRECICE_ASSERT(_configuredAction.type != std::string(""));
  action::Action::Timing timing = getTiming();

  // Determine data and mesh
  std::vector<int> sourceDataIDs;
  int              targetDataID = -1;
  mesh::PtrMesh    mesh;
  for (mesh::PtrMesh aMesh : _meshConfig->meshes()) {
    if (aMesh->getName() == _configuredAction.mesh) {
      mesh = aMesh;
      for (const mesh::PtrData &data : mesh->data()) {
        if (std::find(_configuredAction.sourceDataVector.begin(), _configuredAction.sourceDataVector.end(), data->getName()) != _configuredAction.sourceDataVector.end()) {
          sourceDataIDs.push_back(data->getID());
        }
        if (data->getName() == _configuredAction.targetData) {
          targetDataID = data->getID();
        }
      }
    }
  }
  if (mesh.get() == nullptr) {
    std::ostringstream stream;
    stream << "Data action uses mesh \"" << _configuredAction.mesh
           << "\" which is not configured";
    throw std::runtime_error{stream.str()};
  }
  if ((not _configuredAction.sourceDataVector.empty()) && (sourceDataIDs.empty())) {
    std::ostringstream stream;
    stream << "Data action uses source data \"" << _configuredAction.sourceDataVector.back()
           << "\" which is not configured";
    throw std::runtime_error{stream.str()};
  }
  if ((not _configuredAction.targetData.empty()) && (targetDataID == -1)) {
    std::ostringstream stream;
    stream << "Data action uses target data \"" << _configuredAction.targetData
           << "\" which is not configured";
    throw std::runtime_error{stream.str()};
  }
  action::PtrAction action;
  if (_configuredAction.type == NAME_MULTIPLY_BY_AREA) {
    PRECICE_CHECK(mesh->getDimensions() == 2,
                  "The action \"" << NAME_MULTIPLY_BY_AREA << "\" is only available for a solverinterface dimensionality of 2. "
                                                              "Please check the \"dimensions\" attribute of the <solverinterface> or use a custom action.");
    action = action::PtrAction(
        new action::ScaleByAreaAction(timing, targetDataID,
                                      mesh, action::ScaleByAreaAction::SCALING_MULTIPLY_BY_AREA));
  } else if (_configuredAction.type == NAME_DIVIDE_BY_AREA) {
    PRECICE_CHECK(mesh->getDimensions() == 2,
                  "The action \"" << NAME_DIVIDE_BY_AREA << "\" is only available for a solverinterface dimensionality of 2. "
                                                            "Please check the \"dimensions\" attribute of the <solverinterface> or use a custom action.");
    action = action::PtrAction(
        new action::ScaleByAreaAction(timing, targetDataID,
                                      mesh, action::ScaleByAreaAction::SCALING_DIVIDE_BY_AREA));
  } else if (_configuredAction.type == NAME_SCALE_BY_COMPUTED_DT_RATIO) {
    action = action::PtrAction(
        new action::ScaleByDtAction(timing, sourceDataIDs.back(), targetDataID,
                                    mesh, action::ScaleByDtAction::SCALING_BY_COMPUTED_DT_RATIO));
  } else if (_configuredAction.type == NAME_SCALE_BY_COMPUTED_DT_PART_RATIO) {
    action = action::PtrAction(
        new action::ScaleByDtAction(timing, sourceDataIDs.back(), targetDataID,
                                    mesh, action::ScaleByDtAction::SCALING_BY_COMPUTED_DT_PART_RATIO));
  } else if (_configuredAction.type == NAME_SCALE_BY_DT) {
    action = action::PtrAction(
        new action::ScaleByDtAction(timing, sourceDataIDs.back(), targetDataID,
                                    mesh, action::ScaleByDtAction::SCALING_BY_DT));
  } else if (_configuredAction.type == NAME_COMPUTE_CURVATURE) {
    action = action::PtrAction(
        new action::ComputeCurvatureAction(timing, targetDataID,
                                           mesh));
  } else if (_configuredAction.type == NAME_SUMMATION) {
    action = action::PtrAction(
        new action::SummationAction(timing, sourceDataIDs, targetDataID, mesh));
  }
#ifndef PRECICE_NO_PYTHON
  else if (_configuredAction.type == NAME_PYTHON) {
    action = action::PtrAction(
        new action::PythonAction(timing, _configuredAction.path, _configuredAction.module,
                                 mesh, targetDataID, sourceDataIDs.back()));
  }
#endif
  PRECICE_ASSERT(action.get() != nullptr);
  _actions.push_back(action);
}

action::Action::Timing ActionConfiguration::getTiming() const
{
  PRECICE_TRACE(_configuredAction.timing);
  action::Action::Timing timing;
  if (_configuredAction.timing == VALUE_REGULAR_PRIOR) {
    timing = action::Action::ALWAYS_PRIOR;
  } else if (_configuredAction.timing == VALUE_REGULAR_POST) {
    timing = action::Action::ALWAYS_POST;
  } else if (_configuredAction.timing == VALUE_ON_EXCHANGE_PRIOR) {
    timing = action::Action::ON_EXCHANGE_PRIOR;
  } else if (_configuredAction.timing == VALUE_ON_EXCHANGE_POST) {
    timing = action::Action::ON_EXCHANGE_POST;
  } else if (_configuredAction.timing == VALUE_ON_TIME_WINDOW_COMPLETE_POST) {
    timing = action::Action::ON_TIME_WINDOW_COMPLETE_POST;
  } else {
    PRECICE_ERROR("Unknown action timing \"" << _configuredAction.timing << "\"!");
  }
  return timing;
}

} // namespace action
} // namespace precice
