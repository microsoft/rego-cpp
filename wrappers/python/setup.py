"""Setup script for scenepic."""

import os
import multiprocessing
import platform
import subprocess
import sys

from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext

REQUIRES_DEV = [
    "flake8",
    "flake8-bugbear",
    "flake8-builtins",
    "flake8-docstrings",
    "flake8-import-order",
    "flake8-quotes",
    "pep8-naming",
    "pytest-md",
    "pytest-emoji",
    "pytest-cov",
    "pytest",
    "sphinx",
    "sphinx-autodoc-typehints",
    "sphinx-rtd-theme",
    "twine",
    "wheel"
]

with open("README.md", "r") as file:
    LONG_DESCRIPTION = file.read()

with open("../../VERSION", "r") as file:
    VERSION = file.read()


class CMakeExtension(Extension):
    """Extension class for cmake."""

    def __init__(self, name: str, source_dir=""):
        """Initializer.

        Args:
            name (str): The extension name
            source_dir (str, optional): The directory containing the source
                                        files. Defaults to "".
        """
        Extension.__init__(self, name, sources=[])
        self.source_dir = os.path.abspath(source_dir)


class CMakeBuild(build_ext):
    """Build driver for cmake."""

    def run(self):
        """Runs the build extension."""
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext: CMakeExtension):
        """Builds the specified extension.

        Args:
            ext (CMakeExtension): the cmake extension.
        """
        cfg = "Debug" if self.debug else "Release"
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))
        cmake_args = ["-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + extdir,
                      "-DCMAKE_BUILD_TYPE=" + cfg,
                      "-DPYTHON_EXECUTABLE=" + sys.executable]

        build_args = ["--config", cfg, "--target", "_regopy"]
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        if platform.system() == "Windows":
            cmake_args += ["-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}".format(cfg.upper(), extdir)]
            if platform.architecture()[0] == "64bit":
                cmake_args += ["-A", "x64"]
            else:
                cmake_args += ["-A", "Win32"]

            build_args += ["--", "/m"]
        else:
            build_args += ["--", "-j2"]

        subprocess.check_call(["cmake", ext.source_dir] +
                              cmake_args, cwd=self.build_temp)
        subprocess.check_call(["cmake", "--build", "."] +
                              build_args, cwd=self.build_temp)


setup(
    name="regopy",
    version=VERSION,
    author="rego-cpp Team",
    author_email="rego-cpp@microsoft.com",
    description="Python interface for the OPA Rego Language and Runtime",
    license="MIT",
    long_description=LONG_DESCRIPTION,
    long_description_content_type="text/markdown",
    packages=find_packages("src"),
    package_dir={"": "src"},
    python_requires=">=3.6, <4",
    ext_modules=[CMakeExtension("regopy._regopy")],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License"
    ],
    project_urls={
        "Documentation": "https://microsoft.github.io/scenepic/",
        "Bug Reports": "https://github.com/microsoft/scenepic/issues",
        "Source": "https://github.com/microsoft/scenepic",
    },
    url="https://microsoft.github.io/scenepic/",
    install_requires=[],
    extras_require={
        "dev": REQUIRES_DEV,
    },
    tests_require=["pytest"],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
