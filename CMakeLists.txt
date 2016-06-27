cmake_minimum_required(VERSION 2.8)
project(ElmerFoamFSI)

# Compiled objects output paths
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single directory for all executables.")
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single directory for all libraries and archives.")
mark_as_advanced (LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
# Set up RPATH
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

INCLUDE(CTest)
IF(BUILD_TESTING)
  SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
  MARK_AS_ADVANCED(BUILDNAME)
ENDIF(BUILD_TESTING)

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IRAD/")
  add_subdirectory(IRAD)
  SET(IRAD_INCPATH ${CMAKE_SOURCE_DIR}/IRAD/include)
  SET(RUNTEST ${EXECUTABLE_OUTPUT_PATH}/bin/runtest)
  SET(TESTRESULT ${EXECUTABLE_OUTPUT_PATH}/bin/testresults)
ELSE()
  FIND_LIBRARY(IRAD_LIB IRAD)
  FIND_FILE(IRAD_HDR COMM.H)
  FIND_PROGRAM(RUNTEST runtest)
  FIND_PROGRAM(TESTRESULT testresults)
  GET_FILENAME_COMPONENT(IRAD_LIBPATH ${IRAD_LIB} PATH)
  GET_FILENAME_COMPONENT(IRAD_INCPATH ${IRAD_HDR} PATH)
  GET_FILENAME_COMPONENT(IRAD_EXEPATH ${RUNTEST} PATH)
#  link_directories(${IRAD_LIBPATH})
ENDIF()
FIND_LIBRARY(IMPACT_LIB SITCOM)
FIND_LIBRARY(SOLVERUTILS_LIB SolverUtils)
FIND_FILE(IMPACT_HDR com.h)
GET_FILENAME_COMPONENT(IMPACT_LIBPATH ${IMPACT_LIB} PATH)
GET_FILENAME_COMPONENT(IMPACT_INCPATH ${IMPACT_HDR} PATH)

find_package(MPI)
IF(MPI_CXX_COMPILER)
  set (CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
ENDIF()

# Project-specific constructs
include_directories(include ${IRAD_INCPATH} ${IMPACT_INCPATH})

ADD_LIBRARY(ElmerFoamFSI SHARED src/ExampleSourceFile.C)
TARGET_LINK_LIBRARIES(ElmerFoamFSI ${SOLVERUTILS_LIB} ${IRAD_LIB} ${IMPACT_LIB})
ADD_EXECUTABLE(elmerfoamfsi_test src/TestElmerFoamFSI.C)
TARGET_LINK_LIBRARIES(elmerfoamfsi_test ElmerFoamFSI)
ADD_EXECUTABLE(sep src/ExampleSerialProgram.C src/sep.C)
TARGET_LINK_LIBRARIES(sep ElmerFoamFSI)
ADD_EXECUTABLE(elmerfoamfsi src/ElmerFoamDriver.C src/driver.C)
TARGET_LINK_LIBRARIES(elmerfoamfsi ElmerFoamFSI)

IF(MPI_CXX_COMPILER)
  ADD_DEFINITIONS(-D_ELMERFOAMFSI_PARALLEL_)
  ADD_EXECUTABLE(pepi src/ExampleParallelProgram.C src/pepi.C)
  TARGET_LINK_LIBRARIES(pepi ElmerFoamFSI)
  ADD_EXECUTABLE(elmerfoamfsi_parallel_test src/ParallelTestElmerFoamFSI.C) 
  TARGET_LINK_LIBRARIES(elmerfoamfsi_parallel_test ElmerFoamFSI)
  ADD_EXECUTABLE(elmerfoamfsiPar src/ElmerFoamDriverPar.C src/driverPar.C)
  TARGET_LINK_LIBRARIES(elmerfoamfsiPar ElmerFoamFSI)
ENDIF()
INSTALL(TARGETS ElmerFoamFSI elmerfoamfsi RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

# Add target to build the documentation
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ON)
IF(BUILD_DOCUMENTATION)
  FIND_PACKAGE(Doxygen)
  IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
  ENDIF()
  SET( doxyfile_in          ${CMAKE_CURRENT_SOURCE_DIR}/doc/ElmerFoamFSI.cfg.in )
  SET( doxyfile             ${CMAKE_CURRENT_BINARY_DIR}/ElmerFoamFSI.cfg        )
  SET( documentation        ${CMAKE_CURRENT_BINARY_DIR}/docs                 )
  CONFIGURE_FILE( ${doxyfile_in} ${doxyfile} @ONLY )
  ADD_CUSTOM_COMMAND( OUTPUT ${documentation}
                      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                      MAIN_DEPENDENCY ${doxyfile} ${doxyfile_in}
                      DEPENDS ElmerFoamFSI ${doxy_extra_files}
                      COMMENT "Generating documentation")
  ADD_CUSTOM_TARGET( docs DEPENDS ${documentation} )
  INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs DESTINATION share/ElmerFoamFSI )
  FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/doc/PhysicsOfCoupling.pdf DESTINATION
            ${CMAKE_CURRENT_BINARY_DIR}/docs/html)
ENDIF()

ADD_SUBDIRECTORY(testing)
