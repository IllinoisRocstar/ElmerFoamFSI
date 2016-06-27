/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 

#ifndef __FSI_COUPLING_PAR_H__
#define __FSI_COUPLING_PAR_H__

//#include "Parameters.H"
#include "InterfaceLayer.H"
#include "Orchestrator.H"
#include "OpenFoamParAgent.H"
#include "ElmerParAgent.H"
#include "Global.H"
#include <ctime>
#include <unistd.h>

typedef SolverUtils::TransferObjectPar transferagentpar;
typedef openfoamagentpar fluidagentpar;
typedef elmeragentpar    solidagentpar;   
typedef IRAD::Profiler::ProfilerObj ProfilerType;
typedef std::string                 StackType;
typedef IRAD::Comm::CommunicatorObject CommTypeIrad;
typedef IRAD::Global::ParallelGlobalObj<CommTypeIrad,StackType,int,ProfilerType> PGlobalType;

class fsicouplingpar : public impact::orchestrator::couplingbase, public PGlobalType 
{
protected:
  fluidagentpar *fluidsAgent;
  solidagentpar *structuresAgent;
  transferagentpar *transferAgent;
  
  std::string fluidsInterfaceName;
  std::string structuresInterfaceName;
  std::string transferInterfaceName;
  std::string surfUtilInterfaceName;
  std::string simpalInterfaceName;
  std::string simoutInterfaceName;
  std::string siminInterfaceName;

  std::string fluidsWinFnameHDF;
  std::string structuresWinFnameHDF;

  double simulationTime;
  double simulationFinalTime;
  double simulationTimeStep;
  
  int runMode;
  bool writeHDF;
  bool writeVTK;
  int verblevel;

  // probe variables
  int setsProb;
  int probProcId;
  int probNdeId;
  std::string probSolverName;

public:
  fsicouplingpar() : PGlobalType("fsicoupling"), fluidsAgent(NULL), structuresAgent(NULL), transferAgent(NULL), 
                  runMode(0), writeHDF(true), writeVTK(true), setsProb(0) {};
  fsicouplingpar(PGlobalType &globin) : PGlobalType(globin), fluidsAgent(NULL), structuresAgent(NULL), transferAgent(NULL), 
                  runMode(0), writeHDF(true), writeVTK(true), setsProb(0) {
     SetName("fsicouplingpar");
   };
  // verbosity
  void SetVerbLevel(int verb){ verblevel = verb;};
  int VerbLevel() const { return verblevel;}; 

  // probe
  void SetProbe(int inProbProcId, int inProbNdeId, std::string inProbSlvName) {
    if (inProbProcId!=-1) {
       setsProb = 1;
       probProcId = inProbProcId;
       probNdeId = inProbNdeId;
       probSolverName = inProbSlvName;
    }  
  }
  
  // solution transfer
  int TransferDisplacementsToFluid(solidagentpar *solidAgent,fluidagentpar *fluidAgent)
  {
    transferAgent->Interpolate("Displacements","solidDisplacement");
    //transferAgent->Interpolate("solidDisplacement","Displacements");

    return(0);
  };


  int TransferLoadsToStructures(fluidagentpar *fluidAgent,solidagentpar *solidAgent)
  { 

    transferAgent->Transfer("traction","Loads",true);
    
    return(0); 
  };

