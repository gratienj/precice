# distutils: language = c++

# comments on test layout: https://docs.pytest.org/en/latest/goodpractices.html
# run with python -m unittest tests.test_fenicsadapter

cimport precice_future
from unittest import TestCase
import numpy as np

class TestBindings(TestCase):
    """
    Test suite to check correct behaviour of python bindings.
    """
    
    def test_constructor(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        self.assertTrue(True)

    def test_get_dimensions(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        # TODO: it would be nice to be able to mock the output of the interface directly in the test, not in test/SolverInterface.hpp
        fake_dimension = 3  # compare to test/SolverInterface.hpp, fake_dimensions
        self.assertEqual(fake_dimension, solver_interface.get_dimensions())  # TODO: it would be nice to be able to mock the output of the interface directly in the test, not in test/SolverInterface.hpp

    def test_get_mesh_id(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        # TODO: it would be nice to be able to mock the output of the interface directly in the test, not in test/SolverInterface.hpp
        fake_mesh_id = 0  # compare to test/SolverInterface.hpp, fake_mesh_id
        self.assertEqual(fake_mesh_id, solver_interface.get_mesh_id("testMesh"))

    def test_set_mesh_vertices (self):
        solver_interface = precice_future.Interface("test", 0, 1)
        fake_mesh_id = 0  # compare to test/SolverInterface.cpp, fake_mesh_id
        fake_dimension = 3  # compare to test/SolverInterface.cpp, fake_dimensions
        n_fake_vertices = 3  # compare to test/SolverInterface.cpp, n_fake_vertices
        positions = np.random.rand(fake_dimension * n_fake_vertices)
        self.assertTrue(np.array_equal(np.array(range(n_fake_vertices)), solver_interface.set_mesh_vertices(fake_mesh_id, positions)))

    def test_read_write_block_scalar_data(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        write_data = np.array([3, 7, 8], dtype=np.double)
        solver_interface.write_block_scalar_data(1, np.array([1, 2, 3]), write_data)
        read_data = solver_interface.read_block_scalar_data(1, np.array([1, 2, 3]))
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_read_write_block_scalar_data_non_contiguous(self):
        """
        Tests behaviour of solver interface, if a non contiguous array is passed to the interface.

        Note: Check whether np.ndarray is contiguous via np.ndarray.flags.
        """
        solver_interface = precice_future.Interface("test", 0, 1)
        dummy_array = np.random.rand(3, 3)
        write_data = dummy_array[:, 1]
        assert(write_data.flags["C_CONTIGUOUS"] == False)
        solver_interface.write_block_scalar_data(1, np.array([1, 2, 3]), write_data)
        read_data = solver_interface.read_block_scalar_data(1, np.array([1, 2, 3]))
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_read_write_scalar_data(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        write_data = 3
        solver_interface.write_scalar_data(1, 1, write_data)
        read_data = solver_interface.read_scalar_data(1, 1)
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_read_write_block_vector_data(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        write_data = np.array([3, 7, 8, 7 ,6, 5], dtype=np.double)
        solver_interface.write_block_vector_data(1, np.array([1, 2]), write_data)
        read_data = solver_interface.read_block_vector_data(1, np.array([1, 2]))
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_read_write_block_vector_data_non_contiguous(self):
        """
        Tests behaviour of solver interface, if a non contiguous array is passed to the interface.

        Note: Check whether np.ndarray is contiguous via np.ndarray.flags.
        """
        solver_interface = precice_future.Interface("test", 0, 1)
        dummy_array = np.random.rand(6, 3)
        write_data = dummy_array[:, 1]
        assert(write_data.flags["C_CONTIGUOUS"] == False)
        solver_interface.write_block_vector_data(1, np.array([1, 2]), write_data)
        read_data = solver_interface.read_block_vector_data(1, np.array([1, 2]))
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_read_write_vector_data(self):
        solver_interface = precice_future.Interface("test", 0, 1)
        write_data = np.array([1, 2, 3], dtype=np.double)
        solver_interface.write_vector_data(1, 1, write_data)
        read_data = solver_interface.read_vector_data(1, 1)
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_read_write_vector_data_non_contiguous(self):
        """
        Tests behaviour of solver interface, if a non contiguous array is passed to the interface.

        Note: Check whether np.ndarray is contiguous via np.ndarray.flags.
        """
        solver_interface = precice_future.Interface("test", 0, 1)
        dummy_array = np.random.rand(3, 3)
        write_data = dummy_array[:, 1]
        assert(write_data.flags["C_CONTIGUOUS"] == False)
        solver_interface.write_vector_data(1, 1, write_data)
        read_data = solver_interface.read_vector_data(1, 1)
        self.assertTrue(np.array_equal(write_data, read_data))

    def test_set_mesh_vertex(self):
        # TODO: Add missing test
        self.assertTrue(False)

    def test_get_mesh_vertex_size(self):
        # TODO: Add missing test
        self.assertTrue(False)

    def test_get_mesh_vertices(self):
        # TODO: Add missing test
        self.assertTrue(False)

    def test_get_data_id(self):
        # TODO: Add missing test
        self.assertTrue(False)
