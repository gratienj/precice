#include "ParticipantConfiguration.hpp"
#include "precice/impl/MeshContext.hpp"
#include "precice/impl/MappingContext.hpp"
#include "precice/impl/DataContext.hpp"
#include "precice/impl/WatchPoint.hpp"
#include "com/config/CommunicationConfiguration.hpp"
#include "action/config/ActionConfiguration.hpp"
#include "mesh/config/MeshConfiguration.hpp"
#include "mapping/Mapping.hpp"
#include "mapping/config/MappingConfiguration.hpp"
#include "xml/XMLAttribute.hpp"
#include "utils/MasterSlave.hpp"
#include "com/MPIDirectCommunication.hpp"
#include "com/MPIPortsCommunication.hpp"
#include "io/ExportVTK.hpp"
#include "io/ExportVTKXML.hpp"
#include "io/ExportContext.hpp"
#include "io/SharedPointer.hpp"
#include "partition/ReceivedPartition.hpp"

namespace precice {
namespace config {

ParticipantConfiguration:: ParticipantConfiguration
(
  xml::XMLTag&                      parent,
  const mesh::PtrMeshConfiguration& meshConfiguration)
:
  _meshConfig(meshConfiguration)
{
  assertion(_meshConfig);
  using namespace xml;
  std::string doc;
  XMLTag tag(*this, TAG, XMLTag::OCCUR_ONCE_OR_MORE);
  doc = "Represents one solver using preCICE. At least two ";
  doc += "participants have to be defined.";
  tag.setDocumentation(doc);

 auto attrName = XMLAttribute<std::string>(ATTR_NAME)
      .setDocumentation(
              "Name of the participant. Has to match the name given on construction "
              "of the precice::SolverInterface object used by the participant.");
  tag.addAttribute(attrName);

  XMLTag tagWriteData(*this, TAG_WRITE, XMLTag::OCCUR_ARBITRARY);
  doc = "Sets data to be written by the participant to preCICE. ";
  doc += "Data is defined by using the <data> tag.";
  tagWriteData.setDocumentation(doc);
  XMLTag tagReadData(*this, TAG_READ, XMLTag::OCCUR_ARBITRARY);
  doc = "Sets data to be read by the participant from preCICE. ";
  doc += "Data is defined by using the <data> tag.";
  tagReadData.setDocumentation(doc);
 auto attrDataName = XMLAttribute<std::string>(ATTR_NAME)
      .setDocumentation("Name of the data.");
  tagWriteData.addAttribute(attrDataName);
  tagReadData.addAttribute(attrDataName);
 auto attrMesh = XMLAttribute<std::string>(ATTR_MESH)
      .setDocumentation(
              "Mesh the data belongs to. If data should be read/written to several "
              "meshes, this has to be specified separately for each mesh.");
  tagWriteData.addAttribute(attrMesh);
  tagReadData.addAttribute(attrMesh);
  tag.addSubtag(tagWriteData);
  tag.addSubtag(tagReadData);

  _mappingConfig = mapping::PtrMappingConfiguration(
                   new mapping::MappingConfiguration(tag, _meshConfig));

  _actionConfig = action::PtrActionConfiguration(
                  new action::ActionConfiguration(tag, _meshConfig));

  _exportConfig = io::PtrExportConfiguration(new io::ExportConfiguration(tag));

  XMLTag tagWatchPoint(*this, TAG_WATCH_POINT, XMLTag::OCCUR_ARBITRARY);
  doc = "A watch point can be used to follow the transient changes of data ";
  doc += "and mesh vertex coordinates at a given point";
  tagWatchPoint.setDocumentation(doc);
  doc = "Name of the watch point. Is taken in combination with the participant ";
  doc += "name to construct the filename the watch point data is written to.";
  attrName.setDocumentation(doc);
  tagWatchPoint.addAttribute(attrName);
  doc = "Mesh to be watched.";
  attrMesh.setDocumentation(doc);
  tagWatchPoint.addAttribute(attrMesh);
 auto attrCoordinate = XMLAttribute<Eigen::VectorXd>(ATTR_COORDINATE)
      .setDocumentation(
              "The coordinates of the watch point. If the watch point is not put exactly "
              "on the mesh to observe, the closest projection of the point onto the "
              "mesh is considered instead, and values/coordinates are interpolated "
              "linearly to that point.");
  tagWatchPoint.addAttribute(attrCoordinate);
  tag.addSubtag(tagWatchPoint);

  XMLTag tagUseMesh(*this, TAG_USE_MESH, XMLTag::OCCUR_ARBITRARY);
  doc = "Makes a mesh (see tag <mesh> available to a participant.";
  tagUseMesh.setDocumentation(doc);
  attrName.setDocumentation("Name of the mesh.");
  tagUseMesh.addAttribute(attrName);
//  XMLAttribute<Eigen::VectorXd> attrLocalOffset(ATTR_LOCAL_OFFSET);
//  doc = "The mesh can have an offset only applied for the local participant. ";
//  doc += "Vector-valued example: '1.0; 0.0; 0.0'";
//  attrLocalOffset.setDocumentation(doc);
//  attrLocalOffset.setDefaultValue(Eigen::VectorXd::Constant(3, 0));
//  tagUseMesh.addAttribute(attrLocalOffset);

 auto attrFrom = XMLAttribute<std::string>(ATTR_FROM, "")
      .setDocumentation(
              "If a created mesh should be used by "
              "another solver, this attribute has to specify the creating participant's"
              " name. The creator has to use the attribute \"provide\" to signal he is "
              "providing the mesh geometry.");
  tagUseMesh.addAttribute(attrFrom);
  auto attrSafetyFactor = makeXMLAttribute(ATTR_SAFETY_FACTOR, 0.1)
      .setDocumentation(
              "If a mesh is received from another partipant (see tag <from>), it needs to be"
              "decomposed at the receiving participant. To speed up this process, "
              "a geometric filter (see tag <geometric-filter>), i.e. filtering by bounding boxes around the local mesh, can be used. "
              "This safety factor defines by which factor this local information is "
              "increased. An example: 0.1 means that the bounding box is 110% of its original size.");
  tagUseMesh.addAttribute(attrSafetyFactor);

 auto attrGeoFilter = XMLAttribute<std::string>(ATTR_GEOMETRIC_FILTER)
      .setDocumentation(
              "If a mesh is received from another partipant (see tag <from>), it needs to be"
              "decomposed at the receiving participant. To speed up this process, "
              "a geometric filter, i.e. filtering by bounding boxes around the local mesh, can be used. "
              "2 different variants are implemented: a \"filter-first\" strategy, "
              "which is beneficial for a huge mesh and a low number of processors, and a "
              "\"broadcast/filter\" strategy, which performs better for a very high number of "
              "processors. Both result in the same distribution (if the safety factor is sufficiently large)."
              "For very asymmetric cases, the filter can also be switched off completely (\"no-filter\").")
      .setOptions({ VALUE_FILTER_FIRST, VALUE_BROADCAST_FILTER, VALUE_NO_FILTER })
      .setDefaultValue(VALUE_BROADCAST_FILTER);
  tagUseMesh.addAttribute(attrGeoFilter);

  auto attrProvide = makeXMLAttribute(ATTR_PROVIDE, false)
      .setDocumentation(
              "If this attribute is set to \"on\", the "
              "participant has to create the mesh geometry before initializing preCICE.");
  tagUseMesh.addAttribute(attrProvide);
  tag.addSubtag(tagUseMesh);

  std::list<XMLTag> serverTags;
  XMLTag::Occurrence serverOcc = XMLTag::OCCUR_NOT_OR_ONCE;
  {
    XMLTag tagServer(*this, "sockets", serverOcc, TAG_SERVER);
    doc = "When a solver runs in parallel, it can use preCICE in form of a ";
    doc += "separately running server (deprecated feature). This is enabled by this tag. ";
    doc += "The communication between participant and server is done by sockets.";
    tagServer.setDocumentation(doc);

    auto attrPort = makeXMLAttribute("port", 0)
        .setDocumentation(
                "Port number (16-bit unsigned integer) to be used for socket "
                "communiation. The default is \"0\", what means that OS will "
                "dynamically search for a free port (if at least one exists) and "
                "bind it automatically.");
    tagServer.addAttribute(attrPort);

    auto attrNetwork = makeXMLAttribute(ATTR_NETWORK, "lo")
        .setDocumentation(
                "Network name to be used for socket communiation. "
                "Default is \"lo\", i.e., the local host loopback.");
    tagServer.addAttribute(attrNetwork);

   auto attrExchangeDirectory = makeXMLAttribute(ATTR_EXCHANGE_DIRECTORY, "")
        .setDocumentation(
                "Directory where connection information is exchanged. By default, the "
                "directory of startup is chosen, and both solvers have to be started "
                "in the same directory.");
    tagServer.addAttribute(attrExchangeDirectory);

    serverTags.push_back(tagServer);
  }
  {
    XMLTag tagServer(*this, "mpi", serverOcc, TAG_SERVER);
    doc = "When a solver runs in parallel, it can use preCICE in form of a ";
    doc += "separately running server (deprecated feature). This is enabled by this tag. ";
    doc += "The communication between participant and server is done by mpi ";
    doc += "with startup in separated communication spaces.";
    tagServer.setDocumentation(doc);

   auto attrExchangeDirectory = makeXMLAttribute(ATTR_EXCHANGE_DIRECTORY, "")
        .setDocumentation(
                "Directory where connection information is exchanged. By default, the "
                "directory of startup is chosen, and both solvers have to be started "
                "in the same directory.");
    tagServer.addAttribute(attrExchangeDirectory);

    serverTags.push_back(tagServer);
  }
  {
    XMLTag tagServer(*this, "mpi-single", serverOcc, TAG_SERVER);
    doc = "When a solver runs in parallel, it can use preCICE in form of a ";
    doc += "separately running server (deprecated feature). This is enabled by this tag. ";
    doc += "The communication between participant and server is done by mpi ";
    doc += "with startup in a common communication space.";
    tagServer.setDocumentation(doc);
    serverTags.push_back(tagServer);
  }
  for (XMLTag& tagServer : serverTags){
    tag.addSubtag(tagServer);
  }

  std::list<XMLTag> masterTags;
  XMLTag::Occurrence masterOcc = XMLTag::OCCUR_NOT_OR_ONCE;
  {
    XMLTag tagMaster(*this, "sockets", masterOcc, TAG_MASTER);
    doc = "A solver in parallel has to use either a Master or a Server (Master is recommended), but not both. ";
    doc += "If you use a Master, you do not have to start-up a further executable, ";
    doc += "all communication is handled peer to peer. One solver process becomes the ";
    doc += " Master handling the synchronization of all slaves. Here, you define then ";
    doc += " the communication between the Master and all slaves. ";
    doc += "The communication between Master and slaves is done by sockets.";
    tagMaster.setDocumentation(doc);

    auto attrPort = makeXMLAttribute("port", 0)
        .setDocumentation(
                "Port number (16-bit unsigned integer) to be used for socket "
                "communiation. The default is \"0\", what means that OS will "
                "dynamically search for a free port (if at least one exists) and "
                "bind it automatically.");
    tagMaster.addAttribute(attrPort);

    auto attrNetwork = makeXMLAttribute(ATTR_NETWORK, "lo")
        .setDocumentation(
          "Network name to be used for socket communiation. "
          "Default is \"lo\", i.e., the local host loopback.");
    tagMaster.addAttribute(attrNetwork);

   auto attrExchangeDirectory = makeXMLAttribute(ATTR_EXCHANGE_DIRECTORY, "")
        .setDocumentation(
                "Directory where connection information is exchanged. By default, the "
                "directory of startup is chosen.");
    tagMaster.addAttribute(attrExchangeDirectory);

    masterTags.push_back(tagMaster);
  }
  {
    XMLTag tagMaster(*this, "mpi", masterOcc, TAG_MASTER);
    doc = "A solver in parallel has to use either a Master or a Server (Master is recommended), but not both. ";
    doc += "If you use a Master, you do not have to start-up a further executable, ";
    doc += "all communication is handled peer to peer. One solver process becomes the ";
    doc += " Master handling the synchronization of all slaves. Here, you define then ";
    doc += " the communication between the Master and all slaves. ";
    doc += "The communication between Master and slaves is done by mpi ";
    doc += "with startup in separated communication spaces.";
    tagMaster.setDocumentation(doc);

   auto attrExchangeDirectory = makeXMLAttribute(ATTR_EXCHANGE_DIRECTORY, "")
        .setDocumentation(
                "Directory where connection information is exchanged. By default, the "
                "directory of startup is chosen.");
    tagMaster.addAttribute(attrExchangeDirectory);

    masterTags.push_back(tagMaster);
  }
  {
    XMLTag tagMaster(*this, "mpi-single", masterOcc, TAG_MASTER);
    doc = "A solver in parallel has to use either a Master or a Server (Master is recommended), but not both. ";
    doc += "If you use a Master, you do not have to start-up a further executable, ";
    doc += "all communication is handled peer to peer. One solver process becomes the ";
    doc += " Master handling the synchronization of all slaves. Here, you define then ";
    doc += " the communication between the Master and all slaves. ";
    doc += "The communication between Master and slaves is done by mpi ";
    doc += "with startup in one communication spaces. (This choice is recommended)";
    tagMaster.setDocumentation(doc);

    masterTags.push_back(tagMaster);
  }
  for (XMLTag& tagMaster : masterTags){
    tag.addSubtag(tagMaster);
  }

  parent.addSubtag(tag);
}

void ParticipantConfiguration:: setDimensions
(
  int dimensions )
{
  TRACE(dimensions);
  assertion((dimensions == 2) || (dimensions == 3), dimensions);
  _dimensions = dimensions;
}


void ParticipantConfiguration:: xmlTagCallback
(
  xml::XMLTag& tag )
{
  TRACE(tag.getName() );
  if (tag.getName() == TAG){
    std::string name = tag.getStringAttributeValue(ATTR_NAME);
    impl::PtrParticipant p(new impl::Participant(name, _meshConfig));
    _participants.push_back(p);
  }
  else if (tag.getName() == TAG_USE_MESH){
    assertion(_dimensions != 0); // setDimensions() has been called
    std::string name = tag.getStringAttributeValue(ATTR_NAME);
    Eigen::VectorXd offset(_dimensions);
    /// @todo offset currently not supported
    //offset = tag.getEigenVectorXdAttributeValue(ATTR_LOCAL_OFFSET, _dimensions);
    std::string from = tag.getStringAttributeValue(ATTR_FROM);
    double safetyFactor = tag.getDoubleAttributeValue(ATTR_SAFETY_FACTOR);
    partition::ReceivedPartition::GeometricFilter geoFilter = getGeoFilter(tag.getStringAttributeValue(ATTR_GEOMETRIC_FILTER));
    if (safetyFactor < 0){
      std::ostringstream stream;
      stream << "Safety Factor must be positive or 0";
      throw stream.str();
    }
    bool provide = tag.getBooleanAttributeValue(ATTR_PROVIDE);
    mesh::PtrMesh mesh = _meshConfig->getMesh(name);
    if (mesh.get() == nullptr){
      std::ostringstream stream;
      stream << "Participant \"" << _participants.back()->getName()
             << "\" uses mesh \"" << name << "\" which is not defined";
      throw stream.str();
    }
    if ((geoFilter != partition::ReceivedPartition::GeometricFilter::BROADCAST_FILTER || safetyFactor != 0.1) && from==""){
      std::ostringstream stream;
      stream << "Participant \"" << _participants.back()->getName()
             << "\" uses mesh \"" << name << "\" which is not received (no \"from\"), but has a geometric-filter and/or"
             << " a safety factor defined. This is not valid.";
      throw stream.str();
    }
    _participants.back()->useMesh ( mesh, offset, false, from, safetyFactor, provide, geoFilter );
  }
  else if ( tag.getName() == TAG_WRITE ) {
    std::string dataName = tag.getStringAttributeValue(ATTR_NAME);
    std::string meshName = tag.getStringAttributeValue(ATTR_MESH);
    mesh::PtrMesh mesh = _meshConfig->getMesh ( meshName );
    CHECK(mesh, "Participant "
          << "\"" << _participants.back()->getName() << "\" has to use "
          << "mesh \"" << meshName << "\" in order to write data to it!" );
    mesh::PtrData data = getData ( mesh, dataName );
    _participants.back()->addWriteData ( data, mesh );
  }
  else if ( tag.getName() == TAG_READ ) {
    std::string dataName = tag.getStringAttributeValue(ATTR_NAME);
    std::string meshName = tag.getStringAttributeValue(ATTR_MESH);
    mesh::PtrMesh mesh = _meshConfig->getMesh ( meshName );
    CHECK(mesh, "Participant "
          << "\"" << _participants.back()->getName() << "\" has to use "
          << "mesh \"" << meshName << "\" in order to read data from it!" );
    mesh::PtrData data = getData ( mesh, dataName );
    _participants.back()->addReadData ( data, mesh );
  }
  else if ( tag.getName() == TAG_WATCH_POINT ){
    assertion(_dimensions != 0); // setDimensions() has been called
    WatchPointConfig config;
    config.name = tag.getStringAttributeValue(ATTR_NAME);
    config.nameMesh = tag.getStringAttributeValue(ATTR_MESH);
    config.coordinates = tag.getEigenVectorXdAttributeValue(ATTR_COORDINATE, _dimensions);
    _watchPointConfigs.push_back(config);
  }
  else if (tag.getNamespace() == TAG_SERVER){
    com::CommunicationConfiguration comConfig;
    com::PtrCommunication com = comConfig.createCommunication(tag);
    _participants.back()->setClientServerCommunication(com);
  }
  else if (tag.getNamespace() == TAG_MASTER){
    com::CommunicationConfiguration comConfig;
    com::PtrCommunication com = comConfig.createCommunication(tag);
    utils::MasterSlave::_communication = com;

    _participants.back()->setUseMaster(true);
  }
}

void ParticipantConfiguration:: xmlEndTagCallback
(
  xml::XMLTag& tag )
{
  if (tag.getName() == TAG){
    finishParticipantConfiguration(_participants.back());
  }
}

void ParticipantConfiguration:: addParticipant
(
  const impl::PtrParticipant&             participant,
  const mapping::PtrMappingConfiguration& mappingConfig )
{
  _participants.push_back ( participant );
  _mappingConfig = mappingConfig;
  finishParticipantConfiguration ( participant );
}

const std::vector<impl::PtrParticipant>&
ParticipantConfiguration:: getParticipants() const
{
  return _participants;
}

partition::ReceivedPartition::GeometricFilter ParticipantConfiguration:: getGeoFilter(const std::string& geoFilter) const
{
  if (geoFilter == VALUE_FILTER_FIRST){
    return partition::ReceivedPartition::GeometricFilter::FILTER_FIRST;
  }
  else if (geoFilter == VALUE_BROADCAST_FILTER){
    return partition::ReceivedPartition::GeometricFilter::BROADCAST_FILTER;
  }
  else {
    assertion(geoFilter == VALUE_NO_FILTER);
    return partition::ReceivedPartition::GeometricFilter::NO_FILTER;
  }
}

mesh::PtrMesh ParticipantConfiguration:: copy
(
  const mesh::PtrMesh& mesh ) const
{
  int dim = mesh->getDimensions();
  std::string name(mesh->getName());
  bool flipNormals = mesh->isFlipNormals();
  mesh::Mesh* meshCopy = new mesh::Mesh("Local_" + name, dim, flipNormals);
  for (const mesh::PtrData& data : mesh->data()){
    meshCopy->createData(data->getName(), data->getDimensions());
  }
  return mesh::PtrMesh(meshCopy);
}

const mesh::PtrData & ParticipantConfiguration:: getData
(
  const mesh::PtrMesh& mesh,
  const std::string&   nameData ) const
{
  for ( const mesh::PtrData & data : mesh->data() ){
    if ( data->getName() == nameData ) {
      return data;
    }
  }
  ERROR("Participant \"" << _participants.back()->getName()
                 << "\" assignes data \"" << nameData << "\" wrongly to mesh \""
                 << mesh->getName() << "\"!" );
}

void ParticipantConfiguration:: finishParticipantConfiguration
(
  const impl::PtrParticipant& participant )
{
  TRACE(participant->getName());

  // Set input/output meshes for data mappings and mesh requirements
  using ConfMapping = mapping::MappingConfiguration::ConfiguredMapping;
  for (const ConfMapping& confMapping : _mappingConfig->mappings()){
    int fromMeshID = confMapping.fromMesh->getID();
    int toMeshID = confMapping.toMesh->getID();

    CHECK(participant->isMeshUsed(fromMeshID),
          "Participant \"" << participant->getName() << "\" has mapping"
          << " from mesh \"" << confMapping.fromMesh->getName() << "\" which he does not use!");
    CHECK(participant->isMeshUsed(toMeshID),
          "Participant \"" << participant->getName() << "\" has mapping"
          << " to mesh \"" << confMapping.toMesh->getName() << "\" which he does not use!");
    if(participant->useMaster()){
      if((confMapping.direction == mapping::MappingConfiguration::WRITE &&
          confMapping.mapping->getConstraint()==mapping::Mapping::CONSISTENT) ||
         (confMapping.direction == mapping::MappingConfiguration::READ &&
          confMapping.mapping->getConstraint()==mapping::Mapping::CONSERVATIVE)){
        ERROR(
                       "If a participant uses a master parallelization, only the mapping"
                    << " combinations read-consistent and write-conservative are allowed");
      }
    }



    impl::MeshContext& fromMeshContext = participant->meshContext(fromMeshID);
    impl::MeshContext& toMeshContext = participant->meshContext(toMeshID);

    if(confMapping.direction == mapping::MappingConfiguration::READ){
      CHECK(toMeshContext.provideMesh, "A read mapping needs to map to a provided mesh. "
          "Mesh " << confMapping.toMesh->getName() << " is not provided.");
      CHECK(not fromMeshContext.receiveMeshFrom.empty(), "A read mapping needs to map from a received mesh. "
          "Mesh " << confMapping.fromMesh->getName() << " is not received.");
    }
    else{
      CHECK(fromMeshContext.provideMesh, "A write mapping needs to map from a provided mesh. "
          "Mesh " << confMapping.fromMesh->getName() << " is not provided.");
      CHECK(not toMeshContext.receiveMeshFrom.empty(), "A write mapping needs to map to a received mesh. "
          "Mesh " << confMapping.toMesh->getName() << " is not received.");
    }

    if(confMapping.isRBF){
      fromMeshContext.geoFilter = partition::ReceivedPartition::GeometricFilter::NO_FILTER;
      toMeshContext.geoFilter = partition::ReceivedPartition::GeometricFilter::NO_FILTER;
    }

    precice::impl::MappingContext* mappingContext = new precice::impl::MappingContext();
    mappingContext->fromMeshID = fromMeshID;
    mappingContext->toMeshID = toMeshID;
    mappingContext->timing = confMapping.timing;

    mapping::PtrMapping& map = mappingContext->mapping;
    assertion(map.get() == nullptr);
    map = confMapping.mapping;

    const mesh::PtrMesh& input = fromMeshContext.mesh;
    const mesh::PtrMesh& output = toMeshContext.mesh;
    DEBUG("Configure mapping for input=" << input->getName()
           << ", output=" << output->getName());
    map->setMeshes(input, output);

    if (confMapping.direction == mapping::MappingConfiguration::WRITE){
      participant->addWriteMappingContext(mappingContext);
    }
    else {
      assertion(confMapping.direction == mapping::MappingConfiguration::READ);
      participant->addReadMappingContext(mappingContext);
    }

    fromMeshContext.meshRequirement = std::max(
            fromMeshContext.meshRequirement, map->getInputRequirement());
    toMeshContext.meshRequirement = std::max(
            toMeshContext.meshRequirement, map->getOutputRequirement());

    fromMeshContext.fromMappingContext = *mappingContext;
    toMeshContext.toMappingContext = *mappingContext;
  }
  _mappingConfig->resetMappings();

  // Set participant data for data contexts
  for (impl::DataContext& dataContext : participant->writeDataContexts()){
    int fromMeshID = dataContext.mesh->getID();
    CHECK(participant->isMeshUsed(fromMeshID),
          "Participant \"" << participant->getName() << "\" has to use mesh \""
          << dataContext.mesh->getName() << "\" when writing data to it!");

    for (impl::MappingContext& mappingContext : participant->writeMappingContexts()){
      if(mappingContext.fromMeshID==fromMeshID){
        dataContext.mappingContext = mappingContext;
        impl::MeshContext& meshContext = participant->meshContext(mappingContext.toMeshID);
        for (mesh::PtrData data : meshContext.mesh->data()){
          if(data->getName()==dataContext.fromData->getName()){
            dataContext.toData = data;
          }
        }
        CHECK(dataContext.fromData!=dataContext.toData,
              "The mesh \"" << meshContext.mesh->getName() << "\" needs to use the data \""
              << dataContext.fromData->getName() << "\"! to allow the write mapping");
      }
    }
  }

  for (impl::DataContext& dataContext : participant->readDataContexts()){
    int toMeshID = dataContext.mesh->getID();
    CHECK(participant->isMeshUsed(toMeshID),
          "Participant \"" << participant->getName() << "\" has to use mesh \""
          << dataContext.mesh->getName() << "\" when writing data to it!");

    for (impl::MappingContext& mappingContext : participant->readMappingContexts()){
      if(mappingContext.toMeshID==toMeshID){
        dataContext.mappingContext = mappingContext;
        impl::MeshContext& meshContext = participant->meshContext(mappingContext.fromMeshID);
        for (mesh::PtrData data : meshContext.mesh->data()){
          if(data->getName()==dataContext.toData->getName()){
            dataContext.fromData = data;
          }
        }
        CHECK(dataContext.toData!=dataContext.fromData,
              "The mesh \"" << meshContext.mesh->getName() << "\" needs to use the data \""
              << dataContext.toData->getName() << "\"! to allow the read mapping");
      }
    }
  }

  // Add actions
  for (const action::PtrAction& action : _actionConfig->actions()){
    bool used = _participants.back()->isMeshUsed(action->getMesh()->getID());
    CHECK(used, "Data action of participant "
          << _participants.back()->getName()
          << "\" uses mesh which is not used by the participant!");
    _participants.back()->addAction(action);
  }
  _actionConfig->resetActions();

  // Add export contexts
  for (io::ExportContext& context : _exportConfig->exportContexts()){
    io::PtrExport exporter;
    if (context.type == VALUE_VTK){
      if(_participants.back()->useMaster()){
        exporter = io::PtrExport(new io::ExportVTKXML(context.plotNormals));
      }
      else{
        exporter = io::PtrExport(new io::ExportVTK(context.plotNormals));
      }
    }
    else {
      ERROR("Unknown export type!");
    }
    context.exporter = exporter;

    _participants.back()->addExportContext(context);
  }
  _exportConfig->resetExports();

  // Create watch points
  for ( const WatchPointConfig & config : _watchPointConfigs ){
    mesh::PtrMesh mesh;
    for ( const impl::MeshContext* context : participant->usedMeshContexts() ){
      if ( context->mesh->getName() == config.nameMesh ){
        mesh = context->mesh;
      }
    }
    CHECK(mesh,
          "Participant \"" << participant->getName()
          << "\" defines watchpoint \"" << config.name
          << "\" for mesh \"" << config.nameMesh
          << "\" which is not used by him!" );
    std::string filename = "precice-" + participant->getName() + "-watchpoint-" + config.name + ".log";
    impl::PtrWatchPoint watchPoint( new impl::WatchPoint(config.coordinates, mesh, filename) );
    participant->addWatchPoint ( watchPoint );
  }
  _watchPointConfigs.clear ();
}


}} // namespace precice, config