  void TestTransfer()
  {
    std::stringstream outString;
    double *fluidCoordinates = NULL;
    double *solidCoordinates = NULL;
    std::string fluidsCoordinateName(fluidsInterfaceName+".nc");
    std::string solidsCoordinateName(structuresInterfaceName+".nc");
    COM_get_array(fluidsCoordinateName.c_str(),fluidsAgent->PaneID(),&fluidCoordinates);
    COM_get_array(solidsCoordinateName.c_str(),structuresAgent->PaneID(),&solidCoordinates);
    int numberFluidNodes = fluidsAgent->Coordinates().size()/3;
    int numberSolidNodes = structuresAgent->Coordinates().size()/3;
    if(!fluidCoordinates || !solidCoordinates){
      outString << "FSICoupling::TestTransfer:Error: Failed to get coordinate arrays. Exiting."
                << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
      exit(1);
    }   
    double tolerance = 1e-12;
    double maxdiff = 0;
    const std::vector<double> &fluidCoordArray(fluidsAgent->Coordinates());
    const std::vector<double> &structCoordArray(structuresAgent->Coordinates());
    outString << "BEFORE TRANSFER: " << std::endl;
    for(int i = 0; i < numberFluidNodes;i++){
      outString << "F(" << fluidCoordArray[i*3] << "," << fluidCoordArray[i*3+1] << ","
                << fluidCoordArray[i*3+2] << ")" << std::endl;
    }
    for(int i = 0; i < numberSolidNodes;i++){
      outString << "S(" << structCoordArray[i*3] << "," << structCoordArray[i*3+1] << ","
                << structCoordArray[i*3+2] << ")" << std::endl;
    }
    StdOut(outString.str(),3,true);
    outString.clear();
    outString.str("");
    transferAgent->Interpolate("coords","coords"); // transfer from structures to fluids the node coordinates
    //transferAgent->Transfer("coords","coords"); // transfer from structures to fluids the node coordinates
    outString << "FLUIDS AFTER TRANSFER: " << std::endl;
    for(int i = 0; i < numberFluidNodes;i++){
      double diff1 = std::abs(fluidCoordinates[i*3] - fluidCoordArray[i*3]);
      double diff2 = std::abs(fluidCoordinates[i*3+1] - fluidCoordArray[i*3+1]);
      double diff3 = std::abs(fluidCoordinates[i*3+2] - fluidCoordArray[i*3+2]);
      double diff = std::sqrt(diff1*diff1 + diff2*diff2 + diff3*diff3);
      if(diff > maxdiff) maxdiff = diff;
      if(diff > tolerance){
        outString << "FSICoupling::TestTransfer: Coordinate transfer tolerance exceeded for node " << i+1 
                  << " (" << diff << ")" << std::endl
                  << "(" << fluidCoordinates[i*3] << "," << fluidCoordinates[i*3+1] << "," << fluidCoordinates[i*3+2]
                  << ") : (" << fluidCoordArray[i*3] << "," << fluidCoordArray[i*3+1] << "," << fluidCoordArray[i*3+2]
                  << ")" << std::endl;
      }
    }
    outString << "FSICoupling::TestTransfer: Maximum transferred (s->f) coordinate difference: " << maxdiff << std::endl;
    StdOut(outString.str(),3,true);
    outString.clear();
    outString.str("");
  };
  // operation mode
  void SetRunMode(const std::string &inMode)
  {
    std::stringstream outString;
    if(inMode == "Fluid" ||
       inMode == "fluid"){
      runMode = 1;
    } else if(inMode == "Structure" ||
              inMode == "structure" ||
              inMode == "Solid"     ||
              inMode == "solid"){
      runMode = 2;
    } else {
      runMode = 0;
    }
  };

  void WriteVTK(bool toggle) { writeVTK = toggle;};

  void WriteHDF(bool toggle) { writeHDF = toggle;};


  int WriteAgentToVTK(const std::string &nameRoot,SolverUtils::FEM::SolverAgent &solverAgent)
  {
    std::stringstream outString;
    std::ofstream outStream;
    std::ostringstream timeString;
    timeString << simulationTime;
    std::string fileName(nameRoot+"_"+timeString.str()+".vtk");
    outStream.open(fileName.c_str());
    if(!outStream){
      outString << "FSICoupling::DumpSolution:Error: Could not open output file, "
                << fileName << "." << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
      return(1);
    }
    SolverUtils::WriteVTKToStream(nameRoot, solverAgent, outStream);
    outStream.close();
  };

