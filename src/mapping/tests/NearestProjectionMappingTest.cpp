#include "testing/Testing.hpp"

#include "mapping/NearestProjectionMapping.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/Vertex.hpp"
#include "mesh/Edge.hpp"
#include "mesh/RTree.hpp"

using namespace precice;

BOOST_AUTO_TEST_SUITE(MappingTests)
BOOST_AUTO_TEST_SUITE(NearestProjectionMapping)

BOOST_AUTO_TEST_CASE(testConservativeNonIncremental)                
{
  using namespace mesh;
  int dimensions = 2;

  // Setup geometry to map to
  PtrMesh outMesh ( new Mesh("OutMesh", dimensions, true) );
  PtrData outData = outMesh->createData ( "Data", 1 );
  int outDataID = outData->getID();
  Vertex& v1 = outMesh->createVertex ( Eigen::Vector2d(0.0, 0.0) );
  Vertex& v2 = outMesh->createVertex ( Eigen::Vector2d(1.0, 1.0) );
  outMesh->createEdge ( v1, v2 );
  outMesh->computeState();
  outMesh->allocateDataValues();

  BOOST_TEST_MESSAGE(*outMesh);

  // Base-value for tests
  double value = 1.0;

  // Setup mapping with mapping coordinates and geometry used
  mapping::NearestProjectionMapping mapping(mapping::Mapping::CONSERVATIVE, dimensions);
  {
      PtrMesh inMesh ( new Mesh("InMesh0", dimensions, false) );
      PtrData inData = inMesh->createData ( "Data0", 1 );
      int inDataID = inData->getID();

      // Map value 1.0 from middle of edge to geometry. Expect half of the
      // value to be added to vertex1 and half of it to vertex2.
      Vertex& inv1 = inMesh->createVertex(Eigen::Vector2d(0.5, 0.5));
      // Map value 1.0 from below edge to geometry. Expect vertex1 to get the
      // full data value, i.e. 1.0 and in addition the value from before. In total
      // v1 should have 1.5 * dataValue then.
      Vertex& inv2 = inMesh->createVertex(Eigen::Vector2d(-0.5, -0.5));
      // Do the same thing from above, expect vertex2 to get the full value now.
      Vertex& inv3 = inMesh->createVertex(Eigen::Vector2d(1.5, 1.5));

      inMesh->allocateDataValues();
      inMesh->computeState();

      //assign(inData->values()) = value;
      inData->values() = Eigen::VectorXd::Constant(inData->values().size(), value);
      //assign(values) = 0.0;
      Eigen::VectorXd& values = outData->values();
      values = Eigen::VectorXd::Constant(values.size(), 0.0);

      mapping.setMeshes(inMesh, outMesh);
      mapping.computeMapping();
      mapping.map(inDataID, outDataID);
      BOOST_TEST_CONTEXT(*inMesh) {
          BOOST_TEST(values(0) == value * 1.5);
          BOOST_TEST(values(1) == value * 1.5);
      }
  }
  mapping.clear();
  {
      PtrMesh inMesh ( new Mesh("InMesh1", dimensions, false) );
      PtrData inData = inMesh->createData ( "Data1", 1 );
      int inDataID = inData->getID();

      Vertex& inv1 = inMesh->createVertex(Eigen::Vector2d(-1.0, -1.0));
      Vertex& inv2 = inMesh->createVertex(Eigen::Vector2d(-1.0, -1.0));
      Vertex& inv3 = inMesh->createVertex(Eigen::Vector2d(1.0, 1.0));

      inMesh->allocateDataValues();
      inMesh->computeState();

      //assign(inData->values()) = value;
      inData->values() = Eigen::VectorXd::Constant(inData->values().size(), value);
      //assign(values) = 0.0;
      Eigen::VectorXd& values = outData->values();
      values = Eigen::VectorXd::Constant(values.size(), 0.0);

      mapping.setMeshes(inMesh, outMesh);
      mapping.computeMapping();
      mapping.map(inDataID, outDataID);
      BOOST_TEST_CONTEXT(*inMesh) {
          BOOST_TEST(values(0) == value * 4.0);
          BOOST_TEST(values(1) == value * -1.0);
      }

      // reset output value and remap
      //assign(values) = 0.0;
      values = Eigen::VectorXd::Constant(values.size(), 0.0);

      mapping.map(inDataID, outDataID);
      BOOST_TEST_CONTEXT(*inMesh) {
          BOOST_TEST(values(0) == value * 4.0);
          BOOST_TEST(values(1) == value * -1.0);
      }
  }
}

