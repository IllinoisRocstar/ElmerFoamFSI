///
/// @file
/// @ingroup elmermoduledriver_group
/// @brief Testing object for OpenFoamModuleDriver.
///
/// This file contains the implementation of the
/// overarching testing object for the project.
/// Every project should have one of these as
/// it allows external entities unified methodology
/// for access project or package-specific tests.
///
#ifndef __OPENFOAMMODULEDRIVER_TEST_H__
#define __OPENFOAMMODULEDRIVER_TEST_H__
#ifdef _OPENFOAMMODULEDRIVER_PARALLEL_
#include "COMM.H"
#endif
#include "Testing.H"
#include "Profiler.H"
#include <vector>
#include <cmath>
#include "com.h"
#include "com_devel.hpp"
#include "UnixUtils.H"

COM_EXTERN_MODULE(OpenFoamFSI);
COM_EXTERN_MODULE(OpenFoamFSIPar);

namespace OpenFoamModuleDriver {

  ///
  /// Namespace for storing simple test utility fixtures
  ///
  namespace TestFixture {
    ///
    /// Simple test function fixture returns \f$2x\f$.
    /// 
    /// F1 is a 1st order function in x that has unity integral over [0,1].
    ///
    double F1(double x) { return (2.0*x); };
    ///
    /// Simple test function fixture returns \f$3x^2\f$.
    ///
    /// F2 is quadratic function in x and integrates to unity over [0,1].
    ///
    double F2(double x) { return (3.0*x*x); };
  };
  ///
  /// Project-specific test results type.
  ///
  /// Test results types can be custom implemented
  /// by the project developer.  The requirement
  /// is that they produce the proper test results
  /// output when used with outstreams.
  ///
  typedef IRAD::Util::TestResults TestResults;

  /// 
  /// Project-specific testing object.
  ///
  /// The project-specific testing object encapsulates
  /// all tests for the project. This is the interface
  /// to all testing available for the given project.
  /// It implements the interface given by 
  /// IRAD::Util::TestingObject.
  ///
  /// Every project should define a "TestingObject"
  /// which implements (or interfaces) all of the tests
  /// for the project.
  /// If necessary (e.g. for parallel tests through
  /// batch systems), this is the object responsible
  /// for spawning off children processes to conduct
  /// parallel tests and for gathering the results
  /// of those tests.
  ///
  template<typename ResultsType>
  class TestingObject : public IRAD::Util::TestingObject<ResultsType>
  {
    typedef IRAD::Util::TestingObject<ResultsType> TestingObjectBaseType;
  protected:
    // Example test fixtures for OpenFoamModuleDriver testing.
    //
    // In a real project, there would be many test fixtures
    // in the TestingObject.
    ///
    /// A sample string test fixture. 
    ///
    std::string ExampleTestFixture;
    ///
    /// A set of values for testing quadrature methods.
    /// 
    std::vector<int> N;
    ///
    /// The string for the testing source directory.
    ///
    std::string SourceDirPath;
  public:
    /// 
    /// Default constructor.
    ///
    TestingObject() : TestingObjectBaseType() {};
    
    ///
    /// Sets the string value of the testing source directory.
    /// 
    void SetSourceDirPath(std::string input){
      SourceDirPath = input;
    }


    ///
    /// Tears down the testing fixtures if needed.
    ///
    virtual void Epilogue() {};

    ///
    /// Sets up the data fixtures for the tests.
    /// 
    /// This function gives the object a chance to 
    /// allocate and set up any "fixtures" before the
    /// tests are run.
    ///
    virtual void  Prologue(){
      ExampleTestFixture.assign("ExampleTestData");
      for(int i = 10;i < 10000000;i*=10) N.push_back(i);
    }

    ///
    /// Simple test function fixture returns \f$2x\f$.
    /// 
    /// F1 is a 1st order function in x that has unity integral over [0,1].
    ///
    double F1(double x) { return (2.0*x); };
    ///
    /// Simple test function fixture returns \f$3x^2\f$.
    ///
    /// F2 is quadratic function in x and integrates to unity over [0,1].
    ///
    double F2(double x) { return (3.0*x*x); };

    /// Function to copy input data from source testing directory
    /// for use in unit tests.
    /// 
    /// @param sourceDir std::string directory containing the input data 
    /// @param destDir std::string destination directory for the data
    virtual int GetInputData(std::string sourceDir, std::string destDir){

      //std::string DirName;
      std::string OrgDir;

      //Save original working directory for later
      OrgDir = IRAD::Sys::CWD();

      //Check if data directory exists
      if(!IRAD::Sys::FILEEXISTS(sourceDir)){
        std::cout << "GetInputData Error: Could not find data directory, "
                  << sourceDir << ". Exiting (fail)." << std::endl;
        return -1;
      }

      std::ifstream Inf;
      std::ofstream Ouf;
      std::string InFileName;
      std::string OutFileName;
      int IntDir;

      //Create destDir directory in current directory (if it doesn't
      //already exist) to run our tests in.
      if(!IRAD::Sys::FILEEXISTS(destDir)){
        IntDir = IRAD::Sys::CreateDirectory(destDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not make directory, "
                    << destDir << ". Exiting (fail)." << std::endl;
          return -1;
        }
      }

      //Open the input test data directory
      IRAD::Sys::Directory SourceDir(sourceDir);
     
      //Change directories to destDir directory for running 
      IntDir = IRAD::Sys::ChDir(destDir);
      if(IntDir == -1){
        std::cout << "GetInputData Error: Could not change directories to "
                  << destDir << ". Exiting (fail)." << std::endl;
        return -1;
      }

      //Copy input data to destDir directory for running 
      for(std::vector<std::string>::iterator it = SourceDir.begin();
          it != SourceDir.end(); ++it){ 
        InFileName = sourceDir + "/" + *it;
        OutFileName = IRAD::Sys::CWD() + "/" + *it;

        if(IRAD::Sys::ISLINK(InFileName))
          // preserve links, don't traverse
          int IntDir = IRAD::Sys::SymLink(InFileName, OutFileName);
        else if(IRAD::Sys::ISDIR(InFileName)) {
          // recursively copy directories
          int IntDir = GetInputData(InFileName, OutFileName);
        } else {
          // copy files, travserse into directories recursively
          Inf.open(InFileName.c_str());
          Ouf.open(OutFileName.c_str());
          Ouf << Inf.rdbuf();
          Ouf.close();
          Inf.close();
        }
      }    

      //Change directories to back to original directory 
      IntDir = IRAD::Sys::ChDir(OrgDir);
      if(IntDir == -1){
        std::cout << "GetInputData Error: Could not change directories to "
                  << OrgDir << ". Exiting (fail)." << std::endl;
        return -1;
      }

      return 0;
    }
 

    ///*****************************************************************************
    /// Test for Loading and Unloading OpenFoamModule
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ModuleLoadUnload(ResultsType &result){
      std::cout << "Running Test__ModuleLoad" << std::endl; 
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");
      bool works = false;

      // Get Window handle
      int h=COM_get_window_handle("OFModule");
      if(h <= 0){
        std::cout << " After loading, COM_get_window_handle(\"OFModule\") returns "
                  << h << std::endl;
        std::cout << "ModuleLoadUnload Error: Could not get window handle."
                  << std::endl;
      }
      else
        works=true;

      result.UpdateResult("LoadSolverModule:Works",works);
      if (!works) 
        return;

      //Unload OpenFoam module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");
      works=false;

      //Make sure module unloaded
      h=COM_get_window_handle("OFModule");
      if(h > 0){
        std::cout << "After unloading, COM_get_window_handle(\"OFModule\") returns "
                  << h << std::endl;
        std::cout << "ModuleLoadUnload Error: Did not properly unload module."
                  << std::endl;
      }
      else
        works=true;

      result.UpdateResult("UnloadSolverModule:Works",works);
      return;

    }

    ///*****************************************************************************
    /// Function to test the copy of Unit Test Data to a new directory
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__LoadUnitTestData(ResultsType &result){
      std::cout << "Running Test__LoadUnitTestData" << std::endl; 
 
      bool works=false;
      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsi";
      std::string dirName = SourceDirPath + suffix;

      //Make path for directory to run the test in.
      std::string TestDir = IRAD::Sys::CWD() + "/UnitTestData";
      int IntDir = GetInputData(dirName, TestDir);

      //Check if GetInputData exited correctly.
      if(IntDir != 0){
        std::cout << "LoadUnitTestData Error: GetInputData call failed."
                  << std::endl;
        result.UpdateResult("LoadUnitTestData:Works", false);
        return;
      }

      //Check if TestDir directory (from GetInputData) exists in current directory
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        std::cout << "LoadUnitTestData Error: Could not find directory, "
                  << TestDir << ", for running tests." << std::endl;
        result.UpdateResult("LoadUnitTestData:Works", false);
        return;
      }

      //Save original directory name for later
      std::string OrgDir = IRAD::Sys::CWD();

      //Change directories to TestDir directory for running 
      IntDir = IRAD::Sys::ChDir(TestDir);
      if(IntDir == -1){
        std::cout << "LoadUnitTestData Error: Could not change directories to "
                  << TestDir << "." << std::endl;
        result.UpdateResult("LoadUnitTestData:Works", false);
        return;
      }
      result.UpdateResult("LoadUnitTestData:Works", true);

      IntDir = IRAD::Sys::ChDir(OrgDir);

      return;
    }

    ///*****************************************************************************
    /// Function to test the Initialize function for the OpenFoamModule
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ModuleFunctionInitialize(ResultsType &result){
      std::cout << "Running Test__ModuleFunctionInitialize" << std::endl; 
 
      //load OpenFoam module
      std::cout << "Before loading OFModule" << std::endl;
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");
      std::cout << "After loading OFModule" << std::endl;
      bool works = false;

      // Test Initalize function exists
      int initHandle = COM_get_function_handle("OFModule.InitFoam");
      if(initHandle > 0)
        works=true;
      else {
        std::cout << "ModuleFunctionInitialize Error: Could not get function handle."
                  << std::endl;
      }
      result.UpdateResult("InitFunction:Exists",works);
      if (!works) 
        return;

      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsi";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataHTFsi";
      int IntDir = GetInputData(dirName, testDir);
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // Test proper operation of init
      // For now, just test that the function returned 0 after running
      // We test the "correctness" of the registered data in subsequent tests
      int dummy1=1;
      char *dummy2[2];
      int verb=3;
      dummy2[0] = const_cast<char *>(std::string("functionInit").c_str());
      dummy2[1] = NULL;
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // check the status variable to determine success of initialize
      int* initStatus=NULL;
      int status = COM_get_status("OFModule.initStatus",101);
      if(status <= 0) {
        std::cout << "ModuleFunctionInitialize Error: initStatus not a valid DataItem."
                  << "status = " << status << std::endl;
        return;
      }

      COM_get_array("OFModule.initStatus", 101, &initStatus);

      works=false;
      if(*initStatus == 0)
        works=true;
      else {
        std::cout << "ModuleFunctionInitialize Error: initStatus not set to zero by InitFoam."
                  << std::endl;
      }
      result.UpdateResult("InitFunction:Runs",works);

      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      return;
    }

