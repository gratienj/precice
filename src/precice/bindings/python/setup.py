import os
import subprocess
from enum import Enum

from setuptools import setup
from distutils.extension import Extension
from Cython.Distutils.build_ext import new_build_ext as build_ext
from distutils.command.install import install
from distutils.command.build import build

# name of Interfacing API
APPNAME = "precice"
APPVERSION = "1.4.1"  # todo: should be replaced with precice.get_version() as soon as it exists , see https://github.com/precice/precice/issues/261

PYTHON_BINDINGS_PATH = os.path.dirname(os.path.abspath(__file__))

class MpiImplementations(Enum):
    OPENMPI = 1
    MPICH = 2


def check_mpi_implementation(mpi_compiler_wrapper):
    FNULL = open(os.devnull, 'w')  # used to supress output of subprocess.call

    if subprocess.call([mpi_compiler_wrapper, "-showme:compile"], stdout=FNULL, stderr=FNULL) == 0:
        PRECICE_MPI_IMPLEMENTATION = MpiImplementations.OPENMPI
    elif subprocess.call([mpi_compiler_wrapper, "-compile-info"], stdout=FNULL, stderr=FNULL) == 0:
        PRECICE_MPI_IMPLEMENTATION = MpiImplementations.MPICH
    else:
        raise Exception("unknown/no mpi++")

    return PRECICE_MPI_IMPLEMENTATION


def determine_mpi_args(mpi_compiler_wrapper):
    PRECICE_MPI_IMPLEMENTATION = check_mpi_implementation(mpi_compiler_wrapper)
    # determine which flags to use with mpi compiler wrapper
    if PRECICE_MPI_IMPLEMENTATION is MpiImplementations.OPENMPI:
        mpi_compile_args = subprocess.check_output([mpi_compiler_wrapper, "-showme:compile"]).decode().strip().split(
            ' ')
        mpi_link_args = subprocess.check_output([mpi_compiler_wrapper, "-showme:link"]).decode().strip().split(' ')
    elif PRECICE_MPI_IMPLEMENTATION is MpiImplementations.MPICH:
        mpi_compile_args = subprocess.check_output([mpi_compiler_wrapper, "-compile-info"]).decode().strip().split(' ')[
                           1::]
        mpi_link_args = subprocess.check_output([mpi_compiler_wrapper, "-link-info"]).decode().strip().split(' ')[1::]
    else:  # if PRECICE_MPI_IMPLEMENTATION is not mpich or openmpi quit.
        raise Exception("unknown/no mpi found using compiler %s. Could not build PySolverInterface." % mpi_compiler_wrapper)

    return mpi_compile_args, mpi_link_args


def get_extensions(mpi_compiler_wrapper):
    mpi_compile_args, mpi_link_args = determine_mpi_args(mpi_compiler_wrapper)
    
    compile_args = ["-Wall", "-std=c++11"] + mpi_compile_args
    link_args = ["-lprecice"] + mpi_link_args

    return [
        Extension(
                APPNAME,
                sources=[os.path.join(PYTHON_BINDINGS_PATH, APPNAME) + ".pyx"],
                libraries=[],
                language="c++",
                extra_compile_args=compile_args,
                extra_link_args=link_args
            )
    ]


# some global definitions for an additional user input command
doc_string = 'specify the mpi compiler wrapper'
opt_name = 'mpicompiler='
mpicompiler_default = "mpic++"
add_option = [(opt_name, None, doc_string)]


class my_build_ext(build_ext, object):
    description = "building with optional specification of an alternative mpi compiler wrapper"
    user_options = build_ext.user_options + add_option

    def initialize_options(self):
        self.mpicompiler = mpicompiler_default
        super(my_build_ext, self).initialize_options()
        
    def finalize_options(self):
        print("#####")
        print("calling my_build_ext")
        print("using --%s%s" % (opt_name, self.mpicompiler))

        if not self.distribution.ext_modules:
            print("adding extension")
            self.distribution.ext_modules = get_extensions(self.mpicompiler)

        print("#####")

        super(my_build_ext, self).finalize_options()


class my_install(install, object):
    user_options = install.user_options + add_option

    def initialize_options(self):
        self.mpicompiler = mpicompiler_default
        super(my_install, self).initialize_options()


class my_build(build, object):
    user_options = install.user_options + add_option

    def initialize_options(self):
        self.mpicompiler = mpicompiler_default
        super(my_build, self).initialize_options()

    def finalize_options(self):
        print("#####")
        print("calling my_build")
        print("using --%s%s" % (opt_name, self.mpicompiler))

        if not self.distribution.ext_modules:
            print("adding extension")
            self.distribution.ext_modules = get_extensions(self.mpicompiler)

        print("#####")

        super(my_build, self).finalize_options()

dependencies = ['cython']
dependencies.append('mpi4py')  # only needed, if preCICE was compiled with MPI, see https://github.com/precice/precice/issues/311

# build precice.so python extension to be added to "PYTHONPATH" later
setup(
    name=APPNAME,
    version=APPVERSION,
    description='Python language bindings for preCICE coupling library',
    url='https://github.com/precice/precice',
    author='the preCICE developers',
    author_email='info@precice.org',
    license='LGPL-3.0',
    python_requires='>=3',
    install_requires=dependencies,
    cmdclass={'build_ext': my_build_ext,
              'build': my_build,
              'install': my_install},
)