BOOST_AUTO_TEST_CASE(ConsistentNonIncremental2D)
{
  using namespace mesh;
  int dimensions = 2;

  // Create mesh to map from
  PtrMesh inMesh ( new Mesh("InMesh", dimensions, false) );
  PtrData inData = inMesh->createData ( "InData", 1 );
  int inDataID = inData->getID ();
  Vertex& v1 = inMesh->createVertex ( Eigen::Vector2d(0.0, 0.0) );
  Vertex& v2 = inMesh->createVertex ( Eigen::Vector2d(1.0, 1.0) );
  inMesh->createEdge ( v1, v2 );
  inMesh->computeState();
  inMesh->allocateDataValues();
  double valueVertex1 = 1.0;
  double valueVertex2 = 2.0;
  Eigen::VectorXd& values = inData->values();
  values(0) = valueVertex1;
  values(1) = valueVertex2;

  {
      // Create mesh to map to
      PtrMesh outMesh ( new Mesh("OutMesh0", dimensions, false) );
      PtrData outData = outMesh->createData ( "OutData", 1 );
      int outDataID = outData->getID();

      // Setup mapping with mapping coordinates and geometry used
      mapping::NearestProjectionMapping mapping(mapping::Mapping::CONSISTENT, dimensions);
      mapping.setMeshes ( inMesh, outMesh );
      BOOST_TEST ( mapping.hasComputedMapping() == false );

      Vertex& outv0 = outMesh->createVertex ( Eigen::Vector2d(0.5, 0.5) );
      Vertex& outv1 = outMesh->createVertex ( Eigen::Vector2d(-0.5, -0.5) );
      Vertex& outv2 = outMesh->createVertex ( Eigen::Vector2d(1.5, 1.5) );
      outMesh->allocateDataValues();

      // Compute and perform mapping
      mapping.computeMapping();
      mapping.map ( inDataID, outDataID );

      // Validate results
      BOOST_TEST ( mapping.hasComputedMapping() == true );
      BOOST_TEST ( outData->values()[0] == (valueVertex1 + valueVertex2) * 0.5 );
      BOOST_TEST ( outData->values()[1] == (valueVertex1*1.5 + valueVertex2*-.5));
      BOOST_TEST ( outData->values()[2] == (valueVertex1*-.5 + valueVertex2*1.5));

      // Redo mapping, results should be
      //assign(outData->values()) = 0.0;
      outData->values() = Eigen::VectorXd::Constant(outData->values().size(), 0.0);

      mapping.map ( inDataID, outDataID );
      BOOST_TEST ( outData->values()[0] == (valueVertex1 + valueVertex2) * 0.5 );
      BOOST_TEST ( outData->values()[1] == (valueVertex1*1.5 + valueVertex2*-.5));
      BOOST_TEST ( outData->values()[2] == (valueVertex1*-.5 + valueVertex2*1.5));
  }

  {
      // Create mesh to map to
      PtrMesh outMesh ( new Mesh("OutMesh1", dimensions, false) );
      PtrData outData = outMesh->createData ( "OutData", 1 );
      int outDataID = outData->getID();

      // Setup mapping with mapping coordinates and geometry used
      mapping::NearestProjectionMapping mapping(mapping::Mapping::CONSISTENT, dimensions);
      mapping.setMeshes ( inMesh, outMesh );
      BOOST_TEST ( mapping.hasComputedMapping() == false );

      Vertex& outv0 = outMesh->createVertex ( Eigen::Vector2d(-0.5, -0.5) );
      Vertex& outv1 = outMesh->createVertex ( Eigen::Vector2d(1.5, 1.5) );
      Vertex& outv2 = outMesh->createVertex ( Eigen::Vector2d(0.5, 0.5) );
      outMesh->allocateDataValues();

      //assign(outData->values()) = 0.0;
      outData->values() = Eigen::VectorXd::Constant(outData->values().size(), 0.0);

      mapping.computeMapping();
      mapping.map ( inDataID, outDataID );
      BOOST_TEST ( outData->values()[0] == (valueVertex1*1.5 + valueVertex2*-.5));
      BOOST_TEST ( outData->values()[1] == (valueVertex1*-.5 + valueVertex2*1.5));
      BOOST_TEST ( outData->values()[2] == (valueVertex1 + valueVertex2) * 0.5 );

      // Reset output data to zero and redo the mapping
      //assign(outData->values()) = 0.0;
      outData->values() = Eigen::VectorXd::Constant(outData->values().size(), 0.0);

      mapping.map ( inDataID, outDataID );
      BOOST_TEST ( outData->values()[0] == (valueVertex1*1.5 + valueVertex2*-.5));
      BOOST_TEST ( outData->values()[1] == (valueVertex1*-.5 + valueVertex2*1.5));
      BOOST_TEST ( outData->values()[2] == (valueVertex1 + valueVertex2) * 0.5 );
  }
}


