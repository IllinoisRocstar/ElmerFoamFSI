///
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Testing object for ElmerFoamFSI.
///
/// This file contains the implementation of the
/// overarching testing object for the project.
/// Every project should have one of these as
/// it allows external entities unified methodology
/// for access project or package-specific tests.
///
#ifndef __ELMERFOAMFSI_TEST_H__
#define __ELMERFOAMFSI_TEST_H__
#ifdef _ELMERFOAMFSI_PARALLEL_
#include "COMM.H"
#endif
#include "Testing.H"
#include "ExampleHeader.H"
#include "Profiler.H"
#include <vector>
#include <cmath>
#include "Driver.H"
//#include <array>
//#include "Parameters.H"
//#include "InterfaceLayer.H"
//#include "Orchestrator.H"
//#include "OpenFoamAgent.H"
//#include "ElmerAgent.H"
#include "FsiCoupling.H"
// Loading native modules
COM_EXTERN_MODULE(OpenFoamFSI);
COM_EXTERN_MODULE(ElmerCSC);

namespace ElmerFoamFSI {

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
    typedef std::string                 StackType;
    typedef IRAD::Profiler::ProfilerObj ProfilerType;
    typedef IRAD::Global::GlobalObj<StackType,int,ProfilerType> TestGlobalType;
    typedef IRAD::Util::TestingObject<ResultsType> TestingObjectBaseType;
  protected:
    // Example test fixtures for ElmerFoamFSI testing.
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
    //WK Start
    std::string SourceDirPath;
    //WK End
    TestGlobalType global;
    //NS Start
    std::vector<std::string> componentInterfaceNames;
    ///
    /// Instantiation of class for use with the fsiCoupler & Agent tests
    ///
    fsicoupling fsiCoupler;
    ///
    ///For use with the ElmerStructure and OpenFoamFluids tests
    ///
    solidagent* TeststructuresAgent;
    fluidagent* TestfluidsAgent;
    ///
    ///track verbosity level for tests
    ///
    int verblevel;
    //NS End
  public:
    /// 
    /// Default constructor.
    ///
    TestingObject() : TestingObjectBaseType() , fsiCoupler(global) , TeststructuresAgent(NULL) , TestfluidsAgent(NULL) {};
    ///
    /// Tears down the testing fixtures if needed.
    ///
    virtual void Epilogue() {
      COM_finalize();};
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
      // initializing COM
      bool runs = false;
      int nargs_dupe = 2;
      char **args_dupe;
      args_dupe = new char * [3];
      args_dupe[0] = const_cast<char *>("test");
      args_dupe[1] = const_cast<char *>("-com-mpi");
      args_dupe[2] = NULL;
      COM_init(&nargs_dupe,&args_dupe);
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
    ///
    /// Test for ElmerFoamFSI::ExampleFunction
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function implements a simple test of the 
    /// function ElmerFoamFSI::ExampleFunction - which is designed to
    /// simply return a copy of the input string.
    ///
    virtual void Test__ExampleFunction(ResultsType &result) {
      // This is an actual test of the function called 
      // ExampleFunction.   The name Test__XXXXX will
      // eventually help automated utilities with 
      // running tests by name.
      std::string ExampleResult(ExampleFunction(ExampleTestFixture));      
      result.UpdateResult("ExampleFunction:Works",
                          ExampleResult == ExampleTestFixture);
      result.UpdateResult("ExampleFunction:Fails",
                          ExampleResult != ExampleTestFixture);
    }
    ///
    /// Test for ElmerFoamFSI::TrapezoidQuadrature
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function implements a simple test of the 
    /// function ElmerFoamFSI::TrapezoidQuadrature - which is designed to
    /// integrate a function, f, over an n-way partitioned interval 
    /// \f$[x_0,x_n]\f$ . 
    ///
    /// The test first makes sure that the method integrates a linear 
    /// function exactly, and then that the error term has the proper
    /// dependence on the spacings.
    /// 
    virtual void Test__TrapezoidQuadrature(ResultsType &result) {
      std::ostringstream Ostr;
      std::vector<double> E;
      size_t n = 2*N.size();
      bool runs = true;
      for(std::vector<int>::iterator i = N.begin();i != N.end();i++){
        double Ii = 0.0;
        try {
          Ii = ElmerFoamFSI::TrapezoidQuadrature(TestFixture::F1,0,1,*i);
        } catch (...) {
          runs = false;
        }
        E.push_back(std::fabs(Ii-1.0));
      }
      bool order2 = (E[0] < 1e-14);
      for(std::vector<int>::iterator i = N.begin();i != N.end();i++){
        double Ii = 0.0;
        try {
          Ii = ElmerFoamFSI::TrapezoidQuadrature(TestFixture::F2,0,1,*i);
        } catch (...) {
          runs = false;
        }
        E.push_back(std::fabs(Ii-1.0));
      }
      result.UpdateResult("TrapezoidQuadrature:Runs",runs);
      result.UpdateResult("TrapezoidQuadrature:Accurate",E[n-1] < 1e-12);
      for(int i = N.size();i < n - 1;i++){
        double e = E[i+1]/E[i];
        double n1 = static_cast<double>(N[i-N.size()])/static_cast<double>(N[(i-N.size())+1]);
        double p = std::log(e)/std::log(n1);
        p -= 2;
        p = std::abs(p);
        if(p > 2e-2){
          order2 = false;
        }
      }
      result.UpdateResult("TrapezoidQuadrature:Order2",order2);
    }

    ///
    /// Test for ElmerFoamFSI::MidPointQuadrature
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function implements a simple test of the 
    /// function ElmerFoamFSI::MidPointQuadrature - which is designed to
    /// integrate a function, f, over an interval \f$[x_0,x_n]\f$. 
    ///
    /// The test first makes sure that the method integrates a linear 
    /// function exactly, and then that the error term has the proper
    /// dependence on the spacings.
    /// 
    virtual void Test__MidPointQuadrature(ResultsType &result) {
      std::ostringstream Ostr;
      std::vector<double> Ibar;
      std::vector<double> E;
      size_t n = 2*N.size();
      bool runs = true;
      bool order2 = true;
      for(std::vector<int>::iterator i = N.begin();i != N.end();i++){
        double Ii = 0.0;
        try {
          Ii = ElmerFoamFSI::MidPointQuadrature(TestFixture::F1,0,1,*i);
        } catch (...) {
          runs = false;
        }
        Ibar.push_back(Ii);
        E.push_back(std::fabs(Ii-1.0));
      }
      order2 = (E[0] < 1e-14);
      for(std::vector<int>::iterator i = N.begin();i != N.end();i++){
        double Ii = 0.0;
        try {
          Ii = ElmerFoamFSI::MidPointQuadrature(TestFixture::F2,0,1,*i);
        } catch (...) {
          runs = false;
        }
        Ibar.push_back(Ii);
        E.push_back(std::fabs(Ii-1.0));
      }
      result.UpdateResult("MidPointQuadrature:Runs",runs);
      result.UpdateResult("MidPointQuadrature:Accurate",E[n-1] < 1e-12);
      for(int i = N.size();i < n - 1;i++){
        double e = E[i+1]/E[i];
        double n1 = static_cast<double>(N[i-N.size()])/static_cast<double>(N[(i-N.size())+1]);
        double p = std::log(e)/std::log(n1);
        p -= 2;
        p = std::abs(p);
        if(p > 1e-2){
          order2 = false;
        }
      }
      result.UpdateResult("MidPointQuadrature:Order2",order2);
    }
   
    virtual void Test__FluidsInitialize(ResultsType &result) {
      bool runs = false;
      COM_load_module("OpenFoamFSI","FluidsComponentInterface");
      
//      openfoamagent fluidsAgent;
//      fluidsAgent.Initialize("FluidsComponentInterface");

      COM_unload_module("OpenFoamFSI","FluidsComponentInterface");
      result.UpdateResult("FluidsInitialize:Runs",runs);
    }
    // MS: Start
    /// Test for ElmerFoamFSI::ScaffoldUnitTestFun
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function implements a simple empty test functions which can be used
    //  a scaffold to develop unit tests. 
    ///
    virtual void Test__ScaffoldUnitTestFun(ResultsType &result) {
      bool runs = true;
      std::cout << "This line is added by scafoldTestFun unit test" << std::endl;
      result.UpdateResult("ScaffoldUnitTestFun:Runs",runs);
    }
    // MS: End

  // MS: Test elmerModule load/unloads properly
    virtual void Test__ElmerModuleLoad(ResultsType &result) {
      std::cout << "Running Test__ElmerModuleLoad" << std::endl;

      // load Elmer module
      std::cout << "Loading Elmer Module" << std::endl;
      COM_LOAD_MODULE_STATIC_DYNAMIC(ElmerCSC, "ElmerModule");
      bool works = false;

      // get window handle
      int h=COM_get_window_handle("ElmerModule");
      if(h <= 0){
        std::cout << " After loading, COM_get_window_handle(\"ElmerModule\") returns "
                  << h << std::endl;
        std::cout << "ModuleLoadUnload Error: Could not get window handle."
                  << std::endl;
      }
      else{
        works=true;
        std::cout << "Elmer Module Loaded" << std::endl;
      }
      result.UpdateResult("LoadElmerModule:Works",works);

      // unload Elmer Module
      std::cout << "Unloading Elmer Module" << std::endl;
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(ElmerCSC, "ElmerModule");
      works=false;

      // make sure module unloaded
      h=COM_get_window_handle("ElmerModule");
      if(h > 0){
        std::cout << "After unloading, COM_get_window_handle(\"ElmerModule\") returns "
                  << h << std::endl;
        std::cout << "ModuleLoadUnload Error: Did not properly unload module."
                  << std::endl;
      }
      else{
        works=true;
        std::cout << "Elmer Module Unloaded" << std::endl;
      }

      result.UpdateResult("UnloadElmerModule:Works",works);

      return;
    }
    // MS: End

    // MS: Test OpenFoamModule load/unloads properly
    virtual void Test__OFModuleLoad(ResultsType &result) {
      std::cout << "Running Test__OFModuleLoad" << std::endl;

      // load Elmer module
      std::cout << "Loading OpenFoam Module" << std::endl;
      COM_LOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");
      bool works = false;

      // get window handle
      int h=COM_get_window_handle("OFModule");
      if(h <= 0){
        std::cout << " After loading, COM_get_window_handle(\"OFModule\") returns "
                  << h << std::endl;
        std::cout << "ModuleLoadUnload Error: Could not get window handle."
                  << std::endl;
      }
      else{
        works=true;
        std::cout << "OpenFoam Module Loaded" << std::endl;
      }
      result.UpdateResult("LoadOpenFoamModule:Works",works);

      // unload Elmer Module
      std::cout << "Unloading OpenFoam Module" << std::endl;
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");
      works=false;

      // make sure module unloaded
      h=COM_get_window_handle("OFModule");
      if(h > 0){
        std::cout << "After unloading, COM_get_window_handle(\"OFModule\") returns "
                  << h << std::endl;
        std::cout << "ModuleLoadUnload Error: Did not properly unload module."
                  << std::endl;
      }
      else{
        works=true;
        std::cout << "OpenFoam Module Unloaded" << std::endl;
      }

      result.UpdateResult("UnloadOpenFoamModule:Works",works);
      return;
    }
    // MS: End