    ///*****************************************************************************
    /// Function to test the registration of surface data for the OpenFoamModule 
    ///
    /// checks to make sure that pressure and traction data is registered on the
    /// surface mesh and that they update as expected after a step
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ModuleSurfaceData(ResultsType &result){
      std::cout << "Running Test__ModuleSurfaceData" << std::endl; 
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsi";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataSurfaceData";
      int IntDir = GetInputData(dirName, testDir);
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // call init to setup the mesh
      int dummy1=1;
      char *dummy2[2];
      dummy2[0] = const_cast<char *>(std::string("DataReg").c_str());
      dummy2[1] = NULL;
      int verb=3;
      int initHandle = COM_get_function_handle("OFModule.InitFoam");
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // list of panes in this window
      int numPanes;
      int* paneList;
      COM_get_panes("OFModule", &numPanes, &paneList);

      // only one pane for serial runs
      int pane = paneList[0];

      // check that the pressure is registered
      int status = COM_get_status("OFModule.pressure", pane);
      if(status <= 0) {
        std::cout << "ModuleSurfaceData Error: pressure not a valid DataItem. "
                  << "status = " << status << std::endl;
        result.UpdateResult("PressureData:Registered", false);
        return;
      } else {
        result.UpdateResult("PressureData:Registered", true);
      }

      // get some information about how the item was registered
      char getDataItemLoc;
      COM_Type getDataItemType;
      int getDataItemComponents;
      std::string getDataItemUnits;
      COM_get_dataitem("OFModule.pressure", &getDataItemLoc, &getDataItemType, 
                   &getDataItemComponents, &getDataItemUnits);

      double* pressure=NULL;
      COM_get_array("OFModule.pressure", pane, &pressure);

      // check the array size against what we expect
      // pressures are scalars stored at the cell centers
      // assumes a single connectivity for this pane

      bool pressureCorrect = false;
      int numPres = 0;
      int numPresExpected = 168;
      int numPresCompExpected = 1;
      COM_get_size("OFModule.pressure", pane, &numPres);

      if (numPres != numPresExpected || numPresCompExpected != getDataItemComponents) {
        std::cout << "Wrong number of pressures registered for this mesh." << std::endl;
        std::cout << "Got " << numPres << " Items, Expected: " << numPresExpected << std::endl;
        std::cout << "Got " << getDataItemComponents << " Components, Expected: "
                  << numPresCompExpected << std::endl;
      } else {
        pressureCorrect = true;
      }


      // traction
      status = COM_get_status("OFModule.traction", pane);
      if(status <= 0) {
        std::cout << "ModuleSurfaceData Error: traction not a valid DataItem."
                  << "status = " << status << std::endl;
        result.UpdateResult("TractionData:Registered", false);
        return;
      } else {
        result.UpdateResult("TractionData:Registered", true);
      }

      COM_get_dataitem("OFModule.traction", &getDataItemLoc, &getDataItemType, 
                   &getDataItemComponents, &getDataItemUnits);

      double* traction=NULL;
      COM_get_array("OFModule.traction", pane, &traction);

      // check the array size against what we expect
      // tractions are 3-dimensional vectors stored at the cell centers

      bool tractionCorrect = false;
      int numTrac = 0;
      int numTracExpected = 168;
      int numTracCompExpected = 3;
      COM_get_size("OFModule.traction", pane, &numTrac);

      if (numTrac != numTracExpected || numTracCompExpected != getDataItemComponents) {
        std::cout << "Wrong number of tractions registered for this mesh." << std::endl;
        std::cout << "Got " << numTrac << " Items, Expected: " << numTracExpected << std::endl;
        std::cout << "Got " << getDataItemComponents << " Components, Expected: "
                  << numTracCompExpected << std::endl;
      } else {
        tractionCorrect = true;
      }

      // now step the solution and compare the pressure and traction vs what
      // we expect from the sample problem
      int stepHandle = COM_get_function_handle("OFModule.StepFluid");
      COM_call_function(stepHandle);

      bool works=false;
      //testTime = 1.e-3;
      //toler = 1.e-9;
      if(0)
        works=true;
      else {
        std::cout << "ModuleSurfaceData Error: pressure incorrect "
                  << std::endl;
      }
      result.UpdateResult("PressureData:Correct", pressureCorrect);
      result.UpdateResult("TractionData:Correct", tractionCorrect);

      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      //Unload OpenFoam module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      return;
    }

    ///*****************************************************************************
    /// Function to test the time Step function for the OpenFoamModule 
    /// StepFluid Function
    ///
    /// Loads an openfoam simulation and takes a single step before returning 
    /// control to the driver
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ModuleFunctionStepFluid(ResultsType &result){
      std::cout << "Running Test__ModuleFunctionStepFluid" << std::endl; 
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      bool works = false;


      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsi";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataStepTest";
      int IntDir = GetInputData(dirName, testDir);
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // call init to setup the mesh
      int dummy1=1;
      char *dummy2[2];
      dummy2[0] = const_cast<char *>(std::string("StepFluid").c_str());
      dummy2[1] = NULL;
      int verb=3;
      int initHandle = COM_get_function_handle("OFModule.InitFoam");
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // list of panes in this window
      int numPanes;
      int* paneList;
      COM_get_panes("OFModule", &numPanes, &paneList);

      // only one pane for serial runs
      int pane = paneList[0];

      // check that the current simulation time and endTime are registered
      int status = COM_get_status("OFModule.time", pane);
      bool timeStatus = false;
      if(status <= 0) {
        std::cout << "ModuleFunctionStepFluid Error: time not a valid DataItem."
                  << "status = " << status << std::endl;
        result.UpdateResult("TimeData:Registered", false);
        return;
      } else {
        timeStatus = true;
      }

      status = COM_get_status("OFModule.endTime", pane);
      if(status <= 0) {
        std::cout << "ModuleFunctionStepFluid Error: endTime not a valid DataItem."
                  << "status = " << status << std::endl;
        result.UpdateResult("TimeData:Registered", false);
        return;
      } else {
        timeStatus = timeStatus && true;
      }
      result.UpdateResult("TimeData:Registered", timeStatus);

      double* time=NULL;
      COM_get_array("OFModule.time", pane, &time);
      double* endTime=NULL;
      COM_get_array("OFModule.endTime", pane, &endTime);

      works=false;
      double testTime = 0.;
      double testEndTime = 5.e-3;
      double toler = 1.e-9;
      if(*time - testTime < toler && *endTime - testEndTime < toler)
        works=true;
      else {
        std::cout << "ModuleFunctionStepFluid Error: time or endTime initialized incorrectly"
                  << std::endl;
        std::cout << " time = " << *time << " expected " << testTime << std::endl;
        std::cout << " endTime = " << *endTime << " expected " << testEndTime << std::endl;
      }
      result.UpdateResult("TimeData:Correct",works);

      // Test StepFluid function exists
      works = false;
      int stepHandle = COM_get_function_handle("OFModule.StepFluid");
      if(stepHandle > 0)
        works=true;
      else {
        std::cout << "ModuleFunctionStepFluid Error: Could not get function handle."
                  << std::endl;
        result.UpdateResult("StepFluidFunction:Exists",false);
        return;
      }
      result.UpdateResult("StepFluidFunction:Exists",works);

      // Test StepFluid function steps
      // check that the time got incremented as expected
      COM_call_function(stepHandle);


      works=false;
      testTime = 1.e-3;
      toler = 1.e-9;
      if(*time - testTime < toler )
        works=true;
      else {
        std::cout << "ModuleFunctionStepFluid Error: time did not increment"
                  << std::endl;
        std::cout << " time = " << *time << "expected " << testTime << std::endl;
      }
      result.UpdateResult("StepFluidFunction:Steps",works);

      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      //Unload OpenFoam module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      return;
    }

