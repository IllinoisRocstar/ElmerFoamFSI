///
/// @file
/// @ingroup elmermoduledriver_group
/// @brief Testing object for ElmerModuleDriver.
///
/// This file contains the implementation of the
/// overarching testing object for the project.
/// Every project should have one of these as
/// it allows external entities unified methodology
/// for access project or package-specific tests.
///
#ifndef __ELMERMODULEDRIVER_TEST_H__
#define __ELMERMODULEDRIVER_TEST_H__
#ifdef _ELMERMODULEDRIVER_PARALLEL_
#include "COMM.H"
#endif
#include "Testing.H"
#include "ExampleHeader.H"
#include "Profiler.H"
#include <vector>
#include <cmath>
#include "com.h"
#include "com_devel.hpp"
#include "UnixUtils.H"

COM_EXTERN_MODULE( ElmerCSC );

namespace ElmerModuleDriver {

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
    // Example test fixtures for ElmerModuleDriver testing.
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
    ///
    /// Test for ElmerModuleDriver::ExampleFunction
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    /// This function implements a simple test of the 
    /// function ElmerModuleDriver::ExampleFunction - which is designed to
    /// simply return a copy of the input string.
    ///
    ///
    ///************************************************************************
    /// List of Serial tests
    ///************************************************************************
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
    /// Test for ElmerModuleDriver::TrapezoidQuadrature
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    /// This function implements a simple test of the 
    /// function ElmerModuleDriver::TrapezoidQuadrature - which is designed to
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
          Ii = ElmerModuleDriver::TrapezoidQuadrature(TestFixture::F1,0,1,*i);
        } catch (...) {
          runs = false;
        }
        E.push_back(std::fabs(Ii-1.0));
      }
      bool order2 = (E[0] < 1e-14);
      for(std::vector<int>::iterator i = N.begin();i != N.end();i++){
        double Ii = 0.0;
        try {
          Ii = ElmerModuleDriver::TrapezoidQuadrature(TestFixture::F2,0,1,*i);
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
    /// Test for ElmerModuleDriver::MidPointQuadrature
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    /// This function implements a simple test of the 
    /// function ElmerModuleDriver::MidPointQuadrature - which is designed to
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
          Ii = ElmerModuleDriver::MidPointQuadrature(TestFixture::F1,0,1,*i);
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
          Ii = ElmerModuleDriver::MidPointQuadrature(TestFixture::F2,0,1,*i);
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
  

    /// Function to compare two files
    /// 
    /// @param std::istream &stream1 File 1 to compare
    /// @param std::istream &stream2 File 2 to compare
    /// @param std::stringstream *ss Optional argument which returns
    /// the lines that differ
    //
    // Example use:
    // std::ifstream Inf1;
    // std::ifstream Inf2;
    // Inf1.open("file1");
    // Inf2.open("file2");
    //
    // Just find out if they differ....
    // if(streamdiff(Inf1,Inf2))
    //   std::cout << "Files differ." << std::endl;
    // 
    // Report the differences....
    // (reset the files)
    // streamdiff(Inf1,Inf2,&std::cout);
    //
    int streamdiff(std::istream &stream1,std::istream &stream2,std::stringstream *ss = NULL){
      int retval = 0;
      std::string line1,line2;
      while(!stream1.eof()){
        std::getline(stream1,line1);
        if(!stream2.eof())
          std::getline(stream2,line2);
        if(line1 != line2){
          if(ss){
            *ss << "1> " << line1 << std::endl
                           << "2> " << line2 << std::endl;
            retval = 1;
          }
          else
            return(1);
        }
      }
      if(stream1.eof() != stream2.eof()){
        return(1);
      }
      return(0);
    }
 
    /// Function to copy input data from source testing directory
    /// for use in unit tests.
    /// 
    /// @param suffix std::string for the input data location (it is
    /// appended to the source directory inputted on the command line)
    /// @param TestDir std::string path for directory to run the test in
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
 

    ///*****************************************************************************
    /// Function to test the run function of Initialize,TimeStepper, and Finalize
    /// functionalities of SolverModule. Also, this function tests the registration
    /// and values of mesh and displacment data.
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    ///*****************************************************************************
    virtual void Test__ElmerUnitTests(ResultsType &result){
      std::cout << "Running Test__ElmerUnitTests" << std::endl; 
 
      int IntDir;
      std::string OrgDir, TestDir;
      bool ItAllWorks = true;

      // Arrays that old data for check *******************************************
      double CoordCheck[15][3] =
          { {2.2999999999999998, 0.19296300000000000, 0.0000000000000000},
            {2.2999999999999998, 0.00000000000000000, 0.0000000000000000},
            {2.2999999999999998, 0.39523399999999997, 0.0000000000000000},
            {2.2999999999999998, 0.60476600000000003, 0.0000000000000000},
            {2.2999999999999998, 0.80703700000000000, 0.0000000000000000},
            {2.2999999999999998, 1.00000000000000000, 0.0000000000000000},
            {2.3585799999999999, 1.14142000000000010, 0.0000000000000000},
            {2.5000000000000000, 1.20000000000000000, 0.0000000000000000},
            {2.6414200000000001, 1.14142000000000010, 0.0000000000000000},
            {2.7000000000000002, 1.00000000000000000, 0.0000000000000000},
            {2.7000000000000002, 0.80703700000000000, 0.0000000000000000},
            {2.7000000000000002, 0.60476600000000003, 0.0000000000000000},
            {2.7000000000000002, 0.39523399999999997, 0.0000000000000000},
            {2.7000000000000002, 0.19296300000000000, 0.0000000000000000},
            {2.7000000000000002, 0.00000000000000000, 0.0000000000000000} };
      int ConnCheck[14][2] = 
           { {1,2}, {3,1}, {4,3}, {5,4}, {6,5}, {7,6}, {8,7}, {9,8}, {10,9},
           {11,10}, {12,11}, {13,12}, {14,13}, {15,14} }; 

      // Getting Input data********************************************************
      //***************************************************************************
      
      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/FSIobstacle";

      //Make path for directory to run the test in.
      TestDir = IRAD::Sys::CWD() + "/UnitTestData";
      IntDir = GetInputData(suffix, TestDir);

      //Check if GetInputData exited correctly.
      if(IntDir != 0){
        std::cout << "ElmerUnitTests Error: GetInputData call failed."
                  << std::endl;
        result.UpdateResult("ElmerUnitTests:Run", false);
        return;
      }

      //Check if TestDir directory (from GetInputData) exists in current directory
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        std::cout << "ElmerUnitTests Error: Could not find directory, "
                  << TestDir << ", for running tests." << std::endl;
        result.UpdateResult("ElmerUnitTests:Run", false);
        return;
      }

      //Save original directory name for later
      OrgDir = IRAD::Sys::CWD();

      //Change directories to TestDir directory for running 
      IntDir = IRAD::Sys::ChDir(TestDir);
      if(IntDir == -1){
        std::cout << "ElmerUnitTests Error: Could not change directories to "
                  << TestDir << "." << std::endl;
        result.UpdateResult("ElmerUnitTests:Run", false);
        return;
      }

      //load Elmer module**********************************************************
      //***************************************************************************
      COM_LOAD_MODULE_STATIC_DYNAMIC( ElmerCSC, "Window1");
   
      bool load_unload = true;

      //Get Winodw handle
      int h=COM_get_window_handle("Window1");
      if(h <= 0){
        std::cout << " After loading, COM_get_window_handle(\"Window1\") returns "
                  << h << std::endl;
        load_unload=false;
        std::cout << "ElmerUnitTests Error: Could not get window handle."
                  << "." << std::endl;
        result.UpdateResult("LoadSolverModule:Works",false);
        result.UpdateResult("ElmerUnitTests:Run", false);
        return;
      }
      else
        result.UpdateResult("LoadSolverModule:Works",true);

      ////////////////////////////////////////////////////
      // Get Initialize function handle and call Function
      ////////////////////////////////////////////////////
      int init_handle = COM_get_function_handle("Window1.Initialize");
      bool init_func = (init_handle > 0);
      int init_runs = 0;
      std::cout << "init = " << init_handle << std::endl;
      if(init_handle){
        //Call the function
        COM_call_function(init_handle, &init_runs);
      }
      else{
        std::cout << "InitializeRuns Error: Could not get function handle."
                  << "." << std::endl;
        result.UpdateResult("Initialize:Runs", false);
        return;
      }

      //Check if function changed value of runs properly
      if(init_runs == 1){
        result.UpdateResult("Initialize:Runs", true);
      }
      else{
        result.UpdateResult("Initialize:Runs", false);
        ItAllWorks = false;
      }

      //////////////////////////////////////////////////
      // Check coordinate values
      //////////////////////////////////////////////////
      int coord_handle = COM_get_dataitem_handle("Window1.nc");
      int CoordSize=0;
      bool coordData = (coord_handle > 0);
      bool coordCorrect = true;
      std::cout << "coord_handle = " << coord_handle << std::endl;
      if(coordData){
        COM_get_size("Window1.nc",11,&CoordSize);
        if(CoordSize != 15){
          std::cout << "ElmerUnitTests Error:"
                    << " Coord array is incorrect size!" << std::endl;
          ItAllWorks = false;
          coordCorrect = false;
        }
        else{
          // Get the FSI mesh from the structures solver and print
          // it out to check
          double* Coord;
          COM_get_array("Window1.nc",11,&Coord);
          std::cout << "Coord:" << std::endl;
          for(int i=0; i < CoordSize; i++){
            for(int j=0; j < 3; j++){
              std::cout << Coord[i*3+j] << " ";
              if(fabs(CoordCheck[i][j] - Coord[i*3+j]) > 1.0e-12){
                ItAllWorks = false;
                coordCorrect = false;
              }
            }
            std::cout << std::endl;
          }
        }
        result.UpdateResult("CoordinateData:Correct", coordCorrect);
        result.UpdateResult("CoordinateData:Registered", true);
      }
      else{
        result.UpdateResult("CoordinateData:Registered", false);
        result.UpdateResult("CoordinateData:Correct", false);
        ItAllWorks = false;
      }

      //////////////////////////////////////////////////
      // Check connectivity values
      //////////////////////////////////////////////////
      bool connCorrect = true;
      int ConnSize = 0;
      int* Conn = NULL;

      // Get the FSI mesh from the structures solver
      COM_get_array("Window1.:b2:",11,&Conn);
      if(Conn){
        // Get the FSI mesh size from the structures solver
        COM_get_size("Window1.:b2:",11,&ConnSize);
        if(ConnSize != 14){
          std::cout << "ElmerUnitTests Error:"
                    << " Conn array is incorrect size!" << std::endl;
          ItAllWorks = false;
          connCorrect = false;
        }
        else{
          // check the values
          std::cout << "Conn:" << std::endl;
          for(int i=0; i < ConnSize; i++){
            for(int j=0; j < 2; j++){
              std::cout << Conn[i*2+j] << " ";
              if( (ConnCheck[i][j] - Conn[i*2+j]) != 0){
                ItAllWorks = false;
                connCorrect = false;
              }
            }
            std::cout << std::endl;
          }
        }
        result.UpdateResult("ConnectivityData:Correct", connCorrect);
        result.UpdateResult("ConnectivityData:Registered", true);
      }
      else{
        result.UpdateResult("ConnectivityData:Registered", false);
        result.UpdateResult("ConnectivityData:Correct", false);
        ItAllWorks = false;
      }

      //////////////////////////////////////////////////
      // Check load registration & initialization
      //////////////////////////////////////////////////
      double* Loads = NULL;
      int LoadsSize = 0;
      bool loadsRegistered = true, loadsInitialized = true;
      // Get the FSI loads from the structures solver
      COM_get_array("Window1.Loads",11,&Loads);
      if(Loads){
        std::cout << "Loads not NULL" << std::endl;
        // Get the FSI load size from the structures solver
        COM_get_size("Window1.Loads",11,&LoadsSize);
        if(LoadsSize != 15){
          std::cout << "ElmerUnitTests Error:"
                    << " Loads array is incorrect size!" << std::endl;
          ItAllWorks = false;
          loadsInitialized = false;
        }
        else{
          // check the values
          for(int i=0; i < LoadsSize; i++){
            for(int j=0; j < 3; j++){
              if(fabs(Loads[i*3 + j]) > 1.0e-12){
                loadsInitialized = false;
                ItAllWorks = false;
              }
            }
          }
        }
      }
      else{
        std::cout << "Loads are NULL" << std::endl;
        loadsRegistered = false;
        loadsInitialized = false;
        ItAllWorks = false;
      }
      result.UpdateResult("LoadData:Registered", loadsRegistered); 
      result.UpdateResult("LoadData:Initialized", loadsInitialized); 

      ////////////////////////////////////////////////////
      // Get TimeStepper function handle and call Function
      ////////////////////////////////////////////////////

      // Arrays that hold disp data for check *********************************
      double DispTime1[15][3] =
          {{  7.1605980866005031E-002,   3.5788052113870331E-002,   0.0000000000000000},     
           { -6.8871825230548747E-026,  -6.7515261027908644E-026,   0.0000000000000000},     
           {  0.16559117138148402    ,   6.1512134471618775E-002,   0.0000000000000000},     
           {  0.27105716643234351    ,   7.0647177803958988E-002,   0.0000000000000000},     
           {  0.37408733220172607    ,   6.6788253467455649E-002,   0.0000000000000000},     
           {  0.46975382688278072    ,   5.5232228766620040E-002,   0.0000000000000000},     
           {  0.53059533135003267    ,   1.7568173579321143E-002,   0.0000000000000000},     
           {  0.54088718131665003    ,  -5.0723278288525193E-002,   0.0000000000000000},     
           {  0.49733519851449115    ,  -0.10431856777439819    ,   0.0000000000000000},     
           {  0.42342243323024942    ,  -0.11544236710979815    ,   0.0000000000000000},     
           {  0.32858350010626158    ,  -9.6940873722311166E-002,   0.0000000000000000},     
           {  0.22913389910789400    ,  -7.6890455177259009E-002,   0.0000000000000000},     
           {  0.13256597264490513    ,  -5.4789959945688596E-002,   0.0000000000000000},     
           {  5.4547795662606154E-002,  -2.9570884780950012E-002,   0.0000000000000000},     
           { -6.8871825230548747E-026,  -6.7515261027908632E-026,   0.0000000000000000}};     
      double DispTime2[15][3] =
           {{  7.8382763777475994E-002,    3.7159701956680058E-002,   0.0000000000000000}, 
            { -1.3908554346559520E-053,   -1.3634598387444821E-053,   0.0000000000000000}, 
            {  0.17746729339780631    ,    6.3782065644847119E-002,   0.0000000000000000}, 
            {  0.28589059231716707    ,    7.3724507560836025E-002,   0.0000000000000000}, 
            {  0.39007099539893136    ,    7.0647599706400666E-002,   0.0000000000000000}, 
            {  0.48577572075292708    ,    5.9851545424915178E-002,   0.0000000000000000},  
            {  0.54619785916039254    ,    2.2562624600359866E-002,   0.0000000000000000}, 
            {  0.55515630051911513    ,   -4.5683794688744256E-002,   0.0000000000000000}, 
            {  0.51164185034902454    ,   -9.8434990773815095E-002,   0.0000000000000000}, 
            {  0.43876588136755668    ,   -0.10966508014917155    ,   0.0000000000000000}, 
            {  0.34337311330396592    ,   -9.2057033659364212E-002,   0.0000000000000000}, 
            {  0.24185886373003795    ,   -7.3235312951635573E-002,   0.0000000000000000}, 
            {  0.14168683127415216    ,   -5.2598213602584751E-002,   0.0000000000000000}, 
            {  5.9082767678702033E-002,   -2.8938645702423191E-002,   0.0000000000000000}, 
            { -1.3908554346559520E-053,   -1.3634598387444801E-053,   0.0000000000000000}}; 
      //*********************************************************************
      int runs_handle = COM_get_function_handle("Window1.Run");
      bool runs_func = (runs_handle > 0);
      bool DispCorrect = true;
      int runs_runs = 0;
      double time1 =  8.0;
      double time2 = 100.0;
      double* Disp = NULL;
      int DispSize = 0;
      COM_get_array("Window1.Displacements",11,&Disp);
      std::cout << "runs = " << runs_handle << std::endl;
      if(runs_handle){
        //Call the run function for Time1
        if(Loads){
           std::cout << "Changing load values" << std::endl;
           for(int i=0; i < LoadsSize; i++){
              for(int j=0; j < 3; j++){
                 Loads[i*3 + j] = double(i*3 + j);
                 std::cout << Loads[i*3 + j] << " ";
              }
              std::cout << std::endl;
           }
        }
        COM_call_function(runs_handle, &runs_runs, &time1);
      }
      else{
        std::cout << "TimeStepperRuns Error: Could not get function handle."
                  << "." << std::endl;
        result.UpdateResult("TimeStepper:Runs", false);
        return;
      }

      //Check if function changed value of runs properly
      if(runs_runs > 0){
        result.UpdateResult("TimeStepper:Runs", true);
      }
      else{
        result.UpdateResult("TimeStepper:Runs", false);
        ItAllWorks = false;
      }

      // Check reigstration of displacement data
      if (Disp){
         result.UpdateResult("DisplacementData:Registered", true);
         // Get the FSI displacement size from the structures solver
         COM_get_size("Window1.Displacements",11,&DispSize);
         if(DispSize != 15){
           std::cout << "ElmerUnitTests Error:"
                     << " Disp array is incorrect size!" << std::endl;
           ItAllWorks = false;
           DispCorrect = false;
         }
      }
      else{
        result.UpdateResult("DisplacementData:Registered", false);
        DispCorrect = false;
        ItAllWorks = false;
      }
      
      // Check values of displacements after Run func @ Time1
      if (Disp && runs_handle && DispCorrect){
        std::cout << "Checking Displacements Time 1 (" << time1 << ")" << std::endl;
        for(int i=0; i < DispSize; i++){
           for(int j=0; j < 3; j++){
              std::cout << Disp[i*3+j] << " ";
              if(fabs(DispTime1[i][j] - Disp[i*3+j]) > 1.0e-12){
                ItAllWorks = false;
                DispCorrect = false;
              }
           }
           std::cout << std::endl;
        }
        //Change the load values again for testing
        if(Loads){
           std::cout << "Changing load values" << std::endl;
           for(int i=0; i < LoadsSize; i++){
              for(int j=0; j < 3; j++){
                 Loads[i*3 + j] = double(i*3 + j) + 1.0;
                 std::cout << Loads[i*3 + j] << " ";
              }
              std::cout << std::endl;
           }
        }
        //Call the run function for Time2
        int runs_runs2 = 0;
        COM_call_function(runs_handle, &runs_runs2, &time2);
        // Check values of displacements after Run func @ Time1
        std::cout << "Checking Displacements Time 2 (" << time2 << ")" << std::endl;
        for(int i=0; i < DispSize; i++){
           for(int j=0; j < 3; j++){
              std::cout << Disp[i*3+j] << " ";
              if(fabs(DispTime2[i][j] - Disp[i*3+j]) > 1.0e-12){
                ItAllWorks = false;
                DispCorrect = false;
              }
           }
           std::cout << std::endl;
        }
        //Check if function changed value of runs properly in Loads function
        if(runs_runs > 1 && runs_runs2 > 1){
          result.UpdateResult("TimeLoads:Runs", true);
        }
        else{
          std::cout << "runs_runs = " << runs_runs << std::endl;
          std::cout << "runs_runs2 = " << runs_runs2 << std::endl;
          result.UpdateResult("TimeLoads:Runs", false);
          ItAllWorks = false;
        }
      }
      result.UpdateResult("DisplacementData:Correct", DispCorrect);


      ////////////////////////////////////////////////////
      // Get Finalize function handle and call Function
      ////////////////////////////////////////////////////
      int final_handle = COM_get_function_handle("Window1.Finalize");
      bool final_func = (final_handle > 0);
      int final_runs = 0;
      std::cout << "final = " << final_handle << std::endl;
      if(final_handle){
        //Call the function
        COM_call_function(final_handle, &final_runs);
      }
      else{
        std::cout << "FinalizeRuns Error: Could not get function handle."
                  << "." << std::endl;
        result.UpdateResult("Finalize:Runs", false);
        return;
      }

      //Check if function changed value of runs properly
      if(final_runs == 1){
        result.UpdateResult("Finalize:Runs", true);
      }
      else{
        result.UpdateResult("Finalize:Runs", false);
        ItAllWorks = false;
      }
      ////////////////////////////////////////////////////

      //Unload Elmer module********************************************************
      //***************************************************************************
      COM_UNLOAD_MODULE_STATIC_DYNAMIC( ElmerCSC, "Window1");

      //Make sure module unloaded
      h=COM_get_window_handle("Window1");
      if(h > 0){
        std::cout << "After unloading, COM_get_window_handle(\"Window1\") returns "
                  << h << std::endl;
        load_unload=false;
        std::cout << "ElmerUnitTests Error: Did not properly unload module."
                  << "." << std::endl;
        result.UpdateResult("ElmerUnitTests:Run", false);
        result.UpdateResult("UnloadSolverModule:Works",false);
        return;
      }
      else
        result.UpdateResult("UnloadSolverModule:Works",true);

      //Change directories out of TestDir directory back to original directory
      IntDir = IRAD::Sys::ChDir(OrgDir);
      if(IntDir == -1){
        std::cout << "ElmerUnitTests Error: Could not change directories to "
                  << OrgDir << "." << std::endl;
        result.UpdateResult("ElmerUnitTests:Run", false);
        return;
      }

      result.UpdateResult("ElmerUnitTests:Run", ItAllWorks);

    }
    ///*****************************************************************************
    ///*****************************************************************************

    ///*****************************************************************************
    /// Function to test our load function on a structures only problem.
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    ///*****************************************************************************
    virtual void Test__ElasticBeam3D_LoadFunction(ResultsType &result){
      std::cout << "Running Test__ElasticBeam3D_LoadFunction" << std::endl; 
 
      int IntDir;
      std::string OrgDir, TestDir;
      bool ItAllWorks = true;

      // Getting Input data********************************************************
      //***************************************************************************
      
      //Call GetInputData to copy input test data to temp directory for running tests.
      std::string suffix = "/share/Testing/test_data/ElasticBeam3D_LoadFunction";

      //Make path for directory to run the test in.
      TestDir = IRAD::Sys::CWD() + "/ElasticBeam3D_TestData";
      IntDir = GetInputData(suffix, TestDir);

      //Check if GetInputData exited correctly.
      if(IntDir != 0){
        std::cout << "ElasticBeam3D_LoadFunction Error: GetInputData call failed."
                  << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      //Check if TestDir directory (from GetInputData) exists in current directory
      if(!IRAD::Sys::FILEEXISTS(TestDir)){
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not find directory, "
                  << TestDir << ", for running tests." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      //Save original directory name for later
      OrgDir = IRAD::Sys::CWD();

      //Change directories to TestDir directory for running 
      IntDir = IRAD::Sys::ChDir(TestDir);
      if(IntDir == -1){
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not change directories to "
                  << TestDir << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      //load Elmer module**********************************************************
      //***************************************************************************
      COM_LOAD_MODULE_STATIC_DYNAMIC( ElmerCSC, "Window1");
   
      bool load_unload = true;

      //Get Winodw handle
      int h=COM_get_window_handle("Window1");
      if(h <= 0){
        std::cout << " After loading, COM_get_window_handle(\"Window1\") returns "
                  << h << std::endl;
        load_unload=false;
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not get window handle."
                  << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      ////////////////////////////////////////////////////
      // Get Initialize function handle and call Function
      ////////////////////////////////////////////////////
      int init_handle = COM_get_function_handle("Window1.Initialize");
      bool init_func = (init_handle > 0);
      int init_runs = 0;
      std::cout << "init = " << init_handle << std::endl;
      if(init_handle){
        //Call the function
        COM_call_function(init_handle, &init_runs);
      }
      else{
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not get function handle."
                  << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      //////////////////////////////////////////////////
      // Check load registration & initialization
      //////////////////////////////////////////////////
      double* Loads = NULL;
      int LoadsSize = 0;
      bool loadsRegistered = true, loadsInitialized = true;
      // Get the FSI mesh from the structures solver
      COM_get_array("Window1.Loads",11,&Loads);
      if(Loads){
        COM_get_size("Window1.Loads",11,&LoadsSize);
        std::cout << "Loads not NULL" << std::endl;
        std::cout << "LoadsSize = " << LoadsSize << std::endl;
        // check the values
        for(int i=0; i < LoadsSize; i++){
          for(int j=0; j < 3; j++){
            if(fabs(Loads[i*3 + j]) > 1.0e-12){
               std::cout << "Loads were not intialized to 0.0!" << std::endl;
              result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
              return;
            }
          }
        }
      }
      else{
        std::cout << "Loads are NULL" << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      ////////////////////////////////////////////////////
      // Get TimeStepper function handle and call Function
      ////////////////////////////////////////////////////

      int runs_handle = COM_get_function_handle("Window1.Run");
      bool runs_func = (runs_handle > 0);
      bool DispCorrect = true;
      int runs_runs = 0;
      double time1 =  1.0;
      double* Disp = NULL;
      std::cout << "runs = " << runs_handle << std::endl;
      if(runs_handle){
        //Call the run function for Time1
        if(Loads){
           std::cout << "Changing load values" << std::endl;
           for(int i=0; i < LoadsSize; i++){
              Loads[i*3 + 1] = -4.0e5;
              for(int j=0; j < 3; j++){
                 std::cout << Loads[i*3 + j] << " ";
              }
              std::cout << std::endl;
           }
        }
        COM_call_function(runs_handle, &runs_runs, &time1);
      }
      else{
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not get function handle."
                  << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      ////////////////////////////////////////////////////
      // Get Finalize function handle and call Function
      ////////////////////////////////////////////////////
      int final_handle = COM_get_function_handle("Window1.Finalize");
      bool final_func = (final_handle > 0);
      int final_runs = 0;
      std::cout << "final = " << final_handle << std::endl;
      if(final_handle){
        //Call the function
        COM_call_function(final_handle, &final_runs);
      }
      else{
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not get function handle."
                  << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      ////////////////////////////////////////////////////
     
      ////////////////////////////////////////////////////
      // Compare the output file with an archived output file
      ////////////////////////////////////////////////////
      std::ifstream Inf, InfComp;
      std::stringstream ss;
      std::string word, line;

      Inf.open("case.ep");
      InfComp.open("case.ep_check");

      std::getline(Inf,line);
      std::getline(Inf,line);
      std::getline(InfComp,line);
      std::getline(InfComp,line);

      bool LoadWorks = true; 
      int diffReturn = -1; 
 
      diffReturn = streamdiff(Inf, InfComp, &ss);

      if(diffReturn){
        std::cout << "ElasticBeam3D_LoadFunction Error: case.ep "
                  << "case.ep_check differ!" << std::endl;
        std::cout << ss.str() << std::endl;
        if(ss.str() != " ")
          LoadWorks = false;
      }
  
      std::cout << "LoadWorks = " << LoadWorks << std::endl; 
      result.UpdateResult("ElasticBeam3D_LoadFunction:Works",LoadWorks);

      //Unload Elmer module********************************************************
      //***************************************************************************
      COM_UNLOAD_MODULE_STATIC_DYNAMIC( ElmerCSC , "Window1");

      //Make sure module unloaded
      h=COM_get_window_handle("Window1");
      if(h > 0){
        std::cout << "After unloading, COM_get_window_handle(\"Window1\") returns "
                  << h << std::endl;
        load_unload=false;
        std::cout << "ElasticBeam3D_LoadFunction Error: Did not properly unload module."
                  << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      //Change directories out of TestDir directory back to original directory
      IntDir = IRAD::Sys::ChDir(OrgDir);
      if(IntDir == -1){
        std::cout << "ElasticBeam3D_LoadFunction Error: Could not change directories to "
                  << OrgDir << "." << std::endl;
        result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", false);
        return;
      }

      result.UpdateResult("ElasticBeam3D_LoadFunction:Runs", true);

    }
    ///*****************************************************************************
    ///*****************************************************************************

    ///************************************************************************
    /// Functions to call Serial tests
    ///************************************************************************
    ///
    /// Runs all tests implemented by the ElmerModuleDriver::TestingObject.
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    virtual void Process(ResultsType &result){
      Prologue();
      Test__ExampleFunction(result);
      Test__TrapezoidQuadrature(result);
      Test__MidPointQuadrature(result);
      Test__ElmerUnitTests(result);
      Epilogue();
    }
    
    ///
    /// Runs a test specified by name
    ///
    /// @param name String name of the test to run.
    /// @param result ElmerModuleDriver::TestResults object to store test results.
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
      else if(name == "ElmerUnitTests")
        Test__ElmerUnitTests(result);
      else if(name == "ElasticBeam3D_LoadFunction")
        Test__ElasticBeam3D_LoadFunction(result);
      Epilogue();
    }

    ///
    /// Runs a list of tests specified by name
    /// 
    /// @param test_names list of string test names
    /// @param result ElmerModuleDriver::TestResults object to store test results.
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
#ifdef _ELMERMODULEDRIVER_PARALLEL_
  /// 
  /// Project-specific parallel testing object.
  ///
  /// The project-specific parallel testing object encapsulates
  /// all parallel tests for the project. This is the interface
  /// to all "in-core" parallel testing available for the given project.
  /// It implements the interface given by 
  /// ElmerModuleDriver::TestingObject and has an 
  /// IRAD::Comm::CommunicatorObject for parallel capabilities.
  ///
  template<typename CommType,typename ResultsType>
  class ParallelTestingObject : TestingObject<ResultsType>
  {
  protected:
    // Example test fixtures for parallel ElmerModuleDriver testing.
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
    /// Parallel test for ElmerModuleDriver::TrapezoidQuadrature
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    /// This function implements a simple parallel test of the 
    /// function ElmerModuleDriver::TrapezoidQuadrature - which is designed to
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
                Ii += ElmerModuleDriver::TrapezoidQuadrature(TestFixture::F1,0,1,fixed_n);
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
                  Ii += ElmerModuleDriver::TrapezoidQuadrature(TestFixture::F1,0,1,npart);
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
            Ii = ElmerModuleDriver::MidPointQuadrature(TestFixture::F2,0,1,n);
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
    /// Parallel test for ElmerModuleDriver::MidPointQuadrature
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
    ///
    /// This function implements a simple parallel test of the 
    /// function ElmerModuleDriver::MidPointQuadrature - which is designed to
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
                Ii += ElmerModuleDriver::MidPointQuadrature(TestFixture::F1,0,1,fixed_n);
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
                  Ii += ElmerModuleDriver::MidPointQuadrature(TestFixture::F1,0,1,npart);
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
            Ii = ElmerModuleDriver::MidPointQuadrature(TestFixture::F2,0,1,n);
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

    ///************************************************************************
    /// Functions to call Parallel tests
    ///************************************************************************
    ///
    /// Runs all tests implemented by the ElmerModuleDriver::TestingObject.
    ///
    /// @param result ElmerModuleDriver::TestResults object to store test results.
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
    /// @param result ElmerModuleDriver::TestResults object to store test results.
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
    /// @param result ElmerModuleDriver::TestResults object to store test results.
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
