#pragma once

#include <precice/Participant.hpp>
#include <string>
#include <vector>
#include "testing/TestContext.hpp"

namespace precice::testing {

struct QuickTest {

  struct Mesh {
    std::string name;
  };

  struct Data {
    std::string name;
  };

  QuickTest(Participant &p, Mesh m, Data d)
      : interface(p), meshName(m.name), dataName(d.name)
  {
  }

  QuickTest &setVertices(const std::vector<double> &pos)
  {
    auto n = pos.size() / interface.getMeshDimensions(meshName);
    vertexIDs.resize(n, -1);
    interface.setMeshVertices(meshName, pos, vertexIDs);
    return *this;
  }

  QuickTest &initialize()
  {
    interface.initialize();
    return *this;
  }

  QuickTest &resetMesh()
  {
    interface.resetMesh(meshName);
    return *this;
  }

  void finalize()
  {
    interface.finalize();
  }

  QuickTest &advance()
  {
    interface.advance(interface.getMaxTimeStepSize());
    return *this;
  }

  QuickTest &write(const std::vector<double> &data)
  {
    interface.writeData(meshName, dataName, vertexIDs, data);
    return *this;
  }

  std::vector<double> read()
  {
    auto                n = vertexIDs.size() * interface.getDataDimensions(meshName, dataName);
    std::vector<double> result(n, -0.0);
    interface.readData(meshName, dataName, vertexIDs, interface.getMaxTimeStepSize(), result);
    return result;
  }

  Participant &         interface;
  std::string           meshName;
  std::string           dataName;
  std::vector<VertexID> vertexIDs;
};

inline QuickTest::Mesh operator""_mesh(const char *name, std::size_t)
{
  return {name};
}

inline QuickTest::Data operator""_data(const char *name, std::size_t)
{
  return {name};
}

} // namespace precice::testing
