#include "RequestManager.hpp"
#include <algorithm>
#include <utility>
#include <vector>
#include "com/Communication.hpp"
#include "cplscheme/CouplingScheme.hpp"
#include "precice/impl/SolverInterfaceImpl.hpp"

namespace precice {
namespace impl {

RequestManager::RequestManager(
    SolverInterfaceImpl &        solverInterfaceImpl,
    com::PtrCommunication        clientServerCommunication,
    cplscheme::PtrCouplingScheme couplingScheme)
    : _interface(solverInterfaceImpl),
      _com(std::move(clientServerCommunication)),
      _couplingScheme(std::move(couplingScheme))
{
}

void RequestManager::handleRequests()
{
  PRECICE_TRACE();
  int            clientCommSize = _com->getRemoteCommunicatorSize();
  int            clientCounter  = 0;
  std::list<int> clientRanks;
  PRECICE_DEBUG("ClientCommSize " << clientCommSize);

  std::vector<com::PtrRequest> requests(clientCommSize);
  std::vector<int>             requestIDs(clientCommSize, -1);

  for (int clientRank = 0; clientRank < clientCommSize; clientRank++) {
    requests[clientRank] = _com->aReceive(requestIDs[clientRank], clientRank);
  }

  int  rankSender        = 0;
  int  requestID         = -1;
  bool collectiveRequest = false;
  bool singleRequest     = false;
  while (true) {
    if ((std::find(clientRanks.begin(), clientRanks.end(), rankSender) == clientRanks.end()) &&
        requests[rankSender]->test()) {
      requestID = requestIDs[rankSender];
      PRECICE_CHECK(requestID != -1, "Receiving of request ID failed");
      PRECICE_DEBUG("Received request ID " << requestID << " from rank " << rankSender);
    } else {
      rankSender++;
      if (rankSender == clientCommSize)
        rankSender = 0;
      continue;
    }

    switch (requestID) {
    case REQUEST_INITIALIZE:
      PRECICE_DEBUG("Request initialize by rank " << rankSender);
      clientCounter++;
      PRECICE_ASSERT(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize) {
        handleRequestInitialze(clientRanks);
        collectiveRequest = true;
      }
      break;
    case REQUEST_INITIALIZE_DATA:
      PRECICE_DEBUG("Request initialize data by rank " << rankSender);
      clientCounter++;
      PRECICE_ASSERT(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize) {
        handleRequestInitialzeData(clientRanks);
        collectiveRequest = true;
      }
      break;
    case REQUEST_ADVANCE:
      PRECICE_DEBUG("Request advance by rank " << rankSender);
      clientCounter++;
      PRECICE_ASSERT(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize) {
        handleRequestAdvance(clientRanks);
        collectiveRequest = true;
      }
      break;
    case REQUEST_FINALIZE:
      PRECICE_DEBUG("Request finalize by rank " << rankSender);
      clientCounter++;
      PRECICE_ASSERT(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize) {
        handleRequestFinalize();
        clientCounter = 0;
        clientRanks.clear();
        return;
      }
      break;
    case REQUEST_FULFILLED_ACTION:
      handleRequestFulfilledAction(rankSender);
      singleRequest = true;
      break;
    case REQUEST_SET_MESH_VERTEX:
      handleRequestSetMeshVertex(rankSender);
      singleRequest = true;
      break;
    case REQUEST_GET_MESH_VERTEX_SIZE:
      handleRequestGetMeshVertexSize(rankSender);
      singleRequest = true;
      break;
    case REQUEST_RESET_MESH:
      handleRequestResetMesh(rankSender);
      break;
    case REQUEST_SET_MESH_VERTICES:
      handleRequestSetMeshVertices(rankSender);
      singleRequest = true;
      break;
    case REQUEST_GET_MESH_VERTICES:
      handleRequestGetMeshVertices(rankSender);
      singleRequest = true;
      break;
    case REQUEST_GET_MESH_VERTEX_IDS_FROM_POSITIONS:
      handleRequestGetMeshVertexIDsFromPositions(rankSender);
      singleRequest = true;
      break;
    case REQUEST_SET_MESH_EDGE:
      handleRequestSetMeshEdge(rankSender);
      singleRequest = true;
      break;
    case REQUEST_SET_MESH_TRIANGLE:
      handleRequestSetMeshTriangle(rankSender);
      singleRequest = true;
      break;
    case REQUEST_SET_MESH_TRIANGLE_WITH_EDGES:
      handleRequestSetMeshTriangleWithEdges(rankSender);
      singleRequest = true;
      break;
    case REQUEST_SET_MESH_QUAD:
      handleRequestSetMeshQuad(rankSender);
      singleRequest = true;
      break;
    case REQUEST_SET_MESH_QUAD_WITH_EDGES:
      handleRequestSetMeshQuadWithEdges(rankSender);
      singleRequest = true;
      break;
    case REQUEST_WRITE_BLOCK_SCALAR_DATA:
      handleRequestWriteBlockScalarData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_WRITE_SCALAR_DATA:
      handleRequestWriteScalarData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_WRITE_BLOCK_VECTOR_DATA:
      handleRequestWriteBlockVectorData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_WRITE_VECTOR_DATA:
      handleRequestWriteVectorData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_READ_BLOCK_SCALAR_DATA:
      handleRequestReadBlockScalarData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_READ_SCALAR_DATA:
      handleRequestReadScalarData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_READ_VETOR_DATA:
      handleRequestReadVectorData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_READ_BLOCK_VECTOR_DATA:
      handleRequestReadBlockVectorData(rankSender);
      singleRequest = true;
      break;
    case REQUEST_MAP_WRITE_DATA_FROM:
      PRECICE_DEBUG("Request map written data by rank " << rankSender);
      clientCounter++;
      PRECICE_ASSERT(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize) {
        handleRequestMapWriteDataFrom(clientRanks);
        collectiveRequest = true;
      }
      break;
    case REQUEST_MAP_READ_DATA_TO:
      PRECICE_DEBUG("Request map read data by rank " << rankSender);
      clientCounter++;
      PRECICE_ASSERT(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize) {
        handleRequestMapReadDataTo(clientRanks);
        collectiveRequest = true;
      }
      break;
    case REQUEST_PING:
      //bool ping = true;
      _com->send(true, rankSender);
      singleRequest = true;
      break;
    default:
      PRECICE_ERROR("Unknown RequestID \"" << requestID << "\"");
      break;
    }

    // open receive again and clean up
    if (singleRequest) {
      requestIDs[rankSender] = -1;
      requests[rankSender]   = _com->aReceive(requestIDs[rankSender], rankSender);
      singleRequest          = false;
    } else if (collectiveRequest) {
      PRECICE_ASSERT(clientRanks.size() == static_cast<size_t>(clientCommSize));
      for (int rank : clientRanks) {
        requests[rank] = _com->aReceive(requestIDs[rank], rank);
      }
      clientCounter = 0;
      clientRanks.clear();
      collectiveRequest = false;
    }

    requestID = -1;

    rankSender++;
    if (rankSender == clientCommSize)
      rankSender = 0;
  }
}
void RequestManager::requestPing()
{
  PRECICE_TRACE();
  _com->send(REQUEST_PING, 0);
  int dummy = 0;
  _com->receive(dummy, 0);
}

void RequestManager::requestInitialize()
{
  PRECICE_TRACE();
  _com->send(REQUEST_INITIALIZE, 0);
  _couplingScheme->receiveState(_com, 0);
}

void RequestManager::requestInitialzeData()
{
  PRECICE_TRACE();
  _com->send(REQUEST_INITIALIZE_DATA, 0);
  _couplingScheme->receiveState(_com, 0);
}

void RequestManager::requestAdvance(
    double dt)
{
  PRECICE_TRACE();
  _com->send(REQUEST_ADVANCE, 0);
  _com->send(dt, 0);
  _couplingScheme->receiveState(_com, 0);
}

void RequestManager::requestFinalize()
{
  PRECICE_TRACE();
  _com->send(REQUEST_FINALIZE, 0);
}

void RequestManager::requestFulfilledAction(
    const std::string &action)
{
  PRECICE_TRACE();
  _com->send(REQUEST_FULFILLED_ACTION, 0);
  _com->send(action, 0);
}

int RequestManager::requestSetMeshVertex(
    int              meshID,
    Eigen::VectorXd &position)
{
  PRECICE_TRACE();
  _com->send(REQUEST_SET_MESH_VERTEX, 0);
  _com->send(meshID, 0);
  _com->send(position.data(), position.size(), 0);
  int index = -1;
  _com->receive(index, 0);
  return index;
}

int RequestManager::requestGetMeshVertexSize(
    int meshID)
{
  PRECICE_TRACE(meshID);
  _com->send(REQUEST_GET_MESH_VERTEX_SIZE, 0);
  _com->send(meshID, 0);
  int size = -1;
  _com->receive(size, 0);
  return size;
}

void RequestManager::requestResetMesh(
    int meshID)
{
  PRECICE_TRACE(meshID);
  _com->send(REQUEST_RESET_MESH, 0);
  _com->send(meshID, 0);
}

void RequestManager::requestSetMeshVertices(
    int           meshID,
    int           size,
    const double *positions,
    int *         ids)
{
  PRECICE_TRACE();
  _com->send(REQUEST_SET_MESH_VERTICES, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(positions, size * _interface.getDimensions(), 0);
  _com->receive(ids, size, 0);
}

void RequestManager::requestGetMeshVertices(
    int        meshID,
    int        size,
    const int *ids,
    double *   positions)
{
  PRECICE_TRACE();
  _com->send(REQUEST_GET_MESH_VERTICES, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(ids, size, 0);
  _com->receive(positions, size * _interface.getDimensions(), 0);
}

void RequestManager::requestGetMeshVertexIDsFromPositions(
    int           meshID,
    int           size,
    const double *positions,
    int *         ids)
{
  PRECICE_TRACE(size);
  _com->send(REQUEST_GET_MESH_VERTEX_IDS_FROM_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(positions, size * _interface.getDimensions(), 0);
  _com->receive(ids, size, 0);
}

int RequestManager::requestSetMeshEdge(
    int meshID,
    int firstVertexID,
    int secondVertexID)
{
  PRECICE_TRACE(meshID, firstVertexID, secondVertexID);
  _com->send(REQUEST_SET_MESH_EDGE, 0);
  int data[3] = {meshID, firstVertexID, secondVertexID};
  _com->send(data, 3, 0);
  int createdEdgeID = -1;
  _com->receive(createdEdgeID, 0);
  return createdEdgeID;
}

void RequestManager::requestSetMeshTriangle(
    int meshID,
    int firstEdgeID,
    int secondEdgeID,
    int thirdEdgeID)
{
  PRECICE_TRACE(meshID, firstEdgeID, secondEdgeID, thirdEdgeID);
  _com->send(REQUEST_SET_MESH_TRIANGLE, 0);
  int data[4] = {meshID, firstEdgeID, secondEdgeID, thirdEdgeID};
  _com->send(data, 4, 0);
}

void RequestManager::requestSetMeshTriangleWithEdges(
    int meshID,
    int firstVertexID,
    int secondVertexID,
    int thirdVertexID)
{
  PRECICE_TRACE(meshID, firstVertexID,
                secondVertexID, thirdVertexID);
  _com->send(REQUEST_SET_MESH_TRIANGLE_WITH_EDGES, 0);
  int data[4] = {meshID, firstVertexID, secondVertexID, thirdVertexID};
  _com->send(data, 4, 0);
}

void RequestManager::requestSetMeshQuad(
    int meshID,
    int firstEdgeID,
    int secondEdgeID,
    int thirdEdgeID,
    int fourthEdgeID)
{
  PRECICE_TRACE(meshID, firstEdgeID, secondEdgeID, thirdEdgeID, fourthEdgeID);
  _com->send(REQUEST_SET_MESH_QUAD, 0);
  int data[5] = {meshID, firstEdgeID, secondEdgeID, thirdEdgeID, fourthEdgeID};
  _com->send(data, 5, 0);
}

void RequestManager::requestSetMeshQuadWithEdges(
    int meshID,
    int firstVertexID,
    int secondVertexID,
    int thirdVertexID,
    int fourthVertexID)
{
  PRECICE_TRACE(meshID, firstVertexID, secondVertexID, thirdVertexID, fourthVertexID);
  _com->send(REQUEST_SET_MESH_QUAD_WITH_EDGES, 0);
  int data[5] = {meshID, firstVertexID, secondVertexID, thirdVertexID, fourthVertexID};
  _com->send(data, 5, 0);
}

void RequestManager::requestWriteBlockScalarData(
    int           dataID,
    int           size,
    const int *   valueIndices,
    const double *values)
{
  PRECICE_TRACE(dataID, size);
  _com->send(REQUEST_WRITE_BLOCK_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(size, 0);
  _com->send(valueIndices, size, 0);
  _com->send(values, size, 0);
}

void RequestManager::requestWriteScalarData(
    int    dataID,
    int    valueIndex,
    double value)
{
  PRECICE_TRACE();
  _com->send(REQUEST_WRITE_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->send(value, 0);
}

void RequestManager::requestWriteBlockVectorData(
    int           dataID,
    int           size,
    const int *   valueIndices,
    const double *values)
{
  PRECICE_TRACE(dataID);
  _com->send(REQUEST_WRITE_BLOCK_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(size, 0);
  _com->send(valueIndices, size, 0);
  _com->send(values, size * _interface.getDimensions(), 0);
}

void RequestManager::requestWriteVectorData(
    int           dataID,
    int           valueIndex,
    const double *value)
{
  PRECICE_TRACE();
  _com->send(REQUEST_WRITE_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->send(value, _interface.getDimensions(), 0);
}

void RequestManager::requestReadBlockScalarData(
    int        dataID,
    int        size,
    const int *valueIndices,
    double *   values)
{
  PRECICE_TRACE(dataID, size);
  _com->send(REQUEST_READ_BLOCK_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(size, 0);
  _com->send(valueIndices, size, 0);
  _com->receive(values, size, 0);
}

void RequestManager::requestReadScalarData(
    int     dataID,
    int     valueIndex,
    double &value)
{
  PRECICE_TRACE();
  _com->send(REQUEST_READ_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->receive(value, 0);
}

void RequestManager::requestReadBlockVectorData(
    int        dataID,
    int        size,
    const int *valueIndices,
    double *   values)
{
  PRECICE_TRACE(dataID, size);
  _com->send(REQUEST_READ_BLOCK_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(size, 0);
  _com->send(valueIndices, size, 0);
  _com->receive(values, size * _interface.getDimensions(), 0);
}

void RequestManager::requestReadVectorData(
    int     dataID,
    int     valueIndex,
    double *value)
{
  PRECICE_TRACE();
  _com->send(REQUEST_READ_VETOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->receive(value, _interface.getDimensions(), 0);
}

void RequestManager::requestMapWriteDataFrom(
    int fromMeshID)
{
  PRECICE_TRACE(fromMeshID);
  _com->send(REQUEST_MAP_WRITE_DATA_FROM, 0);
  int ping;
  _com->receive(ping, 0);
  _com->send(fromMeshID, 0);
}

void RequestManager::requestMapReadDataTo(
    int toMeshID)
{
  PRECICE_TRACE(toMeshID);
  _com->send(REQUEST_MAP_READ_DATA_TO, 0);
  int ping;
  _com->receive(ping, 0);
  _com->send(toMeshID, 0);
}

void RequestManager::handleRequestInitialze(
    const std::list<int> &clientRanks)
{
  PRECICE_TRACE()
  _interface.initialize();
  for (int rank : clientRanks) {
    _couplingScheme->sendState(_com, rank);
  }
}

void RequestManager::handleRequestInitialzeData(
    const std::list<int> &clientRanks)
{
  PRECICE_TRACE();
  _interface.initializeData();
  for (int rank : clientRanks) {
    _couplingScheme->sendState(_com, rank);
  }
}

void RequestManager::handleRequestAdvance(
    const std::list<int> &clientRanks)
{
  PRECICE_TRACE();
  auto   iter = clientRanks.begin();
  double oldDt;
  _com->receive(oldDt, *iter);
  iter++;
  for (; iter != clientRanks.end(); iter++) {
    double dt;
    _com->receive(dt, *iter);
    PRECICE_CHECK(math::equals(dt, oldDt),
                  "Ambiguous timestep length when calling request advance from several processes!");
    oldDt = dt;
  }
  _interface.advance(oldDt);
  for (int rank : clientRanks) {
    _couplingScheme->sendState(_com, rank);
  }
}

void RequestManager::handleRequestFinalize()
{
  PRECICE_TRACE();
  _interface.finalize();
}

void RequestManager::handleRequestFulfilledAction(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  std::string action;
  _com->receive(action, rankSender);
  _interface.fulfilledAction(action);
}

void RequestManager::handleRequestSetMeshVertex(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  std::vector<double> position(_interface.getDimensions());
  _com->receive(position.data(), _interface.getDimensions(), rankSender);
  int index = _interface.setMeshVertex(meshID, position.data());
  _com->send(index, rankSender);
}

void RequestManager::handleRequestGetMeshVertexSize(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  int size = _interface.getMeshVertexSize(meshID);
  _com->send(size, rankSender);
}

void RequestManager::handleRequestResetMesh(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  _interface.resetMesh(meshID);
}

void RequestManager::handleRequestSetMeshVertices(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  PRECICE_CHECK(size > 0, "You cannot call setMeshVertices with size=0.");
  std::vector<double> positions(static_cast<size_t>(size) * _interface.getDimensions());
  _com->receive(positions, rankSender);
  std::vector<int> ids(size);
  _interface.setMeshVertices(meshID, size, positions.data(), ids.data());
  _com->send(ids, rankSender);
}

void RequestManager::handleRequestGetMeshVertices(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int meshID = -1;
  int size   = -1;
  _com->receive(meshID, rankSender);
  _com->receive(size, rankSender);
  PRECICE_ASSERT(size > 0, size);
  std::vector<int>    ids(size);
  std::vector<double> positions(static_cast<size_t>(size) * _interface.getDimensions());
  _com->receive(ids, rankSender);
  _interface.getMeshVertices(meshID, size, ids.data(), positions.data());
  _com->send(positions, rankSender);
}

void RequestManager::handleRequestGetMeshVertexIDsFromPositions(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int meshID = -1;
  int size   = -1;
  _com->receive(meshID, rankSender);
  _com->receive(size, rankSender);
  PRECICE_ASSERT(size > 0, size);
  std::vector<int>    ids(size);
  std::vector<double> positions(static_cast<size_t>(size) * _interface.getDimensions());
  _com->receive(positions, rankSender);
  _interface.getMeshVertexIDsFromPositions(meshID, size, positions.data(), ids.data());
  _com->send(ids, rankSender);
}

void RequestManager::handleRequestSetMeshEdge(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int data[3]; // 0: meshID, 1: firstVertexID, 2: secondVertexID
  _com->receive(data, 3, rankSender);
  int createEdgeID = _interface.setMeshEdge(data[0], data[1], data[2]);
  _com->send(createEdgeID, rankSender);
}

void RequestManager::handleRequestSetMeshTriangle(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int data[4]; // 0: meshID, 1,2,3: edge IDs
  _com->receive(data, 4, rankSender);
  _interface.setMeshTriangle(data[0], data[1], data[2], data[3]);
}

void RequestManager::handleRequestSetMeshTriangleWithEdges(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int data[4]; // 0: meshID, 1,2,3: vertex IDs
  _com->receive(data, 4, rankSender);
  _interface.setMeshTriangleWithEdges(data[0], data[1], data[2], data[3]);
}

void RequestManager::handleRequestSetMeshQuad(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int data[5]; // 0: meshID, 1,2,3,4: edge IDs
  _com->receive(data, 5, rankSender);
  _interface.setMeshQuad(data[0], data[1], data[2], data[3], data[4]);
}

void RequestManager::handleRequestSetMeshQuadWithEdges(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int data[5]; // 0: meshID, 1,2,3,4: vertex IDs
  _com->receive(data, 5, rankSender);
  _interface.setMeshQuadWithEdges(data[0], data[1], data[2], data[3], data[4]);
}

void RequestManager::handleRequestWriteScalarData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  double data;
  _com->receive(data, rankSender);
  _interface.writeScalarData(dataID, index, data);
}

void RequestManager::handleRequestWriteBlockScalarData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  std::vector<int> indices(size);
  _com->receive(indices, rankSender);
  std::vector<double> data(size);
  _com->receive(data, rankSender);
  _interface.writeBlockScalarData(dataID, size, indices.data(), data.data());
}

void RequestManager::handleRequestWriteBlockVectorData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  std::vector<int> indices(size);
  _com->receive(indices, rankSender);
  std::vector<double> data(static_cast<size_t>(size) * _interface.getDimensions());
  _com->receive(data, rankSender);
  _interface.writeBlockVectorData(dataID, size, indices.data(), data.data());
}

void RequestManager::handleRequestWriteVectorData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  std::vector<double> data(_interface.getDimensions());
  _com->receive(data.data(), _interface.getDimensions(), rankSender);
  _interface.writeVectorData(dataID, index, data.data());
}

void RequestManager::handleRequestReadScalarData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  double data;
  _interface.readScalarData(dataID, index, data);
  _com->send(data, rankSender); // Send back result
}

void RequestManager::handleRequestReadBlockScalarData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  std::vector<int> indices(size);
  _com->receive(indices, rankSender);
  std::vector<double> data(size);
  _interface.readBlockScalarData(dataID, size, indices.data(), data.data());
  _com->send(data, rankSender);
}

void RequestManager::handleRequestReadBlockVectorData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  std::vector<int> indices(size);
  _com->receive(indices, rankSender);
  std::vector<double> data(static_cast<size_t>(size) * _interface.getDimensions());
  _interface.readBlockVectorData(dataID, size, indices.data(), data.data());
  _com->send(data, rankSender);
}

void RequestManager::handleRequestReadVectorData(
    int rankSender)
{
  PRECICE_TRACE(rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  std::vector<double> data(_interface.getDimensions());
  _interface.readVectorData(dataID, index, data.data());
  _com->send(data.data(), _interface.getDimensions(), rankSender);
}

void RequestManager::handleRequestMapWriteDataFrom(
    const std::list<int> &clientRanks)
{
  PRECICE_TRACE();
  auto iter = clientRanks.begin();
  int  ping = 0;
  _com->send(ping, *iter);
  int oldMeshID;
  _com->receive(oldMeshID, *iter);
  iter++;
  for (; iter != clientRanks.end(); iter++) {
    _com->send(ping, *iter);
    int meshID;
    _com->receive(meshID, *iter);
    PRECICE_CHECK(meshID == oldMeshID,
                  "Ambiguous mesh ID when calling map written data from several processes!");
    oldMeshID = meshID;
  }
  _interface.mapWriteDataFrom(oldMeshID);
}

void RequestManager::handleRequestMapReadDataTo(
    const std::list<int> &clientRanks)
{
  PRECICE_TRACE();
  auto iter = clientRanks.begin();
  int  ping = 0;
  _com->send(ping, *iter);
  int oldMeshID;
  _com->receive(oldMeshID, *iter);
  iter++;
  for (; iter != clientRanks.end(); iter++) {
    _com->send(ping, *iter);
    int meshID;
    _com->receive(meshID, *iter);
    PRECICE_CHECK(meshID == oldMeshID,
                  "Ambiguous mesh IDs (" << meshID << " and " << oldMeshID
                                         << ") when calling map read data from several processes!");
    oldMeshID = meshID;
  }
  _interface.mapReadDataTo(oldMeshID);
}

} // namespace impl
} // namespace precice