    ///*****************************************************************************
    /// Function to test the surface mesh registration for the OpenFoamModule
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ModuleMeshRegistration(ResultsType &result){
      std::cout << "Running Test__ModuleMeshRegistration" << std::endl; 
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      // Test Initalize function exists
      int initHandle = COM_get_function_handle("OFModule.InitFoam");

      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsi";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataMeshTest";
      int IntDir = GetInputData(dirName, testDir);
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // call init to setup the mesh
      int dummy1=1;
      char *dummy2[2];
      dummy2[0] = const_cast<char *>(std::string("meshReg").c_str());
      dummy2[1] = NULL;
      int verb=3;
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // get information about what was registered in this window
      int numDataItems=0;
      std::string output;
      COM_get_dataitems("OFModule", &numDataItems, output);
      std::istringstream Istr(output);
      std::vector<std::string> dataItemNames;
    
      for (int i=0; i<numDataItems; ++i) {
        std::string name;
        Istr >> name;
        dataItemNames.push_back(name);
        std::cout << "DataItem # " << i << ": " << name << std::endl;
      }

      // list of panes in this window
      int numPanes;
      int* paneList;
      COM_get_panes("OFModule", &numPanes, &paneList);
      std::cout << "Number of Panes " << numPanes << std::endl;
      for (int i=0; i<numPanes; ++i) 
        std::cout << "Pane ID # " << i+1 << "=" << paneList[i] << std::endl;

      // only one pane for serial runs
      int pane = paneList[0];

      //////////////////////////////////////////////////
      // Check coordinate values
      //////////////////////////////////////////////////
      bool coordCorrect = false;
      double* Coord;
      COM_get_array("OFModule.nc", pane, &Coord);
      if (Coord == NULL) {
        std::cout << "In Test__ModuleMeshRegistration:  No mesh coordinates registered."
                  << std::endl;
        return; 
      } else {
        result.UpdateResult("CoordinateData:Registered", true);
      } 

      // check for expected number of nodes
      int numNodes = 0;
      int numNodesExpected = 338;
      COM_get_size("OFModule.nc", pane, &numNodes);

      if (numNodes != numNodesExpected) {
        std::cout << "Wrong number of nodes registered for this mesh." << std::endl;
        std::cout << "Got " << numNodes << " Expected: " << numNodesExpected << std::endl;
      } else {
        coordCorrect = true;
      }

      // Arrays eith old data for check 
      // move to auxilary function to clean up...
      //std::vector< std::vector<double> > CoordCheck;
      double CoordCheck[338][3] =
      //CoordCheck.push_back(std::vector<double> tmp(
        { {0.299528, 0.21, -0.025334},
        {0.299528, 0.21, 0.025334},
        {0.295179, 0.21, 0.025334},
        {0.295179, 0.21, -0.025334},
        {0.291124, 0.21, 0.025334},
        {0.291124, 0.21, -0.025334},
        {0.287343, 0.21, 0.025334},
        {0.287343, 0.21, -0.025334},
        {0.283818, 0.21, 0.025334},
        {0.283818, 0.21, -0.025334},
        {0.280531, 0.21, 0.025334},
        {0.280531, 0.21, -0.025334},
        {0.277466, 0.21, 0.025334},
        {0.277466, 0.21, -0.025334},
        {0.274609, 0.21, 0.025334},
        {0.274609, 0.21, -0.025334},
        {0.271945, 0.21, 0.025334},
        {0.271945, 0.21, -0.025334},
        {0.26946, 0.21, 0.025334},
        {0.26946, 0.21, -0.025334},
        {0.267144, 0.21, 0.025334},
        {0.267144, 0.21, -0.025334},
        {0.264984, 0.21, 0.025334},
        {0.264984, 0.21, -0.025334},
        {0.262971, 0.21, 0.025334},
        {0.262971, 0.21, -0.025334},
        {0.261093, 0.21, 0.025334},
        {0.261093, 0.21, -0.025334},
        {0.259343, 0.21, 0.025334},
        {0.259343, 0.21, -0.025334},
        {0.25771, 0.21, 0.025334},
        {0.25771, 0.21, -0.025334},
        {0.256188, 0.21, 0.025334},
        {0.256188, 0.21, -0.025334},
        {0.254769, 0.21, 0.025334},
        {0.254769, 0.21, -0.025334},
        {0.253446, 0.21, 0.025334},
        {0.253446, 0.21, -0.025334},
        {0.252213, 0.21, 0.025334},
        {0.252213, 0.21, -0.025334},
        {0.251062, 0.21, 0.025334},
        {0.251062, 0.21, -0.025334},
        {0.24999, 0.21, 0.025334},
        {0.24999, 0.21, -0.025334},
        {0.24899, 0.21, 0.025334},
        {0.24899, 0.21, -0.025334},
        {0.295179, 0.19, 0.025334},
        {0.299528, 0.19, 0.025334},
        {0.299528, 0.19, -0.025334},
        {0.295179, 0.19, -0.025334},
        {0.291124, 0.19, 0.025334},
        {0.291124, 0.19, -0.025334},
        {0.287343, 0.19, 0.025334},
        {0.287343, 0.19, -0.025334},
        {0.283818, 0.19, 0.025334},
        {0.283818, 0.19, -0.025334},
        {0.280531, 0.19, 0.025334},
        {0.280531, 0.19, -0.025334},
        {0.277466, 0.19, 0.025334},
        {0.277466, 0.19, -0.025334},
        {0.274609, 0.19, 0.025334},
        {0.274609, 0.19, -0.025334},
        {0.271945, 0.19, 0.025334},
        {0.271945, 0.19, -0.025334},
        {0.26946, 0.19, 0.025334},
        {0.26946, 0.19, -0.025334},
        {0.267144, 0.19, 0.025334},
        {0.267144, 0.19, -0.025334},
        {0.264984, 0.19, 0.025334},
        {0.264984, 0.19, -0.025334},
        {0.262971, 0.19, 0.025334},
        {0.262971, 0.19, -0.025334},
        {0.261093, 0.19, 0.025334},
        {0.261093, 0.19, -0.025334},
        {0.259343, 0.19, 0.025334},
        {0.259343, 0.19, -0.025334},
        {0.25771, 0.19, 0.025334},
        {0.25771, 0.19, -0.025334},
        {0.256188, 0.19, 0.025334},
        {0.256188, 0.19, -0.025334},
        {0.254769, 0.19, 0.025334},
        {0.254769, 0.19, -0.025334},
        {0.253446, 0.19, 0.025334},
        {0.253446, 0.19, -0.025334},
        {0.252213, 0.19, 0.025334},
        {0.252213, 0.19, -0.025334},
        {0.251062, 0.19, 0.025334},
        {0.251062, 0.19, -0.025334},
        {0.24999, 0.19, 0.025334},
        {0.24999, 0.19, -0.025334},
        {0.24899, 0.19, -0.025334},
        {0.24899, 0.19, 0.025334},
        {0.6, 0.19, -0.025334},
        {0.6, 0.19, 0.025334},
        {0.6, 0.195, 0.025334},
        {0.6, 0.195, -0.025334},
        {0.6, 0.2, 0.025334},
        {0.6, 0.2, -0.025334},
        {0.6, 0.205, 0.025334},
        {0.6, 0.205, -0.025334},
        {0.6, 0.21, 0.025334},
        {0.6, 0.21, -0.025334},
        {0.594992, 0.21, 0.025334},
        {0.594992, 0.21, -0.025334},
        {0.589984, 0.21, 0.025334},
        {0.589984, 0.21, -0.025334},
        {0.584976, 0.21, 0.025334},
        {0.584976, 0.21, -0.025334},
        {0.579969, 0.21, 0.025334},
        {0.579969, 0.21, -0.025334},
        {0.574961, 0.21, 0.025334},
        {0.574961, 0.21, -0.025334},
        {0.569953, 0.21, 0.025334},
        {0.569953, 0.21, -0.025334},
        {0.564945, 0.21, 0.025334},
        {0.564945, 0.21, -0.025334},
        {0.559937, 0.21, 0.025334},
        {0.559937, 0.21, -0.025334},
        {0.554929, 0.21, 0.025334},
        {0.554929, 0.21, -0.025334},
        {0.549921, 0.21, 0.025334},
        {0.549921, 0.21, -0.025334},
        {0.544913, 0.21, 0.025334},
        {0.544913, 0.21, -0.025334},
        {0.539906, 0.21, 0.025334},
        {0.539906, 0.21, -0.025334},
        {0.534898, 0.21, 0.025334},
        {0.534898, 0.21, -0.025334},
        {0.52989, 0.21, 0.025334},
        {0.52989, 0.21, -0.025334},
        {0.524882, 0.21, 0.025334},
        {0.524882, 0.21, -0.025334},
        {0.519874, 0.21, 0.025334},
        {0.519874, 0.21, -0.025334},
        {0.514866, 0.21, 0.025334},
        {0.514866, 0.21, -0.025334},
        {0.509858, 0.21, 0.025334},
        {0.509858, 0.21, -0.025334},
        {0.50485, 0.21, 0.025334},
        {0.50485, 0.21, -0.025334},
        {0.499843, 0.21, 0.025334},
        {0.499843, 0.21, -0.025334},
        {0.494835, 0.21, 0.025334},
        {0.494835, 0.21, -0.025334},
        {0.489827, 0.21, 0.025334},
        {0.489827, 0.21, -0.025334},
        {0.484819, 0.21, 0.025334},
        {0.484819, 0.21, -0.025334},
        {0.479811, 0.21, 0.025334},
        {0.479811, 0.21, -0.025334},
        {0.474803, 0.21, 0.025334},
        {0.474803, 0.21, -0.025334},
        {0.469795, 0.21, 0.025334},
        {0.469795, 0.21, -0.025334},
        {0.464787, 0.21, 0.025334},
        {0.464787, 0.21, -0.025334},
        {0.45978, 0.21, 0.025334},
        {0.45978, 0.21, -0.025334},
        {0.454772, 0.21, 0.025334},
        {0.454772, 0.21, -0.025334},
        {0.449764, 0.21, 0.025334},
        {0.449764, 0.21, -0.025334},
        {0.444756, 0.21, 0.025334},
        {0.444756, 0.21, -0.025334},
        {0.439748, 0.21, 0.025334},
        {0.439748, 0.21, -0.025334},
        {0.43474, 0.21, 0.025334},
        {0.43474, 0.21, -0.025334},
        {0.429732, 0.21, 0.025334},
        {0.429732, 0.21, -0.025334},
        {0.424724, 0.21, 0.025334},
        {0.424724, 0.21, -0.025334},
        {0.419717, 0.21, 0.025334},
        {0.419717, 0.21, -0.025334},
        {0.414709, 0.21, 0.025334},
        {0.414709, 0.21, -0.025334},
        {0.409701, 0.21, 0.025334},
        {0.409701, 0.21, -0.025334},
        {0.404693, 0.21, 0.025334},
        {0.404693, 0.21, -0.025334},
        {0.399685, 0.21, 0.025334},
        {0.399685, 0.21, -0.025334},
        {0.394677, 0.21, 0.025334},
        {0.394677, 0.21, -0.025334},
        {0.389669, 0.21, 0.025334},
        {0.389669, 0.21, -0.025334},
        {0.384661, 0.21, 0.025334},
        {0.384661, 0.21, -0.025334},
        {0.379654, 0.21, 0.025334},
        {0.379654, 0.21, -0.025334},
        {0.374646, 0.21, 0.025334},
        {0.374646, 0.21, -0.025334},
        {0.369638, 0.21, 0.025334},
        {0.369638, 0.21, -0.025334},
        {0.36463, 0.21, 0.025334},
        {0.36463, 0.21, -0.025334},
        {0.359622, 0.21, 0.025334},
        {0.359622, 0.21, -0.025334},
        {0.354614, 0.21, 0.025334},
        {0.354614, 0.21, -0.025334},
        {0.349606, 0.21, 0.025334},
        {0.349606, 0.21, -0.025334},
        {0.344599, 0.21, 0.025334},
        {0.344599, 0.21, -0.025334},
        {0.339591, 0.21, 0.025334},
        {0.339591, 0.21, -0.025334},
        {0.334583, 0.21, 0.025334},
        {0.334583, 0.21, -0.025334},
        {0.329575, 0.21, 0.025334},
        {0.329575, 0.21, -0.025334},
        {0.324567, 0.21, 0.025334},
        {0.324567, 0.21, -0.025334},
        {0.319559, 0.21, 0.025334},
        {0.319559, 0.21, -0.025334},
        {0.314551, 0.21, 0.025334},
        {0.314551, 0.21, -0.025334},
        {0.309543, 0.21, 0.025334},
        {0.309543, 0.21, -0.025334},
        {0.304536, 0.21, 0.025334},
        {0.304536, 0.21, -0.025334},
        {0.304536, 0.19, 0.025334},
        {0.304536, 0.19, -0.025334},
        {0.309543, 0.19, 0.025334},
        {0.309543, 0.19, -0.025334},
        {0.314551, 0.19, 0.025334},
        {0.314551, 0.19, -0.025334},
        {0.319559, 0.19, 0.025334},
        {0.319559, 0.19, -0.025334},
        {0.324567, 0.19, 0.025334},
        {0.324567, 0.19, -0.025334},
        {0.329575, 0.19, 0.025334},
        {0.329575, 0.19, -0.025334},
        {0.334583, 0.19, 0.025334},
        {0.334583, 0.19, -0.025334},
        {0.339591, 0.19, 0.025334},
        {0.339591, 0.19, -0.025334},
        {0.344599, 0.19, 0.025334},
        {0.344599, 0.19, -0.025334},
        {0.349606, 0.19, 0.025334},
        {0.349606, 0.19, -0.025334},
        {0.354614, 0.19, 0.025334},
        {0.354614, 0.19, -0.025334},
        {0.359622, 0.19, 0.025334},
        {0.359622, 0.19, -0.025334},
        {0.36463, 0.19, 0.025334},
        {0.36463, 0.19, -0.025334},
        {0.369638, 0.19, 0.025334},
        {0.369638, 0.19, -0.025334},
        {0.374646, 0.19, 0.025334},
        {0.374646, 0.19, -0.025334},
        {0.379654, 0.19, 0.025334},
        {0.379654, 0.19, -0.025334},
        {0.384661, 0.19, 0.025334},
        {0.384661, 0.19, -0.025334},
        {0.389669, 0.19, 0.025334},
        {0.389669, 0.19, -0.025334},
        {0.394677, 0.19, 0.025334},
        {0.394677, 0.19, -0.025334},
        {0.399685, 0.19, 0.025334},
        {0.399685, 0.19, -0.025334},
        {0.404693, 0.19, 0.025334},
        {0.404693, 0.19, -0.025334},
        {0.409701, 0.19, 0.025334},
        {0.409701, 0.19, -0.025334},
        {0.414709, 0.19, 0.025334},
        {0.414709, 0.19, -0.025334},
        {0.419717, 0.19, 0.025334},
        {0.419717, 0.19, -0.025334},
        {0.424724, 0.19, 0.025334},
        {0.424724, 0.19, -0.025334},
        {0.429732, 0.19, 0.025334},
        {0.429732, 0.19, -0.025334},
        {0.43474, 0.19, 0.025334},
        {0.43474, 0.19, -0.025334},
        {0.439748, 0.19, 0.025334},
        {0.439748, 0.19, -0.025334},
        {0.444756, 0.19, 0.025334},
        {0.444756, 0.19, -0.025334},
        {0.449764, 0.19, 0.025334},
        {0.449764, 0.19, -0.025334},
        {0.454772, 0.19, 0.025334},
        {0.454772, 0.19, -0.025334},
        {0.45978, 0.19, 0.025334},
        {0.45978, 0.19, -0.025334},
        {0.464787, 0.19, 0.025334},
        {0.464787, 0.19, -0.025334},
        {0.469795, 0.19, 0.025334},
        {0.469795, 0.19, -0.025334},
        {0.474803, 0.19, 0.025334},
        {0.474803, 0.19, -0.025334},
        {0.479811, 0.19, 0.025334},
        {0.479811, 0.19, -0.025334},
        {0.484819, 0.19, 0.025334},
        {0.484819, 0.19, -0.025334},
        {0.489827, 0.19, 0.025334},
        {0.489827, 0.19, -0.025334},
        {0.494835, 0.19, 0.025334},
        {0.494835, 0.19, -0.025334},
        {0.499843, 0.19, 0.025334},
        {0.499843, 0.19, -0.025334},
        {0.50485, 0.19, 0.025334},
        {0.50485, 0.19, -0.025334},
        {0.509858, 0.19, 0.025334},
        {0.509858, 0.19, -0.025334},
        {0.514866, 0.19, 0.025334},
        {0.514866, 0.19, -0.025334},
        {0.519874, 0.19, 0.025334},
        {0.519874, 0.19, -0.025334},
        {0.524882, 0.19, 0.025334},
        {0.524882, 0.19, -0.025334},
        {0.52989, 0.19, 0.025334},
        {0.52989, 0.19, -0.025334},
        {0.534898, 0.19, 0.025334},
        {0.534898, 0.19, -0.025334},
        {0.539906, 0.19, 0.025334},
        {0.539906, 0.19, -0.025334},
        {0.544913, 0.19, 0.025334},
        {0.544913, 0.19, -0.025334},
        {0.549921, 0.19, 0.025334},
        {0.549921, 0.19, -0.025334},
        {0.554929, 0.19, 0.025334},
        {0.554929, 0.19, -0.025334},
        {0.559937, 0.19, 0.025334},
        {0.559937, 0.19, -0.025334},
        {0.564945, 0.19, 0.025334},
        {0.564945, 0.19, -0.025334},
        {0.569953, 0.19, 0.025334},
        {0.569953, 0.19, -0.025334},
        {0.574961, 0.19, 0.025334},
        {0.574961, 0.19, -0.025334},
        {0.579969, 0.19, 0.025334},
        {0.579969, 0.19, -0.025334},
        {0.584976, 0.19, 0.025334},
        {0.584976, 0.19, -0.025334},
        {0.589984, 0.19, 0.025334},
        {0.589984, 0.19, -0.025334},
        {0.594992, 0.19, 0.025334},
        {0.594992, 0.19, -0.025334} };

      // check the node positions
      for(int i=0; i < numNodes; i++) {
        for(int j=0; j < 3; j++){
          if(fabs(CoordCheck[i][j] - Coord[i*3+j]) > 1.0e-6){
            coordCorrect = false;
            std::cout << "Incorrect coordinate value for node=" << i 
                      << " index " << j << " Expected " << CoordCheck[i][j] 
                      << " Got " << Coord[i*3+j] << std::endl;
          }
        }
      }
      result.UpdateResult("CoordinateData:Correct", coordCorrect);

      //////////////////////////////////////////////////
      // Check connectivity values
      //////////////////////////////////////////////////
      bool connCorrect = true;

      // get connectivity tables for panes
      int numConn;
      std::string stringNames;
      COM_get_connectivities("OFModule", pane, &numConn, stringNames);
      std::istringstream ConnISS(stringNames);
      std::vector<std::string> connNames;

      for (int i=0; i<numConn; ++i) {
        std::string name;
        ConnISS >> name;
        connNames.push_back(name);
        std::cout << "Connectivity Table # " << i+1 << ": " << name << std::endl;
      }

      // number of nodes per element
      char getDataItemLoc;
      COM_Type getDataItemType;
      int numElementNodes;
      std::string getDataItemUnits;
      std::string fullConnName("OFModule."+connNames[0]);
      COM_get_dataitem(fullConnName, &getDataItemLoc, &getDataItemType, 
                       &numElementNodes, &getDataItemUnits);

      std::cout << "getDataItemLoc " << getDataItemLoc << std::endl;
      std::cout << "getDataItemType " << getDataItemType << std::endl;
      std::cout << "numElementNodes " << numElementNodes << std::endl;
      std::cout << "getDataItemUnits " << getDataItemUnits << std::endl;

      int* Conn;
      COM_get_array(fullConnName.c_str(), pane, &Conn);

      if (Conn == NULL) {
        std::cout << "In Test__ModuleMeshRegistration:  No mesh connectivity registered."
                  << std::endl;
        return; 
      } else {
        result.UpdateResult("ConnectivityData:Registered", true);
      } 

      // assumes a single connectivity for this pane
      int numElem = 0;
      int numElemExpected = 168;
      COM_get_size(fullConnName, pane, &numElem);

      if (numElem != numElemExpected) {
        std::cout << "Wrong number of elements registered for this mesh." << std::endl;
        std::cout << "Got " << numElem << " Expected: " << numElemExpected << std::endl;
      } else {
        connCorrect = true;
      }

      int ConnCheck[168][4] = 
        { {1, 2, 3, 4},
        {3, 5, 6, 4},
        {5, 7, 8, 6},
        {7, 9, 10, 8},
        {9, 11, 12, 10},
        {11, 13, 14, 12},
        {13, 15, 16, 14},
        {15, 17, 18, 16},
        {17, 19, 20, 18},
        {19, 21, 22, 20},
        {21, 23, 24, 22},
        {23, 25, 26, 24},
        {25, 27, 28, 26},
        {27, 29, 30, 28},
        {29, 31, 32, 30},
        {31, 33, 34, 32},
        {33, 35, 36, 34},
        {35, 37, 38, 36},
        {37, 39, 40, 38},
        {39, 41, 42, 40},
        {41, 43, 44, 42},
        {43, 45, 46, 44},
        {47, 48, 49, 50},
        {51, 47, 50, 52},
        {53, 51, 52, 54},
        {55, 53, 54, 56},
        {57, 55, 56, 58},
        {59, 57, 58, 60},
        {61, 59, 60, 62},
        {63, 61, 62, 64},
        {65, 63, 64, 66},
        {67, 65, 66, 68},
        {69, 67, 68, 70},
        {71, 69, 70, 72},
        {73, 71, 72, 74},
        {75, 73, 74, 76},
        {77, 75, 76, 78},
        {79, 77, 78, 80},
        {81, 79, 80, 82},
        {83, 81, 82, 84},
        {85, 83, 84, 86},
        {87, 85, 86, 88},
        {89, 87, 88, 90},
        {91, 92, 89, 90},
        {93, 94, 95, 96},
        {96, 95, 97, 98},
        {98, 97, 99, 100},
        {100, 99, 101, 102},
        {101, 103, 104, 102},
        {103, 105, 106, 104},
        {105, 107, 108, 106},
        {107, 109, 110, 108},
        {109, 111, 112, 110},
        {111, 113, 114, 112},
        {113, 115, 116, 114},
        {115, 117, 118, 116},
        {117, 119, 120, 118},
        {119, 121, 122, 120},
        {121, 123, 124, 122},
        {123, 125, 126, 124},
        {125, 127, 128, 126},
        {127, 129, 130, 128},
        {129, 131, 132, 130},
        {131, 133, 134, 132},
        {133, 135, 136, 134},
        {135, 137, 138, 136},
        {137, 139, 140, 138},
        {139, 141, 142, 140},
        {141, 143, 144, 142},
        {143, 145, 146, 144},
        {145, 147, 148, 146},
        {147, 149, 150, 148},
        {149, 151, 152, 150},
        {151, 153, 154, 152},
        {153, 155, 156, 154},
        {155, 157, 158, 156},
        {157, 159, 160, 158},
        {159, 161, 162, 160},
        {161, 163, 164, 162},
        {163, 165, 166, 164},
        {165, 167, 168, 166},
        {167, 169, 170, 168},
        {169, 171, 172, 170},
        {171, 173, 174, 172},
        {173, 175, 176, 174},
        {175, 177, 178, 176},
        {177, 179, 180, 178},
        {179, 181, 182, 180},
        {181, 183, 184, 182},
        {183, 185, 186, 184},
        {185, 187, 188, 186},
        {187, 189, 190, 188},
        {189, 191, 192, 190},
        {191, 193, 194, 192},
        {193, 195, 196, 194},
        {195, 197, 198, 196},
        {197, 199, 200, 198},
        {199, 201, 202, 200},
        {201, 203, 204, 202},
        {203, 205, 206, 204},
        {205, 207, 208, 206},
        {207, 209, 210, 208},
        {209, 211, 212, 210},
        {211, 213, 214, 212},
        {213, 215, 216, 214},
        {215, 217, 218, 216},
        {217, 219, 220, 218},
        {219, 2, 1, 220},
        {48, 221, 222, 49},
        {221, 223, 224, 222},
        {223, 225, 226, 224},
        {225, 227, 228, 226},
        {227, 229, 230, 228},
        {229, 231, 232, 230},
        {231, 233, 234, 232},
        {233, 235, 236, 234},
        {235, 237, 238, 236},
        {237, 239, 240, 238},
        {239, 241, 242, 240},
        {241, 243, 244, 242},
        {243, 245, 246, 244},
        {245, 247, 248, 246},
        {247, 249, 250, 248},
        {249, 251, 252, 250},
        {251, 253, 254, 252},
        {253, 255, 256, 254},
        {255, 257, 258, 256},
        {257, 259, 260, 258},
        {259, 261, 262, 260},
        {261, 263, 264, 262},
        {263, 265, 266, 264},
        {265, 267, 268, 266},
        {267, 269, 270, 268},
        {269, 271, 272, 270},
        {271, 273, 274, 272},
        {273, 275, 276, 274},
        {275, 277, 278, 276},
        {277, 279, 280, 278},
        {279, 281, 282, 280},
        {281, 283, 284, 282},
        {283, 285, 286, 284},
        {285, 287, 288, 286},
        {287, 289, 290, 288},
        {289, 291, 292, 290},
        {291, 293, 294, 292},
        {293, 295, 296, 294},
        {295, 297, 298, 296},
        {297, 299, 300, 298},
        {299, 301, 302, 300},
        {301, 303, 304, 302},
        {303, 305, 306, 304},
        {305, 307, 308, 306},
        {307, 309, 310, 308},
        {309, 311, 312, 310},
        {311, 313, 314, 312},
        {313, 315, 316, 314},
        {315, 317, 318, 316},
        {317, 319, 320, 318},
        {319, 321, 322, 320},
        {321, 323, 324, 322},
        {323, 325, 326, 324},
        {325, 327, 328, 326},
        {327, 329, 330, 328},
        {329, 331, 332, 330},
        {331, 333, 334, 332},
        {333, 335, 336, 334},
        {335, 337, 338, 336},
        {337, 94, 93, 338} };

      for(int i=0; i < numElem; i++){
        for(int j=0; j < numElementNodes; j++){
          if(fabs(ConnCheck[i][j] - Conn[i*numElementNodes+j]) > 1.0e-6){
            connCorrect = false;
            std::cout << "Incorrect connectivity value at element " << i 
                      << " index " << j << " Expected " << ConnCheck[i][j] 
                      << " Got " << Conn[i*numElementNodes+j] << std::endl;
          }
        }
      }
      result.UpdateResult("ConnectivityData:Correct", connCorrect);


      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      COM_free_buffer(&paneList);
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

      return;
    }

