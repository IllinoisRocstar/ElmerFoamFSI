cmake_minimum_required(VERSION 2.8)
project(ElmerModuleDriver)

# Compiled objects output paths
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single directory for all executables.")
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single directory for all libraries and archives.")
mark_as_advanced (LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
# Set up RPATH
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

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

IF(EXISTS ${CMAKE_SOURCE_DIR}/IRAD)
  add_subdirectory(IRAD)
  SET(IRAD_INCPATH ${CMAKE_SOURCE_DIR}/IRAD/include)
  SET(RUNTEST ${EXECUTABLE_OUTPUT_PATH}/bin/runtest)
  SET(TESTRESULT ${EXECUTABLE_OUTPUT_PATH}/bin/testresults)
  SET(IRAD_LIB IRAD)
ELSE()
  FIND_LIBRARY(IRAD_LIB IRAD)
  FIND_FILE(IRAD_HDR COMM.H)
  FIND_PROGRAM(RUNTEST runtest)
  FIND_PROGRAM(TESTRESULT testresults)
  GET_FILENAME_COMPONENT(IRAD_LIBPATH ${IRAD_LIB} PATH)
  GET_FILENAME_COMPONENT(IRAD_INCPATH ${IRAD_HDR} PATH)
  GET_FILENAME_COMPONENT(IRAD_EXEPATH ${RUNTEST} PATH)
ENDIF()
include_directories(${IRAD_INCPATH} include )

find_package(MPI)
IF(MPI_CXX_COMPILER)
  set (CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
ENDIF()

# Project-specific constructs

ADD_LIBRARY(ElmerModuleDriver SHARED src/ExampleSourceFile.C)
TARGET_LINK_LIBRARIES(ElmerModuleDriver ${IRAD_LIB})
ADD_EXECUTABLE(sep src/ExampleSerialProgram.C src/sep.C)
TARGET_LINK_LIBRARIES(sep ElmerModuleDriver)
ADD_EXECUTABLE(make_project src/MakeProject.C)
TARGET_LINK_LIBRARIES(make_project ElmerModuleDriver)

IF(MPI_CXX_COMPILER)
  ADD_DEFINITIONS(-D_ELMERMODULEDRIVER_PARALLEL_)
  ADD_EXECUTABLE(pepi src/ExampleParallelProgram.C src/pepi.C)
  TARGET_LINK_LIBRARIES(pepi ElmerModuleDriver)
ENDIF()

# Information for ElmerModuleDriver CMake
FIND_LIBRARY(IMPACT_LIBF SITCOMF)
FIND_LIBRARY(IMPACT_LIB SITCOM)
FIND_FILE(IMPACT_HDR comf90.h)
GET_FILENAME_COMPONENT(IMPACT_LIBPATH ${IMPACT_LIB} PATH)
GET_FILENAME_COMPONENT(IMPACT_INCPATH ${IMPACT_HDR} PATH)
include_directories(${IMPACT_INCPATH} ${IMPACT_INCPATH})

FIND_LIBRARY(SOLVERUTILS SolverUtils)
#message("SOLVER_LIB = ${SOLVER_LIB}")
#message("IMPACT_LIB = ${IMPACT_LIB}")

add_executable(SolverModuleDriver src/SolverModuleDriver.C)
target_link_libraries(SolverModuleDriver ${SOLVERUTILS} ${IRAD_LIB} ${IMPACT_LIBF} ${IMPACT_LIB})

# Add target to build the documentation
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ON)
IF(BUILD_DOCUMENTATION)
  FIND_PACKAGE(Doxygen)
  IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
  ENDIF()
#  FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/doc DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  
  SET( doxyfile_in          ${CMAKE_CURRENT_SOURCE_DIR}/doc/ElmerModuleDriver.cfg.in )
  SET( doxyfile             ${CMAKE_CURRENT_BINARY_DIR}/ElmerModuleDriver.cfg        )
  SET( documentation        ${CMAKE_CURRENT_BINARY_DIR}/docs                 )

  CONFIGURE_FILE( ${doxyfile_in} ${doxyfile} @ONLY )

  ADD_CUSTOM_COMMAND( OUTPUT ${documentation}
                      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                      MAIN_DEPENDENCY ${doxyfile} ${doxyfile_in}
                      DEPENDS ElmerModuleDriver ${doxy_extra_files}
                      COMMENT "Generating documentation")

  ADD_CUSTOM_TARGET( docs DEPENDS ${documentation} )

  INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs DESTINATION share/ElmerModuleDriver )
ENDIF()

INSTALL(TARGETS SolverModuleDriver ElmerModuleDriver sep RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
IF(MPI_CXX_COMPILER)
  INSTALL(TARGETS pepi RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ENDIF()
# ========= USE IN EXISTING PROJECT ==============

ADD_SUBDIRECTORY (testing)
