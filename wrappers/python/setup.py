"""Setup script for regopy."""

import multiprocessing
import os
import platform
import subprocess

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
    "pytest",
    "sphinx",
    "sphinx-autodoc-typehints",
    "sphinx-rtd-theme",
    "enum-tools[sphinx]",
    "twine",
    "wheel"
]

with open("README.md", "r") as file:
    LONG_DESCRIPTION = file.read()

VERSION = "1.2.0"


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

        src_path = os.path.abspath("rego-cpp")
        if not os.path.exists("rego-cpp"):
            repo = os.environ.get(
                "REGOCPP_REPO", "https://github.com/microsoft/rego-cpp.git")
            tag = os.environ.get("REGOCPP_TAG", "main")

            subprocess.check_call(["git", "clone", repo, src_path])
            subprocess.check_call(["git", "checkout", tag], cwd=src_path)

        cmake_args = [f"-S {src_path}",
                      f"-B {self.build_temp}",
                      f"-DCMAKE_INSTALL_PREFIX={extdir}",
                      "-DREGOCPP_BUILD_SHARED=ON",
                      f"-DCMAKE_BUILD_TYPE={cfg}",
                      "-DSNMALLOC_ENABLE_DYNAMIC_LOADING=ON"]

        build_args = ["--build", self.build_temp,
                      "--config", cfg,
                      "--target", "rego_shared",
                      "--verbose"]
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        num_workers = multiprocessing.cpu_count()
        build_args += ["--parallel", str(num_workers)]

        subprocess.check_call(["cmake"] + cmake_args)
        subprocess.check_call(["cmake"] + build_args)

        output_dir = os.path.join(self.build_temp, "src")
        if os.path.exists(os.path.join(output_dir, "Release")):
            output_dir = os.path.join(output_dir, "Release")

        if platform.uname()[0] == "Windows":
            dynamic_lib_ext = ".dll"
        elif platform.uname()[0] == "Darwin":
            dynamic_lib_ext = ".dylib"
        else:
            dynamic_lib_ext = ".so"

        os.makedirs(extdir, exist_ok=True)
        for name in os.listdir(output_dir):
            if name.endswith(dynamic_lib_ext):
                ext_path = os.path.join(output_dir, name)
                self.copy_file(ext_path, extdir)

        self.copy_file(ext_path, extdir)


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
    include_package_data=True,
    python_requires=">=3.6, <4",
    ext_modules=[CMakeExtension("regopy.rego_shared")],
    package_data={"regopy": ["*.dll", "*.so", "*.dylib"]},
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    project_urls={
        "Documentation": "https://microsoft.github.io/rego-cpp/",
        "Bug Reports": "https://github.com/microsoft/rego-cpp/issues",
        "Source": "https://github.com/microsoft/rego-cpp",
    },
    url="https://microsoft.github.io/rego-cpp/",
    install_requires=[],
    extras_require={
        "dev": REQUIRES_DEV,
    },
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
