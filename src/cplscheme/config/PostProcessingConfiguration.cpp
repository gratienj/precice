#include "PostProcessingConfiguration.hpp"
#include "cplscheme/impl/AitkenPostProcessing.hpp"
#include "cplscheme/impl/BaseQNPostProcessing.hpp"
#include "cplscheme/impl/BroydenPostProcessing.hpp"
#include "cplscheme/impl/ConstantPreconditioner.hpp"
#include "cplscheme/impl/ConstantRelaxationPostProcessing.hpp"
#include "cplscheme/impl/HierarchicalAitkenPostProcessing.hpp"
#include "cplscheme/impl/IQNILSPostProcessing.hpp"
#include "cplscheme/impl/MMPostProcessing.hpp"
#include "cplscheme/impl/MVQNPostProcessing.hpp"
#include "cplscheme/impl/PostProcessing.hpp"
#include "cplscheme/impl/ResidualPreconditioner.hpp"
#include "cplscheme/impl/ResidualSumPreconditioner.hpp"
#include "cplscheme/impl/ValuePreconditioner.hpp"
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/config/MeshConfiguration.hpp"
#include "xml/XMLAttribute.hpp"
#include "xml/XMLTag.hpp"

namespace precice
{
namespace cplscheme
{

PostProcessingConfiguration::PostProcessingConfiguration(
    const mesh::PtrMeshConfiguration &meshConfig)
    : TAG("post-processing"),
      TAG_RELAX("relaxation"),
      TAG_INIT_RELAX("initial-relaxation"),
      TAG_MAX_USED_ITERATIONS("max-used-iterations"),
      TAG_TIMESTEPS_REUSED("timesteps-reused"),
      TAG_DATA("data"),
      TAG_FILTER("filter"),
      TAG_ESTIMATEJACOBIAN("estimate-jacobian"),
      TAG_PRECONDITIONER("preconditioner"),
      TAG_IMVJRESTART("imvj-restart-mode"),
      ATTR_NAME("name"),
      ATTR_MESH("mesh"),
      ATTR_SCALING("scaling"),
      ATTR_VALUE("value"),
      ATTR_ENFORCE("enforce"),
      ATTR_SINGULARITYLIMIT("limit"),
      ATTR_TYPE("type"),
      ATTR_BUILDJACOBIAN("always-build-jacobian"),
      ATTR_IMVJCHUNKSIZE("chunk-size"),
      ATTR_RSLS_REUSEDTSTEPS("reused-timesteps-at-restart"),
      ATTR_RSSVD_TRUNCATIONEPS("truncation-threshold"),
      ATTR_PRECOND_NONCONST_TIMESTEPS("freeze-after"),
      VALUE_CONSTANT("constant"),
      VALUE_AITKEN("aitken"),
      VALUE_HIERARCHICAL_AITKEN("hierarchical-aitken"),
      VALUE_IQNILS("IQN-ILS"),
      VALUE_MVQN("IQN-IMVJ"),
      VALUE_ManifoldMapping("MM"),
      VALUE_BROYDEN("broyden"),
      VALUE_QR1FILTER("QR1"),
      VALUE_QR1_ABSFILTER("QR1-absolute"),
      VALUE_QR2FILTER("QR2"),
      VALUE_CONSTANT_PRECONDITIONER("constant"),
      VALUE_VALUE_PRECONDITIONER("value"),
      VALUE_RESIDUAL_PRECONDITIONER("residual"),
      VALUE_RESIDUAL_SUM_PRECONDITIONER("residual-sum"),
      VALUE_LS_RESTART("RS-LS"),
      VALUE_ZERO_RESTART("RS-0"),
      VALUE_SVD_RESTART("RS-SVD"),
      VALUE_SLIDE_RESTART("RS-SLIDE"),
      VALUE_NO_RESTART("no-restart"),
      _meshConfig(meshConfig),
      _postProcessing(),
      _coarseModelOptimizationConfig(),
      _neededMeshes(),
      _preconditioner(),
      _config(),
      _isAddManifoldMappingTagAllowed(true)
{
  assertion(meshConfig.get() != nullptr);
}

void PostProcessingConfiguration::connectTags(xml::XMLTag &parent)
{
  using namespace xml;

  // static int recursionCounter = 0;
  // recursionCounter++;

  XMLTag::Occurrence occ = XMLTag::OCCUR_NOT_OR_ONCE;
  std::list<XMLTag>  tags;
  {
    XMLTag tag(*this, VALUE_CONSTANT, occ, TAG);
    addTypeSpecificSubtags(tag);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, VALUE_AITKEN, occ, TAG);
    addTypeSpecificSubtags(tag);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, VALUE_HIERARCHICAL_AITKEN, occ, TAG);
    addTypeSpecificSubtags(tag);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, VALUE_IQNILS, occ, TAG);
    addTypeSpecificSubtags(tag);
    tags.push_back(tag);
  }
  {
    XMLTag tag(*this, VALUE_MVQN, occ, TAG);

    auto alwaybuildJacobian = makeXMLAttribute(ATTR_BUILDJACOBIAN, false)
        .setDocumentation("If set to true, the IMVJ will set up the Jacobian matrix"
                " in each coupling iteration, which is inefficient. If set to false (or not set)"
                " the Jacobian is only build in the last iteration and the updates are computed using (relatively) cheap MATVEC products.");
    tag.addAttribute(alwaybuildJacobian);

    addTypeSpecificSubtags(tag);
    tags.push_back(tag);
  }
  //if(recursionCounter <= 1){
  if (_isAddManifoldMappingTagAllowed) {
    {
      XMLTag tag(*this, VALUE_ManifoldMapping, occ, TAG);
      addTypeSpecificSubtags(tag);
      tags.push_back(tag);
    }
    _isAddManifoldMappingTagAllowed = false;
  }
  {
    XMLTag tag(*this, VALUE_BROYDEN, occ, TAG);
    addTypeSpecificSubtags(tag);
    tags.push_back(tag);
  }

  for (XMLTag &tag : tags) {
    parent.addSubtag(tag);
  }
}

