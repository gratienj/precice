#include "ActionConfiguration.hpp"
#include "utils/xml/XMLAttribute.hpp"
#include "utils/xml/ValidatorEquals.hpp"
#include "utils/xml/ValidatorOr.hpp"
#include "action/ModifyCoordinatesAction.hpp"
#include "action/ScaleByAreaAction.hpp"
#include "action/ScaleByDtAction.hpp"
#include "action/ComputeCurvatureAction.hpp"
#include "action/BalanceVertexPositionAction.hpp"
#include "action/PythonAction.hpp"
#include "mesh/config/MeshConfiguration.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/Data.hpp"

namespace precice {
namespace action {

tarch::logging::Log ActionConfiguration::
  _log("precice::config::ActionConfiguration");

//const std::string& ActionConfiguration:: getTag()
//{
//  static std::string tag = "action";
//  return tag;
//}

ActionConfiguration:: ActionConfiguration
(
  utils::XMLTag&                    parent,
  const mesh::PtrMeshConfiguration& meshConfig )
:
  TAG("action"),
  NAME_DIVIDE_BY_AREA ( "divide-by-area" ),
  NAME_MULTIPLY_BY_AREA ( "multiply-by-area" ),
  NAME_SCALE_BY_COMPUTED_DT_RATIO ( "scale-by-computed-dt-ratio" ),
  NAME_SCALE_BY_COMPUTED_DT_PART_RATIO ( "scale-by-computed-dt-part-ratio" ),
  NAME_SCALE_BY_DT ( "scale-by-dt" ),
  NAME_ADD_TO_COORDINATES ( "add-to-coordinates" ),
  NAME_SUBTRACT_FROM_COORDINATES ( "subtract-from-coordinates" ),
  NAME_COMPUTE_CURVATURE ( "compute-curvature" ),
  NAME_BALANCE_VERTEX_POSITIONS ( "balance-vertex-positions" ),
  NAME_PYTHON ( "python" ),
  TAG_SOURCE_DATA ( "source-data" ),
  TAG_TARGET_DATA ( "target-data" ),
  TAG_CONVERGENCE_TOLERANCE ( "convergence-tolerance" ),
  TAG_MAX_ITERATIONS ( "max-iterations" ),
  TAG_MODULE_PATH ( "path" ),
  TAG_MODULE_NAME ( "module" ),
  ATTR_TYPE ( "type" ),
  ATTR_TIMING ( "timing" ),
  ATTR_NAME ( "name" ),
  ATTR_VALUE ( "value" ),
  ATTR_MESH ( "mesh" ),
  VALUE_REGULAR_PRIOR ( "regular-prior" ),
  VALUE_REGULAR_POST ( "regular-post" ),
  VALUE_ON_EXCHANGE_PRIOR ( "on-exchange-prior" ),
  VALUE_ON_EXCHANGE_POST ( "on-exchange-post" ),
  VALUE_ON_TIMESTEP_COMPLETE_POST("on-timestep-complete-post"),
  _meshConfig ( meshConfig ),
  _configuredAction(),
  _actions()
{
  using namespace utils;
  std::string doc;
  XMLTag tagSourceData(*this, TAG_SOURCE_DATA, XMLTag::OCCUR_ONCE);
  tagSourceData.setDocumentation("Data to read from.");
  XMLTag tagTargetData(*this, TAG_TARGET_DATA, XMLTag::OCCUR_ONCE);
  tagSourceData.setDocumentation("Data to read from and write to.");

  XMLAttribute<std::string> attrName(ATTR_NAME);
  attrName.setDocumentation("Name of data.");
  tagSourceData.addAttribute(attrName);
  tagTargetData.addAttribute(attrName);

  std::list<XMLTag> tags;
  XMLTag::Occurrence occ = XMLTag::OCCUR_ARBITRARY;
  {
    XMLTag tag(*this, NAME_ADD_TO_COORDINATES, occ, TAG);
    doc = "Adds data values to mesh vertex coordinates. Data type has to be vector.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagSourceData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_SUBTRACT_FROM_COORDINATES, occ, TAG);
    doc = "Subtracts data values to mesh vertex coordinates. Data type has to be vector.";
    tag.setDocumentation(doc);
    tag.addSubtag(tagSourceData);
    tags.push_back(tag);
  }
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
    XMLTag tag(*this, NAME_COMPUTE_CURVATURE, occ, TAG);
    tag.setDocumentation("Computes curvature values at mesh vertices.");
    tag.addSubtag(tagTargetData);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, NAME_BALANCE_VERTEX_POSITIONS, occ, TAG);
    doc = "Moves mesh vertices in iterative process until they are equally ";
    doc += "distributed over the mesh surface.";
    tag.setDocumentation(doc);
    XMLTag tagConvergenceTol(*this, TAG_CONVERGENCE_TOLERANCE, XMLTag::OCCUR_ONCE);
    doc = "Convergence tolerance for iterations, measured as L2-norm of vertex ";
    doc += "displacements.";
    tagConvergenceTol.setDocumentation(doc);
    XMLTag tagMaxIterations(*this, TAG_MAX_ITERATIONS, XMLTag::OCCUR_ONCE);
    tagMaxIterations.setDocumentation("Iteration limit.");
    XMLAttribute<double> attrDoubleValue(ATTR_VALUE);
    attrDoubleValue.setDocumentation("Convergence tolerance value.");
    XMLAttribute<int> attrIntValue(ATTR_VALUE);
    attrIntValue.setDocumentation("Maximal iterations value.");
    tagConvergenceTol.addAttribute(attrDoubleValue);
    tagMaxIterations.addAttribute(attrIntValue);
    tag.addSubtag(tagConvergenceTol);
    tag.addSubtag(tagMaxIterations);
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
    XMLAttribute<std::string> attrName(ATTR_NAME);
    tagModulePath.addAttribute(attrName);
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