  virtual int DumpSolution()
  {
    std::stringstream outString;
    outString << "FSICoupling: Dumping solutions." << std::endl;
    StdOut(outString.str(),2,true);
    outString.clear();
    outString.str("");
    if(runMode < 2){
      if(writeHDF)
        SolverUtils::WriteWindow(fluidsInterfaceName,simulationTime);
      if(false)
        WriteAgentToVTK("fluid",*fluidsAgent);
    }  
    if(!(runMode == 1)){
      if(writeHDF)
        SolverUtils::WriteWindow(structuresInterfaceName,simulationTime);
      if(false)
        WriteAgentToVTK("structure",*structuresAgent);
    }
    outString << "FSICoupling: Done with solution dump." << std::endl;
    StdOut(outString.str(),2,true);
    outString.clear();
    outString.str("");
    return(0);
  }
  
  void virtual writeWin(const char* winName, std::string timeMark, std::string& wFnameHdf) 
  {
     int OUT_set = COM_get_function_handle( (simoutInterfaceName + ".set_option").c_str());
     int OUT_write = COM_get_function_handle( (simoutInterfaceName + ".write_dataitem").c_str());
     int OUT_write_ctrl = COM_get_function_handle( (simoutInterfaceName + ".write_rocin_control_file").c_str());
     
     std::string win_out_pre( winName);
     win_out_pre.append(".");
     int OUT_all = COM_get_dataitem_handle((win_out_pre+"all").c_str());
    
     
     std::stringstream ss;
     ss.clear();
     ss.str("");
     ss << "_proc_"
        << Rank();

     std::string hdf_fname, ctrl_fname;
     hdf_fname = (std::string)winName +"_window" + ss.str();
     ctrl_fname = (std::string)winName + "_time_" + timeMark +".txt";

     COM_call_function( OUT_set, "format", "HDF4");
     COM_call_function( OUT_write, (hdf_fname + ".hdf").c_str(), &OUT_all, winName, "0000");
     COM_call_function( OUT_write_ctrl, winName, hdf_fname.c_str(), ctrl_fname.c_str());
   
     wFnameHdf = (std::string)winName +"_window_proc_*.hdf";
  }

  virtual int Initialize(std::vector<std::string> &componentInterfaceNames, 
          double finalTime, double timeStep){

    FunctionEntry("Initialize"); 
    std::stringstream outString; 
    outString << "Final Time = " << finalTime << std::endl;
    outString << "Time Step  = " << timeStep << std::endl;
    StdOut(outString.str(),0,true);
    outString.clear();
    outString.str("");
    
    if(componentInterfaceNames.size() < 2)
      return(1);
       
    fluidsInterfaceName     = componentInterfaceNames[0];
    structuresInterfaceName = componentInterfaceNames[1];
    transferInterfaceName   = componentInterfaceNames[2];
    surfUtilInterfaceName   = componentInterfaceNames[3];
    simpalInterfaceName     = componentInterfaceNames[4];
    simoutInterfaceName     = componentInterfaceNames[5];

    fluidsAgent     = new fluidagentpar;
    structuresAgent = new solidagentpar;
    transferAgent   = new transferagentpar(transferInterfaceName, (this->Communicator()).GetCommunicator(),
                                        verblevel);
       
    // Initialize the fluids module and writing the window
    if(runMode != 2)
      fluidsAgent->Initialize(fluidsInterfaceName, verblevel);
    Communicator().Barrier();
    writeWin((const char *)fluidsInterfaceName.c_str(), "0", fluidsWinFnameHDF); 

       
    // Initialize the structures module and writing the window
    if(runMode != 1)
      structuresAgent->Initialize(structuresInterfaceName, verblevel);
    writeWin((const char *)structuresInterfaceName.c_str(), "0", structuresWinFnameHDF); 
    Communicator().Barrier();

    // accessing a dataitem for fast debugging
    /*
    double* Disps;
    int stride, cap;
    COM_get_array((structuresInterfaceName+".Displacements").c_str(), structuresAgent->PaneID(), 
                   &Disps, &stride, &cap);
    for (int i=0; i<stride*cap; i++)
       std::cout << "Rank " << Rank()
                 << "Item = " << Disps[i]
                 << std::endl;
    */
 
    // Initialize the transfer module's common refinement
    if(runMode == 0) {
      transferAgent->Overlay(structuresInterfaceName, fluidsInterfaceName,
                             structuresWinFnameHDF, fluidsWinFnameHDF);
      //TestTransfer();
    }

    // setting up probe for the solver
    // only implementd for fluids solver
    COM_set_array((fluidsInterfaceName+".setsProb").c_str(),0,&setsProb);
    if(setsProb){
      std::string newDataItemName(probSolverName+".probProcId");
      COM_set_array(newDataItemName.c_str(),0,&probProcId);
      newDataItemName = probSolverName+".probNdeId";
      COM_set_array(newDataItemName.c_str(),0,&probNdeId);
    }
    
    componentAgents.resize(2);
    componentAgents[0] = fluidsAgent;
    componentAgents[1] = structuresAgent;
    
    simulationTime = 0; // ? (restart)
    simulationFinalTime = finalTime;
    simulationTimeStep = timeStep;

    //DumpSolution();
    

    FunctionExit("Initialize"); 
    return(0);
 
  };


