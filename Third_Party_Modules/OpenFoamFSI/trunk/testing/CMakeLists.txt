cmake_minimum_required(VERSION 2.8)
project(OpenFoamModuleDriver)

# Compiled objects output paths
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single directory for all executables.")
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single directory for all libraries and archives.")
mark_as_advanced (LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
# Set up RPATH
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#Project-specific constructs
INCLUDE_DIRECTORIES(include)

# 
# ========= USE IN EXISTING PROJECT ============
#
# In order to use this IllinoisRocstar Project Template
# in an existing project COPY this file from this point
# on and paste it into your project's CMakeLists.txt 
# file *before* your existing INSTALL directives.
#
INCLUDE(CTest)
IF(BUILD_TESTING)
  SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
  MARK_AS_ADVANCED(BUILDNAME)
ENDIF(BUILD_TESTING)

find_package(MPI)
IF(MPI_CXX_COMPILER)
  set (CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
ENDIF()

# Information for OpenFoamModuleDriver CMake
#FIND_LIBRARY(IMPACT_LIBF SITCOMF)
#FIND_LIBRARY(IMPACT_LIB SITCOM)
#FIND_FILE(IMPACT_HDR comf90.h)
#GET_FILENAME_COMPONENT(IMPACT_LIBPATH ${IMPACT_LIB} PATH)
#GET_FILENAME_COMPONENT(IMPACT_INCPATH ${IMPACT_HDR} PATH)
#include_directories(${IMPACT_INCPATH})
#
#include_directories(${IRAD_INCPATH})

#FIND_LIBRARY(MATC_LIB matc)
#FIND_LIBRARY(HUTI_LIB huti)
#FIND_LIBRARY(ARPACK_LIB arpack)
#FIND_LIBRARY(PARPACK_LIB parpack)
#FIND_LIBRARY(BINIO_LIB binio)
#FIND_LIBRARY(ELMER_LIB elmersolver)

#FIND_LIBRARY(SOLVER_LIB SolverModule)
#message("SOLVER_LIB = ${SOLVER_LIB}")
#message("IMPACT_LIB = ${IMPACT_LIB}")

# Project-specific constructs
ADD_EXECUTABLE(OFModuleTest src/TestOpenFoamModuleDriver.C)
target_link_libraries(OFModuleTest ${OF_FSILIB} ${FINITEVOL_LIB}
${LDUSOLVERS_LIB} ${FOAM_LIB} ${IMPACT_LIB} ${SolverUtils_LIB} ${IRAD_LIB})


#IF(MPI_CXX_COMPILER)
#  ADD_DEFINITIONS(-D_ELMERMODULEDRIVER_PARALLEL_)
#  ADD_EXECUTABLE(elmermoduledriver_parallel_test src/ParallelTestOpenFoamModuleDriver.C)
#  TARGET_LINK_LIBRARIES(elmermoduledriver_parallel_test ${IMPACT_LIB} ${IMPACT_LIBF} ${SOLVER_LIB} ${ELMER_LIB} ${MATC_LIB} ${HUTI_LIB} ${ARPACK_LIB} ${PARPACK_LIB} ${BINIO_LIB} OpenFoamModuleDriver)
##  TARGET_LINK_LIBRARIES(elmermoduledriver_parallel_test ${IMPACT_LIB} ${IMPACT_LIBF} OpenFoamModuleDriver)
#ENDIF()

# Add target to build the documentation
#option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ON)
#IF(BUILD_DOCUMENTATION)
#
#  FIND_PACKAGE(Doxygen)
#  IF(NOT DOXYGEN_FOUND)
#    MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
#  ENDIF()
#  
##  FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/doc DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#  
#  
#  SET( doxyfile_in          ${CMAKE_CURRENT_SOURCE_DIR}/doc/OpenFoamModuleDriver.cfg.in )
#  SET( doxyfile             ${CMAKE_CURRENT_BINARY_DIR}/OpenFoamModuleDriver.cfg        )
#  SET( documentation        ${CMAKE_CURRENT_BINARY_DIR}/docs                 )
#
#  CONFIGURE_FILE( ${doxyfile_in} ${doxyfile} @ONLY )
#
#  ADD_CUSTOM_COMMAND( OUTPUT ${documentation}
#                      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
#                      MAIN_DEPENDENCY ${doxyfile} ${doxyfile_in}
#                      DEPENDS OpenFoamModuleDriver ${doxy_extra_files}
#                      COMMENT "Generating documentation")
#
#  ADD_CUSTOM_TARGET( docs DEPENDS ${documentation} )
#
#  INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs DESTINATION share/OpenFoamModuleDriver )
#ENDIF()

# The 3 tests below actually run all the tests
ADD_TEST(RunOpenFoamModuleDriverUnitTests
${EXECUTABLE_OUTPUT_PATH}/OFModuleTest -s ${PROJECT_SOURCE_DIR} -com-mpi -o testresults.txt)
ADD_TEST(RunTestScripts ${RUNTEST} -l ${PROJECT_SOURCE_DIR}/share/Testing/test_scripts/tests.list -s ${PROJECT_SOURCE_DIR} -b ${CMAKE_BINARY_DIR}/bin -a ${IRAD_EXEPATH} -v 2 -o testresults.txt)
ADD_TEST(OpenFoamFsiRegressionTest:Works ${TESTRESULT} OpenFoamFsiRegressionTest:Works testresults.txt)
ADD_TEST(HronTurekFsiRegressionTest:Works ${TESTRESULT} HronTurekFsiRegressionTest:Works testresults.txt)
ADD_TEST(LoadSolverModule:Works ${TESTRESULT} LoadSolverModule:Works testresults.txt)
ADD_TEST(UnloadSolverModule:Works ${TESTRESULT} LoadSolverModule:Works testresults.txt)
ADD_TEST(InitFunction:Exists ${TESTRESULT} InitFunction:Exists testresults.txt)
ADD_TEST(LoadUnitTestData:Works ${TESTRESULT} LoadUnitTestData:Works testresults.txt)
ADD_TEST(InitFunction:Runs ${TESTRESULT} InitFunction:Runs testresults.txt)
ADD_TEST(CoordinateData:Registered ${TESTRESULT} CoordinateData:Registered testresults.txt)
ADD_TEST(CoordinateData:Correct ${TESTRESULT} CoordinateData:Correct testresults.txt)
ADD_TEST(ConnectivityData:Registered ${TESTRESULT} ConnectivityData:Registered testresults.txt)
ADD_TEST(ConnectivityData:Correct ${TESTRESULT} ConnectivityData:Correct testresults.txt)
ADD_TEST(TimeData:Registered ${TESTRESULT} TimeData:Registered testresults.txt)
ADD_TEST(TimeData:Correct ${TESTRESULT} TimeData:Correct testresults.txt)
ADD_TEST(StepFluidFunction:Exists ${TESTRESULT} StepFluidFunction:Exists testresults.txt)
ADD_TEST(StepFluidFunction:Steps ${TESTRESULT} StepFluidFunction:Steps testresults.txt)
ADD_TEST(PressureData:Registered ${TESTRESULT} PressureData:Registered testresults.txt)
ADD_TEST(TractionData:Registered ${TESTRESULT} TractionData:Registered testresults.txt)
ADD_TEST(PressureData:Correct ${TESTRESULT} PressureData:Correct testresults.txt)
ADD_TEST(TractionData:Correct ${TESTRESULT} TractionData:Correct testresults.txt)
#ADD_TEST(RunFunction:Exists ${TESTRESULT} RunFunction:Exists testresults.txt)

#
#  all this stuff here is saved from the examples and can be deleted
#
#ADD_TEST(FSIobstacleTest ${RUNTEST} -f ${PROJECT_SOURCE_DIR}/share/Testing/test_scripts/FSIobstacle.csh -s ${PROJECT_SOURCE_DIR} -b ${CMAKE_BINARY_DIR}/bin -v 2 -o elmermoduledriver_testresults.txt)
#ADD_TEST(VortexTest ${RUNTEST} -f ${PROJECT_SOURCE_DIR}/share/Testing/test_scripts/Vortex.csh -s ${PROJECT_SOURCE_DIR} -b ${CMAKE_BINARY_DIR}/bin -v 2 -o elmermoduledriver_testresults.txt)

#ADD_TEST(RunPlatformSpecificTests ${RUNTEST} -p ${PROJECT_SOURCE_DIR}/share/Platforms/platforms -o testresults.txt)
#ADD_TEST(RunSpecificTestScript    ${RUNTEST} -f ${PROJECT_SOURCE_DIR}/share/Platforms/test_stub.csh -o testresults.txt)

# Simple test of return code of a program

#message("EXECUTABLE_OUTPUT_PATH = ${EXECUTABLE_OUTPUT_PATH}")
#message("PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
#message("CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

#ADD_TEST(ExampleProgram:Runs ${EXECUTABLE_OUTPUT_PATH}/sep cmake_install.cmake)

# The remaining "tests" just use the "testresults" utility to check the results of the tests run above
#ADD_TEST(TestStubWorks ${TESTRESULT} TestStubWorks testresults.txt)
#ADD_TEST(ExampleProgram:Works ${TESTRESULT} ExampleProgram:Works elmermoduledriver_testresults.txt)
#ADD_TEST(ExampleFunction:Works ${TESTRESULT} ExampleFunction:Works elmermoduledriver_testresults.txt)
#ADD_TEST(TrapezoidQuadrature:Runs ${TESTRESULT} TrapezoidQuadrature:Runs elmermoduledriver_testresults.txt)
#ADD_TEST(TrapezoidQuadrature:Accurate ${TESTRESULT} TrapezoidQuadrature:Accurate elmermoduledriver_testresults.txt)
#ADD_TEST(TrapezoidQuadrature:Order ${TESTRESULT} TrapezoidQuadrature:Order2 elmermoduledriver_testresults.txt)
#ADD_TEST(MidPointQuadrature:Runs ${TESTRESULT} MidPointQuadrature:Runs elmermoduledriver_testresults.txt)
#ADD_TEST(MidPointQuadrature:Accurate ${TESTRESULT} MidPointQuadrature:Accurate elmermoduledriver_testresults.txt)
#ADD_TEST(MidPointQuadrature:Order ${TESTRESULT} MidPointQuadrature:Order2 elmermoduledriver_testresults.txt)
#ADD_TEST(FSIregressionTest:Works ${TESTRESULT} FSIregressionTest:Works elmermoduledriver_testresults.txt)
#ADD_TEST(VortexRegressionTest:Works ${TESTRESULT} VortexRegressionTest:Works elmermoduledriver_testresults.txt)
#ADD_TEST(Initialize:Runs ${TESTRESULT} Initialize:Runs testresults.txt)
#ADD_TEST(ModuleFunctions:Runs ${TESTRESULT} ModuleFunctions:Runs elmermoduledriver_testresults.txt)


#ADD_TEST(CoordinateData:Registered ${TESTRESULT} CoordinateData:Registered elmermoduledriver_testresults.txt)
#ADD_TEST(CoordinateData:Correct ${TESTRESULT} CoordinateData:Correct elmermoduledriver_testresults.txt)
#ADD_TEST(ConnectivityData:Registered ${TESTRESULT} ConnectivityData:Registered elmermoduledriver_testresults.txt)
#ADD_TEST(ConnectivityData:Correct ${TESTRESULT} ConnectivityData:Correct elmermoduledriver_testresults.txt)
#ADD_TEST(TimeStepper:Runs ${TESTRESULT} TimeStepper:Runs elmermoduledriver_testresults.txt)
#ADD_TEST(DisplacementData:Registered ${TESTRESULT} DisplacementData:Registered elmermoduledriver_testresults.txt)
#ADD_TEST(DisplacementData:Correct ${TESTRESULT} DisplacementData:Correct elmermoduledriver_testresults.txt)
#ADD_TEST(Finalize:Runs ${TESTRESULT} Finalize:Runs elmermoduledriver_testresults.txt)
#ADD_TEST(UnloadSolverModule:Works ${TESTRESULT} UnloadSolverModule:Works elmermoduledriver_testresults.txt)
#ADD_TEST(OpenFoamUnitTests:Run ${TESTRESULT} OpenFoamUnitTests:Run elmermoduledriver_testresults.txt)

IF(MPI_CXX_COMPILER)
   ADD_EXECUTABLE(OFModuleTestPar src/ParallelTestOpenFoamModuleDriver.C)
   target_link_libraries(OFModuleTestPar ${IMPACT_LIB} ${SolverUtils_LIB} ${IRAD_LIB})
   # The parallel unit/regression tests are actually run by following two lines
   ADD_TEST(RunParallelPlatformUnitTests:Runs ${RUNTEST} -p ${PROJECT_SOURCE_DIR}/share/Platforms/parallel_platforms -s
   ${PROJECT_SOURCE_DIR} -b ${CMAKE_BINARY_DIR}/bin -o testresults.txt)
   ADD_TEST(RunParallelTestScripts ${RUNTEST} -l ${PROJECT_SOURCE_DIR}/share/Testing/test_scripts/testsPar.list -s ${PROJECT_SOURCE_DIR} -b ${CMAKE_BINARY_DIR}/bin -a ${IRAD_EXEPATH} -v 2 -o testresults.txt)
   # The remainder use "testresults" utility to check the results of the parallel tests run above
   ADD_TEST(ParOpenFoamFsiRegressionTest:Worksi ${TESTRESULT} ParOpenFoamFsiRegressionTest:Works testresults.txt)
   ADD_TEST(ParOpenFoamModuleRegressionTest:Works ${TESTRESULT} ParOpenFoamModuleRegressionTest:Works testresults.txt)
   ADD_TEST(ParCommunicatorPassToModule:Pass ${TESTRESULT} CommunicatorPassToModule:Pass testresults.txt)
   ADD_TEST(LoadParSolverModule:Works ${TESTRESULT} LoadParSolverModule:Works testresults.txt)
   ADD_TEST(UnloadParSolverModule:Works ${TESTRESULT} LoadParSolverModule:Works testresults.txt)
   ADD_TEST(ParLoadUnitTestData:Works ${TESTRESULT} ParLoadUnitTestData:Works testresults.txt)
   ADD_TEST(ParInitFunction:Exists ${TESTRESULT} ParInitFunction:Exists testresults.txt)
   ADD_TEST(ParInitFunction:Runs ${TESTRESULT} ParInitFunction:Runs testresults.txt)
   ADD_TEST(ParPressureData:Registered ${TESTRESULT} ParPressureData:Registered testresults.txt)
   ADD_TEST(ParTractionData:Registered ${TESTRESULT} ParTractionData:Registered testresults.txt)
   ADD_TEST(ParPressureData:Correct ${TESTRESULT} ParPressureData:Correct testresults.txt)
   ADD_TEST(ParTractionData:Correct ${TESTRESULT} ParTractionData:Correct testresults.txt)
   ADD_TEST(ParCoordinateData:Registered ${TESTRESULT} ParCoordinateData:Registered testresults.txt)
   ADD_TEST(ParCoordinateData:Correct ${TESTRESULT} ParCoordinateData:Correct testresults.txt)
   ADD_TEST(ParConnectivityData:Registered ${TESTRESULT} ParConnectivityData:Registered testresults.txt)
   ADD_TEST(ParConnectivityData:Correct ${TESTRESULT} ParConnectivityData:Correct testresults.txt)
   ADD_TEST(ParTimeData:Registered ${TESTRESULT} ParTimeData:Registered testresults.txt)
   ADD_TEST(ParTimeData:Correct ${TESTRESULT} ParTimeData:Correct testresults.txt)
   ADD_TEST(ParStepFluidFunction:Exists ${TESTRESULT} ParStepFluidFunction:Exists testresults.txt)
   ADD_TEST(ParStepFluidFunction:Steps ${TESTRESULT} ParStepFluidFunction:Steps testresults.txt)
ENDIF()

INSTALL(TARGETS OFModuleTest RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
#IF(MPI_CXX_COMPILER)
  #INSTALL(TARGETS elmermoduledriver_parallel_test RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
#ENDIF()
# ========= USE IN EXISTING PROJECT ==============