  //XMLAttribute<std::string> attrType ( ATTR_TYPE );
  //ValidatorEquals<std::string> validDivideByArea ( VALUE_DIVIDE_BY_AREA );
  //ValidatorEquals<std::string> validMultiplyByArea ( VALUE_MULTIPLY_BY_AREA );
  //ValidatorEquals<std::string> validScaleByCompDtRatio ( VALUE_SCALE_BY_COMPUTED_DT_RATIO );
  //ValidatorEquals<std::string> validScaleByCompDtPartRatio ( VALUE_SCALE_BY_COMPUTED_DT_PART_RATIO );
  //ValidatorEquals<std::string> validScaleByDt ( VALUE_SCALE_BY_DT );
  //ValidatorEquals<std::string> validAddToCoordinates ( VALUE_ADD_TO_COORDINATES );
  //ValidatorEquals<std::string> validSubtractFromCoordinates ( VALUE_SUBTRACT_FROM_COORDINATES );
  //ValidatorEquals<std::string> validComputeCurvature ( VALUE_COMPUTE_CURVATURE );
  //ValidatorEquals<std::string> validBalanceVertexPos ( VALUE_BALANCE_VERTEX_POSITIONS );
  //ValidatorEquals<std::string> validPython ( VALUE_PYTHON );
  //attrType.setValidator ( validDivideByArea || validMultiplyByArea
  //    || validScaleByCompDtRatio || validScaleByCompDtPartRatio || validScaleByDt
  //    || validAddToCoordinates   || validSubtractFromCoordinates
  //    || validComputeCurvature || validBalanceVertexPos || validPython );
  //tag.addAttribute ( attrType );