impl::PtrPostProcessing PostProcessingConfiguration::getPostProcessing()
{
  return _postProcessing;
}

PtrPostProcessingConfiguration PostProcessingConfiguration::getCoarseModelOptimizationConfig()
{
  return _coarseModelOptimizationConfig;
}

void PostProcessingConfiguration::xmlTagCallback(
    xml::XMLTag &callingTag)
{
  TRACE(callingTag.getFullName());

  if (callingTag.getNamespace() == TAG) {
    _config.type = callingTag.getName();

    if (_config.type == VALUE_MVQN)
      _config.alwaysBuildJacobian = callingTag.getBooleanAttributeValue(ATTR_BUILDJACOBIAN);
  }

  if (callingTag.getName() == TAG_RELAX) {
    _config.relaxationFactor = callingTag.getDoubleAttributeValue(ATTR_VALUE);
  } else if (callingTag.getName() == TAG_DATA) {
    std::string dataName = callingTag.getStringAttributeValue(ATTR_NAME);
    _meshName            = callingTag.getStringAttributeValue(ATTR_MESH);
    double scaling       = 1.0;
    if (_config.type == VALUE_IQNILS || _config.type == VALUE_MVQN ||
        _config.type == VALUE_ManifoldMapping || _config.type == VALUE_BROYDEN) {
      scaling = callingTag.getDoubleAttributeValue(ATTR_SCALING);
    }

    for (mesh::PtrMesh mesh : _meshConfig->meshes()) {
      if (mesh->getName() == _meshName) {
        for (mesh::PtrData data : mesh->data()) {
          if (dataName == data->getName()) {
            _config.dataIDs.push_back(data->getID());
            _config.scalings.insert(std::make_pair(data->getID(), scaling));
          }
        }
      }
    }

    if (_config.dataIDs.empty()) {
      std::ostringstream stream;
      stream << "Data with name \"" << dataName << "\" associated to mesh \""
             << _meshName << "\" not found on configuration of post-processing";
      throw stream.str();
    }
    _neededMeshes.push_back(_meshName);
  } else if (callingTag.getName() == TAG_INIT_RELAX) {
    _config.relaxationFactor       = callingTag.getDoubleAttributeValue(ATTR_VALUE);
    _config.forceInitialRelaxation = callingTag.getBooleanAttributeValue(ATTR_ENFORCE);
  } else if (callingTag.getName() == TAG_MAX_USED_ITERATIONS) {
    _config.maxIterationsUsed = callingTag.getIntAttributeValue(ATTR_VALUE);
  } else if (callingTag.getName() == TAG_TIMESTEPS_REUSED) {
    _config.timestepsReused = callingTag.getIntAttributeValue(ATTR_VALUE);
  } else if (callingTag.getName() == TAG_FILTER) {
    auto f = callingTag.getStringAttributeValue(ATTR_TYPE);
    if (f == VALUE_QR1FILTER) {
      _config.filter = impl::PostProcessing::QR1FILTER;
    } else if (f == VALUE_QR1_ABSFILTER) {
      _config.filter = impl::PostProcessing::QR1FILTER_ABS;
    } else if (f == VALUE_QR2FILTER) {
      _config.filter = impl::PostProcessing::QR2FILTER;
    } else {
      assertion(false);
    }
    _config.singularityLimit = callingTag.getDoubleAttributeValue(ATTR_SINGULARITYLIMIT);
  } else if (callingTag.getName() == TAG_ESTIMATEJACOBIAN) {
    if (_config.type == VALUE_ManifoldMapping)
      _config.estimateJacobian = callingTag.getBooleanAttributeValue(ATTR_VALUE);
  } else if (callingTag.getName() == TAG_PRECONDITIONER) {
    _config.preconditionerType       = callingTag.getStringAttributeValue(ATTR_TYPE);
    _config.precond_nbNonConstTSteps = callingTag.getIntAttributeValue(ATTR_PRECOND_NONCONST_TIMESTEPS);
  } else if (callingTag.getName() == TAG_IMVJRESTART) {

    if (_config.alwaysBuildJacobian)
      ERROR("IMVJ can not be in restart mode while parameter always-build-jacobian is set true.");

#ifndef PRECICE_NO_MPI
    _config.imvjChunkSize = callingTag.getIntAttributeValue(ATTR_IMVJCHUNKSIZE);
    auto f                = callingTag.getStringAttributeValue(ATTR_TYPE);
    if (f == VALUE_NO_RESTART) {
      _config.imvjRestartType = impl::MVQNPostProcessing::NO_RESTART;
    } else if (f == VALUE_ZERO_RESTART) {
      _config.imvjRestartType = impl::MVQNPostProcessing::RS_ZERO;
    } else if (f == VALUE_LS_RESTART) {
      _config.imvjRSLS_reustedTimesteps = callingTag.getIntAttributeValue(ATTR_RSLS_REUSEDTSTEPS);
      _config.imvjRestartType           = impl::MVQNPostProcessing::RS_LS;
    } else if (f == VALUE_SVD_RESTART) {
      _config.imvjRSSVD_truncationEps = callingTag.getDoubleAttributeValue(ATTR_RSSVD_TRUNCATIONEPS);
      _config.imvjRestartType         = impl::MVQNPostProcessing::RS_SVD;
    } else if (f == VALUE_SLIDE_RESTART) {
      _config.imvjRestartType = impl::MVQNPostProcessing::RS_SLIDE;
    } else {
      _config.imvjChunkSize = 0;
      assertion(false);
    }
#else
    ERROR("Post processing IQN-IMVJ only works if preCICE is compiled with MPI");
#endif
  }
}

