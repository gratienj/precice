#include "MappingConfigurationTest.hpp"
#include "mesh/config/DataConfiguration.hpp"
#include "mesh/config/MeshConfiguration.hpp"
#include "mapping/config/MappingConfiguration.hpp"
#include "mapping/Mapping.hpp"
#include "utils/Parallel.hpp"
#include "utils/Globals.hpp"
#include "utils/xml/XMLTag.hpp"

#include "tarch/tests/TestCaseFactory.h"
registerTest(precice::mapping::tests::MappingConfigurationTest)

namespace precice {
namespace mapping {
namespace tests {

tarch::logging::Log MappingConfigurationTest::
  _log ( "precice::mapping::tests::MappingConfigurationTest" );

MappingConfigurationTest::MappingConfigurationTest ()
:
  TestCase ( "mapping::tests::MappingConfigurationTest" ),
  _pathToTests ()
{}

void MappingConfigurationTest:: setUp()
{
  _pathToTests = utils::Globals::getPathToSources() + "/mapping/tests/";
}

void MappingConfigurationTest:: run()
{
  PRECICE_MASTER_ONLY {
    preciceTrace("run()");
    std::string file(_pathToTests + "mapping-config.xml");
    using utils::XMLTag;
    XMLTag tag = utils::getRootTag();
    mesh::PtrDataConfiguration dataConfig( new mesh::DataConfiguration(tag) );
    dataConfig->setDimensions(3);
    mesh::PtrMeshConfiguration meshConfig(new mesh::MeshConfiguration(tag, dataConfig));
    meshConfig->setDimensions(3);
    MappingConfiguration mappingConfig(tag, meshConfig);
    utils::configure(tag, file);
    //validate ( success );

    validateEquals(meshConfig->meshes().size(), 3);
    validateEquals(mappingConfig.mappings().size(), 3);
    validateEquals(mappingConfig.mappings()[0].timing, MappingConfiguration::ON_DEMAND);
    validateEquals(mappingConfig.mappings()[0].fromMesh, meshConfig->meshes()[0]);
    validateEquals(mappingConfig.mappings()[0].toMesh, meshConfig->meshes()[2]);
    validateEquals(mappingConfig.mappings()[0].direction, MappingConfiguration::WRITE);

    validateEquals(mappingConfig.mappings()[1].timing, MappingConfiguration::INITIAL);
    validateEquals(mappingConfig.mappings()[1].fromMesh, meshConfig->meshes()[2]);
    validateEquals(mappingConfig.mappings()[1].toMesh, meshConfig->meshes()[1]);
    validateEquals(mappingConfig.mappings()[1].direction, MappingConfiguration::READ);

    validateEquals(mappingConfig.mappings()[2].timing, MappingConfiguration::ON_ADVANCE);
    validateEquals(mappingConfig.mappings()[2].fromMesh, meshConfig->meshes()[1]);
    validateEquals(mappingConfig.mappings()[2].toMesh, meshConfig->meshes()[0]);
    validateEquals(mappingConfig.mappings()[2].direction, MappingConfiguration::WRITE);
  }
}

}}} // namespace precice, mapping, tests