  XMLAttribute<std::string> attrTiming ( ATTR_TIMING );
  doc = "Determines when (relative to advancing the coupling scheme) the action is executed.";
  attrTiming.setDocumentation(doc);
  ValidatorEquals<std::string> validRegularPrior ( VALUE_REGULAR_PRIOR );
  ValidatorEquals<std::string> validRegularPost ( VALUE_REGULAR_POST );
  ValidatorEquals<std::string> validOnExchangePrior ( VALUE_ON_EXCHANGE_PRIOR );
  ValidatorEquals<std::string> validOnExchangePost ( VALUE_ON_EXCHANGE_POST );
  ValidatorEquals<std::string> validOnTimestepCompletePost(VALUE_ON_TIMESTEP_COMPLETE_POST);
  attrTiming.setValidator(validRegularPrior || validRegularPost ||
                          validOnExchangePrior || validOnExchangePost ||
                          validOnTimestepCompletePost);

  XMLAttribute<std::string> attrMesh(ATTR_MESH);
  attrMesh.setDocumentation("Determines mesh used in action.");
  for (XMLTag& tag : tags) {
    tag.addAttribute(attrTiming);
    tag.addAttribute(attrMesh);
    parent.addSubtag(tag);
  }
}

//bool ActionConfiguration:: parseSubtag
//(
//  utils::XMLTag::XMLReader* xmlReader )
//{
//  preciceTrace ( "parseSubtag()" );
//  using namespace utils;
//  _configured = Configured();
//
//  XMLTag tag ( TAG, utils::XMLTag::OCCUR_ONCE );
//
//  XMLAttribute<std::string> attrType ( ATTR_TYPE );
//  ValidatorEquals<std::string> validDivideByArea ( VALUE_DIVIDE_BY_AREA );
//  ValidatorEquals<std::string> validMultiplyByArea ( VALUE_MULTIPLY_BY_AREA );
//  ValidatorEquals<std::string> validScaleByCompDtRatio ( VALUE_SCALE_BY_COMPUTED_DT_RATIO );
//  ValidatorEquals<std::string> validScaleByCompDtPartRatio ( VALUE_SCALE_BY_COMPUTED_DT_PART_RATIO );
//  ValidatorEquals<std::string> validScaleByDt ( VALUE_SCALE_BY_DT );
//  ValidatorEquals<std::string> validAddToCoordinates ( VALUE_ADD_TO_COORDINATES );
//  ValidatorEquals<std::string> validSubtractFromCoordinates ( VALUE_SUBTRACT_FROM_COORDINATES );
//  ValidatorEquals<std::string> validComputeCurvature ( VALUE_COMPUTE_CURVATURE );
//  ValidatorEquals<std::string> validBalanceVertexPos ( VALUE_BALANCE_VERTEX_POSITIONS );
//  ValidatorEquals<std::string> validPython ( VALUE_PYTHON );
//  attrType.setValidator ( validDivideByArea || validMultiplyByArea
//      || validScaleByCompDtRatio || validScaleByCompDtPartRatio || validScaleByDt
//      || validAddToCoordinates   || validSubtractFromCoordinates
//      || validComputeCurvature || validBalanceVertexPos || validPython );
//  tag.addAttribute ( attrType );
//
//  XMLAttribute<std::string> attrTiming ( ATTR_TIMING );
//  ValidatorEquals<std::string> validRegularPrior ( VALUE_REGULAR_PRIOR );
//  ValidatorEquals<std::string> validRegularPost ( VALUE_REGULAR_POST );
//  ValidatorEquals<std::string> validOnExchangePrior ( VALUE_ON_EXCHANGE_PRIOR );
//  ValidatorEquals<std::string> validOnExchangePost ( VALUE_ON_EXCHANGE_POST );
//  attrTiming.setValidator ( validRegularPrior || validRegularPost ||
//                            validOnExchangePrior || validOnExchangePost );
//  tag.addAttribute ( attrTiming );
//
//  XMLAttribute<std::string> attrMesh ( ATTR_MESH );
//  tag.addAttribute ( attrMesh );
//
//  _isValid = tag.parse ( xmlReader, *this );
//  if ( _isValid ){
//    createAction ();
//  }
//  return _isValid;
//}

void ActionConfiguration:: xmlTagCallback
(
  utils::XMLTag& callingTag )
{
  preciceTrace1("xmlTagCallback()", callingTag.getName());
  if (callingTag.getNamespace() == TAG){
    _configuredAction = ConfiguredAction();
    _configuredAction.type = callingTag.getName();
    _configuredAction.timing = callingTag.getStringAttributeValue(ATTR_TIMING);
    _configuredAction.mesh = callingTag.getStringAttributeValue(ATTR_MESH);
    //addSubtags ( callingTag, _configured.type );
  }
  else if (callingTag.getName() == TAG_SOURCE_DATA){
    _configuredAction.sourceData = callingTag.getStringAttributeValue(ATTR_NAME);
  }
  else if (callingTag.getName() == TAG_TARGET_DATA){
    _configuredAction.targetData = callingTag.getStringAttributeValue(ATTR_NAME);
  }
  else if (callingTag.getName() == TAG_CONVERGENCE_TOLERANCE){
    _configuredAction.convergenceTolerance =
        callingTag.getDoubleAttributeValue(ATTR_VALUE);
  }
  else if (callingTag.getName() == TAG_MAX_ITERATIONS){
    _configuredAction.maxIterations = callingTag.getIntAttributeValue(ATTR_VALUE);
  }
  else if (callingTag.getName() == TAG_MODULE_PATH){
    _configuredAction.path = callingTag.getStringAttributeValue(ATTR_NAME);
  }
  else if (callingTag.getName() == TAG_MODULE_NAME){
    _configuredAction.module = callingTag.getStringAttributeValue(ATTR_NAME);
  }
}

void ActionConfiguration:: xmlEndTagCallback
(
  utils::XMLTag& callingTag )
{
  if (callingTag.getNamespace() == TAG){
    createAction();
    //_isValid = true;
  }
}

//bool ActionConfiguration:: isValid() const
//{
//  return _isValid;
//}

int ActionConfiguration:: getUsedMeshID() const
{
  for (mesh::PtrMesh mesh : _meshConfig->meshes()) {
    if (mesh->getName() == _configuredAction.mesh){
      return mesh->getID();
    }
  }
  preciceError("getUsedMeshID()", "No mesh ID found!");
  return -1; // To please compiler
}

//const action::PtrAction & ActionConfiguration:: getAction () const
//{
//  assertion ( _isValid );
//  assertion ( _action.use_count() > 0 );
//  return _action;
//}

//void ActionConfiguration:: addSubtags
//(
//  utils::XMLTag&     callingTag,
//  const std::string& type )
//{
//  preciceTrace1 ( "addSubtags()", callingTag.getName() );
//  assertion ( type != std::string("") );
//  using utils::XMLTag;
//  using utils::XMLAttribute;
//  XMLTag tagSourceData ( TAG_SOURCE_DATA, XMLTag::OCCUR_ONCE );
//  XMLTag tagTargetData ( TAG_TARGET_DATA, XMLTag::OCCUR_ONCE );
//
//  XMLAttribute<std::string> attrName ( ATTR_NAME );
//  tagSourceData.addAttribute ( attrName );
//  tagTargetData.addAttribute ( attrName );
//
//  if ( _configured.type == VALUE_ADD_TO_COORDINATES ){
//    callingTag.addSubtag ( tagSourceData );
//  }
//  else if ( _configured.type == VALUE_SUBTRACT_FROM_COORDINATES ){
//    callingTag.addSubtag ( tagSourceData );
//  }
//  else if ( _configured.type == VALUE_MULTIPLY_BY_AREA ){
//    callingTag.addSubtag ( tagTargetData );
//  }
//  else if ( _configured.type == VALUE_DIVIDE_BY_AREA ){
//    callingTag.addSubtag ( tagTargetData );
//  }
//  else if ( _configured.type == VALUE_SCALE_BY_COMPUTED_DT_RATIO ){
//    callingTag.addSubtag ( tagSourceData );
//    callingTag.addSubtag ( tagTargetData );
//  }
//  else if ( _configured.type == VALUE_SCALE_BY_COMPUTED_DT_PART_RATIO ){
//    callingTag.addSubtag ( tagSourceData );
//    callingTag.addSubtag ( tagTargetData );
//  }
//  else if ( _configured.type == VALUE_SCALE_BY_DT ){
//    callingTag.addSubtag ( tagSourceData );
//    callingTag.addSubtag ( tagTargetData );
//  }
//  else if ( _configured.type == VALUE_COMPUTE_CURVATURE ){
//    callingTag.addSubtag ( tagTargetData );
//  }
//  else if ( _configured.type == VALUE_BALANCE_VERTEX_POSITIONS ){
//    XMLTag tagConvergenceTol ( TAG_CONVERGENCE_TOLERANCE, XMLTag::OCCUR_ONCE );
//    XMLTag tagMaxIterations ( TAG_MAX_ITERATIONS, XMLTag::OCCUR_ONCE );
//    XMLAttribute<double> attrDoubleValue ( ATTR_VALUE );
//    XMLAttribute<int> attrIntValue ( ATTR_VALUE );
//    tagConvergenceTol.addAttribute ( attrDoubleValue );
//    tagMaxIterations.addAttribute ( attrIntValue );
//    callingTag.addSubtag ( tagConvergenceTol );
//    callingTag.addSubtag ( tagMaxIterations );
//  }
//  else if ( _configured.type == VALUE_PYTHON ){
//    XMLTag tagModulePath ( TAG_MODULE_PATH, XMLTag::OCCUR_NOT_OR_ONCE );
//    XMLTag tagModule ( TAG_MODULE_NAME, XMLTag::OCCUR_ONCE );
//    XMLAttribute<std::string> attrName ( ATTR_NAME );
//    tagModulePath.addAttribute ( attrName );
//    tagModule.addAttribute ( attrName );
//    callingTag.addSubtag ( tagModulePath );
//    callingTag.addSubtag ( tagModule );
//    XMLTag tagOptionalSourceData ( TAG_SOURCE_DATA, XMLTag::OCCUR_NOT_OR_ONCE );
//    XMLTag tagOptionalTargetData ( TAG_TARGET_DATA, XMLTag::OCCUR_NOT_OR_ONCE );
//    tagOptionalSourceData.addAttribute ( attrName );
//    tagOptionalTargetData.addAttribute ( attrName );
//    callingTag.addSubtag ( tagOptionalSourceData );
//    callingTag.addSubtag ( tagOptionalTargetData );
//  }
//  else {
//    preciceError ( "addSubtags()", "Unknown action type \""
//                   << _configured.type << "\"!" );
//  }
//}

void ActionConfiguration:: createAction()
{
  preciceTrace("createAction()");

  assertion(_configuredAction.type != std::string(""));
  action::Action::Timing timing = getTiming();

  // Determine data and mesh
  int sourceDataID = -1;
  int targetDataID = -1;
  mesh::PtrMesh mesh;
  for (mesh::PtrMesh aMesh : _meshConfig->meshes()) {
    if (aMesh->getName() == _configuredAction.mesh){
      mesh = aMesh;
      for (const mesh::PtrData &data : mesh->data()) {
        if (data->getName() == _configuredAction.sourceData){
          sourceDataID = data->getID ();
        }
        if (data->getName() == _configuredAction.targetData){
          targetDataID = data->getID();
        }
      }
    }
  }
  if (mesh.get() == nullptr){
    std::ostringstream stream;
    stream << "Data action uses mesh \"" << _configuredAction.mesh
           << "\" which is not configured";
    throw stream.str();
  }
  if ((not _configuredAction.sourceData.empty()) && (sourceDataID == -1)){
    std::ostringstream stream;
    stream << "Data action uses source data \"" << _configuredAction.sourceData
           << "\" which is not configured";
    throw stream.str();
  }
  if ((not _configuredAction.targetData.empty()) && (targetDataID == -1)){
    std::ostringstream stream;
    stream << "Data action uses target data \"" << _configuredAction.targetData
           << "\" which is not configured";
    throw stream.str();
  }
  action::PtrAction action;
  if (_configuredAction.type == NAME_ADD_TO_COORDINATES){
    typedef action::ModifyCoordinatesAction Action;
    Action::Mode mode = Action::ADD_TO_COORDINATES_MODE;
    action = action::PtrAction (
        new Action(timing, sourceDataID, mesh, mode) );
  }
  else if (_configuredAction.type == NAME_SUBTRACT_FROM_COORDINATES){
    typedef action::ModifyCoordinatesAction Action;
    Action::Mode mode = Action::SUBTRACT_FROM_COORDINATES_MODE;
    action = action::PtrAction (
        new Action(timing, sourceDataID, mesh, mode) );
  }
  else if (_configuredAction.type == NAME_MULTIPLY_BY_AREA){
    action = action::PtrAction(
        new action::ScaleByAreaAction(timing, targetDataID,
        mesh, action::ScaleByAreaAction::SCALING_MULTIPLY_BY_AREA));
  }
  else if (_configuredAction.type == NAME_DIVIDE_BY_AREA){
    action = action::PtrAction(
        new action::ScaleByAreaAction(timing, targetDataID,
        mesh, action::ScaleByAreaAction::SCALING_DIVIDE_BY_AREA));
  }
  else if (_configuredAction.type == NAME_SCALE_BY_COMPUTED_DT_RATIO){
    action = action::PtrAction(
        new action::ScaleByDtAction(timing, sourceDataID, targetDataID,
        mesh, action::ScaleByDtAction::SCALING_BY_COMPUTED_DT_RATIO));
  }
  else if ( _configuredAction.type == NAME_SCALE_BY_COMPUTED_DT_PART_RATIO ){
    action = action::PtrAction(
        new action::ScaleByDtAction(timing, sourceDataID, targetDataID,
        mesh, action::ScaleByDtAction::SCALING_BY_COMPUTED_DT_PART_RATIO));
  }
  else if ( _configuredAction.type == NAME_SCALE_BY_DT ){
    action = action::PtrAction(
        new action::ScaleByDtAction(timing, sourceDataID, targetDataID,
        mesh, action::ScaleByDtAction::SCALING_BY_DT));
  }
  else if (_configuredAction.type == NAME_COMPUTE_CURVATURE){
    action = action::PtrAction(
        new action::ComputeCurvatureAction(timing, targetDataID,
        mesh));
  }
  else if ( _configuredAction.type == NAME_BALANCE_VERTEX_POSITIONS ){
    action = action::PtrAction (
        new action::BalanceVertexPositionAction(timing, mesh,
        _configuredAction.convergenceTolerance, _configuredAction.maxIterations) );
  }
  else if ( _configuredAction.type == NAME_PYTHON ){
    action = action::PtrAction (
        new action::PythonAction(timing, _configuredAction.path, _configuredAction.module,
        mesh, targetDataID, sourceDataID) );
  }
  assertion(action.get() != nullptr);
  _actions.push_back(action);
}

action::Action::Timing ActionConfiguration:: getTiming () const
{
  preciceTrace1 ( "getTiming()", _configuredAction.timing );
  action::Action::Timing timing;
  if ( _configuredAction.timing == VALUE_REGULAR_PRIOR ){
    timing = action::Action::ALWAYS_PRIOR;
  }
  else if ( _configuredAction.timing == VALUE_REGULAR_POST ){
    timing = action::Action::ALWAYS_POST;
  }
  else if ( _configuredAction.timing == VALUE_ON_EXCHANGE_PRIOR ){
    timing = action::Action::ON_EXCHANGE_PRIOR;
  }
  else if ( _configuredAction.timing == VALUE_ON_EXCHANGE_POST ){
    timing = action::Action::ON_EXCHANGE_POST;
  }
  else if (_configuredAction.timing == VALUE_ON_TIMESTEP_COMPLETE_POST){
    timing = action::Action::ON_TIMESTEP_COMPLETE_POST;
  }
  else {
    preciceError ( "getTiming()", "Unknown action timing \""
                   <<  _configuredAction.timing << "\"!" );
  }
  return timing;
}

}} // namespace precice, config

