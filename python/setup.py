from skbuild import setup

setup(
    name="exactextract",
    version="0.2",
    url="https://github.com/isciences/exactextract",
    cmake_args=["-DCMAKE_BUILD_TYPE=Release", "-DBUILD_CLI=NO", "-DBUILD_TEST=NO", "-DBUILD_DOC=NO"],
    cmake_source_dir="..",
    packages=["exactextract"],
    package_dir={"exactextract":"src/exactextract"}
)
