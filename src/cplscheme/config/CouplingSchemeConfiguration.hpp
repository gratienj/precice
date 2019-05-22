#pragma once

#include <string>
#include <tuple>
#include <vector>
#include "cplscheme/Constants.hpp"
#include "cplscheme/CouplingScheme.hpp"
#include "cplscheme/MultiCouplingScheme.hpp"
#include "cplscheme/SharedPointer.hpp"
#include "cplscheme/impl/SharedPointer.hpp"
#include "logging/Logger.hpp"
#include "m2n/config/M2NConfiguration.hpp"
#include "mesh/SharedPointer.hpp"
#include "precice/config/SharedPointer.hpp"
#include "precice/impl/MeshContext.hpp"
#include "xml/XMLTag.hpp"

namespace precice
{
namespace cplscheme
{
class CompositionalCouplingScheme;
class BaseCouplingScheme;
}
}

// Forward declaration to friend the boost test struct
namespace CplSchemeTests
{
namespace ParallelImplicitCouplingSchemeTests
{
struct testParseConfigurationWithRelaxation;
}
namespace SerialImplicitCouplingSchemeTests
{
struct testParseConfigurationWithRelaxation;
}
}

// ----------------------------------------------------------- CLASS DEFINITION
namespace precice
{
namespace cplscheme
{
/// Configuration for coupling schemes.
class CouplingSchemeConfiguration : public xml::XMLTag::Listener
{
public:
  /**
   * @brief Constructor.
   *
   * @param[in] parent  Used to add subtags to hierarchical XML structure.
   * @param[in] meshConfig For checking if a used mesh is defined.
   * @param[in] comConfig For checking if a communication between participants to be coupled is defined.
   */
  CouplingSchemeConfiguration(
      xml::XMLTag &                               parent,
      const mesh::PtrMeshConfiguration &          meshConfig,
      const m2n::M2NConfiguration::SharedPointer &m2nConfig);

  /// Destructor, empty.
  virtual ~CouplingSchemeConfiguration() {}

  /// Check, if a coupling scheme is configured for a participant.
  bool hasCouplingScheme(const std::string &participantName) const;

  /// Returns the configured coupling scheme.
  const PtrCouplingScheme &getCouplingScheme(const std::string &participantName) const;

  /// Returns the name of one dataset exchanged in the coupling scheme.
  const std::string &getDataToExchange(int index) const;

  /// Callback method required when using xml::XMLTag.
  virtual void xmlTagCallback(xml::XMLTag &callingTag);

  /// Callback method required when using xml::XMLTag.
  virtual void xmlEndTagCallback(xml::XMLTag &callingTag);

  /// Adds a manually configured coupling scheme for a participant.
  void addCouplingScheme(PtrCouplingScheme cplScheme, const std::string &participantName);

private:

  mutable logging::Logger _log{"cplscheme::CouplingSchemeConfiguration"};

  const std::string TAG;
  const std::string TAG_PARTICIPANTS;
  const std::string TAG_PARTICIPANT;
  const std::string TAG_EXCHANGE;
  const std::string TAG_MAX_TIME;
  const std::string TAG_MAX_TIMESTEPS;
  const std::string TAG_TIMESTEP_LENGTH;
  const std::string TAG_ABS_CONV_MEASURE;
  const std::string TAG_REL_CONV_MEASURE;
  const std::string TAG_RES_REL_CONV_MEASURE;
  const std::string TAG_MIN_ITER_CONV_MEASURE;
  const std::string TAG_MAX_ITERATIONS;
  const std::string TAG_EXTRAPOLATION;

  const std::string ATTR_DATA;
  const std::string ATTR_MESH;
  const std::string ATTR_PARTICIPANT;
  const std::string ATTR_INITIALIZE;
  const std::string ATTR_TYPE;
  const std::string ATTR_FIRST;
  const std::string ATTR_SECOND;
  const std::string ATTR_VALUE;
  const std::string ATTR_VALID_DIGITS;
  const std::string ATTR_METHOD;
  const std::string ATTR_LIMIT;
  const std::string ATTR_MIN_ITERATIONS;
  const std::string ATTR_NAME;
  const std::string ATTR_TIMESTEP_INTERVAL;
  const std::string ATTR_FROM;
  const std::string ATTR_TO;
  const std::string ATTR_SUFFICES;
  const std::string ATTR_CONTROL;
  const std::string ATTR_LEVEL;

  const std::string VALUE_SERIAL_EXPLICIT;
  const std::string VALUE_PARALLEL_EXPLICIT;
  const std::string VALUE_SERIAL_IMPLICIT;
  const std::string VALUE_PARALLEL_IMPLICIT;
  const std::string VALUE_MULTI;
  const std::string VALUE_FIXED;
  const std::string VALUE_FIRST_PARTICIPANT;

