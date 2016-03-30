/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 

#include "Driver.H"
#include "Parameters.H"
//#include "InterfaceLayer.H"
//#include "Orchestrator.H"
//#include "OpenFoamAgent.H"
//#include "ElmerAgent.H"
//#include "Global.H"
#include "FsiCoupling.H"

//typedef SolverUtils::TransferObject transferagent;
//typedef openfoamagent fluidagent;
//typedef elmeragent    solidagent;   
//typedef IRAD::Profiler::ProfilerObj ProfilerType;
//typedef std::string                 StackType;
//typedef IRAD::Global::GlobalObj<StackType,int,ProfilerType> GlobalType;

COM_EXTERN_MODULE(ElmerCSC);
COM_EXTERN_MODULE(OpenFoamFSI);

namespace ElmerFoamFSI {

  int SerialDriverProgram::DriverRun()
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
    outString << "User Parameters: " << std::endl
                << userParameters << std::endl;
    StdOut(outString.str(), 1, true);
    outString.clear();
    outString.str("");


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
    if(timestep <= 1e-5){
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
    COM_load_module("SurfUtil","SurfUtil");
    COM_load_module("Simpal","Simpal");
    FunctionExit("LoadModules");

    std::cout << "line " << __LINE__ << std::endl;
    fsicoupling fsiCoupler(*this);
    std::cout << "line " << __LINE__ << std::endl;
    fsiCoupler.SetRunMode(runMode);
    fsiCoupler.SetVerbLevel(verblevel);

    std::vector<std::string> componentInterfaceNames;
    componentInterfaceNames.push_back("FluidsComponentInterface");
    componentInterfaceNames.push_back("StructuresComponentInterface");
    componentInterfaceNames.push_back("TransferInterface");
    componentInterfaceNames.push_back("SurfUtil");
    componentInterfaceNames.push_back("Simpal");

    FunctionEntry("coupler.Init");
    fsiCoupler.Initialize(componentInterfaceNames, time_final, timestep);
    FunctionExit("coupler.Init");

    FunctionEntry("coupler.Run");
    fsiCoupler.Run();
    FunctionExit("coupler.Run");

    FunctionEntry("coupler.Finalize");
    fsiCoupler.Finalize();
    FunctionExit("coupler.Finalize");

    //COM_unload_module(fluidSolverName.c_str(),"FluidsComponentInterface");
    COM_unload_module(solidSolverName.c_str(),"StructuresComponentInterface");

    outString << "Before Running FunctionExit" << std::endl;
    StdOut(outString.str(),2,true);
    outString.clear();
    outString.str("");

    FunctionExit("Run");    
    outString << "After Running FunctionExit" << std::endl;
    StdOut(outString.str(),2,true);
    outString.clear();
    outString.str("");
    
    // return 0 for success
    return(0);
  };
};
