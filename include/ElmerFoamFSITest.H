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
//#include "Parameters.H"
//#include "InterfaceLayer.H"
//#include "Orchestrator.H"
//#include "OpenFoamAgent.H"
//#include "ElmerAgent.H"

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
  public:
    /// 
    /// Default constructor.
    ///
    TestingObject() : TestingObjectBaseType() {};

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
    }   


    //Copied from ElmerDriver testing script
    virtual int GetInputData(std::string suffix,std::string TestDir){

      std::cout << "In GetInputData" << std::endl;
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
    virtual bool __helper(){
 
      int IntDir;
      std::string OrgDir, TestDir;     

      //call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/SimpleStatic";
  
      //Make path for directory to run the test in
      TestDir = IRAD::Sys::CWD() + "/SimpleStatic";
      IntDir  = GetInputData(suffix, TestDir);
  
      //check if GetInputData exited correctly.
      if(IntDir != 0){
        std::cout << "GetInputData failed!"
                  << std::endl;
        return false;
      }
      // Check if TestDir directory exists in current Directory
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        std::cout << "Could not find directory, "
                  << TestDir << ", for running tests." << std::endl;
        return false;
      }
      return true;     
    }  
    //testing the get input?
    virtual void Test__GetInput(ResultsType &result){
      bool works =__helper();
      result.UpdateResult("GetInput:Works",works);
      
    }
      
    void Test__ReadInputWorks(ResultsType &result){

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
      // WK: Start
      Test__ElmerHandlesCheck(result);
      //Test__GetInput(result);
      Test__ReadInputWorks(result);
      // WK: End
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
      //else if(name == "GetInput")
      //  Test__GetInput(result);
      else if(name == "ReadInputWorks")
        Test__ReadInputWorks(result);
      // WK: End
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