    // WK : Start 
    // Test if Elmer can load properly, and its functions handles can be accessed
    virtual void Test__ElmerHandlesCheck(ResultsType & result){
      bool works, loadWorks, globWorks, initWorks, runWorks, finWorks, unloadWorks = false;

      std::cout << "Running Test__ElmerHandlesCheck" << std::endl;
    
      //Loading Elmer Module
      std::cout << "Loading Elmer Module" << std::endl;
      COM_LOAD_MODULE_STATIC_DYNAMIC(ElmerCSC,"ElmerModule");

      //Checking Elmer Module is indeed loaded.
      int ElmerHandle = COM_get_window_handle("ElmerModule");
      if(ElmerHandle <=0){
        std::cout << "After loading, COM_get_window_handle(\"ElmerModule\") returns "
                  << ElmerHandle << std::endl;
        std::cout << "Error : Could not get window handle." << std::endl;
      }
      else{
        loadWorks = true;
        std::cout << "Elmer Module Loaded." << std::endl;
      }

      //Checking the registration of global data.
      int globHandle = COM_get_dataitem_handle("ElmerModule.global");
      if(globHandle <= 0){
        std::cout << "After loading, COM_get_dataitem_handle(\"ElmerModule.global\") returns "
                  << globHandle << std::endl;
        std::cout << "Error : Could not get the data item handle." << std::endl;
      }
      else{
        globWorks = true;
        std::cout << "Global data handle accessed." << std::endl;
      }
      
      //Checking the ElmerModule function Initialize handle
      int initHandle = COM_get_function_handle("ElmerModule.Initialize");
      if(initHandle <= 0){
        std::cout << "After loading, COM_get_function_handle(\"ElmerModule.Initialize\") returns "
                  << initHandle << std::endl;
        std::cout << "Error : Could not get the Initialize function handle." << std::endl;
      }
      else{
        initWorks = true;
        std::cout << "Initialize function handle accessed." << std::endl;
      }

      //Checking the ElmerModule function Run handle
      int runHandle = COM_get_function_handle("ElmerModule.Run");
      if(runHandle <= 0){
        std::cout << "After loading, COM_get_function_handle(\"ElmerModule.Run\") returns "
                  << runHandle << std::endl;
        std::cout << "Error : Could not get the Run function handle." << std::endl;
      }
      else{
        runWorks = true;
        std::cout << "Run function handle accessed." << std::endl;
      }

      //Checking the ElmerModule function Finalize handle
      int finHandle = COM_get_function_handle("ElmerModule.Finalize");
      if(finHandle <= 0){
        std::cout << "After loading, COM_get_function_handle(\"ElmerModule.Finalize\") returns "
                  << finHandle << std::endl;
        std::cout << "Error : Could not get the Finalize function handle." << std::endl;
      }
      else{
        finWorks = true;
        std::cout << "Finalize function handle accessed." << std::endl;
      }

      //Unloading Elmer Module
      std::cout << "Unloading Elmer Module" << std::endl;
      COM_UNLOAD_MODULE_STATIC_DYNAMIC(ElmerCSC, "ElmerModule");
      
      //Checking Elmer Module is indeed unloaded.
      ElmerHandle = COM_get_window_handle("ElmerModule");
      if(ElmerHandle > 0){
        std::cout << "After unloading, COM_get_window_handle(\"ElmerModule\") returns "
                  << ElmerHandle << std::endl;
        std::cout << "Error : Module was not properly unloaded." << std::endl;
      }
      else{
        unloadWorks = true;
        std::cout << "ElmerModule Unloaded." << std::endl;
      } 
      
      if(loadWorks & globWorks & initWorks & runWorks & finWorks & unloadWorks) works = true;
      result.UpdateResult("ElmerHandlesCheck:Works",works);
    }
        
    /// Sets the string value of the testing source directory
    ///
    void SetSourceDirPath(std::string input){
      SourceDirPath = input;
      std::cout << "Source Path = " << SourceDirPath << std::endl;
    }   


    //Copied from ElmerDriver testing script
    virtual int GetInputData(std::string suffix,std::string TestDir){

      std::string DirName;
      std::string OrgDir;

      //Save original working directory for later
      OrgDir = IRAD::Sys::CWD();

      //Create full path for test input data directory
      DirName = SourceDirPath + suffix;

      //Check if data directory exists
      if(!IRAD::Sys::FILEEXISTS(DirName)){
        std::cout << "GetInputData Error: Could not find data directory, "
                  << DirName << ". Exiting (fail)." << std::endl;
        return -1;
      }

      std::ifstream Inf;
      std::ofstream Ouf;
      std::string InFileName;
      std::string OutFileName;
      int IntDir;

      //Create TestDir directory in current directory (if it doesn't
      //already exist) to run our tests in.
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        IntDir = IRAD::Sys::CreateDirectory(TestDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not make directory, "
                    << TestDir << ". Exiting (fail)." << std::endl;
          return -1;
        }
      }

      //Open the input test data directory
      IRAD::Sys::Directory SourceDir(DirName);
     
      //Change directories to TestDir directory for running 
      IntDir = IRAD::Sys::ChDir(TestDir);
      if(IntDir == -1){
        std::cout << "GetInputData Error: Could not change directories to "
                  << TestDir << ". Exiting (fail)." << std::endl;
        return -1;
      }

      //Copy input data to TestDir directory for running 
      for(std::vector<std::string>::iterator it = SourceDir.begin();
          it != SourceDir.end(); ++it){ 
        InFileName = DirName + "/" + *it;

        // WK recursively copying the directory as well
        bool isDir = IRAD::Sys::ISDIR(InFileName.c_str());
        if( isDir ){
          std::string dirSuffix = suffix + "/"  + *it;
          std::string dirTestDir = TestDir + "/" + *it;
          IntDir  = GetInputData(dirSuffix, dirTestDir);
          
          //checking If recursive GetInputData exited correctly
          if(IntDir != 0){
            std::cout << "GetInput failed at the recursive level"
                      << std::endl;
            return -1;
          }
        }

        Inf.open(InFileName.c_str());
        OutFileName = IRAD::Sys::CWD() + "/" + *it;
        Ouf.open(OutFileName.c_str());
        Ouf << Inf.rdbuf();
        Ouf.close();
        Inf.close();
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
    

    // Helper function
    virtual int __helper(){
 
      int IntDir;
      std::string OrgDir, TestDir;     

      //call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/testing/share/Testing/test_data/SimpleStatic";
  
      //Make path for directory to run the test in
      TestDir = IRAD::Sys::CWD() + "/SimpleStatic";
      std::cout << "Getting Input Data!!" << std::endl;
      IntDir  = GetInputData(suffix, TestDir);
  
      //check if GetInputData exited correctly.
      if(IntDir != 0){
        std::cout << "GetInputData failed!"
                  << std::endl;
        return 1;
      }
      // Check if TestDir directory exists in current Directory
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        std::cout << "Could not find directory, "
                  << TestDir << ", for running tests." << std::endl;
        return 2;
      }
      return 0;     
    }  
    
    
    //testing the getinput
    virtual void Test__GetInput(ResultsType &result){
      int out = __helper();
      bool works;
      
      if(out == 0) works = true;
      else works = false;
      result.UpdateResult("GetInput:Works",works);
    }
    
    
    ///
    /// Initalize function for the fsiCoupler and agents 
    ///
    virtual void Test__fsiCouplerAgentsInitialize(){
      int work =__helper();
      int IntDir;
      verblevel = 3;
      TeststructuresAgent = new solidagent;
      TestfluidsAgent = new fluidagent;
      
      std::string fluidDir;
      std::string simStaticDir;
      fluidDir = IRAD::Sys::CWD() + "/SimpleStatic/fluid";
      simStaticDir = IRAD::Sys::CWD() + "/SimpleStatic";
      if(work == 1)
        std::cout << "GetInputData failed 2!" << std::endl;
      else if (work == 2)
      	std::cout << "Could not find directory for running tests" << std::endl;
      	
      if(!(IRAD::Sys::FILEEXISTS(fluidDir)||(IRAD::Sys::FILEEXISTS(simStaticDir)))){
        std::cout << "SimpleStatic / fluid Folders do not exists!" << std::endl;
        return; 
      }
      else{
        IntDir = IRAD::Sys::ChDir(simStaticDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
        }
        std::system("chmod 755 Allclean Allrun AllrunPar makeLinks makeSerialLinks removeSerialLinks"); 
        std::system("./Allclean");
        std::system("./Allrun");
        IntDir = IRAD::Sys::ChDir(fluidDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
        }
      }
      
      std::string fluidSolverName("OpenFoamFSI");
      std::string solidSolverName("ElmerCSC");
      std::string transferServiceName("SurfX");
      std::string runMode("1");

      std::cout << "COM loading all modules" << std::endl;
      COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
      COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
      COM_load_module("SurfUtil","SurfUtil");
      COM_load_module("Simpal","Simpal");

      componentInterfaceNames.push_back("FluidsComponentInterface");
      componentInterfaceNames.push_back("StructuresComponentInterface");
      componentInterfaceNames.push_back("TransferInterface");
      componentInterfaceNames.push_back("SurfUtil");
      componentInterfaceNames.push_back("Simpal");
 
      fsiCoupler.SetRunMode(runMode);
      fsiCoupler.SetVerbLevel(verblevel);
      std::cout << "fsiCouplerAgentsInitialize Successful!" << std::endl;
    }
    
      
/*    void Test__ReadInputWorks(ResultsType &result){

      int argc = 2;
      char ** argv;
      argv = new char*[3];
      argv[0] = const_cast<char *>("elmerfoamfsi_test");
      argv[1] = const_cast<char *>("test.config");
      argv[2] = NULL;
      std::cout << "Do I even get here? " << std::endl;       
      ElmerFoamFSI::SerialDriverProgram MyProgram(argc,argv);
      std::cout << "Do I even get here? 2" << std::endl;       
      
      int retval = MyProgram.Initialize();
      if(retval){
        MyProgram.ErrOut("Error in Prram initialization. \n");
      }
      std::cout << "Before Reading Input ! " << std::endl;       
      MyProgram.ReadInput();
      std::cout << "After  Reading Input ! " << std::endl;
      int works = 0;
      //result.UpdateResult("ReadInputWorks", works);
    }
*/
		///
		/// Test the functionality of the fsiCoupler
    virtual void Test__FsiCoupler(ResultsType &result){
      std::cout << "Running Test__FsiCoupler" << std::endl;
      double time_final = 0.05;
      double timestep = 2.0e-3;
      bool works = true;
      
      //Call the Agents Initialize function above
      //as well as the Initialize function of fsiCoupler
      std::cout << "Initalizing..." << std::endl;
      Test__fsiCouplerAgentsInitialize();
      fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);  
      std::cout << "Initialize Completed!" << std::endl;
      
      //test to make sure that classInterfaceNames are populated 
      //correctly to match componentInterfaceNames
      std::vector<std::string> classInterfaceNames;
      classInterfaceNames =  fsiCoupler.getVariable();
      
