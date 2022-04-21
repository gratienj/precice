#include "io/ExportVTP.hpp"
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>
#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"

namespace precice {
namespace io {

std::string ExportVTP::getVTKFormat() const
{
  return "PolyData";
}

std::string ExportVTP::getPrimaryExtension() const
{
  return ".pvtp";
}

std::string ExportVTP::getPieceExtension() const
{
  return ".vtp";
}

std::string ExportVTP::getPieceAttributes(const mesh::Mesh &mesh) const
{
  std::ostringstream oss;
  oss << "NumberOfPoints=\"" << mesh.vertices().size() << "\" ";
  oss << "NumberOfLines=\"" << mesh.edges().size() << "\" ";
  oss << "NumberOfPolys=\"" << mesh.triangles().size() << "\"";
  return oss.str();
}

void ExportVTP::writePrimaryCells(std::ostream &out) const
{
  out << "      <PLines>\n";
  out << "         <PDataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\"/>\n";
  out << "         <PDataArray type=\"Int32\" Name=\"offsets\"      NumberOfComponents=\"1\"/>\n";
  out << "      </PLines>\n";
  out << "      <PPolys>\n";
  out << "         <PDataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\"/>\n";
  out << "         <PDataArray type=\"Int32\" Name=\"offsets\"      NumberOfComponents=\"1\"/>\n";
  out << "      </PPolys>\n";
}

void ExportVTP::exportConnectivity(
    std::ostream &    outFile,
    const mesh::Mesh &mesh) const
{
  outFile << "         <Lines>\n";
  outFile << "            <DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\">\n";
  outFile << "               ";
  for (const mesh::Edge &edge : mesh.edges()) {
    writeLine(edge, outFile);
  }
  outFile << '\n';
  outFile << "            </DataArray> \n";
  outFile << "            <DataArray type=\"Int32\" Name=\"offsets\" NumberOfComponents=\"1\" format=\"ascii\">\n";
  outFile << "               ";
  for (size_t i = 1; i <= mesh.edges().size(); i++) {
    outFile << 2 * i << "  ";
  }
  outFile << '\n';
  outFile << "            </DataArray>\n";
  outFile << "         </Lines>\n";
  outFile << "         <Polys>\n";
  outFile << "            <DataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\" format=\"ascii\">\n";
  outFile << "               ";
  for (const mesh::Triangle &triangle : mesh.triangles()) {
    writeTriangle(triangle, outFile);
  }
  outFile << '\n';
  outFile << "            </DataArray> \n";
  outFile << "            <DataArray type=\"Int32\" Name=\"offsets\" NumberOfComponents=\"1\" format=\"ascii\">\n";
  outFile << "               ";
  for (size_t i = 1; i <= mesh.triangles().size(); i++) {
    outFile << 3 * i << "  ";
  }
  outFile << '\n';
  outFile << "            </DataArray>\n";
  outFile << "         </Polys>\n";
}
} // namespace io
} // namespace precice
