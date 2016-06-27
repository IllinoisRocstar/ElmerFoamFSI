/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 

#include "Driver.H"
#include "Parameters.H"
#include "FsiCouplingPar.H"

//typedef SolverUtils::TransferObject transferagent;
//typedef openfoamagent fluidagent;
//typedef elmeragent    solidagent;   
//typedef IRAD::Profiler::ProfilerObj ProfilerType;
//typedef std::string                 StackType;
//typedef IRAD::Global::GlobalObj<StackType,int,ProfilerType> GlobalType;

COM_EXTERN_MODULE(ElmerCSCParallel);
COM_EXTERN_MODULE(OpenFoamFSIPar);

namespace ElmerFoamFSI {

  int ParallelDriverProgram::DriverRun()
  {
    
    // FunctionEntry("NAME"): Updates the user-defined stack and 
    // the program profiles with timing information. The placement
    // of FunctionEntry and FunctionExit calls is at the developer's
    // discretion.  
    FunctionEntry("Run");


    int myid  = _communicator.Rank();
    int nproc = _communicator.Size();
    bool use_file = !output_name.empty();
 
    // Put out a quick blurb about number of procs just
    // to give the user confidence we are actually running
    // in parallel.  Note that when using the "StdOut" method
    // that it automatically does output only on rank 0. If 
    // the developer wants asyncrhonous output, then they 
    // have to revert to using standard streams.
    std::ostringstream RepStr;
    if(verblevel > 1)
      RepStr << "Running on " << nproc << " processors." << std::endl;
    StdOut(RepStr.str());
    _communicator.Barrier();
    if(verblevel > 1)
      StdOut("All procesors ready.\n");

    // reading user parameters from input file
    std::ostringstream outString;
    IRAD::Util::Parameters userParameters;
    std::ifstream paramFileIn;
    paramFileIn.open(input_name.c_str());
    paramFileIn >> userParameters;
    outString.clear();
    outString.str("");
    outString << "User Parameters: " << std::endl
                << userParameters << std::endl;
    StdOut(outString.str(), 1, true);
    _communicator.Barrier();

    std::string fluidSolverName(userParameters.GetValue("FluidSolver"));
    std::string solidSolverName(userParameters.GetValue("SolidSolver"));
    std::string transferServiceName(userParameters.GetValue("TransferService"));
    std::string runMode(userParameters.GetValue("RunMode"));
    
    // processing other extra parameters passed by user
    // check request for probing
    // ProbSolverName =
    //      FluidsComponentInterface     : for probing fluid solver
    //      StructuresComponentInterface : for probing structures solver

    int probProcId(-1), probNdeId(0);
    std::string probSolverName("");
    if (userParameters.IsSet("ProbSolverName")) {
       probSolverName = userParameters.GetValue("ProbSolverName");
       probProcId = userParameters.GetValue<int>("ProbProcId");
       probNdeId = userParameters.GetValue<int>("ProbNdeId");
       if (probProcId>=nproc || probProcId<0) {
	 outString << "The requested process ID for probing is not valid.\n";
	 ErrOut(outString.str());
	 _communicator.SetExit(1);
       } else {
	  outString.clear();
	  outString.str("");
	  outString << "Probing "
                    << probSolverName
                    << " solution at rank " 
                    << probProcId 
                    << " registered node "
                    << probNdeId
                    << std::endl;
	  StdOut(outString.str(), 1, true);
      }
    }
    // Check to see if an error condition was set 
    // in the file open block above.  If so, then
    // return with an error code.
    if(_communicator.Check()){
        // don't forget to tell the profiler/stacker the
        // function is exiting.
        FunctionExit("Run");
        return(1);
    }

    // Open the specified output file for writing on rank 0
    if(use_file && !myid){
      Ouf.open(output_name.c_str(),std::ios::app);
      if(!Ouf){
        // If the output file failed to open, notify
        // to error stream and return non-zero
        std::ostringstream Ostr;
        Ostr << "Error: Unable to open output file, " << output_name << ".";
        ErrOut(Ostr.str());
        // In parallel, we don't return right away since 
        // this part of the code is only done on proc 0.
        // Instead, the error value is set in the communicator
        // which will indicate to all processors that there 
        // has been some error.
        _communicator.SetExit(1);
      }
    }
    // Check to see if an error condition was set 
    // in the file open block above.  If so, then
    // return with an error code.
    if(_communicator.Check()){
        // don't forget to tell the profiler/stacker the
        // function is exiting.
        FunctionExit("Run");
        return(1);
    }

    double time_final = 0;
    time_final = userParameters.GetValue<double>("FinalTime");
    if(time_final <= 0){
      // setting to default
      time_final = 1.0e-3;
    }
    
    double timestep = 0;
    timestep = userParameters.GetValue<double>("TimeStep");
    if(timestep <= 1e-5){
      // setting to default
      timestep = 1.0e-5;
    }

    if(fluidSolverName.empty()){
      outString << "Failed to specify FluidSolver parameter.\n";
      ErrOut(outString.str());
      _communicator.SetExit(1);
    }
    if(solidSolverName.empty()){
      outString << "Failed to specify SolidSolver parameter.\n";
      ErrOut(outString.str());
      _communicator.SetExit(1);
    }
    
    // Check to see if an error condition was set 
    // in the file open block above.  If so, then
    // return with an error code.
    if(_communicator.Check()){
       // don't forget to tell the profiler/stacker the
       // function is exiting.
       FunctionExit("Run");
       return(1);
    }

    FunctionEntry("LoadModules");
    COM_load_module(fluidSolverName.c_str(),"FluidsComponentInterface");
    COM_load_module(solidSolverName.c_str(),"StructuresComponentInterface");
    COM_load_module("SurfUtil","SurfUtil");
    COM_load_module("Simpal","Simpal");
    COM_load_module("SimOUT","Simout");
    FunctionExit("LoadModules");

    fsicouplingpar fsiCouplerPar(*this);
    fsiCouplerPar.SetRunMode(runMode);
    fsiCouplerPar.SetVerbLevel(verblevel);
    fsiCouplerPar.SetProbe(probProcId, probNdeId, probSolverName);

    
    std::vector<std::string> componentInterfaceNames;
    componentInterfaceNames.push_back("FluidsComponentInterface");
    componentInterfaceNames.push_back("StructuresComponentInterface");
    componentInterfaceNames.push_back("TransferInterface");
    componentInterfaceNames.push_back("SurfUtil");
    componentInterfaceNames.push_back("Simpal");
    componentInterfaceNames.push_back("Simout");

     
    FunctionEntry("fsiCouplerPar.Initialize");
    fsiCouplerPar.Initialize(componentInterfaceNames, time_final, timestep);
    FunctionExit("fsiCouplerPar.Initialize");
    _communicator.Barrier();
    
    FunctionEntry("fsiCouplerPar.Run");
    fsiCouplerPar.Run();
    FunctionExit("fsiCouplerPar.Run");    
    _communicator.Barrier();

    FunctionEntry("fsiCouplerPar.Finalize");
    fsiCouplerPar.Finalize();
    FunctionExit("fsiCouplerPar.Finalize");
    _communicator.Barrier();
    
    COM_unload_module(fluidSolverName.c_str(),"FluidsComponentInterface");
    COM_unload_module(solidSolverName.c_str(),"StructuresComponentInterface");
    _communicator.Barrier();
    
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