  struct Config {
    std::string                   type;
    std::string                   name;
    std::vector<std::string>      participants;
    std::string                   controller;
    bool                          setController = false;
    double                        maxTime = CouplingScheme::UNDEFINED_TIME;
    int                           maxTimesteps = CouplingScheme::UNDEFINED_TIMESTEPS;
    double                        timestepLength = CouplingScheme::UNDEFINED_TIMESTEP_LENGTH;
    int                           validDigits = 16;
    constants::TimesteppingMethod dtMethod = constants::FIXED_DT;
    /// Tuples of exchange data, mesh, and participant name.
    typedef std::tuple<mesh::PtrData, mesh::PtrMesh, std::string, std::string, bool> Exchange;
    std::vector<Exchange>                                                            exchanges;
    /// Tuples of data ID, mesh ID, and convergence measure.
    std::vector<std::tuple<mesh::PtrData, bool, std::string, int, impl::PtrConvergenceMeasure>> convMeasures;
    int                                                                               maxIterations = -1;
    int                                                                               extrapolationOrder = 0;

  } _config;

  mesh::PtrMeshConfiguration _meshConfig;

  m2n::M2NConfiguration::SharedPointer _m2nConfig;

  PtrPostProcessingConfiguration _postProcConfig;

  /// Map from participant name to coupling scheme (composition).
  std::map<std::string, PtrCouplingScheme> _couplingSchemes;

  /// If a participant has more than one coupling scheme, a composition is created.
  std::map<std::string, CompositionalCouplingScheme *> _couplingSchemeCompositions;

  void addTypespecifcSubtags(const std::string &type, xml::XMLTag &tag);

  void addTransientLimitTags(xml::XMLTag &tag);

  void addTagParticipants(xml::XMLTag &tag);

  void addTagParticipant(xml::XMLTag &tag);

  void addTagExchange(xml::XMLTag &tag);

  void addTagAbsoluteConvergenceMeasure(xml::XMLTag &tag);

  void addTagRelativeConvergenceMeasure(xml::XMLTag &tag);

  void addTagResidualRelativeConvergenceMeasure(xml::XMLTag &tag);

  void addTagMinIterationConvergenceMeasure(xml::XMLTag &tag);

  void addBaseAttributesTagConvergenceMeasure(xml::XMLTag &tag);

  void addTagMaxIterations(xml::XMLTag &tag);

  void addTagExtrapolation(xml::XMLTag &tag);

  void addTagPostProcessing(xml::XMLTag &tag);

  void addAbsoluteConvergenceMeasure(
      const std::string &dataName,
      const std::string &meshName,
      double             limit,
      bool               suffices,
      int                level);

  void addRelativeConvergenceMeasure(
      const std::string &dataName,
      const std::string &meshName,
      double             limit,
      bool               suffices,
      int                level);

  void addResidualRelativeConvergenceMeasure(
      const std::string &dataName,
      const std::string &meshName,
      double             limit,
      bool               suffices,
      int                level);

  void addMinIterationConvergenceMeasure(
      const std::string &dataName,
      const std::string &meshName,
      int                minIterations,
      bool               suffices,
      int                level);

  mesh::PtrData getData(
      const std::string &dataName,
      const std::string &meshName) const;

  PtrCouplingScheme createSerialExplicitCouplingScheme(
      const std::string &accessor) const;

  PtrCouplingScheme createParallelExplicitCouplingScheme(
      const std::string &accessor) const;

  PtrCouplingScheme createSerialImplicitCouplingScheme(
      const std::string &accessor) const;

  PtrCouplingScheme createParallelImplicitCouplingScheme(
      const std::string &accessor) const;

  PtrCouplingScheme createMultiCouplingScheme(
      const std::string &accessor) const;

  /// returns name of the actual scheme holder (i.e. server name)
  std::string determineCouplingSchemeHolder(
      const std::string &accessorName) const;

  constants::TimesteppingMethod getTimesteppingMethod(
      const std::string &method) const;

  /// Adds configured exchange data to be sent or received to scheme.
  void addDataToBeExchanged(
      BaseCouplingScheme &scheme,
      const std::string & accessor) const;

  /**
   * @brief Adds configured exchange data to be sent or received to scheme.
   * Only used specifically for MultiCouplingScheme
   */
  void addMultiDataToBeExchanged(
      MultiCouplingScheme &scheme,
      const std::string &  accessor) const;

  void checkIfDataIsExchanged(
      int dataID) const;

  bool checkIfDataIsCoarse(int id) const;

  friend struct CplSchemeTests::ParallelImplicitCouplingSchemeTests::testParseConfigurationWithRelaxation;  // For whitebox tests
  friend struct CplSchemeTests::SerialImplicitCouplingSchemeTests::testParseConfigurationWithRelaxation;  // For whitebox tests

};
}
} // namespace precice, cplscheme