#if 0
BOOST_AUTO_TEST_CASE(ConsistentNonIncrementalPseudo3D)
{
  using namespace mesh;
  int dimensions = 3;

  // Create mesh to map from
  PtrMesh inMesh ( new Mesh("InMesh", dimensions, false) );
  PtrData inData = inMesh->createData ( "InData", 1 );
  int inDataID = inData->getID ();
  Vertex& v1 = inMesh->createVertex ( Eigen::Vector3d(0.0, 0.0, 0.0) );
  Vertex& v2 = inMesh->createVertex ( Eigen::Vector3d(1.0, 1.0, 0.0) );
  Vertex& v3 = inMesh->createVertex ( Eigen::Vector3d(2.0, 2.0, 0.0) );
  Edge & e12 = inMesh->createEdge ( v1, v2 );
  Edge & e23 = inMesh->createEdge ( v2, v3 );
  Edge & e31 = inMesh->createEdge ( v3, v1 );
  inMesh->createTriangle(e12, e23, e31);

  inMesh->computeState();
  inMesh->allocateDataValues();
  double valueVertex1 = 1.0;
  double valueVertex2 = 2.0;
  double valueVertex3 = 3.0;
  Eigen::VectorXd& values = inData->values();
  values(0) = valueVertex1;
  values(1) = valueVertex2;
  values(2) = valueVertex3;

  // Create mesh to map to
  PtrMesh outMesh ( new Mesh("OutMesh", dimensions, false) );
  PtrData outData = outMesh->createData ( "OutData", 1 );
  int outDataID = outData->getID();

  // Setup mapping with mapping coordinates and geometry used
  mapping::NearestProjectionMapping mapping(mapping::Mapping::CONSISTENT, dimensions);
  mapping.setMeshes ( inMesh, outMesh );
  BOOST_TEST ( mapping.hasComputedMapping() == false );

  Vertex& outv0 = outMesh->createVertex ( Eigen::Vector3d(0.5, 0.5, 0.0) );
  Vertex& outv1 = outMesh->createVertex ( Eigen::Vector3d(-0.5, -0.5, 0.0) );
  Vertex& outv2 = outMesh->createVertex ( Eigen::Vector3d(1.5, 1.5, 0.0) );
  outMesh->allocateDataValues();

  // Compute and perform mapping
  mapping.computeMapping();
  mapping.map ( inDataID, outDataID );

  // Validate results
  BOOST_TEST ( mapping.hasComputedMapping() == true );
  BOOST_TEST ( outData->values()[0] == (valueVertex1 + valueVertex2) * 0.5 );
  BOOST_TEST ( outData->values()[1] == valueVertex1 );
  BOOST_TEST ( outData->values()[2] == valueVertex2 );

  // Redo mapping, results should be
  //assign(outData->values()) = 0.0;
  outData->values() = Eigen::VectorXd::Constant(outData->values().size(), 0.0);

  mapping.map ( inDataID, outDataID );
  BOOST_TEST ( outData->values()[0] == (valueVertex1 + valueVertex2) * 0.5 );
  BOOST_TEST ( outData->values()[1] == valueVertex1 );
  BOOST_TEST ( outData->values()[2] == valueVertex2 );

  // Change vertex coordinates and redo mapping
  outv0.setCoords ( Eigen::Vector3d(-0.5, -0.5, 0.0) );
  outv1.setCoords ( Eigen::Vector3d(1.5, 1.5, 0.0) );
  outv2.setCoords ( Eigen::Vector3d(0.5, 0.5, 0.0) );
  //assign(outData->values()) = 0.0;
  outData->values() = Eigen::VectorXd::Constant(outData->values().size(), 0.0);

  mapping.clear();
  mapping.computeMapping();
  mapping.map ( inDataID, outDataID );
  BOOST_TEST ( outData->values()[0] == valueVertex1 );
  BOOST_TEST ( outData->values()[1] == valueVertex2 );
  BOOST_TEST ( outData->values()[2] == (valueVertex1 + valueVertex2) * 0.5 );

  // Reset output data to zero and redo the mapping
  //assign(outData->values()) = 0.0;
  outData->values() = Eigen::VectorXd::Constant(outData->values().size(), 0.0);

  mapping.map ( inDataID, outDataID );
  BOOST_TEST ( outData->values()[0] == valueVertex1 );
  BOOST_TEST ( outData->values()[1] == valueVertex2 );
  BOOST_TEST ( outData->values()[2] == (valueVertex1 + valueVertex2) * 0.5 );
}
#endif