      for(int i = 0; i < componentInterfaceNames.size(); i++){
        if(componentInterfaceNames.at(i) != classInterfaceNames.at(i)){
          works = false;
          break;
        }
      }
      result.UpdateResult("fsiCoupler:Works",works);
    }

    
    ///
    /// Run an initialization check to make sure that the ElmerStructures
    /// are running as they should
    ///
    virtual void Test__ElmerStructures(ResultsType &result){
    	bool works = true;
	
    	std::cout << "Initalizing Structure..." << std::endl;
    	Test__fsiCouplerAgentsInitialize();
    	int initresult = TeststructuresAgent->Initialize(componentInterfaceNames[1], verblevel);
    	std::cout << "Initialization Completed!" << std::endl;
    	
    	//Make sure the initializing doesn't return an error
    	if (initresult != 0){ 
    		std::cout << "structuresAgent->Initialize returns error. " <<
    		             "Refer to ElmerAgent.H for more information. " << std::endl;
    		works = false;
    	}
    	
    	//Test to make sure the node coordinates are populated
    	//More of an exercise to make sure the data is loaded correctly 
    	//face coordinates are not in use currently
    	std::vector<double> nodecoordinates(TeststructuresAgent->returnNodeCoords());
    	std::cout << "Node size is: " << nodecoordinates.size() << std::endl;
    	if (nodecoordinates.size() < 0){
    		std::cout << "There is a problem with obtaining the nodecoordinate data" <<
    		             "Refer to ElmerAgent.H for more information. " << std::endl;
    		works = false;
    	}
    	
    	double arr[306] = {0.1, 0.1, 0, 0, 0.1, 0, 0, 0.1, 0.02, 0.1, 0.1, 0.02, 0, 0.1, 0.04, 0.1, 0.1, 0.04, 0, 0.1, 0.06, 0.1, 0.1, 0.06, 0, 0.1, 0.08, 0.1, 0.1, 0.08, 0, 0.1, 0.1, 0.1, 0.1, 0.1, 0, 0.1, 0.12, 0.1, 0.1, 0.12, 0, 0.1, 0.14, 0.1, 0.1, 0.14, 0, 0.1, 0.16, 0.1, 0.1, 0.16, 0, 0.1, 0.18, 0.1, 0.1, 0.18, 0, 0.1, 0.2, 0.1, 0.1, 0.2, 0, 0.1, 0.22, 0.1, 0.1, 0.22, 0, 0.1, 0.24, 0.1, 0.1, 0.24, 0, 0.1, 0.26, 0.1, 0.1, 0.26, 0, 0.1, 0.28, 0.1, 0.1, 0.28, 0, 0.1, 0.3, 0.1, 0.1, 0.3, 0, 0.1, 0.32, 0.1, 0.1, 0.32, 0, 0.1, 0.34, 0.1, 0.1, 0.34, 0, 0.1, 0.36, 0.1, 0.1, 0.36, 0, 0.1, 0.38, 0.1, 0.1, 0.38, 0, 0.1, 0.4, 0.1, 0.1, 0.4, 0, 0.1, 0.42, 0.1, 0.1, 0.42, 0, 0.1, 0.44, 0.1, 0.1, 0.44, 0, 0.1, 0.46, 0.1, 0.1, 0.46, 0, 0.1, 0.48, 0.1, 0.1, 0.48, 0, 0.1, 0.5, 0.1, 0.1, 0.5, 0, 0.1, 0.52, 0.1, 0.1, 0.52, 0, 0.1, 0.54, 0.1, 0.1, 0.54, 0, 0.1, 0.56, 0.1, 0.1, 0.56, 0, 0.1, 0.58, 0.1, 0.1, 0.58, 0, 0.1, 0.6, 0.1, 0.1, 0.6, 0, 0.1, 0.62, 0.1, 0.1, 0.62, 0, 0.1, 0.64, 0.1, 0.1, 0.64, 0, 0.1, 0.66, 0.1, 0.1, 0.66, 0, 0.1, 0.68, 0.1, 0.1, 0.68, 0, 0.1, 0.7, 0.1, 0.1, 0.7, 0, 0.1, 0.72, 0.1, 0.1, 0.72, 0, 0.1, 0.74, 0.1, 0.1, 0.74, 0, 0.1, 0.76, 0.1, 0.1, 0.76, 0, 0.1, 0.78, 0.1, 0.1, 0.78, 0, 0.1, 0.8, 0.1, 0.1, 0.8, 0, 0.1, 0.82, 0.1, 0.1, 0.82, 0, 0.1, 0.84, 0.1, 0.1, 0.84, 0, 0.1, 0.86, 0.1, 0.1, 0.86, 0, 0.1, 0.88, 0.1, 0.1, 0.88, 0, 0.1, 0.9, 0.1, 0.1, 0.9, 0, 0.1, 0.92, 0.1, 0.1, 0.92, 0, 0.1, 0.94, 0.1, 0.1, 0.94, 0, 0.1, 0.96, 0.1, 0.1, 0.96, 0, 0.1, 0.98, 0.1, 0.1, 0.98, 0, 0.1, 1, 0.1, 0.1, 1};
    	
    	for (int i = 0; i < 306; i++){
    		if (arr[i] != nodecoordinates.at(i)){
    			std::cout << "Node Coordinate Inconsistencies." << std::endl;
    			works = false;
    			break;
    		}
    	}
    	result.UpdateResult("ElmerStructures:Works",works);
    	delete TeststructuresAgent;
    }
    
    
    ///
    /// Run an initialization check to make sure the OpenFoamFluids 
    /// are running as they should
    ///
    virtual void Test__OpenFoamFluids(ResultsType &result){
      bool works = true;
	
    	std::cout << "Initializing Fluids..." << std::endl;
    	Test__fsiCouplerAgentsInitialize();
    	int initresult = TestfluidsAgent->Initialize(componentInterfaceNames[0], verblevel);
    	std::cout << "Initialization Completed!" << std::endl;
    	
    	//Make sure the initializing doesn't return an error
    	if (initresult != 0){ 
    		std::cout << "fluidsAgent->Initialize returns error. " <<
    		             "Refer to OpenFoamAgent.H for more information. " << std::endl;
    		works = false;
    	}
    	
    	//Test to make sure the node coordinates are populated
    	//More of an exercise to make sure the data is loaded correctly 
    	//face coordinates are not in use currently
    	std::vector<double> nodecoordinates(TestfluidsAgent->returnNodeCoords());
    	std::cout << "Fluid Node size is: " << nodecoordinates.size() << std::endl;
    	if (nodecoordinates.size() < 0){
    		std::cout << "There is a problem with obtaining the nodecoordinate data" <<
    		             "Refer to OpenFoamAgent.H for more information. " << std::endl;
    		works = false;
    	}
    	
    	double arr[306] = {0.1, 0.1, 0, 0.1, 0.1, 0.02, 0, 0.1, 0.02, 0, 0.1, 0, 0.1, 0.1, 0.04, 0, 0.1, 0.04, 0.1, 0.1, 0.06, 0, 0.1, 0.06, 0.1, 0.1, 0.08, 0, 0.1, 0.08, 0.1, 0.1, 0.1, 0, 0.1, 0.1, 0.1, 0.1, 0.12, 0, 0.1, 0.12, 0.1, 0.1, 0.14, 0, 0.1, 0.14, 0.1, 0.1, 0.16, 0, 0.1, 0.16, 0.1, 0.1, 0.18, 0, 0.1, 0.18, 0.1, 0.1, 0.2, 0, 0.1, 0.2, 0.1, 0.1, 0.22, 0, 0.1, 0.22, 0.1, 0.1, 0.24, 0, 0.1, 0.24, 0.1, 0.1, 0.26, 0, 0.1, 0.26, 0.1, 0.1, 0.28, 0, 0.1, 0.28, 0.1, 0.1, 0.3, 0, 0.1, 0.3, 0.1, 0.1, 0.32, 0, 0.1, 0.32, 0.1, 0.1, 0.34, 0, 0.1, 0.34, 0.1, 0.1, 0.36, 0, 0.1, 0.36, 0.1, 0.1, 0.38, 0, 0.1, 0.38, 0.1, 0.1, 0.4, 0, 0.1, 0.4, 0.1, 0.1, 0.42, 0, 0.1, 0.42, 0.1, 0.1, 0.44, 0, 0.1, 0.44, 0.1, 0.1, 0.46, 0, 0.1, 0.46, 0.1, 0.1, 0.48, 0, 0.1, 0.48, 0.1, 0.1, 0.5, 0, 0.1, 0.5, 0.1, 0.1, 0.52, 0, 0.1, 0.52, 0.1, 0.1, 0.54, 0, 0.1, 0.54, 0.1, 0.1, 0.56, 0, 0.1, 0.56, 0.1, 0.1, 0.58, 0, 0.1, 0.58, 0.1, 0.1, 0.6, 0, 0.1, 0.6, 0.1, 0.1, 0.62, 0, 0.1, 0.62, 0.1, 0.1, 0.64, 0, 0.1, 0.64, 0.1, 0.1, 0.66, 0, 0.1, 0.66, 0.1, 0.1, 0.68, 0, 0.1, 0.68, 0.1, 0.1, 0.7, 0, 0.1, 0.7, 0.1, 0.1, 0.72, 0, 0.1, 0.72, 0.1, 0.1, 0.74, 0, 0.1, 0.74, 0.1, 0.1, 0.76, 0, 0.1, 0.76, 0.1, 0.1, 0.78, 0, 0.1, 0.78, 0.1, 0.1, 0.8, 0, 0.1, 0.8, 0.1, 0.1, 0.82, 0, 0.1, 0.82, 0.1, 0.1, 0.84, 0, 0.1, 0.84, 0.1, 0.1, 0.86, 0, 0.1, 0.86, 0.1, 0.1, 0.88, 0, 0.1, 0.88, 0.1, 0.1, 0.9, 0, 0.1, 0.9, 0.1, 0.1, 0.92, 0, 0.1, 0.92, 0.1, 0.1, 0.94, 0, 0.1, 0.94, 0.1, 0.1, 0.96, 0, 0.1, 0.96, 0.1, 0.1, 0.98, 0, 0.1, 0.98, 0.1, 0.1, 1, 0, 0.1, 1};
    	
    	for (int i = 0; i < 306; i++){
    		if (arr[i] != nodecoordinates.at(i)){
    			std::cout << "Node Coordinate Inconsistencies." << std::endl;
    			works = false;
    			break;
    		}
    	}
      result.UpdateResult("OpenFoamFluids:Works",works);
      delete TestfluidsAgent;
    }


    /// Test for fsiCoupling::TrasferDisplacementsToFluid function.
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function uses simple static problem data to test the transferagent->interpolate function.
    /// Displacement vector for solid agent is set to be the value of linear function along 
    /// the z direction. After interpolation, displacement vector for fluid agent should have 
    /// similar values.
    virtual void Test__TransferDisplacementsToFluid(ResultsType & result){

      double time_final = 0.05;
      double timestep = 2.0e-3;
      int verblevel = 3;
      bool works = true;
      int IntDir;

      // Copying the SimpleStatic data using getinputdata() helper function.
      bool work =__helper();
      std::string fluidDir;
      std::string simStaticDir;
      fluidDir = IRAD::Sys::CWD() + "/SimpleStatic/fluid";
      simStaticDir = IRAD::Sys::CWD() + "/SimpleStatic";
      if(!(IRAD::Sys::FILEEXISTS(fluidDir)||(IRAD::Sys::FILEEXISTS(simStaticDir)))){
        std::cout << "SimpleStatic / fluid Folders do not exists!" << std::endl;
        works = false;           
        result.UpdateResult("fsiCoupler:Works",works);
        return; 
      }
      else{
        IntDir = IRAD::Sys::ChDir(simStaticDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
        std::system("chmod 755 Allclean Allrun AllrunPar makeLinks makeSerialLinks removeSerialLinks"); 
        std::system("./Allclean");
        std::system("./Allrun");
        IntDir = IRAD::Sys::ChDir(fluidDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
      }
      
 
      fsicoupling fsiCoupler(global); 
      std::string fluidSolverName("OpenFoamFSI");
      std::string solidSolverName("ElmerCSC");
      std::string transferServiceName("SurfX");
      std::string runMode("1");


      std::cout << "COM loading all this junks" << std::endl;
      COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
      COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
      COM_load_module("SurfUtil","SurfUtil");
      COM_load_module("Simpal","Simpal");

     
      std::vector<std::string> componentInterfaceNames;
      componentInterfaceNames.push_back("FluidsComponentInterface");
      componentInterfaceNames.push_back("StructuresComponentInterface");
      componentInterfaceNames.push_back("TransferInterface");
      componentInterfaceNames.push_back("SurfUtil");
      componentInterfaceNames.push_back("Simpal");
 
      fsiCoupler.SetRunMode(runMode);
      fsiCoupler.SetVerbLevel(verblevel);
      
      fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);  

      // Mimicing FsiCoupler::run, intializing variables, etc.
      int innerCount = 0;
      int maxSubSteps = 1000;
      int dumpinterval = 1;
      int systemStep = 0;

	  double *fluidDisp1 = NULL;
	  double *fluidDisp2 = NULL;
	  double *solidDisp1 = NULL;
	  double *solidDisp2 = NULL;

 
      // Get the displacement vector values for both solid and fluid domain.
	  std::string fluidsCoordinateName(fsiCoupler.getFluidIntName()+".solidDisplacement");
	  std::string solidsCoordinateName(fsiCoupler.getSolidIntName()+".Displacements");
	  COM_get_array(fluidsCoordinateName.c_str(),(fsiCoupler.getFluidAgent())->PaneID(),&fluidDisp1);
	  COM_get_array(solidsCoordinateName.c_str(),(fsiCoupler.getStructureAgent())->PaneID(),&solidDisp1);


      // Setting Y-component of the displacement vector to be the linear function displacement 
      // along z direction throughout nodes
      for(int i = 0; i <fsiCoupler.getStructureAgent()->Coordinates().size()/3; i++){
        solidDisp1[3*i+1] = fsiCoupler.getStructureAgent()->Coordinates()[3*i+2];
      }
 
	  if(!(fsiCoupler.getRunMode())){
		fsiCoupler.TransferDisplacementsToFluid(fsiCoupler.getStructureAgent(),fsiCoupler.getFluidAgent());
	  }
      
      // After transferDisplacementToFluids, get displacement vectors for both agents to compare
      COM_get_array(fluidsCoordinateName.c_str(),(fsiCoupler.getFluidAgent())->PaneID(),&fluidDisp2);
      COM_get_array(solidsCoordinateName.c_str(),(fsiCoupler.getStructureAgent())->PaneID(),&solidDisp2);

      double totalFluidDis, totalSolidDis = 0;
      // Calculate the total displacements for solid displacements and fluid displacements after transfer
      for(int i=0; i < fsiCoupler.getStructureAgent()->Coordinates().size()/3; i++){
        totalFluidDis += fluidDisp2[3*i+1];
        totalSolidDis += solidDisp2[3*i+1];
      }
      
      std::cout << "total fluid dis = " << totalFluidDis << std::endl;
      std::cout << "total solid dis = " << totalSolidDis << std::endl;
    
      // Using Error % = (Experimented value - Theoretical value) / Theoretical value
      double totalError = (totalFluidDis - totalSolidDis) / totalSolidDis;
      // If the error % is greater than 5%, let's call it a fail.
      std::cout << "total error = " << totalError << std::endl;
      if(totalError  >= .05) works = false;
     
      result.UpdateResult("TransferDisplacementsToFluid:Works",works);
    }
    
    /// Test for fluidAgent->Run() function.
    virtual void Test__FluidAgentRun(ResultsType &result){
    
      double time_final = 0.05;
      double timestep = 2.0e-3;
      int verblevel = 3;
      bool works = true;
      int IntDir;

      // Copying the SimpleStatic data using getinputdata() helper function.
      bool work =__helper();
      std::string fluidDir;
      std::string simStaticDir;
      fluidDir = IRAD::Sys::CWD() + "/SimpleStatic/fluid";
      simStaticDir = IRAD::Sys::CWD() + "/SimpleStatic";
      if(!(IRAD::Sys::FILEEXISTS(fluidDir)||(IRAD::Sys::FILEEXISTS(simStaticDir)))){
        std::cout << "SimpleStatic / fluid Folders do not exists!" << std::endl;
        works = false;           
        result.UpdateResult("fsiCoupler:Works",works);
        return; 
      }
      else{
        IntDir = IRAD::Sys::ChDir(simStaticDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
        std::system("chmod 755 Allclean Allrun AllrunPar makeLinks makeSerialLinks removeSerialLinks"); 
        std::system("./Allclean");
        std::system("./Allrun");
        IntDir = IRAD::Sys::ChDir(fluidDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
      }
      
      fsicoupling fsiCoupler(global); 
      std::string fluidSolverName("OpenFoamFSI");
      std::string solidSolverName("ElmerCSC");
      std::string transferServiceName("SurfX");
      std::string runMode("1");

      std::cout << "COM loading all this junks" << std::endl;
      COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
      COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
      COM_load_module("SurfUtil","SurfUtil");
      COM_load_module("Simpal","Simpal");
     
      std::vector<std::string> componentInterfaceNames;
      componentInterfaceNames.push_back("FluidsComponentInterface");
      componentInterfaceNames.push_back("StructuresComponentInterface");
      componentInterfaceNames.push_back("TransferInterface");
      componentInterfaceNames.push_back("SurfUtil");
      componentInterfaceNames.push_back("Simpal");
 
      fsiCoupler.SetRunMode(runMode);
      fsiCoupler.SetVerbLevel(verblevel);      
      fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);  

      // Mimicing FsiCoupler::run, intializing variables, etc.
      int innerCount = 0;
      int maxSubSteps = 1000;
      int dumpinterval = 1;
      int systemStep = 0;

	  double *fluidDisp1 = NULL;
	  double *fluidDisp2 = NULL;
	  double *solidDisp1 = NULL;
	  double *solidDisp2 = NULL;
 
      // Get the displacement vector values for both solid and fluid domain.
	  std::string fluidsCoordinateName(fsiCoupler.getFluidIntName()+".solidDisplacement");
	  std::string solidsCoordinateName(fsiCoupler.getSolidIntName()+".Displacements");
	  COM_get_array(fluidsCoordinateName.c_str(),(fsiCoupler.getFluidAgent())->PaneID(),&fluidDisp1);
	  COM_get_array(solidsCoordinateName.c_str(),(fsiCoupler.getStructureAgent())->PaneID(),&solidDisp1);

	  if(!(fsiCoupler.getRunMode())){
		fsiCoupler.TransferDisplacementsToFluid(fsiCoupler.getStructureAgent(),fsiCoupler.getFluidAgent());
	  }
      double simultime = fsiCoupler.getSimulationTime();

     
      std::cout << "simultime is =" << simultime << std::endl;
      if(fsiCoupler.getRunMode() < 2){
        fsiCoupler.getFluidAgent()->InitializeTimeStep(simultime);
        fsiCoupler.getFluidAgent()->Run(fsiCoupler.getSimulationTime()+fsiCoupler.getSimulationFinalTime());
      } 

      int stride = 0;
      int cap = 0;
      double *tractions = NULL;
      COM_get_array((fsiCoupler.getFluidIntName()+".traction").c_str(),101,&tractions,&stride,&cap);
      int isize = cap*stride;
      
      /*for(int i = 0; i < isize/3 ; i++){
		std::cout << "tractions[" << i << "] = " << tractions[3*i] << ",  " << tractions[3*i+1] << ",  " << tractions[3*i+2] << std::endl;
      } */
          
      works = tractionCheck(tractions,isize);

      result.UpdateResult("FluidAgentRun:Works",works);
    }
    bool tractionCheck(double * tractions, int isize){
      double tractionSave[isize];

      // Hard coding the value
      tractionSave[0] = 0;
	  tractionSave[1] = -1;
	  tractionSave[2] = 2.63292e-18;
	  tractionSave[3] = 0;
	  tractionSave[4] = -1;
	  tractionSave[5] = 1.32171e-18;
	  tractionSave[6] = 0;
	  tractionSave[7] = -1;
	  tractionSave[8] = 3.64051e-21;
	  tractionSave[9] = 0;
	  tractionSave[10] = -1;
	  tractionSave[11] = 6.63492e-19;
	  tractionSave[12] = 0;
	  tractionSave[13] = -1;
	  tractionSave[14] = 1.98684e-18;
	  tractionSave[15] = 0;
	  tractionSave[16] = -1;
	  tractionSave[17] = 1.98606e-18;
	  tractionSave[18] = 0;
	  tractionSave[19] = -1;
	  tractionSave[20] = 1.98645e-18;
	  tractionSave[21] = 0;
	  tractionSave[22] = -1;
	  tractionSave[23] = 1.32296e-18;
	  tractionSave[24] = 0;
	  tractionSave[25] = -1;
	  tractionSave[26] = 6.63492e-19;
	  tractionSave[27] = 0;
	  tractionSave[28] = -1;
	  tractionSave[29] = 1.97793e-18;
	  tractionSave[30] = 0;
	  tractionSave[31] = -1;
	  tractionSave[32] = 1.31444e-18;
	  tractionSave[33] = 0;
	  tractionSave[34] = -1;
	  tractionSave[35] = 1.31606e-18;
	  tractionSave[36] = 0;
	  tractionSave[37] = -1;
	  tractionSave[38] = 1.31808e-18;
	  tractionSave[39] = 0;
	  tractionSave[40] = -1;
	  tractionSave[41] = 6.61478e-19;
	  tractionSave[42] = 0;
	  tractionSave[43] = -1;
	  tractionSave[44] = 2.01408e-21;
	  tractionSave[45] = 0;
	  tractionSave[46] = -1;
	  tractionSave[47] = -6.56599e-19;
	  tractionSave[48] = 0;
	  tractionSave[49] = -1;
	  tractionSave[50] = -6.58225e-19;
	  tractionSave[51] = 0;
	  tractionSave[52] = -1;
	  tractionSave[53] = -6.56211e-19;
	  tractionSave[54] = 0;
	  tractionSave[55] = -1;
	  tractionSave[56] = -6.58225e-19;
	  tractionSave[57] = 0;
	  tractionSave[58] = -1;
	  tractionSave[59] = 6.65506e-19;
	  tractionSave[60] = 0;
	  tractionSave[61] = -1;
	  tractionSave[62] = 1.32172e-18;
	  tractionSave[63] = 0;
	  tractionSave[64] = -1;
	  tractionSave[65] = 6.61478e-19;
	  tractionSave[66] = 0;
	  tractionSave[67] = -1;
	  tractionSave[68] = 2.32349e-79;
	  tractionSave[69] = 0;
	  tractionSave[70] = -1;
	  tractionSave[71] = 2.01408e-21;
	  tractionSave[72] = 0;
	  tractionSave[73] = -1;
	  tractionSave[74] = 1.55135e-84;
	  tractionSave[75] = 0;
	  tractionSave[76] = -1;
	  tractionSave[77] = -2.01408e-21;
	  tractionSave[78] = 0;
	  tractionSave[79] = -1;
	  tractionSave[80] = -6.56599e-19;
	  tractionSave[81] = 0;
	  tractionSave[82] = -1;
	  tractionSave[83] = -6.4893e-19;
	  tractionSave[84] = 0;
	  tractionSave[85] = -1;
	  tractionSave[86] = -6.43663e-19;
	  tractionSave[87] = 0;
	  tractionSave[88] = -1;
	  tractionSave[89] = -1.96577e-18;
	  tractionSave[90] = 0;
	  tractionSave[91] = -1;
	  tractionSave[92] = -6.44051e-19;
	  tractionSave[93] = 0;
	  tractionSave[94] = -1;
	  tractionSave[95] = 1.05339e-20;
	  tractionSave[96] = 0;
	  tractionSave[97] = -1;
	  tractionSave[98] = 6.89339e-21;
	  tractionSave[99] = 0;
	  tractionSave[100] = -1;
	  tractionSave[101] = -6.59852e-19;
	  tractionSave[102] = 0;
	  tractionSave[103] = -1;
	  tractionSave[104] = -6.66745e-19;
	  tractionSave[105] = 0;
	  tractionSave[106] = -1;
	  tractionSave[107] = -6.72787e-19;
	  tractionSave[108] = 0;
	  tractionSave[109] = -1;
	  tractionSave[110] = -6.75652e-19;
	  tractionSave[111] = 0;
	  tractionSave[112] = -1;
	  tractionSave[113] = -1.29356e-20;
	  tractionSave[114] = 0;
	  tractionSave[115] = -1;
	  tractionSave[116] = -1.33349e-18;
	  tractionSave[117] = 0;
	  tractionSave[118] = -1;
	  tractionSave[119] = -2.64994e-18;
	  tractionSave[120] = 0;
	  tractionSave[121] = -1;
	  tractionSave[122] = -3.31142e-18;
	  tractionSave[123] = 0;
	  tractionSave[124] = -1;
	  tractionSave[125] = -1.98645e-18;
	  tractionSave[126] = 0;
	  tractionSave[127] = -1;
	  tractionSave[128] = -1.32296e-18;
	  tractionSave[129] = 0;
	  tractionSave[130] = -1;
	  tractionSave[131] = 8.21297e-134;
	  tractionSave[132] = 0;
	  tractionSave[133] = -1;
	  tractionSave[134] = 2.07766e-136;
	  tractionSave[135] = 0;
	  tractionSave[136] = -1;
	  tractionSave[137] = -5.26695e-21;
	  tractionSave[138] = 0;
	  tractionSave[139] = -1;
	  tractionSave[140] = -6.61478e-19;
	  tractionSave[141] = 0;
	  tractionSave[142] = -1;
	  tractionSave[143] = -6.89339e-21;
	  tractionSave[144] = 0;
	  tractionSave[145] = -1;
	  tractionSave[146] = -8.89947e-21;
	  tractionSave[147] = 0;
	  tractionSave[148] = -1;
	  tractionSave[149] = -6.6386e-19;
       
      //When comparing double, exact comparison might not do the job. Hence im giving 5% error margin. 
      for(int i = 0; i < isize; i++){
        if( !(double_equals(tractions[i],tractionSave[i]))){
          return false;
        }
      }
      return true;
    }
    // Helper function to compare two doubles with allowed margin of error (.1%)
    bool double_equals(double a, double b, double epsilon = .001){
      return (std::abs(a-b) < epsilon);
    }

    // Helper function that computes the average coordinates of a point, given four coordinates points
    std::vector<double> avgCoord(int first, int second, int third, int fourth, fluidagent*  fluidAgent ){
      std::vector<double> Cord(3);
      // X direction
	  Cord[0] = (fluidAgent->Coordinates()[3*first] + fluidAgent->Coordinates()[3*second] + fluidAgent->Coordinates()[3*third] + fluidAgent->Coordinates()[3*fourth] ) / 4; 
      // Y direction
      Cord[1] = (fluidAgent->Coordinates()[3*first+1] + fluidAgent->Coordinates()[3*second+1] + fluidAgent->Coordinates()[3*third+1] + fluidAgent->Coordinates()[3*fourth+1] ) / 4;
      // Z direction
      Cord[2] = (fluidAgent->Coordinates()[3*first+2] + fluidAgent->Coordinates()[3*second+2] + fluidAgent->Coordinates()[3*third+2] + fluidAgent->Coordinates()[3*fourth+2] ) / 4;
      return Cord;
    }


    /// Test for TransferLoadsToStructures() function.
    virtual void Test__TransferLoadsToStructures(ResultsType &result){
    
      double time_final = 0.05;
      double timestep = 2.0e-3;
      int verblevel = 3;
      bool works = true;
      int IntDir;

      // Copying the SimpleStatic data using getinputdata() helper function.
      bool work =__helper();
      std::string fluidDir;
      std::string simStaticDir;
      fluidDir = IRAD::Sys::CWD() + "/SimpleStatic/fluid";
      simStaticDir = IRAD::Sys::CWD() + "/SimpleStatic";
      if(!(IRAD::Sys::FILEEXISTS(fluidDir)||(IRAD::Sys::FILEEXISTS(simStaticDir)))){
        std::cout << "SimpleStatic / fluid Folders do not exists!" << std::endl;
        works = false;           
        result.UpdateResult("fsiCoupler:Works",works);
        return; 
      }
      else{
        IntDir = IRAD::Sys::ChDir(simStaticDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
        std::system("chmod 755 Allclean Allrun AllrunPar makeLinks makeSerialLinks removeSerialLinks"); 
        std::system("./Allclean");
        std::system("./Allrun");
        IntDir = IRAD::Sys::ChDir(fluidDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
      }
      
      fsicoupling fsiCoupler(global); 
      std::string fluidSolverName("OpenFoamFSI");
      std::string solidSolverName("ElmerCSC");
      std::string transferServiceName("SurfX");
      std::string runMode("1");

      COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
      COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
      COM_load_module("SurfUtil","SurfUtil");
      COM_load_module("Simpal","Simpal");
     
      std::vector<std::string> componentInterfaceNames;
      componentInterfaceNames.push_back("FluidsComponentInterface");
      componentInterfaceNames.push_back("StructuresComponentInterface");
      componentInterfaceNames.push_back("TransferInterface");
      componentInterfaceNames.push_back("SurfUtil");
      componentInterfaceNames.push_back("Simpal");
 
      fsiCoupler.SetRunMode(runMode);
      fsiCoupler.SetVerbLevel(verblevel);      
      fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);  

      // Mimicing FsiCoupler::run, intializing variables, etc.
      int innerCount = 0;
      int maxSubSteps = 1000;
      int dumpinterval = 1;
      int systemStep = 0;

 
	  if(!(fsiCoupler.getRunMode())){
		fsiCoupler.TransferDisplacementsToFluid(fsiCoupler.getStructureAgent(),fsiCoupler.getFluidAgent());
	  }
      double simultime = fsiCoupler.getSimulationTime();

     
      std::cout << "simultime is =" << simultime << std::endl;
      if(fsiCoupler.getRunMode() < 2){
        fsiCoupler.getFluidAgent()->InitializeTimeStep(simultime);
        fsiCoupler.getFluidAgent()->Run(fsiCoupler.getSimulationTime()+fsiCoupler.getSimulationFinalTime());
      }
     
      int * conn= NULL;
      int  connStride, connCap=0;
      COM_get_array( (fsiCoupler.getFluidIntName()+".:q4:").c_str(),101, &conn,&connStride,&connCap);
      std::cout << "connStride , connCap = " << connStride << ", " << connCap << std::endl;
      for (int i =0; i < connCap; i++){
         std::cout << "Conn Coordinates[" << i << "] = " << conn[4*i] << ", " << conn[4*i+1] << ", " << conn[4*i+2] << ", " << conn[4*i+3] << std::endl;
      }


     
      std::vector<double> cellCoordVec(connCap*3);
      std::vector<double> avgCoor(3);
      for(int i =0; i < connCap; i++){ 
		  
          avgCoor = avgCoord(conn[4*i]-1,conn[4*i+1]-1,conn[4*i+2]-1,conn[4*i+3]-1, fsiCoupler.getFluidAgent());
		  cellCoordVec[3*i] = avgCoor[0];
		  cellCoordVec[3*i+1] = avgCoor[1];
		  cellCoordVec[3*i+2] = avgCoor[2];
		  std::cout << "CellCoordVec[" << i << "] = " << cellCoordVec[3*i] << ", " << cellCoordVec[3*i+1] << ", " << cellCoordVec[3*i+2] << std::endl;
		  avgCoor.clear();
      }
 

      double * tractions;
      int stride = 0;
      int cap = 0;
      // Get the traction vector values for fluid domain.
	  COM_get_array((fsiCoupler.getFluidIntName()+".traction").c_str(),101,&tractions,&stride,&cap);
      std::cout << "stride and cap = " << stride << "," << cap << std::endl;
      //traction becomes Ycoord * 3 + 2
      int isize = cap * stride ;
      for(int i = 0; i < isize/3 ; i++){
        //tractions[3*i+1] = 3*fsiCoupler.getFluidAgent()->Coordinates()[3*i+2]+2;
        //tractions[3*i+1] = fsiCoupler.getFluidAgent()->Coordinates()[3*i+2];
        //tractions[3*i+1] = 1;
        tractions[3*i+1] = cellCoordVec[3*i+2];
		std::cout << "tractions[" << i << "] = " << tractions[3*i] << ",  " << tractions[3*i+1] << ",  " << tractions[3*i+2] << ",  Coordinates are = " << cellCoordVec[3*i] << ", " << cellCoordVec[3*i+1] << ", " << cellCoordVec[3*i+2] <<  std::endl;
      }
 
      double * solidLoads1 = NULL;
      int solidLoadStride1 = 0;
      int solidLoadCap1 =0;
      // Get the load vector from solid domain
      COM_get_array((fsiCoupler.getSolidIntName()+".Loads").c_str(),11,&solidLoads1,&solidLoadStride1,&solidLoadCap1);
      int solidLoadSize1 = solidLoadCap1*solidLoadStride1;
      
      // Execute Transfer LoadsToStructures
      if(!fsiCoupler.getRunMode()) fsiCoupler.TransferLoadsToStructures(fsiCoupler.getFluidAgent(), fsiCoupler.getStructureAgent());
  
      double * solidLoads = NULL;
      int solidLoadStride = 0;
      int solidLoadCap =0;
      // Get the load vector from solid domain
      COM_get_array((fsiCoupler.getSolidIntName()+".Loads").c_str(),11,&solidLoads,&solidLoadStride,&solidLoadCap);
      int solidLoadSize = solidLoadCap*solidLoadStride;
      
      std::cout << "solidLoadCap = " << solidLoadCap << ", solidLoadStride = " << solidLoadStride << std::endl;
      for(int i = 0; i < solidLoadSize/3; i++){
        std::cout << "solidLoads(" << i <<") = " << solidLoads[3*i] << ", " << solidLoads[3*i+1] << ", " << solidLoads[3*i+2] << " Where coordinate is = " << fsiCoupler.getStructureAgent()->Coordinates()[3*i] << ", " << fsiCoupler.getStructureAgent()->Coordinates()[3*i+1] << ", " << fsiCoupler.getStructureAgent()->Coordinates()[3*i+2]  << std::endl;   
      }


      double totalTraction, totalLoad = 0;
      for(int i = 0; i < solidLoadSize; i++)  totalLoad += solidLoads[i];
      totalLoad = totalLoad/2;
      for(int i = 0; i < isize/3 ; i++) totalTraction +=tractions[3*i+1];   
      
      
      std::cout << "total loads = " << totalLoad << std::endl;
      std::cout << "total tractions = " << totalTraction << std::endl;
    
      // Using Error % = (Experimented value - Theoretical value) / Theoretical value
      double totalError = (totalLoad - totalTraction) / totalTraction;
      // If the error % is greater than 5%, let's call it a fail.
      std::cout << "total error = " << totalError << std::endl;
      if(totalError  >= .05) works = false;

      double maxError = 0;
      double errr = 0;
      for(int i = 0; i < isize/3 ; i++){
        for(int j = 0; j <= 9; j = j + 3){
          //std::cout << "tractions = " << tractions[3*i+1] << std::endl;
          //std::cout << "solidLoads = " << solidLoads[7*i+1-i+j] << std::endl;
          errr = std::abs( (  solidLoads[7*i+1-i+j] - tractions[3*i+1]) / tractions[3*i+1] );
          if(errr >= maxError){
            maxError = errr;
            std::cout << "maxError = " << maxError << std::endl;
            std::cout << "Happens at i,j = " << i << ", " << j << std::endl;
          }
        }
      }
      std::cout << "Max error = " << maxError << std::endl;
      result.UpdateResult("TransferLoadsToStructures:Works",works);
    }


    /// Test for solidAgent->Run() function.
    virtual void Test__SolidAgentRun(ResultsType &result){

    
      double time_final = 0.05;
      double timestep = 2.0e-3;
      int verblevel = 3;
      bool works = true;
      int IntDir;

      // Copying the SimpleStatic data using getinputdata() helper function.
      bool work =__helper();
      std::string fluidDir;
      std::string simStaticDir;
      fluidDir = IRAD::Sys::CWD() + "/SimpleStatic/fluid";
      simStaticDir = IRAD::Sys::CWD() + "/SimpleStatic";
      if(!(IRAD::Sys::FILEEXISTS(fluidDir)||(IRAD::Sys::FILEEXISTS(simStaticDir)))){
        std::cout << "SimpleStatic / fluid Folders do not exists!" << std::endl;
        works = false;           
        result.UpdateResult("fsiCoupler:Works",works);
        return; 
      }
      else{
        IntDir = IRAD::Sys::ChDir(simStaticDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
        std::system("chmod 755 Allclean Allrun AllrunPar makeLinks makeSerialLinks removeSerialLinks"); 
        std::system("./Allclean");
        std::system("./Allrun");
        IntDir = IRAD::Sys::ChDir(fluidDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
      }
      
      fsicoupling fsiCoupler(global); 
      std::string fluidSolverName("OpenFoamFSI");
      std::string solidSolverName("ElmerCSC");
      std::string transferServiceName("SurfX");
      std::string runMode("1");

      COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
      COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
      COM_load_module("SurfUtil","SurfUtil");
      COM_load_module("Simpal","Simpal");
     
      std::vector<std::string> componentInterfaceNames;
      componentInterfaceNames.push_back("FluidsComponentInterface");
      componentInterfaceNames.push_back("StructuresComponentInterface");
      componentInterfaceNames.push_back("TransferInterface");
      componentInterfaceNames.push_back("SurfUtil");
      componentInterfaceNames.push_back("Simpal");
 
      fsiCoupler.SetRunMode(runMode);
      fsiCoupler.SetVerbLevel(verblevel);      
      fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);  

      // Mimicing FsiCoupler::run, intializing variables, etc.
      int innerCount = 0;
      int maxSubSteps = 1000;
      int dumpinterval = 1;
      int systemStep = 0;

 
	  if(!(fsiCoupler.getRunMode())){
		fsiCoupler.TransferDisplacementsToFluid(fsiCoupler.getStructureAgent(),fsiCoupler.getFluidAgent());
	  }
      double simultime = fsiCoupler.getSimulationTime();

     
      std::cout << "simultime is =" << simultime << std::endl;
      if(fsiCoupler.getRunMode() < 2){
        fsiCoupler.getFluidAgent()->InitializeTimeStep(simultime);
        fsiCoupler.getFluidAgent()->Run(fsiCoupler.getSimulationTime()+fsiCoupler.getSimulationFinalTime());
      }

      // setting a traction value so that you have bigger numbers for solid displacement later
      double * tractions;
      int stride = 0;
      int cap = 0;
      // Get the traction vector values for fluid domain.
	  COM_get_array((fsiCoupler.getFluidIntName()+".traction").c_str(),101,&tractions,&stride,&cap);
      std::cout << "stride and cap = " << stride << "," << cap << std::endl;
      //traction becomes Ycoord * 3 + 2
      int isize = cap * stride ;
      for(int i = 0; i < isize/3 ; i++){
        //tractions[3*i+1] = 3*fsiCoupler.getFluidAgent()->Coordinates()[3*i+2]+2;
        //tractions[3*i+1] = fsiCoupler.getFluidAgent()->Coordinates()[3*i+2];
        //tractions[3*i+1] = 1;
        tractions[3*i+1] = 50000;
		std::cout << "tractions[" << i << "] = " << tractions[3*i] << ",  " << tractions[3*i+1] << ",  " << tractions[3*i+2] << std::endl;
      } 


      if(!fsiCoupler.getRunMode()) fsiCoupler.TransferLoadsToStructures(fsiCoupler.getFluidAgent(), fsiCoupler.getStructureAgent());


      // Solid Agent Run executing
      if( fsiCoupler.getRunMode() !=1){
        fsiCoupler.getStructureAgent()->InitializeTimeStep(simultime);
        fsiCoupler.getStructureAgent()->Run(fsiCoupler.getSimulationTime()+fsiCoupler.getSimulationFinalTime());
      }     

	  double *solidDisp = NULL;
 
      // Get the displacement vector values for solid.
	  std::string solidsCoordinateName(fsiCoupler.getSolidIntName()+".Displacements");
	  COM_get_array(solidsCoordinateName.c_str(),(fsiCoupler.getStructureAgent())->PaneID(),&solidDisp);
      
      
      works = DisplacementCheck(solidDisp, 300);
      result.UpdateResult("SolidAgentRun:Works",works);
    }

    bool DisplacementCheck(double * disp, int size){
      double solidDisSave[size];

      // Hard coding the value	 
	  solidDisSave[0] = 6.37522e-28;
	  solidDisSave[1] = 0.000298136;
	  solidDisSave[2] = 5.90759e-05;
	  solidDisSave[3] = 6.37522e-28;
	  solidDisSave[4] = 0.000298136;
	  solidDisSave[5] = 5.90759e-05;
	  solidDisSave[6] = 4.50796e-28;
	  solidDisSave[7] = 0.000291629;
	  solidDisSave[8] = 5.25282e-05;
	  solidDisSave[9] = 4.50796e-28;
	  solidDisSave[10] = 0.000291629;
	  solidDisSave[11] = 5.25282e-05;
	  solidDisSave[12] = 4.50796e-28;
	  solidDisSave[13] = 0.000286197;
	  solidDisSave[14] = 4.63341e-05;
	  solidDisSave[15] = 8.55785e-28;
	  solidDisSave[16] = 0.000286197;
	  solidDisSave[17] = 4.63341e-05;
	  solidDisSave[18] = 4.50796e-28;
	  solidDisSave[19] = 0.000282032;
	  solidDisSave[20] = 4.07105e-05;
	  solidDisSave[21] = 4.50796e-28;
	  solidDisSave[22] = 0.000282032;
	  solidDisSave[23] = 4.07105e-05;
	  solidDisSave[24] = 4.50796e-28;
	  solidDisSave[25] = 0.000278934;
	  solidDisSave[26] = 3.56933e-05;
	  solidDisSave[27] = 4.50796e-28;
	  solidDisSave[28] = 0.000278934;
	  solidDisSave[29] = 3.56933e-05;
	  solidDisSave[30] = 4.50796e-28;
	  solidDisSave[31] = 0.0002767;
	  solidDisSave[32] = 3.12723e-05;
	  solidDisSave[33] = 8.55785e-28;
	  solidDisSave[34] = 0.0002767;
	  solidDisSave[35] = 3.12723e-05;
	  solidDisSave[36] = 4.50796e-28;
	  solidDisSave[37] = 0.000275146;
	  solidDisSave[38] = 2.74114e-05;
	  solidDisSave[39] = 4.50796e-28;
	  solidDisSave[40] = 0.000275146;
	  solidDisSave[41] = 2.74114e-05;
	  solidDisSave[42] = 4.50796e-28;
	  solidDisSave[43] = 0.000274116;
	  solidDisSave[44] = 2.40609e-05;
	  solidDisSave[45] = 4.50796e-28;
	  solidDisSave[46] = 0.000274116;
	  solidDisSave[47] = 2.40609e-05;
	  solidDisSave[48] = 8.55785e-28;
	  solidDisSave[49] = 0.000273477;
	  solidDisSave[50] = 2.11655e-05;
	  solidDisSave[51] = 4.50796e-28;
	  solidDisSave[52] = 0.000273477;
	  solidDisSave[53] = 2.11655e-05;
	  solidDisSave[54] = 8.55785e-28;
	  solidDisSave[55] = 0.000273123;
	  solidDisSave[56] = 1.86691e-05;
	  solidDisSave[57] = 4.50796e-28;
	  solidDisSave[58] = 0.000273123;
	  solidDisSave[59] = 1.86691e-05;
	  solidDisSave[60] = 4.50796e-28;
	  solidDisSave[61] = 0.00027297;
	  solidDisSave[62] = 1.65176e-05;
	  solidDisSave[63] = 4.50796e-28;
	  solidDisSave[64] = 0.00027297;
	  solidDisSave[65] = 1.65176e-05;
	  solidDisSave[66] = 4.50796e-28;
	  solidDisSave[67] = 0.000272951;
	  solidDisSave[68] = 1.46611e-05;
	  solidDisSave[69] = 4.50796e-28;
	  solidDisSave[70] = 0.000272951;
	  solidDisSave[71] = 1.46611e-05;
	  solidDisSave[72] = 4.50796e-28;
	  solidDisSave[73] = 0.000273019;
	  solidDisSave[74] = 1.30543e-05;
	  solidDisSave[75] = 4.50796e-28;
	  solidDisSave[76] = 0.000273019;
	  solidDisSave[77] = 1.30543e-05;
	  solidDisSave[78] = 4.50796e-28;
	  solidDisSave[79] = 0.000273137;
	  solidDisSave[80] = 1.16571e-05;
	  solidDisSave[81] = 4.50796e-28;
	  solidDisSave[82] = 0.000273137;
	  solidDisSave[83] = 1.16571e-05;
	  solidDisSave[84] = 4.50796e-28;
	  solidDisSave[85] = 0.000273281;
	  solidDisSave[86] = 1.04348e-05;
	  solidDisSave[87] = 8.55785e-28;
	  solidDisSave[88] = 0.000273281;
	  solidDisSave[89] = 1.04348e-05;
	  solidDisSave[90] = 8.55785e-28;
	  solidDisSave[91] = 0.000273436;
	  solidDisSave[92] = 9.35745e-06;
	  solidDisSave[93] = 8.55785e-28;
	  solidDisSave[94] = 0.000273436;
	  solidDisSave[95] = 9.35745e-06;
	  solidDisSave[96] = 8.55785e-28;
	  solidDisSave[97] = 0.000273593;
	  solidDisSave[98] = 8.39999e-06;
	  solidDisSave[99] = 4.50796e-28;
	  solidDisSave[100] = 0.000273593;
	  solidDisSave[101] = 8.39999e-06;
	  solidDisSave[102] = 8.55785e-28;
	  solidDisSave[103] = 0.000273749;
	  solidDisSave[104] = 7.54154e-06;
	  solidDisSave[105] = 8.55785e-28;
	  solidDisSave[106] = 0.000273749;
	  solidDisSave[107] = 7.54154e-06;
	  solidDisSave[108] = 8.55785e-28;
	  solidDisSave[109] = 0.000273907;
	  solidDisSave[110] = 6.76512e-06;
	  solidDisSave[111] = 8.55785e-28;
	  solidDisSave[112] = 0.000273907;
	  solidDisSave[113] = 6.76512e-06;
	  solidDisSave[114] = 4.50796e-28;
	  solidDisSave[115] = 0.000274069;
	  solidDisSave[116] = 6.05734e-06;
	  solidDisSave[117] = 4.50796e-28;
	  solidDisSave[118] = 0.000274069;
	  solidDisSave[119] = 6.05734e-06;
	  solidDisSave[120] = 8.55785e-28;
	  solidDisSave[121] = 0.000274242;
	  solidDisSave[122] = 5.40816e-06;
	  solidDisSave[123] = 4.50796e-28;
	  solidDisSave[124] = 0.000274242;
	  solidDisSave[125] = 5.40816e-06;
	  solidDisSave[126] = 8.55785e-28;
	  solidDisSave[127] = 0.000274433;
	  solidDisSave[128] = 4.81074e-06;
	  solidDisSave[129] = 4.50796e-28;
	  solidDisSave[130] = 0.000274433;
	  solidDisSave[131] = 4.81074e-06;
	  solidDisSave[132] = 4.50796e-28;
	  solidDisSave[133] = 0.000274649;
	  solidDisSave[134] = 4.26143e-06;
	  solidDisSave[135] = 8.55785e-28;
	  solidDisSave[136] = 0.000274649;
	  solidDisSave[137] = 4.26143e-06;
	  solidDisSave[138] = 8.55785e-28;
	  solidDisSave[139] = 0.000274897;
	  solidDisSave[140] = 3.75988e-06;
	  solidDisSave[141] = 8.55785e-28;
	  solidDisSave[142] = 0.000274897;
	  solidDisSave[143] = 3.75988e-06;
	  solidDisSave[144] = 4.50796e-28;
	  solidDisSave[145] = 0.000275182;
	  solidDisSave[146] = 3.30926e-06;
	  solidDisSave[147] = 4.50796e-28;
	  solidDisSave[148] = 0.000275182;
	  solidDisSave[149] = 3.30926e-06;
	  solidDisSave[150] = 4.50796e-28;
	  solidDisSave[151] = 0.000275506;
	  solidDisSave[152] = 2.91661e-06;
	  solidDisSave[153] = 4.50796e-28;
	  solidDisSave[154] = 0.000275506;
	  solidDisSave[155] = 2.91661e-06;
	  solidDisSave[156] = 4.50796e-28;
	  solidDisSave[157] = 0.000275867;
	  solidDisSave[158] = 2.59325e-06;
	  solidDisSave[159] = 4.50796e-28;
	  solidDisSave[160] = 0.000275867;
	  solidDisSave[161] = 2.59325e-06;
	  solidDisSave[162] = 4.50796e-28;
	  solidDisSave[163] = 0.000276258;
	  solidDisSave[164] = 2.3553e-06;
	  solidDisSave[165] = 8.55785e-28;
	  solidDisSave[166] = 0.000276258;
	  solidDisSave[167] = 2.3553e-06;
	  solidDisSave[168] = 4.50796e-28;
	  solidDisSave[169] = 0.000276662;
	  solidDisSave[170] = 2.22417e-06;
	  solidDisSave[171] = 4.50796e-28;
	  solidDisSave[172] = 0.000276662;
	  solidDisSave[173] = 2.22417e-06;
	  solidDisSave[174] = 4.50796e-28;
	  solidDisSave[175] = 0.000277053;
	  solidDisSave[176] = 2.22707e-06;
	  solidDisSave[177] = 8.55785e-28;
	  solidDisSave[178] = 0.000277053;
	  solidDisSave[179] = 2.22707e-06;
	  solidDisSave[180] = 8.55785e-28;
	  solidDisSave[181] = 0.000277391;
	  solidDisSave[182] = 2.3973e-06;
	  solidDisSave[183] = 4.50796e-28;
	  solidDisSave[184] = 0.000277391;
	  solidDisSave[185] = 2.3973e-06;
	  solidDisSave[186] = 4.50796e-28;
	  solidDisSave[187] = 0.000277621;
	  solidDisSave[188] = 2.77441e-06;
	  solidDisSave[189] = 4.50796e-28;
	  solidDisSave[190] = 0.000277621;
	  solidDisSave[191] = 2.77441e-06;
	  solidDisSave[192] = 8.55785e-28;
	  solidDisSave[193] = 0.000277665;
	  solidDisSave[194] = 3.40386e-06;
	  solidDisSave[195] = 4.50796e-28;
	  solidDisSave[196] = 0.000277665;
	  solidDisSave[197] = 3.40386e-06;
	  solidDisSave[198] = 4.50796e-28;
	  solidDisSave[199] = 0.000277422;
	  solidDisSave[200] = 4.33615e-06;
	  solidDisSave[201] = 4.50796e-28;
	  solidDisSave[202] = 0.000277422;
	  solidDisSave[203] = 4.33615e-06;
	  solidDisSave[204] = 4.50796e-28;
	  solidDisSave[205] = 0.000276762;
	  solidDisSave[206] = 5.62512e-06;
	  solidDisSave[207] = 4.50796e-28;
	  solidDisSave[208] = 0.000276762;
	  solidDisSave[209] = 5.62512e-06;
	  solidDisSave[210] = 4.50796e-28;
	  solidDisSave[211] = 0.000275523;
	  solidDisSave[212] = 7.32512e-06;
	  solidDisSave[213] = 4.50796e-28;
	  solidDisSave[214] = 0.000275523;
	  solidDisSave[215] = 7.32512e-06;
	  solidDisSave[216] = 4.50796e-28;
	  solidDisSave[217] = 0.000273506;
	  solidDisSave[218] = 9.48674e-06;
	  solidDisSave[219] = 4.50796e-28;
	  solidDisSave[220] = 0.000273506;
	  solidDisSave[221] = 9.48674e-06;
	  solidDisSave[222] = 4.50796e-28;
	  solidDisSave[223] = 0.000270473;
	  solidDisSave[224] = 1.21506e-05;
	  solidDisSave[225] = 4.50796e-28;
	  solidDisSave[226] = 0.000270473;
	  solidDisSave[227] = 1.21506e-05;
	  solidDisSave[228] = 4.50796e-28;
	  solidDisSave[229] = 0.000266148;
	  solidDisSave[230] = 1.53391e-05;
	  solidDisSave[231] = 4.50796e-28;
	  solidDisSave[232] = 0.000266148;
	  solidDisSave[233] = 1.53391e-05;
	  solidDisSave[234] = 4.50796e-28;
	  solidDisSave[235] = 0.000260217;
	  solidDisSave[236] = 1.90445e-05;
	  solidDisSave[237] = 8.55785e-28;
	  solidDisSave[238] = 0.000260217;
	  solidDisSave[239] = 1.90445e-05;
	  solidDisSave[240] = 4.50796e-28;
	  solidDisSave[241] = 0.000252337;
	  solidDisSave[242] = 2.32151e-05;
	  solidDisSave[243] = 8.55785e-28;
	  solidDisSave[244] = 0.000252337;
	  solidDisSave[245] = 2.32151e-05;
	  solidDisSave[246] = 4.50796e-28;
	  solidDisSave[247] = 0.000242147;
	  solidDisSave[248] = 2.77362e-05;
	  solidDisSave[249] = 8.55785e-28;
	  solidDisSave[250] = 0.000242147;
	  solidDisSave[251] = 2.77362e-05;
	  solidDisSave[252] = 4.50796e-28;
	  solidDisSave[253] = 0.00022929;
	  solidDisSave[254] = 3.24075e-05;
	  solidDisSave[255] = 8.55785e-28;
	  solidDisSave[256] = 0.00022929;
	  solidDisSave[257] = 3.24075e-05;
	  solidDisSave[258] = 8.55785e-28;
	  solidDisSave[259] = 0.000213439;
	  solidDisSave[260] = 3.69166e-05;
	  solidDisSave[261] = 4.50796e-28;
	  solidDisSave[262] = 0.000213439;
	  solidDisSave[263] = 3.69166e-05;
	  solidDisSave[264] = 4.50796e-28;
	  solidDisSave[265] = 0.000194344;
	  solidDisSave[266] = 4.08076e-05;
	  solidDisSave[267] = 4.50796e-28;
	  solidDisSave[268] = 0.000194344;
	  solidDisSave[269] = 4.08076e-05;
	  solidDisSave[270] = 4.50796e-28;
	  solidDisSave[271] = 0.000171881;
	  solidDisSave[272] = 4.34459e-05;
	  solidDisSave[273] = 4.50796e-28;
	  solidDisSave[274] = 0.000171881;
	  solidDisSave[275] = 4.34459e-05;
	  solidDisSave[276] = 8.55785e-28;
	  solidDisSave[277] = 0.000146113;
	  solidDisSave[278] = 4.39784e-05;
	  solidDisSave[279] = 4.50796e-28;
	  solidDisSave[280] = 0.000146113;
	  solidDisSave[281] = 4.39784e-05;
	  solidDisSave[282] = 4.50796e-28;
	  solidDisSave[283] = 0.000117336;
	  solidDisSave[284] = 4.1277e-05;
	  solidDisSave[285] = 8.55785e-28;
	  solidDisSave[286] = 0.000117336;
	  solidDisSave[287] = 4.1277e-05;
	  solidDisSave[288] = 4.50796e-28;
	  solidDisSave[289] = 8.60282e-05;
	  solidDisSave[290] = 3.38566e-05;
	  solidDisSave[291] = 4.50796e-28;
	  solidDisSave[292] = 8.60282e-05;
	  solidDisSave[293] = 3.38566e-05;
	  solidDisSave[294] = 4.50796e-28;
	  solidDisSave[295] = 5.2381e-05;
	  solidDisSave[296] = 1.96674e-05;
	  solidDisSave[297] = 4.50796e-28;
	  solidDisSave[298] = 5.2381e-05;
	  solidDisSave[299] = 1.96674e-05;

      //When comparing double, exact comparison might not do the job. Hence im giving 5% error margin. 
      for(int i = 0; i < size; i++){
        if( !(double_equals(disp[i],solidDisSave[i]))){
          return false;
        }
      }
      return true;
    }
    virtual void Test__SolidAgentFinalize(ResultsType &result){
    

      // Setting up the variable   
      double time_final = 0.05;
      double timestep = 2.0e-3;
      int verblevel = 3;
      bool works = true;
      int IntDir;

      // Copying the SimpleStatic data using getinputdata() helper function.
      bool work =__helper();
      std::string fluidDir;
      std::string simStaticDir;
      fluidDir = IRAD::Sys::CWD() + "/SimpleStatic/fluid";
      simStaticDir = IRAD::Sys::CWD() + "/SimpleStatic";
      if(!(IRAD::Sys::FILEEXISTS(fluidDir)||(IRAD::Sys::FILEEXISTS(simStaticDir)))){
        std::cout << "SimpleStatic / fluid Folders do not exists!" << std::endl;
        works = false;           
        result.UpdateResult("fsiCoupler:Works",works);
        return; 
      }
      else{
        IntDir = IRAD::Sys::ChDir(simStaticDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
        std::system("chmod 755 Allclean Allrun AllrunPar makeLinks makeSerialLinks removeSerialLinks"); 
        std::system("./Allclean");
        std::system("./Allrun");
        IntDir = IRAD::Sys::ChDir(fluidDir);
        if(IntDir == -1){
          std::cout << "GetInputData Error: Could not change directory to " 
                    << fluidDir << ". Exiting (fail). " << std::endl;
          works = false;
          result.UpdateResult("fsiCoupler:Works",works);
        }
      }
      
      fsicoupling fsiCoupler(global); 
      std::string fluidSolverName("OpenFoamFSI");
      std::string solidSolverName("ElmerCSC");
      std::string transferServiceName("SurfX");
      std::string runMode("1");

      COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
      COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
      COM_load_module("SurfUtil","SurfUtil");
      COM_load_module("Simpal","Simpal");
     
      std::vector<std::string> componentInterfaceNames;
      componentInterfaceNames.push_back("FluidsComponentInterface");
      componentInterfaceNames.push_back("StructuresComponentInterface");
      componentInterfaceNames.push_back("TransferInterface");
      componentInterfaceNames.push_back("SurfUtil");
      componentInterfaceNames.push_back("Simpal");
 
      fsiCoupler.SetRunMode(runMode);
      fsiCoupler.SetVerbLevel(verblevel);      
      fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);  
      
      // Testing the Elmer finalize
      fsiCoupler.getStructureAgent()->Finalize();
      
      COM_unload_module(solidSolverName.c_str(),"StructuresComponentInterface");       
      int h = COM_get_window_handle("StructuresComponentInterface");
      if(h > 0 ){
        std::cout << "After unloading, COM_get_window_handle(\"StructuresComponentInerface\") returns "                    << h << std::endl;
        works = false;
      }

      result.UpdateResult("SolidAgentFinalize:Works",works);
      
    }

    // WK : End
 
    ///
    /// Runs all tests implemented by the ElmerFoamFSI::TestingObject.
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    virtual void Process(ResultsType &result){
      Prologue();
      Test__ExampleFunction(result);
      Test__TrapezoidQuadrature(result);
      Test__MidPointQuadrature(result);
      //Test__FluidsInitialize(result);
      // MS: Start
      Test__ScaffoldUnitTestFun(result);
      for (int i=0; i<10; i++)
	      Test__ElmerModuleLoad(result);
      Test__OFModuleLoad(result);
      // MS: End
      /*
      // WK: Start
      Test__ElmerHandlesCheck(result);
      Test__FsiCoupler(result);
      Test__TransferDisplacementsToFluid(result);
      Test__FluidAgentRun(result);
	  Test__TransferLoadsToStructures(result);
      Test__SolidAgentRun(result);
      Test__SolidAgentFinalize(result);
      // WK: End
      */
      // NS: Start
      Test__ElmerStructures(result);
      Test__OpenFoamFluids(result);
      // NS: End
      Epilogue();
    }
    
    ///
    /// Runs a test specified by name
    ///
    /// @param name String name of the test to run.
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    virtual void RunTest(const std::string &name,ResultsType &result)
    {
      Prologue();
      if(name == "ExampleFunction")
        Test__ExampleFunction(result);
      else if(name == "TrapezoidQuadrature")
        Test__TrapezoidQuadrature(result);
      else if(name == "MidPointQuadrature")
        Test__MidPointQuadrature(result);
      else if(name == "FluidsInitialize")
        Test__FluidsInitialize(result);
      // MS: Start
      else if(name == "ScaffoldUnitTestFun")
        Test__ScaffoldUnitTestFun(result);
      else if(name == "ElmerModuleLoad")
        Test__ElmerModuleLoad(result);
      else if(name == "OpenFoamModuleLoad")
        Test__OFModuleLoad(result);
      // MS: End
      // WK: Start
      else if(name == "ElmerHandlesCheck")
        Test__ElmerHandlesCheck(result);
      else if(name == "FsiCoupler")
        Test__FsiCoupler(result);
      else if(name == "TransferDisplacementsToFluid")
        Test__TransferDisplacementsToFluid(result);
      else if(name == "FluidAgentRun")
        Test__FluidAgentRun(result);
      else if(name == "TransferLoadsToStructures")
        Test__TransferLoadsToStructures(result);
      else if(name == "SolidAgentRun")
        Test__SolidAgentRun(result);
      else if(name == "SolidAgentFinalize")
        Test__SolidAgentFinalize(result);
      // WK: End
      // NS: Start
      else if(name == "ElmerStructures")
        Test__ElmerStructures(result);
      else if(name == "OpenFoamFluids")
        Test__OpenFoamFluids(result);  
			// NS: End
      Epilogue();
    }

    ///
    /// Runs a list of tests specified by name
    /// 
    /// @param test_names list of string test names
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    virtual void ProcessTests(std::list<std::string> &test_names,ResultsType &result){
      Prologue();
      std::list<std::string>::iterator tni = test_names.begin();
      while(tni != test_names.end())
        RunTest(*tni++,result);
      Epilogue();
    }
  };

#ifdef _ELMERFOAMFSI_PARALLEL_
  /// 
  /// Project-specific parallel testing object.
  ///
  /// The project-specific parallel testing object encapsulates
  /// all parallel tests for the project. This is the interface
  /// to all "in-core" parallel testing available for the given project.
  /// It implements the interface given by 
  /// ElmerFoamFSI::TestingObject and has an 
  /// IRAD::Comm::CommunicatorObject for parallel capabilities.
  ///
  template<typename CommType,typename ResultsType>
  class ParallelTestingObject : TestingObject<ResultsType>
  {
  protected:
    // Example test fixtures for parallel ElmerFoamFSI testing.
    //
    /// 
    /// The CommunicatorObject for the parallel testing object.
    ///
    CommType _communicator;
    ///
    /// A set of values for testing quadrature methods.
    /// 
    std::vector<int> N;
  public:
    ///
    /// Constructor with incoming communicator.
    ///
    ParallelTestingObject(CommType &incomm) :
      TestingObject<ResultsType>(), _communicator(incomm) {};
    CommType &GetCommunicator() { return(_communicator); };

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

    ///
    /// Parallel test for ElmerFoamFSI::TrapezoidQuadrature
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function implements a simple parallel test of the 
    /// function ElmerFoamFSI::TrapezoidQuadrature - which is designed to
    /// integrate a function, f, over an n-way partitioned interval 
    /// \f$[x_0,x_n]\f$ in parallel. 
    ///
    /// The test first makes sure that the method integrates a linear 
    /// function exactly, and then that the error term has the proper
    /// dependence on the spacings. Then the algorithm scaling is checked
    /// by running on subsets of the total number of processors and 
    /// checking time-to-solution.
    /// 
    virtual void Test__ParallelTrapezoidQuadrature(ResultsType &result) {
      std::ostringstream Ostr;
      int fixed_n = 1000000;
      int rank  = _communicator.Rank();
      int nproc = _communicator.Size(); 
      double Ii = 0.0;
      bool runs = true;
      bool accurate = true;
      bool order2 = true;
      bool scales = true;
      std::vector<double> E;
      std::vector<double> times;
      for(int i = 1;i <= nproc;i*=nproc){
        CommType subcomm;
        int color = (rank < i);
        _communicator.Split(color,rank,subcomm);
        int nproc_color = subcomm.Size();
        double time0 = IRAD::Profiler::Time();
        if(color){
          Ii = 0.0;
          for(int j = 0;j < 200;j++){
            if(runs){
              try {
                Ii += ElmerFoamFSI::TrapezoidQuadrature(TestFixture::F1,0,1,fixed_n);
              } catch (...) {
                subcomm.SetExit(1);
              }
              if(subcomm.Check())
                runs = false;
            }
          }
          if(runs){
            double Itot = 0.0;
            subcomm.AllReduce(Ii, Itot,IRAD::Comm::DTDOUBLE, IRAD::Comm::SUMOP);
            double error = 0.0;
            error = std::abs(Itot - (200*nproc_color));
            accurate = (error < 1e-14);
            order2 = accurate;
          }
        }
        times.push_back(IRAD::Profiler::Time() - time0);
        if(nproc == 1)
          break;
      }
      if(!rank){
        double dt = *times.rbegin() - *times.begin();
        double percent_change = dt/(*times.begin());
        scales = (percent_change < (nproc*.005));
      }
      result.UpdateResult("ParallelTrapezoidQuadrature:Runs",runs);
      result.UpdateResult("ParallelTrapezoidQuadrature:Accurate",accurate);
      if(nproc > 1) // only report scaling results when procs are more than 1.
        result.UpdateResult("ParallelTrapezoidQuadrature:WeakScaling",scales);
      times.resize(0);
      if(nproc > 1){ // If running on more than one proc, then do strong scaling test.
        for(int i = 1;i <= nproc;i*=nproc){
          CommType subcomm;
          int color = (rank < i);
          _communicator.Split(color,rank,subcomm);
          int nproc_color = subcomm.Size();
          double time0 = IRAD::Profiler::Time();
          if(color){
            int npart = *N.rbegin()/nproc_color;
            Ii = 0.0;
            for(int j = 0;j < 200;j++){
              if(runs){
                try {
                  Ii += ElmerFoamFSI::TrapezoidQuadrature(TestFixture::F1,0,1,npart);
                } catch (...) {
                  subcomm.SetExit(1);
                }
                if(subcomm.Check())
                  runs = false;
              }
            }
            if(runs){
              double Itot = 0.0;
              subcomm.AllReduce(Ii, Itot,IRAD::Comm::DTDOUBLE, IRAD::Comm::SUMOP);
              double error = 0.0;
              error = std::abs(Itot - (200*nproc_color));
              accurate = (error < 1e-14);
              order2 = accurate;
            }
          }
          times.push_back(IRAD::Profiler::Time() - time0);
        }
        if(!rank){
          double n_t = (*times.begin())/(*times.rbegin());
          double n_p = n_t - nproc;
          n_p = std::abs(n_p)/nproc;
          scales = (n_p < 2e-1);
        }
        result.UpdateResult("ParallelTrapezoidQuadrateure:StrongScaling",scales);
      }
      for(int i = nproc;i <= 1000000;i*=10){
        if(runs){
          Ii = 0.0;
          int n = i/nproc;
          try {
            Ii = ElmerFoamFSI::MidPointQuadrature(TestFixture::F2,0,1,n);
          } catch (...) {
            _communicator.SetExit(1);
          }
          if(_communicator.Check())
            runs = false;
          if(runs){
            double Itot = 0.0;
            _communicator.AllReduce(Ii, Itot,IRAD::Comm::DTDOUBLE, IRAD::Comm::SUMOP);
            double error = 0.0;
            error = std::abs(Itot - nproc);
            E.push_back(error);
          }
        }
      }
      int esize = E.size();
      for(int i = 0;i < esize-1;i++){
        double e = E[i+1]/E[i];
        double n1 = .1;
        double p = std::log(e)/std::log(n1);
        p -= 2;
        p = std::abs(p);
        if(p > 1e-1){
          order2 = false;
        }
      }
      result.UpdateResult("ParallelTrapezoidQuadrature:Order2",order2);
    }
    
    
    ///
    /// Parallel test for ElmerFoamFSI::MidPointQuadrature
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    /// This function implements a simple parallel test of the 
    /// function ElmerFoamFSI::MidPointQuadrature - which is designed to
    /// integrate a function, f, over an n-way partitioned interval 
    /// \f$[x_0,x_n]\f$ in parallel. 
    ///
    /// The test first makes sure that the method integrates a linear 
    /// function exactly, and then that the error term has the proper
    /// dependence on the spacings.  Then the parallel scaling of the
    /// algorithm is checked by running on subsets of the total number
    /// of processors and checking time-to-solution.
    /// 
    virtual void Test__ParallelMidPointQuadrature(ResultsType &result) {
      std::ostringstream Ostr;
      int fixed_n = 1000000;
      int rank  = _communicator.Rank();
      int nproc = _communicator.Size(); 
      double Ii = 0.0;
      bool runs = true;
      bool accurate = true;
      bool order2 = true;
      bool scales = true;
      std::vector<double> E;
      std::vector<double> times;
      for(int i = 1;i <= nproc;i*=nproc){
        CommType subcomm;
        int color = (rank < i);
        _communicator.Split(color,rank,subcomm);
        int nproc_color = subcomm.Size();
        double time0 = IRAD::Profiler::Time();
        if(color){
          Ii = 0.0;
          for(int j = 0;j < 200;j++){
            if(runs){
              try {
                Ii += ElmerFoamFSI::MidPointQuadrature(TestFixture::F1,0,1,fixed_n);
              } catch (...) {
                subcomm.SetExit(1);
              }
              if(subcomm.Check())
                runs = false;
            }
          }
          if(runs){
            double Itot = 0.0;
            subcomm.AllReduce(Ii, Itot,IRAD::Comm::DTDOUBLE, IRAD::Comm::SUMOP);
            double error = 0.0;
            error = std::abs(Itot - (200*nproc_color));
            accurate = (error < 1e-14);
            order2 = accurate;
          }
        }
        times.push_back(IRAD::Profiler::Time() - time0);
        if(nproc == 1)
          break;
      }
      if(!rank){
        double dt = *times.rbegin() - *times.begin();
        double percent_change = dt/(*times.begin());
        scales = (percent_change < (nproc*.005));
        // std::cout << "Weak scaling timings:" << std::endl; 
        // std::vector<double>::iterator ti = weak_times.begin();
        // while(ti != weak_times.end()){
        //   std::cout << ti-weak_times.begin()+1 << "   " 
        //             << *ti << std::endl;
        //   ti++;
        // }
      }
      result.UpdateResult("ParallelMidPointQuadrature:Runs",runs);
      result.UpdateResult("ParallelMidPointQuadrature:Accurate",accurate);
      if(nproc > 1) // only report scaling results for nproc > 1
        result.UpdateResult("ParallelMidPointQuadrature:WeakScaling",scales);
      times.resize(0);
      if(nproc > 1){ // only do scaling test if nproc > 1
        for(int i = 1;i <= nproc;i*=nproc){
          CommType subcomm;
          int color = (rank < i);
          _communicator.Split(color,rank,subcomm);
          int nproc_color = subcomm.Size();
          double time0 = IRAD::Profiler::Time();
          if(color){
            int npart = *N.rbegin()/nproc_color;
            Ii = 0.0;
            for(int j = 0;j < 200;j++){
              if(runs){
                try {
                  Ii += ElmerFoamFSI::MidPointQuadrature(TestFixture::F1,0,1,npart);
                } catch (...) {
                  subcomm.SetExit(1);
                }
                if(subcomm.Check())
                  runs = false;
              }
            }
            if(runs){
              double Itot = 0.0;
              subcomm.AllReduce(Ii, Itot,IRAD::Comm::DTDOUBLE, IRAD::Comm::SUMOP);
              double error = 0.0;
              error = std::abs(Itot - (200*nproc_color));
              accurate = (error < 1e-14);
              order2 = accurate;
            }
          }
          times.push_back(IRAD::Profiler::Time() - time0);
        }
        if(!rank){
          double n_t = (*times.begin())/(*times.rbegin());
          double n_p = n_t - nproc;
          n_p = std::abs(n_p)/nproc;
          scales = (n_p < 2e-1); 
       }
        result.UpdateResult("ParallelMidPointQuadrateure:StrongScaling",scales);
      }
      for(int i = nproc;i <= 1000000;i*=10){
        if(runs){
          Ii = 0.0;
          int n = i/nproc;
          try {
            Ii = ElmerFoamFSI::MidPointQuadrature(TestFixture::F2,0,1,n);
          } catch (...) {
            _communicator.SetExit(1);
          }
          if(_communicator.Check())
            runs = false;
          if(runs){
            double Itot = 0.0;
            _communicator.AllReduce(Ii, Itot,IRAD::Comm::DTDOUBLE, IRAD::Comm::SUMOP);
            double error = 0.0;
            error = std::abs(Itot - nproc);
            E.push_back(error);
          }
        }
      }
      int esize = E.size();
      for(int i = 0;i < esize-1;i++){
        double e = E[i+1]/E[i];
        double n1 = .1;
        double p = std::log(e)/std::log(n1);
        p -= 2;
        p = std::abs(p);
        if(p > 1e-1){
          order2 = false;
        }
      }
      result.UpdateResult("ParallelMidPointQuadrature:Order2",order2);
    }

    ///
    /// Runs all tests implemented by the ElmerFoamFSI::TestingObject.
    ///
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    virtual void Process(ResultsType &result){
      Prologue();
      Test__ParallelTrapezoidQuadrature(result);
      Test__ParallelMidPointQuadrature(result);
      Epilogue();
    }
    
    ///
    /// Runs a test specified by name
    ///
    /// @param name String name of the test to run.
    /// @param result ElmerFoamFSI::TestResults object to store test results.
    ///
    virtual void RunTest(const std::string &name,ResultsType &result)
    {
      if(name == "ParallelTrapezoidQuadrature")
        Test__ParallelTrapezoidQuadrature(result);
      if(name == "ParallelMidPointQuadrature")
        Test__ParallelMidPointQuadrature(result);
    }

    ///
    /// Runs a list of tests specified by name
    /// 
    /// @param test_names list of string test names
    /// @param result ElmerFoamFSI::TestResults object to store test results.
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