  virtual int Run(){
    FunctionEntry("Run");
    // Enter timestepping
    int innerCount = 0;
    int maxSubSteps = 1000;
    int dumpinterval = 1;
    int systemStep = 0;

    // gathering some information for user
    time_t nowStart = time(0);
    char* dtChar = ctime(&nowStart);
    char* hostName;
    gethostname(hostName, 150);

    std::stringstream outString;
    outString << std::endl << std::endl 
              << "************************************************* " << std::endl;
    outString << "*           Starting Stepping in Time           * " << std::endl;
    outString << "************************************************* " << std::endl;
    outString << "* Starting Simulation at " <<  dtChar;
    outString << "* Summary of the simulation:                      " << std::endl;
    outString << "* " << std::endl;
    outString << "*     Simulation Type       = ElmerFoamFSIPar     " << std::endl;
    outString << "*     Simulation start time = " << simulationTime << std::endl;
    outString << "*     Simulation final time = " << simulationFinalTime << std::endl;
    outString << "*     Simulation time step  = " << simulationTimeStep << std::endl;
    outString << "*     Number of time steps  = " << int(simulationFinalTime/simulationTimeStep) << std::endl;
    outString << "* " <<std::endl;
    outString << "*     Hostname = " << hostName << std::endl;
    outString << "*     Number of processors  = " << NProc() << std::endl;
    outString << "************************************************* " << std::endl;
    outString << std::endl;
    StdOut(outString.str(),0,true);
    outString.clear();
    outString.str("");
    while(simulationTime < simulationFinalTime){
      
      // Write some stuff to screen
      outString << "System timestep " << ++systemStep 
                << " @ Time = " << simulationTime << std::endl;
      StdOut(outString.str(),1,true);
      outString.clear();
      outString.str("");

          
      if(!runMode){
        // Transfer displacements @ Tn to fluids
        outString << "Transferring displacements from structures to fluids @ time(" 
                  << simulationTime << ")" << std::endl;
        StdOut(outString.str(),1,true);
        outString.clear();
        outString.str("");
        TransferDisplacementsToFluid(structuresAgent,fluidsAgent);
      }

      if(runMode < 2){
        fluidsAgent->InitializeTimeStep(simulationTime);
        // Step fluids to get loads @ T(n+1)
        outString << "Stepping fluids to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        StdOut(outString.str(),1,true);
        outString.clear();
        outString.str("");
        fluidsAgent->Run(simulationTime+simulationTimeStep);
      }

      if(!runMode){
        outString << "Transferring loads from fluids to structures @ time(" 
                  << simulationTime << ")" << std::endl;
        StdOut(outString.str(),1,true);
        // Transfer loads @ T(n+1) to structures
        //std::cout << "FSICoupling: Transferring loads from fluids to structures @ time(" 
        //          << simulationTime+simulationTimeStep << ")" << std::endl;
        TransferLoadsToStructures(fluidsAgent,structuresAgent);
      }

      if(!(runMode==1)){
        structuresAgent->InitializeTimeStep(simulationTime);
        // Step structures to get displacements @ T(n+1)
        outString << "Stepping structures to time("
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        StdOut(outString.str(),1,true);
        outString.clear();
        outString.str("");
        structuresAgent->Run(simulationTime+simulationTimeStep);
      }
      
      // Finalize timestep and advance time T(n) --> T(n+1)
      bool converged = true;
      if(converged){
        simulationTime += simulationTimeStep;
        outString << "Converged at time(" 
                  << simulationTime << ")" << std::endl; 
        StdOut(outString.str(),1,true);
        outString.clear();
        outString.str("");
        if(runMode < 2)
          fluidsAgent->FinalizeTimeStep(simulationTime+simulationTimeStep);
        if(!(runMode == 1))
          structuresAgent->FinalizeTimeStep(simulationTime+simulationTimeStep);
        innerCount = 0;
      } else {
        innerCount++;
        if(innerCount > maxSubSteps){
          outString << "Failed to converge after "
                    << maxSubSteps << ", giving up." << std::endl; 
          StdOut(outString.str(),0,true);
          outString.clear();
          outString.str("");
          return(1);
        }
      }

      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");

      if(!(systemStep%dumpinterval)){
        //DumpSolution();
      }
      
    }
    Communicator().Barrier(); 
   
    // gathering some information for user
    time_t nowEnd = time(0);
    dtChar = ctime(&nowEnd);
    outString << std::endl << std::endl 
              << "************************************************* " << std::endl;
    outString << "*           Finishing Stepping in Time          * " << std::endl;
    outString << "************************************************* " << std::endl;
    outString << "* Ending simulation at " <<  dtChar;
    outString << "* Total simulation time (s) = " <<  nowEnd - nowStart << std::endl;
    outString << "* Summary of the simulation:                      " << std::endl;
    outString << "* " << std::endl;
    outString << "*     Simulation Type       = ElmerFoamFSIPar     " << std::endl;
    outString << "*     Simulation final time = " << simulationFinalTime << std::endl;
    outString << "*     Simulation time step  = " << simulationTimeStep << std::endl;
    outString << "*     Number of time steps  = " << int(simulationFinalTime/simulationTimeStep) << std::endl;
    outString << "* " <<std::endl;
    outString << "*     Hostname = " << hostName << std::endl;
    outString << "*     Number of processors  = " << NProc() << std::endl;
    outString << "************************************************* " << std::endl;
    outString << std::endl;
    StdOut(outString.str(),0,true);

    FunctionExit("Run");

    return(0);
  };

