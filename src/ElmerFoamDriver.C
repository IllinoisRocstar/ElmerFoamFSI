/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 

#include "Driver.H"
#include "Parameters.H"
#include "InterfaceLayer.H"
#include "Orchestrator.H"
#include "OpenFoamAgent.H"
#include "ElmerAgent.H"

typedef SolverUtils::TransferObject transferagent;
typedef openfoamagent fluidagent;
typedef elmeragent    solidagent;    

class fsicoupling : public impact::orchestrator::couplingbase
{
protected:
  fluidagent *fluidsAgent;
  solidagent *structuresAgent;
  transferagent *transferAgent;
  
  std::string fluidsInterfaceName;
  std::string structuresInterfaceName;
  std::string transferInterfaceName;


  double simulationTime;
  double simulationFinalTime;
  double simulationTimeStep;
  
  int runMode;
  bool writeHDF;
  bool writeVTK;

public:
  fsicoupling() : fluidsAgent(NULL), structuresAgent(NULL), transferAgent(NULL), 
                  runMode(0), writeHDF(true), writeVTK(true) {};
  int TransferDisplacementsToFluid(solidagent *solidAgent,fluidagent *fluidAgent)
  {
    transferAgent->Interpolate("Displacements","solidDisplacement");
    return(0);
  };
  int TransferLoadsToStructures(fluidagent *fluidAgent,solidagent *solidAgent)
  { 
    int stride = 0;
    int cap = 0;
    double *tractions = NULL;
    COM_get_array((fluidsInterfaceName+".traction").c_str(),101,&tractions,&stride,&cap);
    int isize = cap*stride;
    for(int i = 0;i < isize;i++)
      std::cout << "Load(" << i << ") = " << tractions[i] << std::endl;
    transferAgent->Transfer("traction","Loads",true);
    return(0); 
  };
  void SetRunMode(const std::string &inMode)
  {
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
  void TestTransfer()
  {
    double *fluidCoordinates = NULL;
    double *solidCoordinates = NULL;
    std::string fluidsCoordinateName(fluidsInterfaceName+".nc");
    std::string solidsCoordinateName(structuresInterfaceName+".nc");
    COM_get_array(fluidsCoordinateName.c_str(),fluidsAgent->PaneID(),&fluidCoordinates);
    COM_get_array(solidsCoordinateName.c_str(),structuresAgent->PaneID(),&solidCoordinates);
    int numberFluidNodes = fluidsAgent->Coordinates().size()/3;
    int numberSolidNodes = structuresAgent->Coordinates().size()/3;
    if(!fluidCoordinates || !solidCoordinates){
      std::cerr << "FSICoupling::TestTransfer:Error: Failed to get coordinate arrays. Exiting."
                << std::endl;
      exit(1);
    }
   
    double tolerance = 1e-12;
    double maxdiff = 0;
    const std::vector<double> &fluidCoordArray(fluidsAgent->Coordinates());
    const std::vector<double> &structCoordArray(structuresAgent->Coordinates());
    std::cout << "BEFORE TRANSFER: " << std::endl;
    for(int i = 0; i < numberFluidNodes;i++){
      std::cout << "F(" << fluidCoordArray[i*3] << "," << fluidCoordArray[i*3+1] << ","
                << fluidCoordArray[i*3+2] << ")" << std::endl;
    }
    for(int i = 0; i < numberSolidNodes;i++){
      std::cout << "S(" << structCoordArray[i*3] << "," << structCoordArray[i*3+1] << ","
                << structCoordArray[i*3+2] << ")" << std::endl;
    }
    transferAgent->Interpolate("coords","coords"); // transfer from structures to fluids the node coordinates
    std::cout << "FLUIDS AFTER TRANSFER: " << std::endl;
    for(int i = 0; i < numberFluidNodes;i++){
      double diff1 = std::abs(fluidCoordinates[i*3] - fluidCoordArray[i*3]);
      double diff2 = std::abs(fluidCoordinates[i*3+1] - fluidCoordArray[i*3+1]);
      double diff3 = std::abs(fluidCoordinates[i*3+2] - fluidCoordArray[i*3+2]);
      double diff = std::sqrt(diff1*diff1 + diff2*diff2 + diff3*diff3);
      if(diff > maxdiff) maxdiff = diff;
      if(diff > tolerance){
        std::cout << "FSICoupling::TestTransfer: Coordinate transfer tolerance exceeded for node " << i+1 
                  << " (" << diff << ")" << std::endl
                  << "(" << fluidCoordinates[i*3] << "," << fluidCoordinates[i*3+1] << "," << fluidCoordinates[i*3+2]
                  << ") : (" << fluidCoordArray[i*3] << "," << fluidCoordArray[i*3+1] << "," << fluidCoordArray[i*3+2]
                  << ")" << std::endl;
      }
    }
    std::cout << "FSICoupling::TestTransfer: Maximum transferred (s->f) coordinate difference: " << maxdiff << std::endl;

  };
  int WriteAgentToVTK(const std::string &nameRoot,SolverUtils::FEM::SolverAgent &solverAgent)
  {
    std::ofstream outStream;
    std::ostringstream timeString;
    timeString << simulationTime;
    std::string fileName(nameRoot+"_"+timeString.str()+".vtk");
    outStream.open(fileName.c_str());
    if(!outStream){
      std::cerr << "FSICoupling::DumpSolution:Error: Could not open output file, "
                << fileName << "." << std::endl;
      return(1);
    }
    SolverUtils::WriteVTKToStream(nameRoot, solverAgent, outStream);
    outStream.close();
  };
  virtual int DumpSolution()
  {
    std::cout << "FSICoupling: Dumping solutions." << std::endl;
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
    return(0);
  }
  virtual int Initialize(std::vector<std::string> &componentInterfaceNames){
    
    if(componentInterfaceNames.size() < 2)
      return(1);
    
    fluidsInterfaceName     = componentInterfaceNames[0];
    structuresInterfaceName = componentInterfaceNames[1];
    transferInterfaceName   = componentInterfaceNames[2];

    fluidsAgent     = new fluidagent;
    structuresAgent = new solidagent;
    transferAgent   = new transferagent(transferInterfaceName);
    
    // Initialize the fluids module
    if(runMode != 2)
      fluidsAgent->Initialize(fluidsInterfaceName);

    // Initialize the structures module
    if(runMode != 1)
      structuresAgent->Initialize(structuresInterfaceName);

    // Initialize the transfer module's common refinement
    if(runMode == 0) {
      transferAgent->SetVerbose(52);
      transferAgent->Overlay(structuresInterfaceName,fluidsInterfaceName);
      //      TestTransfer();
    }
    //    exit(1);

    componentAgents.resize(2);
    componentAgents[0] = fluidsAgent;
    componentAgents[1] = structuresAgent;
    
    simulationTime = 0; // ? (restart)
    simulationFinalTime = 10.0;
    simulationTimeStep = 1e-3;

    DumpSolution();

    return(0);
  };
  virtual int Run(){
    // Enter timestepping
    int innerCount = 0;
    int maxSubSteps = 1000;
    int dumpinterval = 1;
    int systemStep = 0;
    while(simulationTime < simulationFinalTime){
      
      // if(!(time%screenInterval) && (innerCount == 0)){
      // Write some stuff to screen
      std::cout << "FSICoupling: System timestep " << ++systemStep 
                << " @ Time = " << simulationTime << std::endl;
      // }
          
      if(!runMode){
        // Transfer displacements @ Tn to fluids
        std::cout << "FSICoupling: Transferring displacements from structures to fluids @ time(" 
                  << simulationTime << ")" << std::endl;
        TransferDisplacementsToFluid(structuresAgent,fluidsAgent);
      }
      if(runMode < 2){
        fluidsAgent->InitializeTimeStep(simulationTime);
        // Step fluids to get loads @ T(n+1)
        std::cout << "FSICoupling: Stepping fluids to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        fluidsAgent->Run(simulationTime+simulationTimeStep);
      }

      if(!runMode){
        // Transfer loads @ T(n+1) to structures
        std::cout << "FSICoupling: Transferring loads from fluids to structures @ time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        TransferLoadsToStructures(fluidsAgent,structuresAgent);
      }
      if(!(runMode==1)){
        structuresAgent->InitializeTimeStep(simulationTime);
        // Step structures to get displacements @ T(n+1)
        std::cout << "FSICoupling: Stepping structures to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        structuresAgent->Run(simulationTime+simulationTimeStep);
      }
      
      // Finalize timestep and advance time T(n) --> T(n+1)
      bool converged = true;
      if(converged){
        simulationTime += simulationTimeStep;
        std::cout << "FSICoupling: Converged at time(" 
                  << simulationTime << ")" << std::endl;
        if(runMode < 2)
          fluidsAgent->FinalizeTimeStep(simulationTime+simulationTimeStep);
        if(!(runMode == 1))
          structuresAgent->FinalizeTimeStep(simulationTime+simulationTimeStep);
        innerCount = 0;
      } else {
        innerCount++;
        if(innerCount > maxSubSteps){
          std::cout << "FSICoupling: Failed to converge after "
                    << maxSubSteps << ", giving up." << std::endl;
          return(1);
        }
      }
      
      if(!(systemStep%dumpinterval)){
        DumpSolution();
      }
    }
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
      
};
//   }
// }

COM_EXTERN_MODULE(SolverModule);
COM_EXTERN_MODULE(OpenFoamFSI);

namespace ElmerFoamFSI {

  int SerialDriverProgram::Run()
  {
    // FunctionEntry("NAME"): Updates the user-defined stack and 
    // the program profiles with timing information. The placement
    // of FunctionEntry and FunctionExit calls is at the developer's
    // discretion.  
    FunctionEntry("Run");
    std::ostringstream outString;
    IRAD::Util::Parameters userParameters;
    std::ifstream paramFileIn;
    paramFileIn.open(input_name.c_str());
    paramFileIn >> userParameters;
    if(verblevel > 1){
      outString << "User Parameters: " << std::endl
                << userParameters << std::endl;
      StdOut(outString.str());
      outString.clear();
      outString.str("");
    }

    std::string fluidSolverName(userParameters.GetValue("FluidSolver"));
    std::string solidSolverName(userParameters.GetValue("SolidSolver"));
    std::string transferServiceName(userParameters.GetValue("TransferService"));
    std::string runMode(userParameters.GetValue("RunMode"));

    
    double time_final = 0;
    time_final = userParameters.GetValue<double>("FinalTime");
    if(time_final <= 0){
      // warn = default
      time_final = 1e+24;
    }
    
    double timestep = 0;
    timestep = userParameters.GetValue<double>("TimeStep");
    if(timestep <= 0){
      // warn = default
      timestep = 1e-5;
    }

    if(fluidSolverName.empty()){
      outString << "Failed to specify FluidSolver parameter.\n";
      ErrOut(outString.str());
      FunctionExit("Run");
      return(1);
    }
    if(solidSolverName.empty()){
      outString << "Failed to specify SolidSolver parameter.\n";
      ErrOut(outString.str());
      FunctionExit("Run");
      return(1);
    }

    FunctionEntry("LoadModules");
    COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
    COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
    FunctionExit("LoadModules");

    fsicoupling fsiCoupler;
    fsiCoupler.SetRunMode(runMode);

    std::vector<std::string> componentInterfaceNames;
    componentInterfaceNames.push_back("FluidsComponentInterface");
    componentInterfaceNames.push_back("StructuresComponentInterface");
    componentInterfaceNames.push_back("TransferInterface");

    FunctionEntry("coupler.Init");
    fsiCoupler.Initialize(componentInterfaceNames);
    FunctionExit("coupler.Init");

    FunctionEntry("coupler.Run");
    fsiCoupler.Run();
    FunctionExit("coupler.Run");

    FunctionEntry("coupler.Finalize");
    fsiCoupler.Finalize();
    FunctionExit("coupler.Finalize");

    COM_unload_module(fluidSolverName.c_str(),"FluidsComponentInterface");
    COM_unload_module(solidSolverName.c_str(),"StructuresComponentInterface");

    FunctionExit("Run");    
    // return 0 for success
    return(0);
  };
};