BOOST_AUTO_TEST_CASE(AxisAlignedTriangles)
{
  using namespace precice::mesh;
  constexpr int dimensions = 3;

  // Create mesh to map from with Triangles ABD and BDC
  PtrMesh inMesh(new Mesh("InMesh", dimensions, false));
  PtrData inData = inMesh->createData("InData", 1);
  Vertex& inVA = inMesh->createVertex(Eigen::Vector3d{0,0,0});
  Vertex& inVB = inMesh->createVertex(Eigen::Vector3d{0,1,0});
  Vertex& inVC = inMesh->createVertex(Eigen::Vector3d{1,1,0});
  Vertex& inVD = inMesh->createVertex(Eigen::Vector3d{1,0,0});

  Edge& inEDA = inMesh->createEdge(inVD, inVA);
  Edge& inEAB = inMesh->createEdge(inVA, inVB);
  Edge& inEBD = inMesh->createEdge(inVB, inVD);
  Edge& inEDC = inMesh->createEdge(inVD, inVC);
  Edge& inECB = inMesh->createEdge(inVC, inVB);

  inMesh->createTriangle(inEAB, inEBD, inEDA);
  inMesh->createTriangle(inEBD, inEDC, inECB);
  inMesh->allocateDataValues();
  inMesh->computeState();
  inData->values() << 1.0, 1.0, 1.0, 1.0;

  // Create mesh to map to with one vertex per defined traingle
  PtrMesh outMesh(new Mesh("OutMesh", dimensions, false));
  PtrData outData = outMesh->createData("OutData", 1);
  outMesh->createVertex(Eigen::Vector3d{0.33, 0.33, 0});
  outMesh->createVertex(Eigen::Vector3d{0.66, 0.66, 0});
  outMesh->allocateDataValues();
  outMesh->computeState();
  outData->values() << 0.0, 0.0;

  // Setup mapping with mapping coordinates and geometry used
  precice::mapping::NearestProjectionMapping mapping(mapping::Mapping::CONSISTENT, dimensions);
  mapping.setMeshes(inMesh, outMesh);
  BOOST_TEST(mapping.hasComputedMapping() == false);

  mapping.computeMapping();
  BOOST_TEST(mapping.hasComputedMapping() == true);
  BOOST_TEST_INFO("In Data:" << inData->values());
  BOOST_TEST_INFO("Out Data before Mapping:" << outData->values());
  mapping.map(inData->getID(), outData->getID());
  BOOST_TEST_INFO("Out Data after Mapping:" << outData->values());
  BOOST_TEST(outData->values() == outData->values().cwiseAbs());
}