  virtual int Finalize(){
    // Finalize the fluids module
    if(runMode < 2)
      fluidsAgent->Finalize();
    // Finalize the structures module
    if(!(runMode == 1))
      structuresAgent->Finalize();
    // Anything I need to do to finalize myself
    return(0);
  }
  
  
  /// Helper functions for accessing protected data members.
  std::vector<std::string> getVariable(){
    std::vector<std::string> varVector;
    varVector.push_back(this->fluidsInterfaceName);
    varVector.push_back(this->structuresInterfaceName);
    varVector.push_back(this->transferInterfaceName);
    varVector.push_back(this->surfUtilInterfaceName);
    varVector.push_back(this->simpalInterfaceName);
    varVector.push_back(this->simoutInterfaceName);
    return varVector;
  }

  double getSimulationTime() {return this->simulationTime;}
  double getSimulationTimeStep() {return this->simulationTimeStep;}
  double getSimulationFinalTime() {return this->simulationFinalTime;}
  int getRunMode() {return this->runMode;}

  fluidagentpar * getFluidAgent() {return this->fluidsAgent;}
  solidagentpar * getStructureAgent() {return this->structuresAgent;}

  std::string getFluidIntName() {return this->fluidsInterfaceName;}
  std::string getSolidIntName() {return this->structuresInterfaceName;}

};

#endif
