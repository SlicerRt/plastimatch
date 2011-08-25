SET (CTEST_SOURCE_DIRECTORY "$ENV{HOME}/work/plastimatch")
SET (CTEST_BINARY_DIRECTORY "$ENV{HOME}/build/nightly/gelato_1/plastimatch")
SET (CTEST_CMAKE_COMMAND "/usr/bin/cmake")
SET (CTEST_COMMAND "/usr/bin/ctest -D Nightly")
SET (CTEST_INITIAL_CACHE "
//Name of generator.
CMAKE_GENERATOR:INTERNAL=Unix Makefiles

//Name of the build
BUILDNAME:STRING=lin64-Pisr-Cd-gcc4.3.4

//Name of the computer/site where compile is being run
SITE:STRING=gelato

//The directory containing ITKConfig.cmake.  This is either the
// root of the build tree, or PREFIX/lib/InsightToolkit for an
// installation.
ITK_DIR:PATH=/home/gcs6/build/itk-3.20.0

//Build with shared libraries.
BUILD_SHARED_LIBS:BOOL=ON
")