void PostProcessingConfiguration::xmlEndTagCallback(
    xml::XMLTag &callingTag)
{
  TRACE(callingTag.getName());
  if (callingTag.getNamespace() == TAG) {

    //create preconditioner
    if (callingTag.getName() == VALUE_IQNILS || callingTag.getName() == VALUE_MVQN || callingTag.getName() == VALUE_ManifoldMapping) {

      // if imvj restart-mode is of type RS-SVD, max number of non-const preconditioned time steps is limited by the chunksize
      if (callingTag.getName() == VALUE_MVQN && _config.imvjRestartType > 0)
        if (_config.precond_nbNonConstTSteps > _config.imvjChunkSize)
          _config.precond_nbNonConstTSteps = _config.imvjChunkSize;

      if (_config.preconditionerType == VALUE_CONSTANT_PRECONDITIONER) {
        std::vector<double> factors;
        for (int id : _config.dataIDs) {
          factors.push_back(_config.scalings[id]);
        }
        _preconditioner = impl::PtrPreconditioner(new impl::ConstantPreconditioner(factors));
      } else if (_config.preconditionerType == VALUE_VALUE_PRECONDITIONER) {
        _preconditioner = impl::PtrPreconditioner(new impl::ValuePreconditioner(_config.precond_nbNonConstTSteps));
      } else if (_config.preconditionerType == VALUE_RESIDUAL_PRECONDITIONER) {
        _preconditioner = impl::PtrPreconditioner(new impl::ResidualPreconditioner(_config.precond_nbNonConstTSteps));
      } else if (_config.preconditionerType == VALUE_RESIDUAL_SUM_PRECONDITIONER) {
        _preconditioner = impl::PtrPreconditioner(new impl::ResidualSumPreconditioner(_config.precond_nbNonConstTSteps));
      } else {
        // no preconditioner defined
        std::vector<double> factors;
        for (int id = 0; id < (int) _config.dataIDs.size(); ++id) {
          factors.push_back(1.0);
        }
        _preconditioner = impl::PtrPreconditioner(new impl::ConstantPreconditioner(factors));
      }
    }

    if (callingTag.getName() == VALUE_CONSTANT) {
      _postProcessing = impl::PtrPostProcessing(
          new impl::ConstantRelaxationPostProcessing(
              _config.relaxationFactor, _config.dataIDs));
    } else if (callingTag.getName() == VALUE_AITKEN) {
      _postProcessing = impl::PtrPostProcessing(
          new impl::AitkenPostProcessing(
              _config.relaxationFactor, _config.dataIDs));
    } else if (callingTag.getName() == VALUE_HIERARCHICAL_AITKEN) {
      _postProcessing = impl::PtrPostProcessing(
          new impl::HierarchicalAitkenPostProcessing(
              _config.relaxationFactor, _config.dataIDs));
    } else if (callingTag.getName() == VALUE_IQNILS) {
      _postProcessing = impl::PtrPostProcessing(
          new impl::IQNILSPostProcessing(
              _config.relaxationFactor,
              _config.forceInitialRelaxation,
              _config.maxIterationsUsed,
              _config.timestepsReused,
              _config.filter, _config.singularityLimit,
              _config.dataIDs,
              _preconditioner));
    } else if (callingTag.getName() == VALUE_MVQN) {
#ifndef PRECICE_NO_MPI
      _postProcessing = impl::PtrPostProcessing(
          new impl::MVQNPostProcessing(
              _config.relaxationFactor,
              _config.forceInitialRelaxation,
              _config.maxIterationsUsed,
              _config.timestepsReused,
              _config.filter, _config.singularityLimit,
              _config.dataIDs,
              _preconditioner,
              _config.alwaysBuildJacobian,
              _config.imvjRestartType,
              _config.imvjChunkSize,
              _config.imvjRSLS_reustedTimesteps,
              _config.imvjRSSVD_truncationEps));
#else
      ERROR("Post processing IQN-IMVJ only works if preCICE is compiled with MPI");
#endif
    } else if (callingTag.getName() == VALUE_ManifoldMapping) {

      // create coarse model optimization method recursive
      assertion((_coarseModelOptimizationConfig.get() != nullptr));
      assertion((_coarseModelOptimizationConfig->getPostProcessing().get() != nullptr));

      // create manifold mapping PP
      _postProcessing = impl::PtrPostProcessing(
          new impl::MMPostProcessing(
              _coarseModelOptimizationConfig->getPostProcessing(), // coarse model optimization method
              _config.maxIterationsUsed,
              _config.timestepsReused,
              _config.filter, _config.singularityLimit,
              _config.estimateJacobian,
              _config.dataIDs,                                                   // fine data IDs
              _coarseModelOptimizationConfig->getPostProcessing()->getDataIDs(), // coarse data IDs
              _preconditioner));
    } else if (callingTag.getName() == VALUE_BROYDEN) {
      _postProcessing = impl::PtrPostProcessing(
          new impl::BroydenPostProcessing(
              _config.relaxationFactor,
              _config.forceInitialRelaxation,
              _config.maxIterationsUsed,
              _config.timestepsReused,
              _config.filter, _config.singularityLimit,
              _config.dataIDs,
              _preconditioner));
    } else {
      assertion(false);
    }
  }
}