    ///*****************************************************************************
    ///*****************************************************************************


    ///************************************************************************
    /// Functions to call Serial tests
    ///************************************************************************
    ///
    /// Runs all tests implemented by the OpenFoamModuleDriver::TestingObject.
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    virtual void Process(ResultsType &result){
      Prologue();
      Test__ModuleLoadUnload(result);
      Test__LoadUnitTestData(result);
      Test__ModuleFunctionInitialize(result);
      Test__ModuleMeshRegistration(result);
      Test__ModuleSurfaceData(result);
      Test__ModuleFunctionStepFluid(result);
      Epilogue();
    }
    
    ///
    /// Runs a test specified by name
    ///
    /// @param name String name of the test to run.
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    virtual void RunTest(const std::string &name,ResultsType &result)
    {
      Prologue();
      if(name == "ModuleLoadUnload")
        Test__ModuleLoadUnload(result);
      else if(name == "LoadUnitTestData")
        Test__LoadUnitTestData(result);
      else if(name == "ModuleFunctionInitialize")
        Test__ModuleFunctionInitialize(result);
      else if(name == "ModuleMeshRegistration")
        Test__ModuleMeshRegistration(result);
      else if(name == "ModuleSurfaceData")
        Test__ModuleSurfaceData(result);
      else if(name == "ModuleFunctionStepFluid")
        Test__ModuleFunctionStepFluid(result);
      Epilogue();
    }

    ///
    /// Runs a list of tests specified by name
    /// 
    /// @param test_names list of string test names
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    virtual void ProcessTests(std::list<std::string> &test_names,ResultsType &result){
      Prologue();
      std::list<std::string>::iterator tni = test_names.begin();
      while(tni != test_names.end())
        RunTest(*tni++,result);
      Epilogue();
    }


  };

    ///************************************************************************
    /// List of Parallel tests
    ///************************************************************************
    ///