BOOST_AUTO_TEST_CASE(Query_3D_FullMesh)
{
  using namespace precice::mesh;
  constexpr int dimensions = 3;

  PtrMesh inMesh(new precice::mesh::Mesh("InMesh", 3, false));
  PtrData inData = inMesh->createData("InData", 1);
  const double z1 = 0.1;
  const double z2 = -0.1;
  auto& v00 = inMesh->createVertex(Eigen::Vector3d(0, 0, 0));
  auto& v01 = inMesh->createVertex(Eigen::Vector3d(0, 1, 0));
  auto& v10 = inMesh->createVertex(Eigen::Vector3d(1, 0, z1));
  auto& v11 = inMesh->createVertex(Eigen::Vector3d(1, 1, z1));
  auto& v20 = inMesh->createVertex(Eigen::Vector3d(2, 0, z2));
  auto& v21 = inMesh->createVertex(Eigen::Vector3d(2, 1, z2));
  auto& ell = inMesh->createEdge(v00, v01);
  auto& elt = inMesh->createEdge(v01, v11);
  auto& elr = inMesh->createEdge(v11, v10);
  auto& elb = inMesh->createEdge(v10, v00);
  auto& eld = inMesh->createEdge(v00, v11);
  auto& erl = elr;
  auto& ert = inMesh->createEdge(v11, v21);
  auto& err = inMesh->createEdge(v21, v20);
  auto& erb = inMesh->createEdge(v20, v10);
  auto& erd = inMesh->createEdge(v10, v21);
  auto& tlt = inMesh->createTriangle(ell, elt, eld);
  auto& tlb = inMesh->createTriangle(eld, elb, elr);
  auto& trt = inMesh->createTriangle(erl, ert, erd);
  auto& trb = inMesh->createTriangle(erd, erb, err);

  inMesh->allocateDataValues();
  inData->values() = Eigen::VectorXd::Constant(6, 1.0);

  PtrMesh outMesh(new Mesh("OutMesh", dimensions, false));
  PtrData outData = outMesh->createData("OutData", 1);
  outMesh->createVertex(Eigen::Vector3d{0.7, 0.5, 0.0});
  outMesh->allocateDataValues();
  outData->values() = Eigen::VectorXd::Constant(1, 2.0);

  // Setup mapping with mapping coordinates and geometry used
  precice::mapping::NearestProjectionMapping mapping(mapping::Mapping::CONSISTENT, dimensions);
  mapping.setMeshes(inMesh, outMesh);
  BOOST_TEST(mapping.hasComputedMapping() == false);

  mapping.computeMapping();
  BOOST_TEST(mapping.hasComputedMapping() == true);
  mapping.map(inData->getID(), outData->getID());
  BOOST_TEST(outData->values()[0] == 1.0);
}



BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