void PostProcessingConfiguration::clear()
{
  _config         = ConfigurationData();
  _postProcessing = impl::PtrPostProcessing();
  _neededMeshes.clear();
}

void PostProcessingConfiguration::addTypeSpecificSubtags(
    xml::XMLTag &tag)
{
  using namespace xml;
  if (tag.getName() == VALUE_CONSTANT) {
    XMLTag               tagRelax(*this, TAG_RELAX, XMLTag::OCCUR_ONCE);
    XMLAttribute<double> attrValue(ATTR_VALUE);
    tagRelax.addAttribute(attrValue);
    tag.addSubtag(tagRelax);
  } else if (tag.getName() == VALUE_AITKEN) {
    XMLTag               tagInitRelax(*this, TAG_INIT_RELAX, XMLTag::OCCUR_ONCE);
    XMLAttribute<double> attrValue(ATTR_VALUE);
    tagInitRelax.addAttribute(attrValue);
    XMLAttribute<bool> attrEnforce(ATTR_ENFORCE, false);
    tagInitRelax.addAttribute(attrEnforce);
    tag.addSubtag(tagInitRelax);

    XMLTag                    tagData(*this, TAG_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
    XMLAttribute<std::string> attrName(ATTR_NAME);
    XMLAttribute<std::string> attrMesh(ATTR_MESH);
    tagData.addAttribute(attrName);
    tagData.addAttribute(attrMesh);
    tag.addSubtag(tagData);
  } else if (tag.getName() == VALUE_HIERARCHICAL_AITKEN) {
    XMLTag               tagInitRelax(*this, TAG_INIT_RELAX, XMLTag::OCCUR_ONCE);
    XMLAttribute<double> attrValue(ATTR_VALUE);
    tagInitRelax.addAttribute(attrValue);
    XMLAttribute<bool> attrEnforce(ATTR_ENFORCE, false);
    tagInitRelax.addAttribute(attrEnforce);
    tag.addSubtag(tagInitRelax);

    XMLTag                    tagData(*this, TAG_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
    XMLAttribute<std::string> attrName(ATTR_NAME);
    XMLAttribute<std::string> attrMesh(ATTR_MESH);
    tagData.addAttribute(attrName);
    tagData.addAttribute(attrMesh);
    tag.addSubtag(tagData);
  } else if (tag.getName() == VALUE_IQNILS) {
    XMLTag               tagInitRelax(*this, TAG_INIT_RELAX, XMLTag::OCCUR_ONCE);
    XMLAttribute<double> attrDoubleValue(ATTR_VALUE);
    tagInitRelax.addAttribute(attrDoubleValue);
    XMLAttribute<bool> attrEnforce(ATTR_ENFORCE, false);
    tagInitRelax.addAttribute(attrEnforce);
    tag.addSubtag(tagInitRelax);

    XMLTag            tagMaxUsedIter(*this, TAG_MAX_USED_ITERATIONS, XMLTag::OCCUR_ONCE);
    XMLAttribute<int> attrIntValue(ATTR_VALUE);
    tagMaxUsedIter.addAttribute(attrIntValue);
    tag.addSubtag(tagMaxUsedIter);

    XMLTag tagTimestepsReused(*this, TAG_TIMESTEPS_REUSED, XMLTag::OCCUR_ONCE);
    tagTimestepsReused.addAttribute(attrIntValue);
    tag.addSubtag(tagTimestepsReused);

    XMLTag                    tagData(*this, TAG_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
    XMLAttribute<std::string> attrName(ATTR_NAME);
    XMLAttribute<std::string> attrMesh(ATTR_MESH);
    auto attrScaling = makeXMLAttribute(ATTR_SCALING, 1.0)
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes, "
                "data values can be manually scaled. We recommend, however, to use an automatic scaling via a preconditioner.");
    tagData.addAttribute(attrScaling);
    tagData.addAttribute(attrName);
    tagData.addAttribute(attrMesh);
    tag.addSubtag(tagData);

    XMLTag                       tagFilter(*this, TAG_FILTER, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrFilterName = XMLAttribute<std::string>(ATTR_TYPE)
        .setOptions({
                VALUE_QR1FILTER,
                VALUE_QR1_ABSFILTER,
                VALUE_QR2FILTER});
    tagFilter.addAttribute(attrFilterName);
    XMLAttribute<double> attrSingularityLimit(ATTR_SINGULARITYLIMIT, 1e-16);
    tagFilter.addAttribute(attrSingularityLimit);
    tagFilter.setDocumentation("Type of filtering technique that is used to "
                               "maintain good conditioning in the least-squares system. Possible filters:\n"
                               "  QR1-filter: updateQR-dec with (relative) test R(i,i) < eps *||R||\n"
                               "  QR1_absolute-filter: updateQR-dec with (absolute) test R(i,i) < eps|\n"
                               "  QR2-filter: en-block QR-dec with test |v_orth| < eps * |v|\n"
                               "Please note that a QR1 is based on Given's rotations whereas QR2 uses "
                               "modified Gram-Schmidt. This can give different results even when no columns "
                               "are filtered out.");
    tag.addSubtag(tagFilter);

    XMLTag                       tagPreconditioner(*this, TAG_PRECONDITIONER, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrPreconditionerType = XMLAttribute<std::string>(ATTR_TYPE)
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes a preconditioner"
                " can be applied. A constant preconditioner scales every post-processing data by a constant value, which you can define as"
                " an attribute of data. "
                " A value preconditioner scales every post-processing data by the norm of the data in the previous timestep."
                " A residual preconditioner scales every post-processing data by the current residual."
                " A residual-sum preconditioner scales every post-processing data by the sum of the residuals from the current timestep.")
        .setOptions({
                VALUE_CONSTANT_PRECONDITIONER,
                VALUE_VALUE_PRECONDITIONER,
                VALUE_RESIDUAL_PRECONDITIONER,
                VALUE_RESIDUAL_SUM_PRECONDITIONER});
    tagPreconditioner.addAttribute(attrPreconditionerType);
    auto nonconstTSteps = makeXMLAttribute(ATTR_PRECOND_NONCONST_TIMESTEPS, -1)
        .setDocumentation(
                "After the given number of time steps, the preconditioner weights "
                "are freezed and the preconditioner acts like a constant preconditioner.");
    tagPreconditioner.addAttribute(nonconstTSteps);
    tag.addSubtag(tagPreconditioner);

  } else if (tag.getName() == VALUE_MVQN) {
    XMLTag               tagInitRelax(*this, TAG_INIT_RELAX, XMLTag::OCCUR_ONCE);
    XMLAttribute<double> attrDoubleValue(ATTR_VALUE);
    tagInitRelax.addAttribute(attrDoubleValue);
    XMLAttribute<bool> attrEnforce(ATTR_ENFORCE, false);
    tagInitRelax.addAttribute(attrEnforce);
    tag.addSubtag(tagInitRelax);

    XMLTag                       tagIMVJRESTART(*this, TAG_IMVJRESTART, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrRestartName = XMLAttribute<std::string>(ATTR_TYPE)
        .setOptions({
                VALUE_NO_RESTART,
                VALUE_ZERO_RESTART,
                VALUE_LS_RESTART,
                VALUE_SVD_RESTART,
                VALUE_SLIDE_RESTART})
        .setDefaultValue(VALUE_SVD_RESTART);
    tagIMVJRESTART.addAttribute(attrRestartName);
    tagIMVJRESTART.setDocumentation("Type of IMVJ restart mode that is used\n"
                                    "  no-restart: IMVJ runs in normal mode with explicit representation of Jacobian\n"
                                    "  RS-ZERO:    IMVJ runs in restart mode. After M time steps all Jacobain information is dropped, restart with no information\n"
                                    "  RS-LS:      IMVJ runs in restart mode. After M time steps a IQN-LS like approximation for the initial guess of the Jacobian is computed.\n"
                                    "  RS-SVD:     IMVJ runs in restart mode. After M time steps a truncated SVD of the Jacobian is updated.\n"
                                    "  RS-SLIDE:   IMVJ runs in sliding window restart mode.\n");
    auto attrChunkSize = makeXMLAttribute(ATTR_IMVJCHUNKSIZE, 8)
        .setDocumentation("Specifies the number of time steps M after which the IMVJ restarts, if run in restart-mode. Defaul value is M=8.");
    auto attrReusedTimeStepsAtRestart = makeXMLAttribute(ATTR_RSLS_REUSEDTSTEPS, 8)
        .setDocumentation("If IMVJ restart-mode=RS-LS, the number of reused time steps at restart can be specified.");
    auto attrRSSVD_truncationEps = makeXMLAttribute(ATTR_RSSVD_TRUNCATIONEPS, 1e-4)
        .setDocumentation("If IMVJ restart-mode=RS-SVD, the truncation threshold for the updated SVD can be set.");
    tagIMVJRESTART.addAttribute(attrChunkSize);
    tagIMVJRESTART.addAttribute(attrReusedTimeStepsAtRestart);
    tagIMVJRESTART.addAttribute(attrRSSVD_truncationEps);
    tag.addSubtag(tagIMVJRESTART);

    XMLTag            tagMaxUsedIter(*this, TAG_MAX_USED_ITERATIONS, XMLTag::OCCUR_ONCE);
    XMLAttribute<int> attrIntValue(ATTR_VALUE);
    tagMaxUsedIter.addAttribute(attrIntValue);
    tag.addSubtag(tagMaxUsedIter);

    XMLTag tagTimestepsReused(*this, TAG_TIMESTEPS_REUSED, XMLTag::OCCUR_ONCE);
    tagTimestepsReused.addAttribute(attrIntValue);
    tag.addSubtag(tagTimestepsReused);

    XMLTag                    tagData(*this, TAG_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
    XMLAttribute<std::string> attrName(ATTR_NAME);
    XMLAttribute<std::string> attrMesh(ATTR_MESH);
    auto attrScaling = makeXMLAttribute(ATTR_SCALING, 1.0)
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes, "
                "data values can be manually scaled. We recommend, however, to use an automatic scaling via a preconditioner.");
    tagData.addAttribute(attrScaling);
    tagData.addAttribute(attrName);
    tagData.addAttribute(attrMesh);
    tag.addSubtag(tagData);

    XMLTag               tagFilter(*this, TAG_FILTER, XMLTag::OCCUR_NOT_OR_ONCE);
    XMLAttribute<double> attrSingularityLimit(ATTR_SINGULARITYLIMIT, 1e-16);
    tagFilter.addAttribute(attrSingularityLimit);
   auto attrFilterName = XMLAttribute<std::string>(ATTR_TYPE)
        .setOptions({
                VALUE_QR1FILTER,
                VALUE_QR1_ABSFILTER,
                VALUE_QR2FILTER});
    tagFilter.addAttribute(attrFilterName);
    tagFilter.setDocumentation("Type of filtering technique that is used to "
                               "maintain good conditioning in the least-squares system. Possible filters:\n"
                               "  QR1-filter: updateQR-dec with (relative) test R(i,i) < eps *||R||\n"
                               "  QR1_absolute-filter: updateQR-dec with (absolute) test R(i,i) < eps|\n"
                               "  QR2-filter: en-block QR-dec with test |v_orth| < eps * |v|\n"
                               "Please note that a QR1 is based on Given's rotations whereas QR2 uses "
                               "modified Gram-Schmidt. This can give different results even when no columns "
                               "are filtered out.");
    tag.addSubtag(tagFilter);

    XMLTag                       tagPreconditioner(*this, TAG_PRECONDITIONER, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrPreconditionerType = XMLAttribute<std::string>(ATTR_TYPE)
        .setOptions({
                VALUE_CONSTANT_PRECONDITIONER,
                VALUE_VALUE_PRECONDITIONER,
                VALUE_RESIDUAL_PRECONDITIONER,
                VALUE_RESIDUAL_SUM_PRECONDITIONER})
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes a preconditioner"
                " can be applied. A constant preconditioner scales every post-processing data by a constant value, which you can define as"
                " an attribute of data. "
                " A value preconditioner scales every post-processing data by the norm of the data in the previous timestep."
                " A residual preconditioner scales every post-processing data by the current residual."
                " A residual-sum preconditioner scales every post-processing data by the sum of the residuals from the current timestep.");
    tagPreconditioner.addAttribute(attrPreconditionerType);
    auto nonconstTSteps = makeXMLAttribute(ATTR_PRECOND_NONCONST_TIMESTEPS, -1)
        .setDocumentation("After the given number of time steps, the preconditioner weights are freezed and the preconditioner acts like a constant preconditioner.");
    tagPreconditioner.addAttribute(nonconstTSteps);
    tag.addSubtag(tagPreconditioner);
  } else if (tag.getName() == VALUE_ManifoldMapping) {

    // add coarse model optimization PostProcessing Tag
    // new PP config for coarse model optimization method (recursive definition)
    // _coarseModelOptimizationConfig->clear();
    if (_coarseModelOptimizationConfig.get() == nullptr) {
      _coarseModelOptimizationConfig = PtrPostProcessingConfiguration(
          new PostProcessingConfiguration(_meshConfig));
    }
    _coarseModelOptimizationConfig->setIsAddManifoldMappingTagAllowed(false);
    _coarseModelOptimizationConfig->connectTags(tag);

    XMLTag             tagEstimateJacobian(*this, TAG_ESTIMATEJACOBIAN, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrBoolValue = XMLAttribute<bool>(ATTR_VALUE)
        .setDocumentation(
                "If manifold mapping is used as post-processing one can switch"
                " between explicit estimation and updating of the Jacobian (multi-vector method)"
                " and a matrix free computation. The default is matrix free.");
    tagEstimateJacobian.addAttribute(attrBoolValue);
    tag.addSubtag(tagEstimateJacobian);

    XMLTag            tagMaxUsedIter(*this, TAG_MAX_USED_ITERATIONS, XMLTag::OCCUR_ONCE);
    XMLAttribute<int> attrIntValue(ATTR_VALUE);
    tagMaxUsedIter.addAttribute(attrIntValue);
    tag.addSubtag(tagMaxUsedIter);

    XMLTag tagTimestepsReused(*this, TAG_TIMESTEPS_REUSED, XMLTag::OCCUR_ONCE);
    tagTimestepsReused.addAttribute(attrIntValue);
    tag.addSubtag(tagTimestepsReused);

    XMLTag                    tagData(*this, TAG_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
    XMLAttribute<std::string> attrName(ATTR_NAME);
    XMLAttribute<std::string> attrMesh(ATTR_MESH);
    auto attrScaling = makeXMLAttribute(ATTR_SCALING, 1.0)
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes, "
                "data values can be manually scaled. We recommend, however, to use an automatic scaling via a preconditioner.");
    tagData.addAttribute(attrScaling);
    tagData.addAttribute(attrName);
    tagData.addAttribute(attrMesh);
    tag.addSubtag(tagData);

    XMLTag                       tagFilter(*this, TAG_FILTER, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrFilterName = XMLAttribute<std::string>(ATTR_TYPE)
        .setOptions({
                VALUE_QR1FILTER,
                VALUE_QR1_ABSFILTER,
                VALUE_QR2FILTER});
    tagFilter.addAttribute(attrFilterName);
    XMLAttribute<double> attrSingularityLimit(ATTR_SINGULARITYLIMIT, 1e-16);
    tagFilter.addAttribute(attrSingularityLimit);
    tagFilter.setDocumentation("Type of filtering technique that is used to "
                               "maintain good conditioning in the least-squares system. Possible filters:\n"
                               "  QR1-filter: updateQR-dec with (relative) test R(i,i) < eps *||R||\n"
                               "  QR1_absolute-filter: updateQR-dec with (absolute) test R(i,i) < eps|\n"
                               "  QR2-filter: en-block QR-dec with test |v_orth| < eps * |v|\n"
                               "Please note that a QR1 is based on Given's rotations whereas QR2 uses "
                               "modified Gram-Schmidt. This can give different results even when no columns "
                               "are filtered out.");
    tag.addSubtag(tagFilter);

    XMLTag                       tagPreconditioner(*this, TAG_PRECONDITIONER, XMLTag::OCCUR_NOT_OR_ONCE);
   auto attrPreconditionerType = XMLAttribute<std::string>(ATTR_TYPE)
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes a preconditioner"
                " can be applied. A constant preconditioner scales every post-processing data by a constant value, which you can define as"
                " an attribute of data. "
                " A value preconditioner scales every post-processing data by the norm of the data in the previous timestep."
                " A residual preconditioner scales every post-processing data by the current residual."
                " A residual-sum preconditioner scales every post-processing data by the sum of the residuals from the current timestep.")
    .setOptions({
            VALUE_CONSTANT_PRECONDITIONER,
            VALUE_VALUE_PRECONDITIONER,
            VALUE_RESIDUAL_PRECONDITIONER,
            VALUE_RESIDUAL_SUM_PRECONDITIONER});
    tagPreconditioner.addAttribute(attrPreconditionerType);
    auto nonconstTSteps = makeXMLAttribute(ATTR_PRECOND_NONCONST_TIMESTEPS, -1)
        .setDocumentation(
                "After the given number of time steps, the preconditioner weights are "
                "freezed and the preconditioner acts like a constant preconditioner.");
    tagPreconditioner.addAttribute(nonconstTSteps);
    tag.addSubtag(tagPreconditioner);

  } else if (tag.getName() == VALUE_BROYDEN) {
    XMLTag               tagInitRelax(*this, TAG_INIT_RELAX, XMLTag::OCCUR_ONCE);
    XMLAttribute<double> attrDoubleValue(ATTR_VALUE);
    tagInitRelax.addAttribute(attrDoubleValue);
    XMLAttribute<bool> attrEnforce(ATTR_ENFORCE, false);
    tagInitRelax.addAttribute(attrEnforce);
    tag.addSubtag(tagInitRelax);

    XMLTag            tagMaxUsedIter(*this, TAG_MAX_USED_ITERATIONS, XMLTag::OCCUR_ONCE);
    XMLAttribute<int> attrIntValue(ATTR_VALUE);
    tagMaxUsedIter.addAttribute(attrIntValue);
    tag.addSubtag(tagMaxUsedIter);

    XMLTag tagTimestepsReused(*this, TAG_TIMESTEPS_REUSED, XMLTag::OCCUR_ONCE);
    tagTimestepsReused.addAttribute(attrIntValue);
    tag.addSubtag(tagTimestepsReused);

    XMLTag                    tagData(*this, TAG_DATA, XMLTag::OCCUR_ONCE_OR_MORE);
    XMLAttribute<std::string> attrName(ATTR_NAME);
    XMLAttribute<std::string> attrMesh(ATTR_MESH);
    auto attrScaling = makeXMLAttribute(ATTR_SCALING, 1.0)
        .setDocumentation(
                "To improve the performance of a parallel or a multi coupling schemes, "
                "data values can be manually scaled. We recommend, however, to use an automatic scaling via a preconditioner.");
    tagData.addAttribute(attrScaling);
    tagData.addAttribute(attrName);
    tagData.addAttribute(attrMesh);
    tag.addSubtag(tagData);
  } else {
    ERROR("Post-processing of type \""
          << tag.getName() << "\" is unknown!");
  }
}
}
} // namespace precice, cplscheme