#ifdef _OPENFOAMMODULEDRIVER_PARALLEL_
  /// 
  /// Project-specific parallel testing object.
  ///
  /// The project-specific parallel testing object encapsulates
  /// all parallel tests for the project. This is the interface
  /// to all "in-core" parallel testing available for the given project.
  /// It implements the interface given by 
  /// OpenFoamModuleDriver::TestingObject and has an 
  /// IRAD::Comm::CommunicatorObject for parallel capabilities.
  ///
  template<typename CommType,typename ResultsType>
  class ParallelTestingObject : TestingObject<ResultsType>
  {
  protected:
    // Example test fixtures for parallel OpenFoamModuleDriver testing.
    //
    /// 
    /// The CommunicatorObject for the parallel testing object.
    ///
    CommType _communicator;
    ///
    /// A set of values for testing quadrature methods.
    /// 
    std::vector<int> N;
    ///
    /// The string for the testing source directory.
    ///
    std::string SourceDirPath;
  public:
    ///
    /// Constructor with incoming communicator.
    ///
    ParallelTestingObject(CommType &incomm) :
      TestingObject<ResultsType>(), _communicator(incomm) {};
    CommType &GetCommunicator() { return(_communicator); };

    ///
    /// Sets the string value of the testing source directory.
    /// 
    void SetSourceDirPath(std::string input){
      SourceDirPath = input;
    }

    ///
    /// Tears down the testing fixtures if needed.
    ///
    virtual void Epilogue() {};

    ///
    /// Sets up the data fixtures for the tests.
    /// 
    /// This function gives the object a chance to 
    /// allocate and set up any "fixtures" before the
    /// tests are run.
    ///
    virtual void  Prologue(){
      for(int i = _communicator.Size();i <= 1000000;i*=2) N.push_back(i);
    };

    /// Function to copy input data from source testing directory
    /// for use in unit tests.
    /// 
    /// @param sourceDir std::string directory containing the input data 
    /// @param destDir std::string destination directory for the data
    virtual int GetInputData(std::string sourceDir, std::string destDir){
      
      //std::string DirName;
      std::string OrgDir;

      //Save original working directory for later
      OrgDir = IRAD::Sys::CWD();

      //Check if data directory exists
      if(!IRAD::Sys::FILEEXISTS(sourceDir)){
	std::cout << "GetInputData Error: Could not find data directory, "
		  << sourceDir << ". Exiting (fail)." << std::endl;
	return -1;
      }
  
      std::ifstream Inf;
      std::ofstream Ouf;
      std::string InFileName;
      std::string OutFileName;
      int IntDir;

      //Create destDir directory in current directory (if it doesn't
      //already exist) to run our tests in.
      if(!IRAD::Sys::FILEEXISTS(destDir)){
	IntDir = IRAD::Sys::CreateDirectory(destDir);
	if(IntDir == -1){
	  std::cout << "GetInputData Error: Could not make directory, "
		    << destDir << ". Exiting (fail)." << std::endl;
	  return -1;
	}
      }

      //Open the input test data directory
      IRAD::Sys::Directory SourceDir(sourceDir);
     
      //Change directories to destDir directory for running 
      IntDir = IRAD::Sys::ChDir(destDir);
      if(IntDir == -1){
	std::cout << "GetInputData Error: Could not change directories to "
		  << destDir << ". Exiting (fail)." << std::endl;
	return -1;
      }

      //Copy input data to destDir directory for running 
      for(std::vector<std::string>::iterator it = SourceDir.begin();
	  it != SourceDir.end(); ++it){ 
	InFileName = sourceDir + "/" + *it;
	OutFileName = IRAD::Sys::CWD() + "/" + *it;

	if(IRAD::Sys::ISLINK(InFileName))
	  // preserve links, don't traverse
	  int IntDir = IRAD::Sys::SymLink(InFileName, OutFileName);
	else if(IRAD::Sys::ISDIR(InFileName)) {
	  // recursively copy directories
	  int IntDir = GetInputData(InFileName, OutFileName);
	} else {
	  // copy files, travserse into directories recursively
	  Inf.open(InFileName.c_str());
	  Ouf.open(OutFileName.c_str());
	  Ouf << Inf.rdbuf();
	  Ouf.close();
	  Inf.close();
	}
      }
	  

      //Change directories to back to original directory 
      IntDir = IRAD::Sys::ChDir(OrgDir);
      if(IntDir == -1){
         std::cout << "GetInputData Error: Could not change directories to "
 	           << OrgDir << ". Exiting (fail)." << std::endl;
	return -1;
      }
       

      return 0;
    }


    ///*****************************************************************************
    /// Test for Loading and Unloading OpenFoamModule
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ParModuleLoadUnload(ResultsType &result){
      std::cout << "Running Test__ParModuleLoadUnload" << std::endl; 

      // changing default communicator for COM
      COM_set_default_communicator((GetCommunicator()).GetCommunicator());
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");
      bool works = false;

      // Get Window handle
      int h=COM_get_window_handle("OFModule");
      if(h <= 0){
        std::cout << " After loading, COM_get_window_handle(\"OFModule\") returns "
                  << h << std::endl;
        std::cout << "ParModuleLoadUnload Error: Could not get window handle."
                  << std::endl;
      }
      else
        works=true;

      result.UpdateResult("LoadParSolverModule:Works",works);
      if (!works) 
        return;

      //Unload OpenFoam module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");
      works=false;

      //Make sure module unloaded
      h=COM_get_window_handle("OFModule");
      if(h > 0){
        std::cout << "After unloading, COM_get_window_handle(\"OFModule\") returns "
                  << h << std::endl;
        std::cout << "ParModuleLoadUnload Error: Did not properly unload module."
                  << std::endl;
      }
      else
        works=true;

      result.UpdateResult("UnloadParSolverModule:Works",works);

      return;

    }

    ///*****************************************************************************
    /// Parallel test for OpenFoamModuleDriverPar communicator
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    /// The test first makes sure that the MPI communicator will be passed
    /// to the openFoamModoule properly 
    ///*****************************************************************************
    virtual void Test__ParCommunicatorPassToModule(ResultsType &result) {
      std::cout << "Running Test__ParCommunicatorPassToModule" << std::endl; 
      bool commPass = false;
      // changing default communicator for COM
      COM_set_default_communicator((GetCommunicator()).GetCommunicator());
      // loading openFoamModulePar
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");
      // checking number of processes that module uses
      int* nProcReg;
      int nProc;
      MPI_Comm_size((GetCommunicator()).GetCommunicator(), &nProc);
      COM_get_array("OFModule.nproc", 0, &nProcReg);
      if (*nProcReg == nProc)
         commPass = true;
      // unloading the module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");
      // setting test flag
      result.UpdateResult("CommunicatorPassToModule:Pass",commPass);

    }

   
    ///*****************************************************************************
    /// Function to test the copy of Unit Test Data to a new directory
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ParLoadUnitTestData(ResultsType &result){
      std::cout << "Running Test__ParLoadUnitTestData" << std::endl; 
      
      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsiPar";
      std::string dirName = SourceDirPath + suffix;

      //Make path for directory to run the test in.
      std::string TestDir = IRAD::Sys::CWD() + "/UnitTestDataPar";
      int IntDir = 0;
      if ((GetCommunicator()).Rank() == 0)
         IntDir = GetInputData(dirName, TestDir);

      //creating a barrier to let copy process finish the job
      (GetCommunicator()).Barrier();
     
      //Check if GetInputData exited correctly.
      if(IntDir != 0){
        std::cout << "ParLoadUnitTestData Error: GetInputData call failed."
                  << std::endl;
        result.UpdateResult("ParLoadUnitTestData:Works", false);
        return;
      }

      //Check if TestDir directory (from GetInputData) exists in current directory
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        std::cout << "ParLoadUnitTestData Error: Could not find directory, "
                  << TestDir << ", for running tests." << std::endl;
        result.UpdateResult("ParLoadUnitTestData:Works", false);
        return;
      }

      //Save original directory name for later
      std::string OrgDir = IRAD::Sys::CWD();

      //Change directories to TestDir directory for running 
      IntDir = IRAD::Sys::ChDir(TestDir);
      if(IntDir == -1){
        std::cout << "ParLoadUnitTestData Error: Could not change directories to "
                  << TestDir << "." << std::endl;
        result.UpdateResult("ParLoadUnitTestData:Works", false);
        return;
      }
      result.UpdateResult("ParLoadUnitTestData:Works", true);

      IntDir = IRAD::Sys::ChDir(OrgDir);
      
      return;
    }

    ///*****************************************************************************
    /// Function to test the Initialize function for the OpenFoamModulePar
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ParModuleFunctionInitialize(ResultsType &result){
      std::cout << "Running Test__ParModuleFunctionInitialize" << std::endl; 
      int myRank = (GetCommunicator()).Rank();
 
      //load OpenFoam module
      std::cout << "Before loading OFModule" << std::endl;
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");
      std::cout << "After loading parallel OFModule" << std::endl;
      bool works = false;

      // Test Initalize function exists
      int initHandle = COM_get_function_handle("OFModule.InitFoam");
      if(initHandle > 0)
        works=true;
      else {
        std::cout << "ModuleFunctionInitialize Error: Could not get function handle."
                  << std::endl;
      }
      result.UpdateResult("ParInitFunction:Exists",works);
      if (!works) 
        return;

      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsiPar";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataHTFsiPar";
      int IntDir;
      if (myRank == 0){
         std::cout << "Started copying data..." << std::endl;
	 IntDir = GetInputData(dirName, testDir);
         std::cout << "Finished copying data..." << std::endl;
      }
      // setting a barrier to finish copy step
      (GetCommunicator()).Barrier();
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);
      
      
      // Test proper operation of init
      // For now, just test that the function returned 0 after running
      // We test the "correctness" of the registered data in subsequent tests
      int dummy1=2;
      char *dummy2[2];
      int verb=3;
      dummy2[0] = const_cast<char *>("functionInit");
      dummy2[1] = const_cast<char *>("-parallel");
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // check the status variable to determine success of initialize
      int* initStatus=NULL;
      int status = COM_get_status("OFModule.initStatus",100+myRank);
      if(status <= 0) {
        std::cout << "ModuleFunctionInitialize Error: initStatus not a valid DataItem."
                  << "status = " << status << std::endl;
        return;
      }

      COM_get_array("OFModule.initStatus", 100 + myRank, &initStatus);

      works=false;
      if(*initStatus == 0)
        works=true;
      else {
        std::cout << "ModuleFunctionInitialize Error: initStatus not set to zero by InitFoam."
                  << std::endl;
      }
      
      result.UpdateResult("ParInitFunction:Runs",works);
      
      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      return;
    }

    ///*****************************************************************************
    /// Function to test the registration of surface data for the ParOpenFoamModule 
    ///
    /// checks to make sure that pressure and traction data is registered on the
    /// surface mesh and that they update as expected after a step
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ParModuleSurfaceData(ResultsType &result){
      std::cout << "Running Test__ParModuleSurfaceData" << std::endl; 

      //registering the rank for process
      int myRank = GetCommunicator().Rank();

      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsiPar";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataParSurfaceData";
      int IntDir;
      if (myRank == 0)
         IntDir = GetInputData(dirName, testDir);
      (GetCommunicator()).Barrier();
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // call init to setup the mesh
      int dummy1=2;
      char *dummy2[2];
      dummy2[0] = const_cast<char *>("DataReg");
      dummy2[1] = const_cast<char *>("-parallel");
      int verb=3;
      int initHandle = COM_get_function_handle("OFModule.InitFoam");
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);
      std::cout << "Rank #"<< myRank << ", Initialization is finished. "<<std::endl;

      // list of panes in this window
      int numPanes;
      int* paneList;
      COM_get_panes("OFModule", &numPanes, &paneList);

      // assuming only one pane per process
      int pane = paneList[myRank];

      // check that the pressure is registered
      int status = COM_get_status("OFModule.pressure", pane);
      if(status <= 0) {
        std::cout << "ParModuleSurfaceData Error: pressure not a valid DataItem. "
                  << "status = " << status << std::endl;
        result.UpdateResult("ParPressureData:Registered", false);
        return;
      } else {
        result.UpdateResult("ParPressureData:Registered", true);
      }

      // get some information about how the item was registered
      char getDataItemLoc;
      COM_Type getDataItemType;
      int getDataItemComponents;
      std::string getDataItemUnits;
      COM_get_dataitem("OFModule.pressure", &getDataItemLoc, &getDataItemType, 
                   &getDataItemComponents, &getDataItemUnits);

      double* pressure=NULL;
      COM_get_array("OFModule.pressure", pane, &pressure);

      // check the array size against what we expect
      // pressures are scalars stored at the cell centers
      // assumes a single connectivity for this pane

      bool pressureCorrect = false;
      int numPres = 0;
      std::vector<int> numPresExpected;
      numPresExpected.push_back(154);
      numPresExpected.push_back(14);
      int numPresCompExpected = 1;  
      
      COM_get_size("OFModule.pressure", pane, &numPres);
      std::cout<<"Rank #"<<myRank<<", numPres = "<<numPres<<std::endl;     
      if (numPres != numPresExpected[myRank] || numPresCompExpected != getDataItemComponents) {
        std::cout << "Wrong number of pressures registered for this mesh." << std::endl;
        std::cout << "Got " << numPres << " Items, Expected: " << numPresExpected[myRank] << std::endl;
        std::cout << "Got " << getDataItemComponents << " Components, Expected: "
                  << numPresCompExpected << std::endl;
      } else {
        pressureCorrect = true;
      }


      // traction
      status = COM_get_status("OFModule.traction", pane);
      if(status <= 0) {
        std::cout << "ParModuleSurfaceData Error: traction not a valid DataItem."
                  << "status = " << status << std::endl;
        result.UpdateResult("ParTractionData:Registered", false);
        return;
      } else {
        result.UpdateResult("ParTractionData:Registered", true);
      }

      COM_get_dataitem("OFModule.traction", &getDataItemLoc, &getDataItemType, 
                   &getDataItemComponents, &getDataItemUnits);

      double* traction=NULL;
      COM_get_array("OFModule.traction", pane, &traction);

      // check the array size against what we expect
      // tractions are 3-dimensional vectors stored at the cell centers

      bool tractionCorrect = false;
      int numTrac = 0;
      std::vector<int> numTracExpected;
      numTracExpected.push_back(154);
      numTracExpected.push_back(14);
      int numTracCompExpected = 3;
      COM_get_size("OFModule.traction", pane, &numTrac);

      std::cout<<"Rank #"<<myRank<<", numTrac = "<<numTrac<<std::endl;     
      if (numTrac != numTracExpected[myRank] || numTracCompExpected != getDataItemComponents) {
        std::cout << "Wrong number of tractions registered for this mesh." << std::endl;
        std::cout << "Got " << numTrac << " Items, Expected: " << numTracExpected[myRank] << std::endl;
        std::cout << "Got " << getDataItemComponents << " Components, Expected: "
                  << numTracCompExpected << std::endl;
      } else {
        tractionCorrect = true;
      }
      
      result.UpdateResult("ParPressureData:Correct", pressureCorrect);
      result.UpdateResult("ParTractionData:Correct", tractionCorrect);

      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      //Unload OpenFoam module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      return;
    }

    ///*****************************************************************************
    /// Function to test the surface mesh registration for the OpenFoamModule
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ParModuleMeshRegistration(ResultsType &result){
      std::cout << "Running Test__ParModuleMeshRegistration" << std::endl; 
      int myRank = (GetCommunicator()).Rank();
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      // Test Initalize function exists
      int initHandle = COM_get_function_handle("OFModule.InitFoam");

      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsiPar";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataParMeshTest";
      int IntDir;
      if (myRank == 0)
         IntDir = GetInputData(dirName, testDir);
      (GetCommunicator()).Barrier();
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // call init to setup the mesh
      int dummy1=2;
      char *dummy2[2];
      dummy2[0] = const_cast<char *>("meshReg");
      dummy2[1] = const_cast<char *>("-parallel");
      int verb=3;
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // get information about what was registered in this window
      int numDataItems=0;
      std::string output;
      COM_get_dataitems("OFModule", &numDataItems, output);
      std::istringstream Istr(output);
      std::vector<std::string> dataItemNames;
    
      for (int i=0; i<numDataItems; ++i) {
        std::string name;
        Istr >> name;
        dataItemNames.push_back(name);
        std::cout << "Rank #"
                  << myRank << ", DataItem # " << i << ": " << name << std::endl;
      }

      // list of panes in this window
      int numPanes;
      int* paneList;
      COM_get_panes("OFModule", &numPanes, &paneList);
      std::cout << "Rank #"<< myRank << ", Number of Panes " << numPanes << std::endl;
      for (int i=0; i<numPanes; ++i) 
        std::cout << "Pane ID # " << i+1 << "=" << paneList[i] << std::endl;

      // assuming only one pane per process
      int pane = paneList[0];

      //////////////////////////////////////////////////
      // Check coordinate values
      //////////////////////////////////////////////////
      bool coordCorrect = false;
      double* Coord;
      COM_get_array("OFModule.nc", pane, &Coord);
      if (Coord == NULL) {
        std::cout << "In Test__ParModuleMeshRegistration:  No mesh coordinates registered."
                  << std::endl;
        return; 
      } else {
        result.UpdateResult("ParCoordinateData:Registered", true);
      } 

      // check for expected number of nodes
      int numNodes = 0;
      std::vector<int> numNodesExpected;
      numNodesExpected.push_back(312);
      numNodesExpected.push_back(30);
      COM_get_size("OFModule.nc", pane, &numNodes);

      if (numNodes != numNodesExpected[myRank]) {
        std::cout << "Wrong number of nodes registered for this mesh." << std::endl;
        std::cout << "Got " << numNodes << " Expected: " << numNodesExpected[myRank] << std::endl;
      } else {
        coordCorrect = true;
      }

      // Arrays with old data for check 
      // move to auxilary function to clean up...
      double CoordCheck0[312][3] =
      {{0.299528, 0.21, -0.025334},
      {0.299528, 0.21, 0.025334},
      {0.295179, 0.21, 0.025334},
      {0.295179, 0.21, -0.025334},
      {0.291124, 0.21, 0.025334},
      {0.291124, 0.21, -0.025334},
      {0.287343, 0.21, 0.025334},
      {0.287343, 0.21, -0.025334},
      {0.283818, 0.21, 0.025334},
      {0.283818, 0.21, -0.025334},
      {0.280531, 0.21, 0.025334},
      {0.280531, 0.21, -0.025334},
      {0.277466, 0.21, 0.025334},
      {0.277466, 0.21, -0.025334},
      {0.274609, 0.21, 0.025334},
      {0.274609, 0.21, -0.025334},
      {0.271945, 0.21, 0.025334},
      {0.271945, 0.21, -0.025334},
      {0.26946, 0.21, 0.025334},
      {0.26946, 0.21, -0.025334},
      {0.267144, 0.21, 0.025334},
      {0.267144, 0.21, -0.025334},
      {0.264984, 0.21, 0.025334},
      {0.264984, 0.21, -0.025334},
      {0.262971, 0.21, 0.025334},
      {0.262971, 0.21, -0.025334},
      {0.261093, 0.21, 0.025334},
      {0.261093, 0.21, -0.025334},
      {0.259343, 0.21, 0.025334},
      {0.259343, 0.21, -0.025334},
      {0.25771, 0.21, 0.025334},
      {0.25771, 0.21, -0.025334},
      {0.256188, 0.21, 0.025334},
      {0.256188, 0.21, -0.025334},
      {0.254769, 0.21, 0.025334},
      {0.254769, 0.21, -0.025334},
      {0.253446, 0.21, 0.025334},
      {0.253446, 0.21, -0.025334},
      {0.252213, 0.21, 0.025334},
      {0.252213, 0.21, -0.025334},
      {0.251062, 0.21, 0.025334},
      {0.251062, 0.21, -0.025334},
      {0.24999, 0.21, 0.025334},
      {0.24999, 0.21, -0.025334},
      {0.24899, 0.21, 0.025334},
      {0.24899, 0.21, -0.025334},
      {0.295179, 0.19, 0.025334},
      {0.299528, 0.19, 0.025334},
      {0.299528, 0.19, -0.025334},
      {0.295179, 0.19, -0.025334},
      {0.291124, 0.19, 0.025334},
      {0.291124, 0.19, -0.025334},
      {0.287343, 0.19, 0.025334},
      {0.287343, 0.19, -0.025334},
      {0.283818, 0.19, 0.025334},
      {0.283818, 0.19, -0.025334},
      {0.280531, 0.19, 0.025334},
      {0.280531, 0.19, -0.025334},
      {0.277466, 0.19, 0.025334},
      {0.277466, 0.19, -0.025334},
      {0.274609, 0.19, 0.025334},
      {0.274609, 0.19, -0.025334},
      {0.271945, 0.19, 0.025334},
      {0.271945, 0.19, -0.025334},
      {0.26946, 0.19, 0.025334},
      {0.26946, 0.19, -0.025334},
      {0.267144, 0.19, 0.025334},
      {0.267144, 0.19, -0.025334},
      {0.264984, 0.19, 0.025334},
      {0.264984, 0.19, -0.025334},
      {0.262971, 0.19, 0.025334},
      {0.262971, 0.19, -0.025334},
      {0.261093, 0.19, 0.025334},
      {0.261093, 0.19, -0.025334},
      {0.259343, 0.19, 0.025334},
      {0.259343, 0.19, -0.025334},
      {0.25771, 0.19, 0.025334},
      {0.25771, 0.19, -0.025334},
      {0.256188, 0.19, 0.025334},
      {0.256188, 0.19, -0.025334},
      {0.254769, 0.19, 0.025334},
      {0.254769, 0.19, -0.025334},
      {0.253446, 0.19, 0.025334},
      {0.253446, 0.19, -0.025334},
      {0.252213, 0.19, 0.025334},
      {0.252213, 0.19, -0.025334},
      {0.251062, 0.19, 0.025334},
      {0.251062, 0.19, -0.025334},
      {0.24999, 0.19, 0.025334},
      {0.24999, 0.19, -0.025334},
      {0.24899, 0.19, -0.025334},
      {0.24899, 0.19, 0.025334},
      {0.574961, 0.21, 0.025334},
      {0.569953, 0.21, 0.025334},
      {0.569953, 0.21, -0.025334},
      {0.574961, 0.21, -0.025334},
      {0.564945, 0.21, 0.025334},
      {0.564945, 0.21, -0.025334},
      {0.559937, 0.21, 0.025334},
      {0.559937, 0.21, -0.025334},
      {0.554929, 0.21, 0.025334},
      {0.554929, 0.21, -0.025334},
      {0.549921, 0.21, 0.025334},
      {0.549921, 0.21, -0.025334},
      {0.544913, 0.21, 0.025334},
      {0.544913, 0.21, -0.025334},
      {0.539906, 0.21, 0.025334},
      {0.539906, 0.21, -0.025334},
      {0.534898, 0.21, 0.025334},
      {0.534898, 0.21, -0.025334},
      {0.52989, 0.21, 0.025334},
      {0.52989, 0.21, -0.025334},
      {0.524882, 0.21, 0.025334},
      {0.524882, 0.21, -0.025334},
      {0.519874, 0.21, 0.025334},
      {0.519874, 0.21, -0.025334},
      {0.514866, 0.21, 0.025334},
      {0.514866, 0.21, -0.025334},
      {0.509858, 0.21, 0.025334},
      {0.509858, 0.21, -0.025334},
      {0.50485, 0.21, 0.025334},
      {0.50485, 0.21, -0.025334},
      {0.499843, 0.21, 0.025334},
      {0.499843, 0.21, -0.025334},
      {0.494835, 0.21, 0.025334},
      {0.494835, 0.21, -0.025334},
      {0.489827, 0.21, 0.025334},
      {0.489827, 0.21, -0.025334},
      {0.484819, 0.21, 0.025334},
      {0.484819, 0.21, -0.025334},
      {0.479811, 0.21, 0.025334},
      {0.479811, 0.21, -0.025334},
      {0.474803, 0.21, 0.025334},
      {0.474803, 0.21, -0.025334},
      {0.469795, 0.21, 0.025334},
      {0.469795, 0.21, -0.025334},
      {0.464787, 0.21, 0.025334},
      {0.464787, 0.21, -0.025334},
      {0.45978, 0.21, 0.025334},
      {0.45978, 0.21, -0.025334},
      {0.454772, 0.21, 0.025334},
      {0.454772, 0.21, -0.025334},
      {0.449764, 0.21, 0.025334},
      {0.449764, 0.21, -0.025334},
      {0.444756, 0.21, 0.025334},
      {0.444756, 0.21, -0.025334},
      {0.439748, 0.21, 0.025334},
      {0.439748, 0.21, -0.025334},
      {0.43474, 0.21, 0.025334},
      {0.43474, 0.21, -0.025334},
      {0.429732, 0.21, 0.025334},
      {0.429732, 0.21, -0.025334},
      {0.424724, 0.21, 0.025334},
      {0.424724, 0.21, -0.025334},
      {0.419717, 0.21, 0.025334},
      {0.419717, 0.21, -0.025334},
      {0.414709, 0.21, 0.025334},
      {0.414709, 0.21, -0.025334},
      {0.409701, 0.21, 0.025334},
      {0.409701, 0.21, -0.025334},
      {0.404693, 0.21, 0.025334},
      {0.404693, 0.21, -0.025334},
      {0.399685, 0.21, 0.025334},
      {0.399685, 0.21, -0.025334},
      {0.394677, 0.21, 0.025334},
      {0.394677, 0.21, -0.025334},
      {0.389669, 0.21, 0.025334},
      {0.389669, 0.21, -0.025334},
      {0.384661, 0.21, 0.025334},
      {0.384661, 0.21, -0.025334},
      {0.379654, 0.21, 0.025334},
      {0.379654, 0.21, -0.025334},
      {0.374646, 0.21, 0.025334},
      {0.374646, 0.21, -0.025334},
      {0.369638, 0.21, 0.025334},
      {0.369638, 0.21, -0.025334},
      {0.36463, 0.21, 0.025334},
      {0.36463, 0.21, -0.025334},
      {0.359622, 0.21, 0.025334},
      {0.359622, 0.21, -0.025334},
      {0.354614, 0.21, 0.025334},
      {0.354614, 0.21, -0.025334},
      {0.349606, 0.21, 0.025334},
      {0.349606, 0.21, -0.025334},
      {0.344599, 0.21, 0.025334},
      {0.344599, 0.21, -0.025334},
      {0.339591, 0.21, 0.025334},
      {0.339591, 0.21, -0.025334},
      {0.334583, 0.21, 0.025334},
      {0.334583, 0.21, -0.025334},
      {0.329575, 0.21, 0.025334},
      {0.329575, 0.21, -0.025334},
      {0.324567, 0.21, 0.025334},
      {0.324567, 0.21, -0.025334},
      {0.319559, 0.21, 0.025334},
      {0.319559, 0.21, -0.025334},
      {0.314551, 0.21, 0.025334},
      {0.314551, 0.21, -0.025334},
      {0.309543, 0.21, 0.025334},
      {0.309543, 0.21, -0.025334},
      {0.304536, 0.21, 0.025334},
      {0.304536, 0.21, -0.025334},
      {0.304536, 0.19, 0.025334},
      {0.304536, 0.19, -0.025334},
      {0.309543, 0.19, 0.025334},
      {0.309543, 0.19, -0.025334},
      {0.314551, 0.19, 0.025334},
      {0.314551, 0.19, -0.025334},
      {0.319559, 0.19, 0.025334},
      {0.319559, 0.19, -0.025334},
      {0.324567, 0.19, 0.025334},
      {0.324567, 0.19, -0.025334},
      {0.329575, 0.19, 0.025334},
      {0.329575, 0.19, -0.025334},
      {0.334583, 0.19, 0.025334},
      {0.334583, 0.19, -0.025334},
      {0.339591, 0.19, 0.025334},
      {0.339591, 0.19, -0.025334},
      {0.344599, 0.19, 0.025334},
      {0.344599, 0.19, -0.025334},
      {0.349606, 0.19, 0.025334},
      {0.349606, 0.19, -0.025334},
      {0.354614, 0.19, 0.025334},
      {0.354614, 0.19, -0.025334},
      {0.359622, 0.19, 0.025334},
      {0.359622, 0.19, -0.025334},
      {0.36463, 0.19, 0.025334},
      {0.36463, 0.19, -0.025334},
      {0.369638, 0.19, 0.025334},
      {0.369638, 0.19, -0.025334},
      {0.374646, 0.19, 0.025334},
      {0.374646, 0.19, -0.025334},
      {0.379654, 0.19, 0.025334},
      {0.379654, 0.19, -0.025334},
      {0.384661, 0.19, 0.025334},
      {0.384661, 0.19, -0.025334},
      {0.389669, 0.19, 0.025334},
      {0.389669, 0.19, -0.025334},
      {0.394677, 0.19, 0.025334},
      {0.394677, 0.19, -0.025334},
      {0.399685, 0.19, 0.025334},
      {0.399685, 0.19, -0.025334},
      {0.404693, 0.19, 0.025334},
      {0.404693, 0.19, -0.025334},
      {0.409701, 0.19, 0.025334},
      {0.409701, 0.19, -0.025334},
      {0.414709, 0.19, 0.025334},
      {0.414709, 0.19, -0.025334},
      {0.419717, 0.19, 0.025334},
      {0.419717, 0.19, -0.025334},
      {0.424724, 0.19, 0.025334},
      {0.424724, 0.19, -0.025334},
      {0.429732, 0.19, 0.025334},
      {0.429732, 0.19, -0.025334},
      {0.43474, 0.19, 0.025334},
      {0.43474, 0.19, -0.025334},
      {0.439748, 0.19, 0.025334},
      {0.439748, 0.19, -0.025334},
      {0.444756, 0.19, 0.025334},
      {0.444756, 0.19, -0.025334},
      {0.449764, 0.19, 0.025334},
      {0.449764, 0.19, -0.025334},
      {0.454772, 0.19, 0.025334},
      {0.454772, 0.19, -0.025334},
      {0.45978, 0.19, 0.025334},
      {0.45978, 0.19, -0.025334},
      {0.464787, 0.19, 0.025334},
      {0.464787, 0.19, -0.025334},
      {0.469795, 0.19, 0.025334},
      {0.469795, 0.19, -0.025334},
      {0.474803, 0.19, 0.025334},
      {0.474803, 0.19, -0.025334},
      {0.479811, 0.19, 0.025334},
      {0.479811, 0.19, -0.025334},
      {0.484819, 0.19, 0.025334},
      {0.484819, 0.19, -0.025334},
      {0.489827, 0.19, 0.025334},
      {0.489827, 0.19, -0.025334},
      {0.494835, 0.19, 0.025334},
      {0.494835, 0.19, -0.025334},
      {0.499843, 0.19, 0.025334},
      {0.499843, 0.19, -0.025334},
      {0.50485, 0.19, 0.025334},
      {0.50485, 0.19, -0.025334},
      {0.509858, 0.19, 0.025334},
      {0.509858, 0.19, -0.025334},
      {0.514866, 0.19, 0.025334},
      {0.514866, 0.19, -0.025334},
      {0.519874, 0.19, 0.025334},
      {0.519874, 0.19, -0.025334},
      {0.524882, 0.19, 0.025334},
      {0.524882, 0.19, -0.025334},
      {0.52989, 0.19, 0.025334},
      {0.52989, 0.19, -0.025334},
      {0.534898, 0.19, 0.025334},
      {0.534898, 0.19, -0.025334},
      {0.539906, 0.19, 0.025334},
      {0.539906, 0.19, -0.025334},
      {0.544913, 0.19, 0.025334},
      {0.544913, 0.19, -0.025334},
      {0.549921, 0.19, 0.025334},
      {0.549921, 0.19, -0.025334},
      {0.554929, 0.19, 0.025334},
      {0.554929, 0.19, -0.025334},
      {0.559937, 0.19, 0.025334},
      {0.559937, 0.19, -0.025334},
      {0.564945, 0.19, 0.025334},
      {0.564945, 0.19, -0.025334},
      {0.569953, 0.19, 0.025334},
      {0.569953, 0.19, -0.025334},
      {0.574961, 0.19, 0.025334},
      {0.574961, 0.19, -0.025334}};


      double CoordCheck1[30][3] =
      { { 0.6, 0.19, -0.025334},
      {0.6, 0.19, 0.025334},
      {0.6, 0.195, 0.025334},
      {0.6, 0.195, -0.025334},
      {0.6, 0.2, 0.025334},
      {0.6, 0.2, -0.025334},
      {0.6, 0.205, 0.025334},
      {0.6, 0.205, -0.025334},
      {0.6, 0.21, 0.025334},
      {0.6, 0.21, -0.025334},
      {0.594992, 0.21, 0.025334},
      {0.594992, 0.21, -0.025334},
      {0.589984, 0.21, 0.025334},
      {0.589984, 0.21, -0.025334},
      {0.584976, 0.21, 0.025334},
      {0.584976, 0.21, -0.025334},
      {0.579969, 0.21, 0.025334},
      {0.579969, 0.21, -0.025334},
      {0.574961, 0.21, 0.025334},
      {0.574961, 0.21, -0.025334},
      {0.574961, 0.19, 0.025334},
      {0.579969, 0.19, 0.025334},
      {0.579969, 0.19, -0.025334},
      {0.574961, 0.19, -0.025334},
      {0.584976, 0.19, 0.025334},
      {0.584976, 0.19, -0.025334},
      {0.589984, 0.19, 0.025334},
      {0.589984, 0.19, -0.025334},
      {0.594992, 0.19, 0.025334},
      {0.594992, 0.19, -0.025334} };
      
      // check the node positions
      for(int i=0; i < numNodes; i++) {
        //std::cout << "{";
        for(int j=0; j < 3; j++){
          //std::cout << Coord[i*3+j];
          //if (j<2)
          //   std::cout << ", ";
          if (myRank == 0) {
	     if(fabs(CoordCheck0[i][j] - Coord[i*3+j]) > 1.0e-6){
	       coordCorrect = false;
	       std::cout << "Incorrect coordinate value for node=" << i 
			 << " index " << j << " Expected " << CoordCheck0[i][j] 
			 << " Got " << Coord[i*3+j] << std::endl;
	     }
          } else {
	     if(fabs(CoordCheck1[i][j] - Coord[i*3+j]) > 1.0e-6){
	       coordCorrect = false;
	       std::cout << "Incorrect coordinate value for node=" << i 
			 << " index " << j << " Expected " << CoordCheck1[i][j] 
			 << " Got " << Coord[i*3+j] << std::endl;
	     }
          }
        }
        //std::cout << "}," << std::endl;
      }
      result.UpdateResult("ParCoordinateData:Correct", coordCorrect);

      //////////////////////////////////////////////////
      // Check connectivity values
      //////////////////////////////////////////////////
      bool connCorrect = true;

      // get connectivity tables for panes
      int numConn;
      std::string stringNames;
      COM_get_connectivities("OFModule", pane, &numConn, stringNames);
      std::istringstream ConnISS(stringNames);
      std::vector<std::string> connNames;

      for (int i=0; i<numConn; ++i) {
        std::string name;
        ConnISS >> name;
        connNames.push_back(name);
        std::cout << "Connectivity Table # " << i+1 << ": " << name << std::endl;
      }

      // number of nodes per element
      char getDataItemLoc;
      COM_Type getDataItemType;
      int numElementNodes;
      std::string getDataItemUnits;
      std::string fullConnName("OFModule."+connNames[0]);
      COM_get_dataitem(fullConnName, &getDataItemLoc, &getDataItemType, 
                       &numElementNodes, &getDataItemUnits);

      std::cout << "getDataItemLoc " << getDataItemLoc << std::endl;
      std::cout << "getDataItemType " << getDataItemType << std::endl;
      std::cout << "numElementNodes " << numElementNodes << std::endl;
      std::cout << "getDataItemUnits " << getDataItemUnits << std::endl;

      int* Conn;
      COM_get_array(fullConnName.c_str(), pane, &Conn);

      if (Conn == NULL) {
        std::cout << "In Test__ParModuleMeshRegistration:  No mesh connectivity registered."
                  << std::endl;
        return; 
      } else {
        result.UpdateResult("ParConnectivityData:Registered", true);
      } 

      // assumes a single connectivity for this pane
      int numElem = 0;
      std::vector<int> numElemExpected;
      numElemExpected.push_back(154);
      numElemExpected.push_back(14);
      COM_get_size(fullConnName, pane, &numElem);

      if (numElem != numElemExpected[myRank]) {
        std::cout << "Wrong number of elements registered for this mesh." << std::endl;
        std::cout << "Got " << numElem << " Expected: " << numElemExpected[myRank] << std::endl;
      } else {
        connCorrect = true;
      }

      int ConnCheck0[154][4] =
      { {1, 2, 3, 4},
      {3, 5, 6, 4},
      {5, 7, 8, 6},
      {7, 9, 10, 8},
      {9, 11, 12, 10},
      {11, 13, 14, 12},
      {13, 15, 16, 14},
      {15, 17, 18, 16},
      {17, 19, 20, 18},
      {19, 21, 22, 20},
      {21, 23, 24, 22},
      {23, 25, 26, 24},
      {25, 27, 28, 26},
      {27, 29, 30, 28},
      {29, 31, 32, 30},
      {31, 33, 34, 32},
      {33, 35, 36, 34},
      {35, 37, 38, 36},
      {37, 39, 40, 38},
      {39, 41, 42, 40},
      {41, 43, 44, 42},
      {43, 45, 46, 44},
      {47, 48, 49, 50},
      {51, 47, 50, 52},
      {53, 51, 52, 54},
      {55, 53, 54, 56},
      {57, 55, 56, 58},
      {59, 57, 58, 60},
      {61, 59, 60, 62},
      {63, 61, 62, 64},
      {65, 63, 64, 66},
      {67, 65, 66, 68},
      {69, 67, 68, 70},
      {71, 69, 70, 72},
      {73, 71, 72, 74},
      {75, 73, 74, 76},
      {77, 75, 76, 78},
      {79, 77, 78, 80},
      {81, 79, 80, 82},
      {83, 81, 82, 84},
      {85, 83, 84, 86},
      {87, 85, 86, 88},
      {89, 87, 88, 90},
      {91, 92, 89, 90},
      {93, 94, 95, 96},
      {94, 97, 98, 95},
      {97, 99, 100, 98},
      {99, 101, 102, 100},
      {101, 103, 104, 102},
      {103, 105, 106, 104},
      {105, 107, 108, 106},
      {107, 109, 110, 108},
      {109, 111, 112, 110},
      {111, 113, 114, 112},
      {113, 115, 116, 114},
      {115, 117, 118, 116},
      {117, 119, 120, 118},
      {119, 121, 122, 120},
      {121, 123, 124, 122},
      {123, 125, 126, 124},
      {125, 127, 128, 126},
      {127, 129, 130, 128},
      {129, 131, 132, 130},
      {131, 133, 134, 132},
      {133, 135, 136, 134},
      {135, 137, 138, 136},
      {137, 139, 140, 138},
      {139, 141, 142, 140},
      {141, 143, 144, 142},
      {143, 145, 146, 144},
      {145, 147, 148, 146},
      {147, 149, 150, 148},
      {149, 151, 152, 150},
      {151, 153, 154, 152},
      {153, 155, 156, 154},
      {155, 157, 158, 156},
      {157, 159, 160, 158},
      {159, 161, 162, 160},
      {161, 163, 164, 162},
      {163, 165, 166, 164},
      {165, 167, 168, 166},
      {167, 169, 170, 168},
      {169, 171, 172, 170},
      {171, 173, 174, 172},
      {173, 175, 176, 174},
      {175, 177, 178, 176},
      {177, 179, 180, 178},
      {179, 181, 182, 180},
      {181, 183, 184, 182},
      {183, 185, 186, 184},
      {185, 187, 188, 186},
      {187, 189, 190, 188},
      {189, 191, 192, 190},
      {191, 193, 194, 192},
      {193, 195, 196, 194},
      {195, 197, 198, 196},
      {197, 199, 200, 198},
      {199, 201, 202, 200},
      {201, 2, 1, 202},
      {48, 203, 204, 49},
      {203, 205, 206, 204},
      {205, 207, 208, 206},
      {207, 209, 210, 208},
      {209, 211, 212, 210},
      {211, 213, 214, 212},
      {213, 215, 216, 214},
      {215, 217, 218, 216},
      {217, 219, 220, 218},
      {219, 221, 222, 220},
      {221, 223, 224, 222},
      {223, 225, 226, 224},
      {225, 227, 228, 226},
      {227, 229, 230, 228},
      {229, 231, 232, 230},
      {231, 233, 234, 232},
      {233, 235, 236, 234},
      {235, 237, 238, 236},
      {237, 239, 240, 238},
      {239, 241, 242, 240},
      {241, 243, 244, 242},
      {243, 245, 246, 244},
      {245, 247, 248, 246},
      {247, 249, 250, 248},
      {249, 251, 252, 250},
      {251, 253, 254, 252},
      {253, 255, 256, 254},
      {255, 257, 258, 256},
      {257, 259, 260, 258},
      {259, 261, 262, 260},
      {261, 263, 264, 262},
      {263, 265, 266, 264},
      {265, 267, 268, 266},
      {267, 269, 270, 268},
      {269, 271, 272, 270},
      {271, 273, 274, 272},
      {273, 275, 276, 274},
      {275, 277, 278, 276},
      {277, 279, 280, 278},
      {279, 281, 282, 280},
      {281, 283, 284, 282},
      {283, 285, 286, 284},
      {285, 287, 288, 286},
      {287, 289, 290, 288},
      {289, 291, 292, 290},
      {291, 293, 294, 292},
      {293, 295, 296, 294},
      {295, 297, 298, 296},
      {297, 299, 300, 298},
      {299, 301, 302, 300},
      {301, 303, 304, 302},
      {303, 305, 306, 304},
      {305, 307, 308, 306},
      {307, 309, 310, 308},
      {309, 311, 312, 310} };

      int ConnCheck1[14][4] =
      { {1, 2, 3, 4},
      {4, 3, 5, 6},
      {6, 5, 7, 8},
      {8, 7, 9, 10},
      {9, 11, 12, 10},
      {11, 13, 14, 12},
      {13, 15, 16, 14},
      {15, 17, 18, 16},
      {17, 19, 20, 18},
      {21, 22, 23, 24},
      {22, 25, 26, 23},
      {25, 27, 28, 26},
      {27, 29, 30, 28},
      {29, 2, 1, 30} };


      for(int i=0; i < numElem; i++){
        //std::cout << "{";
        for(int j=0; j < numElementNodes; j++){
          //std::cout<<Conn[i*numElementNodes+j];
          //if (j<3)
          //   std::cout<<", ";
          if (myRank==0) { 
	     if(fabs(ConnCheck0[i][j] - Conn[i*numElementNodes+j]) > 1.0e-6){
	       connCorrect = false;
	       std::cout << "Incorrect connectivity value at element " << i 
			 << " index " << j << " Expected " << ConnCheck0[i][j] 
			 << " Got " << Conn[i*numElementNodes+j] << std::endl;
	     }
         } else {
	     if(fabs(ConnCheck1[i][j] - Conn[i*numElementNodes+j]) > 1.0e-6){
	       connCorrect = false;
	       std::cout << "Incorrect connectivity value at element " << i 
			 << " index " << j << " Expected " << ConnCheck1[i][j] 
			 << " Got " << Conn[i*numElementNodes+j] << std::endl;
	     }
         }
        }
        //std::cout <<"},"<<std::endl;
      }
      result.UpdateResult("ParConnectivityData:Correct", connCorrect);


      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      COM_free_buffer(&paneList);
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      return;
    }

    ///*****************************************************************************
    /// Function to test the time Step function for the OpenFoamModule 
    /// StepFluid Function
    ///
    /// Loads an openfoam simulation and takes a single step before returning 
    /// control to the driver
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///*****************************************************************************
    virtual void Test__ParModuleFunctionStepFluid(ResultsType &result){
      std::cout << "Running Test__ParModuleFunctionStepFluid" << std::endl; 
      int myRank = (GetCommunicator()).Rank();
 
      //load OpenFoam module
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      bool works = false;


      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/HronTurekFsiPar";
      std::string dirName = SourceDirPath + suffix;
      std::string testDir = IRAD::Sys::CWD() + "/UnitTestDataParStepTest";
      int IntDir;
      if (myRank == 0){
         // copy test information
         GetInputData(dirName, testDir);
      }
      (GetCommunicator()).Barrier();
      std::string fluidDir = testDir + "/fluid";
      std::string OrgDir = IRAD::Sys::CWD();
      IntDir = IRAD::Sys::ChDir(fluidDir);

      // call init to setup the mesh
      int dummy1=2;
      char *dummy2[2];
      dummy2[0] = const_cast<char *>("StepFluid");
      dummy2[1] = const_cast<char *>("-parallel");
      int verb=3;
      int initHandle = COM_get_function_handle("OFModule.InitFoam");
      COM_call_function(initHandle, &dummy1, &dummy2, &verb);

      // list of panes in this window
      int numPanes;
      int* paneList;
      COM_get_panes("OFModule", &numPanes, &paneList);

      // assuming only one pane per process
      int pane = paneList[0];

      // check that the current simulation time and endTime are registered
      int status = COM_get_status("OFModule.time", pane);
      bool timeStatus = false;
      if(status <= 0) {
        std::cout << "ModuleFunctionParStepFluid Error: time not a valid DataItem."
                  << "status = " << status << std::endl;
        result.UpdateResult("ParTimeData:Registered", false);
        return;
      } else {
        timeStatus = true;
      }

      status = COM_get_status("OFModule.endTime", pane);
      if(status <= 0) {
        std::cout << "ParModuleFunctionStepFluid Error: endTime not a valid DataItem."
                  << "status = " << status << std::endl;
        result.UpdateResult("ParTimeData:Registered", false);
        return;
      } else {
        timeStatus = timeStatus && true;
      }
      result.UpdateResult("ParTimeData:Registered", timeStatus);

      double* time=NULL;
      COM_get_array("OFModule.time", pane, &time);
      double* endTime=NULL;
      COM_get_array("OFModule.endTime", pane, &endTime);

      works=false;
      double testTime = 0.;
      double testEndTime = 5.0e-3;
      double toler = 1.e-9;
      if(std::fabs(*time - testTime) < toler && std::fabs(*endTime - testEndTime) < toler)
        works=true;
      else {
        std::cout << "ParModuleFunctionStepFluid Error: time or endTime initialized incorrectly"
                  << std::endl;
        std::cout << " time = " << *time << " expected " << testTime << std::endl;
        std::cout << " endTime = " << *endTime << " expected " << testEndTime << std::endl;
      }
      result.UpdateResult("ParTimeData:Correct",works);

      // Test StepFluid function exists
      works = false;
      int stepHandle = COM_get_function_handle("OFModule.StepFluid");
      if(stepHandle > 0)
        works=true;
      else {
        std::cout << "ParModuleFunctionStepFluid Error: Could not get function handle."
                  << std::endl;
        result.UpdateResult("ParStepFluidFunction:Exists",false);
        return;
      }
      result.UpdateResult("ParStepFluidFunction:Exists",works);

      // Test StepFluid function steps
      // check that the time got incremented as expected
      COM_call_function(stepHandle);


      works=false;
      testTime = 5.e-4;
      toler = 1.e-9;
      if (std::fabs(*time - testTime) < toler )
        works=true;
      else {
        std::cout << "ParModuleFunctionStepFluid Error: time did not increment"
                  << std::endl;
        std::cout << " time = " << *time << "expected " << testTime << std::endl;
      }
      result.UpdateResult("ParStepFluidFunction:Steps",works);

      // cleanup
      IntDir = IRAD::Sys::ChDir(OrgDir);
      //Unload OpenFoam module
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");

      return;
    }

    ///************************************************************************
    /// Functions to call Parallel tests
    ///************************************************************************
    ///
    /// Runs all tests implemented by the OpenFoamModuleDriver::TestingObject.
    ///
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    virtual void Process(ResultsType &result){
      Prologue();
      Test__ParModuleLoadUnload(result);
      Test__ParCommunicatorPassToModule(result);
      Test__ParLoadUnitTestData(result);
      Test__ParModuleFunctionInitialize(result);
      Test__ParModuleSurfaceData(result);
      Test__ParModuleMeshRegistration(result);
      Test__ParModuleFunctionStepFluid (result);
      Epilogue();
    }
    
    ///
    /// Runs a test specified by name
    ///
    /// @param name String name of the test to run.
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    virtual void RunTest(const std::string &name,ResultsType &result)
    {
      if(name == "ParCommunicatorPassToModule")
        Test__ParCommunicatorPassToModule(result);
      if(name == "ParSolverLoadUnload")
        Test__ParModuleLoadUnload(result);
      if(name == "ParLoadUnitTestData")
        Test__ParLoadUnitTestData(result);
      if(name == "ParModuleInitFuncion")
        Test__ParModuleFunctionInitialize(result);
      if(name == "ParModuleSurfaceData")
        Test__ParModuleSurfaceData(result);
      if(name == "ParModuleMeshRegistration")
        Test__ParModuleMeshRegistration(result);
      if(name == "ParModuleFunctionStepFluid")
        Test__ParModuleFunctionStepFluid(result);
    }

    ///
    /// Runs a list of tests specified by name
    /// 
    /// @param test_names list of string test names
    /// @param result OpenFoamModuleDriver::TestResults object to store test results.
    ///
    virtual void ProcessTests(std::list<std::string> &test_names,ResultsType &result){
      Prologue();
      std::list<std::string>::iterator tni = test_names.begin();
      while(tni != test_names.end())
        RunTest(*tni++,result);
      Epilogue();
    }


  };
#endif
};
#endif
