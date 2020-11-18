#
# This file lists all tests sources that will be compiled into the test executable
#
target_sources(testprecice
    PRIVATE
    src/acceleration/test/AccelerationMasterSlaveTest.cpp
    src/acceleration/test/ParallelMatrixOperationsTest.cpp
    src/acceleration/test/PreconditionerTest.cpp
    src/acceleration/test/QRFactorizationTest.cpp
    src/action/tests/PythonActionTest.cpp
    src/action/tests/ScaleActionTest.cpp
    src/action/tests/SummationActionTest.cpp
    src/com/tests/CommunicateBoundingBoxTest.cpp
    src/com/tests/CommunicateMeshTest.cpp
    src/com/tests/GenericTestFunctions.hpp
    src/com/tests/MPIDirectCommunicationTest.cpp
    src/com/tests/MPIPortsCommunicationTest.cpp
    src/com/tests/MPISinglePortsCommunicationTest.cpp
    src/com/tests/SocketCommunicationTest.cpp
    src/cplscheme/tests/AbsoluteConvergenceMeasureTest.cpp
    src/cplscheme/tests/CompositionalCouplingSchemeTest.cpp
    src/cplscheme/tests/DummyCouplingScheme.cpp
    src/cplscheme/tests/DummyCouplingScheme.hpp
    src/cplscheme/tests/ExplicitCouplingSchemeTest.cpp
    src/cplscheme/tests/MinIterationConvergenceMeasureTest.cpp
    src/cplscheme/tests/ParallelImplicitCouplingSchemeTest.cpp
    src/cplscheme/tests/RelativeConvergenceMeasureTest.cpp
    src/cplscheme/tests/ResidualRelativeConvergenceMeasureTest.cpp
    src/cplscheme/tests/SerialImplicitCouplingSchemeTest.cpp
    src/io/tests/ExportConfigurationTest.cpp
    src/io/tests/ExportVTKTest.cpp
    src/io/tests/ExportVTKXMLTest.cpp
    src/io/tests/TXTTableWriterTest.cpp
    src/io/tests/TXTWriterReaderTest.cpp
    src/m2n/tests/GatherScatterCommunicationTest.cpp
    src/m2n/tests/PointToPointCommunicationTest.cpp
    src/mapping/tests/MappingConfigurationTest.cpp
    src/mapping/tests/NearestNeighborMappingTest.cpp
    src/mapping/tests/NearestProjectionMappingTest.cpp
    src/mapping/tests/PetRadialBasisFctMappingTest.cpp
    src/mapping/tests/RadialBasisFctMappingTest.cpp
    src/math/tests/BarycenterTest.cpp
    src/math/tests/DifferencesTest.cpp
    src/math/tests/GeometryTest.cpp
    src/mesh/tests/BoundingBoxTest.cpp
    src/mesh/tests/DataConfigurationTest.cpp
    src/mesh/tests/EdgeTest.cpp
    src/mesh/tests/MeshTest.cpp
    src/mesh/tests/RTreeAdapterTests.cpp
    src/mesh/tests/RTreeTests.cpp
    src/mesh/tests/TriangleTest.cpp
    src/mesh/tests/VertexTest.cpp
    src/partition/tests/ProvidedPartitionTest.cpp
    src/partition/tests/ReceivedPartitionTest.cpp
    src/precice/tests/ParallelTests.cpp
    src/precice/tests/SerialTests.cpp
    src/precice/tests/VersioningTests.cpp
    src/precice/tests/WatchIntegralTest.cpp
    src/precice/tests/WatchPointTest.cpp
    src/query/tests/FindClosestTest.cpp
    src/query/tests/FindClosestVertexVisitorTest.cpp
    src/testing/TestContext.cpp
    src/testing/TestContext.hpp
    src/testing/Testing.cpp
    src/testing/Testing.hpp
    src/testing/main.cpp
    src/testing/tests/ExampleTests.cpp
    src/utils/tests/AlgorithmTest.cpp
    src/utils/tests/DimensionsTest.cpp
    src/utils/tests/EigenHelperFunctionsTest.cpp
    src/utils/tests/ManageUniqueIDsTest.cpp
    src/utils/tests/MultiLockTest.cpp
    src/utils/tests/ParallelTest.cpp
    src/utils/tests/PointerVectorTest.cpp
    src/utils/tests/StatisticsTest.cpp
    src/utils/tests/StringTest.cpp
    src/xml/tests/ParserTest.cpp
    src/xml/tests/PrinterTest.cpp
    src/xml/tests/XMLTest.cpp
    )
