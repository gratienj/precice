#include "MeshConfiguration.hpp"
#include "mesh/config/DataConfiguration.hpp"
#include "mesh/Mesh.hpp"
#include "xml/XMLAttribute.hpp"
#include "utils/Helpers.hpp"
#include <sstream>

namespace precice {
namespace mesh {

MeshConfiguration:: MeshConfiguration
(
  xml::XMLTag&       parent,
  PtrDataConfiguration config )
:
  TAG("mesh"),
  ATTR_NAME("name"),
  ATTR_FLIP_NORMALS("flip-normals"),
  TAG_DATA("use-data"),
  TAG_SUB_ID("sub-id"),
  ATTR_SIDE_INDEX("side"),
  _dimensions(0),
  _dataConfig(config),
  _meshes(),
  _setMeshSubIDs(),
  _meshSubIDs(),
  _neededMeshes()
{
  using namespace xml;
  std::string doc;
  XMLTag tag(*this, TAG, xml::XMLTag::OCCUR_ONCE_OR_MORE);
  doc = "Surface mesh consisting of vertices and (optional) of edges and ";
  doc += "triangles (only in 3D). The vertices of a mesh can carry data, ";
  doc += "configured by tag <use-data>. The mesh coordinates have to be ";
  doc += "defined by a participant (see tag <use-mesh>).";
  tag.setDocumentation(doc);

 auto attrName = XMLAttribute<std::string>(ATTR_NAME)
      .setDocumentation("Unique name for the mesh.");
  tag.addAttribute(attrName);

  auto attrFlipNormals = makeXMLAttribute(ATTR_FLIP_NORMALS, false)
      .setDocumentation("Flips mesh normal vector directions.");
  tag.addAttribute(attrFlipNormals);

  XMLTag subtagData(*this, TAG_DATA, XMLTag::OCCUR_ARBITRARY);
  doc = "Assigns a before defined data set (see tag <data>) to the mesh.";
  subtagData.setDocumentation(doc);
  attrName.setDocumentation("Name of the data set.");
  subtagData.addAttribute(attrName);
  tag.addSubtag(subtagData);

  xml::XMLTag tagSubID(*this, TAG_SUB_ID, xml::XMLTag::OCCUR_ARBITRARY);
  doc = "Every mesh has a global ID (determined by preCICE). It is possible ";
  doc += "to set additional sub-ids to distinguish parts of the mesh in ";
  doc += "queries.";
  tagSubID.setDocumentation(doc);
  xml::XMLAttribute<int> attrSideIndex(ATTR_SIDE_INDEX);
  tagSubID.addAttribute(attrSideIndex);
  tag.addSubtag(tagSubID);

  parent.addSubtag(tag);
}

void MeshConfiguration:: setDimensions
(
  int dimensions )
{
  TRACE(dimensions);
  assertion((dimensions == 2) || (dimensions == 3), dimensions);
  _dimensions = dimensions;
}

void MeshConfiguration:: xmlTagCallback
(
  xml::XMLTag& tag )
{
  TRACE(tag.getName());
  if (tag.getName() == TAG){
    assertion(_dimensions != 0);
    std::string name = tag.getStringAttributeValue(ATTR_NAME);
    bool flipNormals = tag.getBooleanAttributeValue(ATTR_FLIP_NORMALS);
    _meshes.push_back(PtrMesh(new Mesh(name, _dimensions, flipNormals)));
    _meshSubIDs.push_back(std::list<std::string>());
  }
  else if (tag.getName() == TAG_SUB_ID){
    int side = tag.getIntAttributeValue(ATTR_SIDE_INDEX);
    std::stringstream conv;
    conv << "side-" << side;
    _meshSubIDs.back().push_back(conv.str());
  }
  else if (tag.getName() == TAG_DATA){
    std::string name = tag.getStringAttributeValue(ATTR_NAME);
    bool found = false;
    for (const DataConfiguration::ConfiguredData& data : _dataConfig->data()){
      if (data.name == name){
        _meshes.back()->createData(data.name, data.dimensions);
        found = true;
        break;
      }
    }
    if (not found){
      std::ostringstream stream;
      stream << "Data with name \"" << name << "\" is not available during "
             << "configuration of mesh \"" << _meshes.back()->getName() << "\"";
      throw stream.str();
    }
  }
}

void MeshConfiguration:: xmlEndTagCallback
(
  xml::XMLTag& tag )
{
}

const PtrDataConfiguration& MeshConfiguration:: getDataConfiguration() const
{
  return _dataConfig;
}

void MeshConfiguration:: addMesh
(
  const mesh::PtrMesh& mesh  )
{
  for (PtrData dataNewMesh : mesh->data()){
    bool found = false;
    for (const DataConfiguration::ConfiguredData & data : _dataConfig->data()){
      if ((dataNewMesh->getName() == data.name)
          && (dataNewMesh->getDimensions() == data.dimensions))
      {
        found = true;
        break;
      }
    }
    CHECK(found, "Data " << dataNewMesh->getName() << " is not available in data configuration!");
  }
  _meshes.push_back(mesh);
}

void MeshConfiguration:: setMeshSubIDs()
{
  assertion ( _meshes.size() == _meshSubIDs.size() );
  assertion ( not _setMeshSubIDs );
  for ( size_t i=0; i < _meshes.size(); i++ ) {
    for ( const std::string & subIDName : _meshSubIDs[i] ) {
      _meshes[i]->setSubID ( subIDName );
    }
  }
  _setMeshSubIDs = true;
}

const std::vector<PtrMesh>& MeshConfiguration:: meshes() const
{
  return _meshes;
}

std::vector<PtrMesh>& MeshConfiguration:: meshes()
{
  return _meshes;
}

mesh::PtrMesh MeshConfiguration:: getMesh
(
  const std::string& meshName ) const
{
  for ( const mesh::PtrMesh & mesh : _meshes ) {
    if ( mesh->getName() == meshName ) {
      return mesh;
    }
  }
  return mesh::PtrMesh();
}

void MeshConfiguration:: addNeededMesh(
  const std::string& participant,
  const std::string& mesh)
{
  TRACE(participant, mesh );
  if(_neededMeshes.count(participant)==0){
    std::vector<std::string> meshes;
    meshes.push_back(mesh);
    _neededMeshes.insert(std::pair<std::string,std::vector<std::string> >(participant, meshes));
  }
  else if(not utils::contained(mesh,_neededMeshes.find(participant)->second)){
    _neededMeshes.find(participant)->second.push_back(mesh);
  }
}

}} // namespace precice, mesh

