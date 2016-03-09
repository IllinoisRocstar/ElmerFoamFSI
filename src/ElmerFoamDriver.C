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
#include "Global.H"

typedef SolverUtils::TransferObject transferagent;
typedef openfoamagent fluidagent;
typedef elmeragent    solidagent;   
typedef IRAD::Profiler::ProfilerObj ProfilerType;
typedef std::string                 StackType;
typedef IRAD::Global::GlobalObj<StackType,int,ProfilerType> GlobalType;

class fsicoupling : public impact::orchestrator::couplingbase, public GlobalType 
{
protected:
  fluidagent *fluidsAgent;
  solidagent *structuresAgent;
  transferagent *transferAgent;
  
  std::string fluidsInterfaceName;
  std::string structuresInterfaceName;
  std::string transferInterfaceName;
  std::string surfUtilInterfaceName;
  std::string simpalInterfaceName;

  double simulationTime;
  double simulationFinalTime;
  double simulationTimeStep;
  
  int runMode;
  bool writeHDF;
  bool writeVTK;
  int verblevel;

public:
  fsicoupling() : GlobalType("fsicoupling"), fluidsAgent(NULL), structuresAgent(NULL), transferAgent(NULL), 
                  runMode(0), writeHDF(true), writeVTK(true) {};
  fsicoupling(GlobalType &globin) : GlobalType(globin), fluidsAgent(NULL), structuresAgent(NULL), transferAgent(NULL), 
                  runMode(0), writeHDF(true), writeVTK(true) {};
  // JK: Making stuff for verblevel in fsicoupling functions
  void SetVerbLevel(int verb){ verblevel = verb;};
  int VerbLevel() const { return verblevel;}; 
  // JK: End verblevel
  int TransferDisplacementsToFluid(solidagent *solidAgent,fluidagent *fluidAgent)
  {
    // Masoud: Checking quality of coordinate transfer
    std::vector<double> crdVecSolid1(solidAgent->Coordinates());
    std::vector<double> crdVecFluid1(fluidAgent->Coordinates());
    //std::cout << "Solid Coodinates are : " << std::endl;
    //for (int i = 0; i < crdVecSolid1.size()/3; i++)
    //    std::cout << crdVecSolid1[i*3] << " " 
    //              << crdVecSolid1[i*3+1] << " " 
    //              << crdVecSolid1[i*3+2] << std::endl;
    //std::cout << "ElmerFoamDriver:TransferDisplacementsToFluid:"
    //          << " Fluid Coodinates are : " << std::endl;
    //std::cout << "Fluid Coodinates are : " << std::endl;
    //for (int i = 0; i < crdVecFluid1.size()/3; i++)
    //    std::cout << crdVecFluid1[i*3] << " " 
    //              << crdVecFluid1[i*3+1] << " " 
    //              << crdVecFluid1[i*3+2] << std::endl;
    // Masoud: End
    
    // Masoud: Checking quality of displacement transfer
    double *fluidDisp1 = NULL;
    double *fluidDisp2 = NULL;
    double *solidDisp1 = NULL;
    double *solidDisp2 = NULL;
    std::string fluidsCoordinateName(fluidsInterfaceName+".solidDisplacement");
    std::string solidsCoordinateName(structuresInterfaceName+".Displacements");
    COM_get_array(fluidsCoordinateName.c_str(),fluidsAgent->PaneID(),&fluidDisp1);
    COM_get_array(solidsCoordinateName.c_str(),structuresAgent->PaneID(),&solidDisp1);
    int numberFluidNodes = fluidsAgent->Coordinates().size()/3;
    int numberSolidNodes = structuresAgent->Coordinates().size()/3;
    //std::cout << "Fluid displacements before transfer: " << std::endl;
    //for (int i = 0; i < numberFluidNodes; i++)
    //    std::cout << fluidDisp1[i*3] << " " 
    //              << fluidDisp1[i*3+1] << " " 
    //              << fluidDisp1[i*3+2] << std::endl;
    std::cout << "ElmerFoamDriver:TransferDisplacementsToFluid: Solid displacements for node 0: " << std::endl;
    //std::cout << "Number of nodes: " << numberSolidNodes  <<std::endl;
    //for (int i = 0; i < numberSolidNodes; i++)
        std::cout << solidDisp1[0*3] << " " 
                  << solidDisp1[0*3+1] << " " 
                  << solidDisp1[0*3+2] << std::endl;
    
    transferAgent->Interpolate("Displacements","solidDisplacement");


    COM_get_array(fluidsCoordinateName.c_str(),fluidsAgent->PaneID(),&fluidDisp2);
    COM_get_array(solidsCoordinateName.c_str(),structuresAgent->PaneID(),&solidDisp2);
    std::cout << "ElmerFoamDriver:TransferDisplacementsToFluid: Fluid displacements for node 0: " << std::endl;
    //std::cout << "Number of nodes: " << numberFluidNodes  <<std::endl;
    //for (int i = 0; i < numberFluidNodes; i++)
        std::cout << fluidDisp2[0*3] << " " 
                  << fluidDisp2[0*3+1] << " " 
                  << fluidDisp2[0*3+2] << std::endl;

    // Original
    // transferAgent->Interpolate("Displacements","solidDisplacement");


    // Masoud: Cheking if displacements are updated properly
    //std::vector<double> crdVecSolid2(solidAgent->Coordinates());
    //std::vector<double> crdVecFluid2(fluidAgent->Coordinates());
    //std::cout << "Fluid Coodinate Updates : " << std::endl;
    //for (int i = 0; i < crdVecFluid2.size(); i = i + 3)
    //{
    //    // adding some value to it
    //    std::cout << crdVecFluid2[i]   - crdVecFluid1[i]  << " " 
    //              << crdVecFluid2[i+1] - crdVecFluid1[i+1]<< " " 
    //              << crdVecFluid2[i+2] - crdVecFluid1[i+2]<< std::endl;
    //}
    //std::cout << "Solid Coodinate Updates : " << std::endl;
    //for (int i = 0; i < crdVecSolid2.size(); i = i + 3)
    //{
    //    std::cout << crdVecSolid2[i]   - crdVecSolid1[i]  << " " 
    //              << crdVecSolid2[i+1] - crdVecSolid1[i+1]<< " " 
    //              << crdVecSolid2[i+2] - crdVecSolid1[i+2]<< std::endl;
    //}
    // Masoud: End

    return(0);
  };
  int TransferLoadsToStructures(fluidagent *fluidAgent,solidagent *solidAgent)
  { 
    int stride = 0;
    int cap = 0;
    double *tractions = NULL;
    COM_get_array((fluidsInterfaceName+".traction").c_str(),101,&tractions,&stride,&cap);
    int isize = cap*stride;
    std::cout << "ElmerFoamDriver:TransferLoadsToStructures: " 
              << "Transfering loads to the structures solver." << std::endl;
    //std::cout << "Tractions (driver): " << std::endl;
    //for(int i = 0;i < isize/3;i++)
    //  std::cout << tractions[3*i + 0] << " "
    //            << tractions[3*i + 1] << " "
    //            << tractions[3*i + 2] << std::endl;
    transferAgent->Transfer("traction","Loads",true);
    //Get loads array from solid solver to check
    int solidLoadStride = 0, solidLoadCap = 0;
    double  *solidLoads = NULL;
    COM_get_array((structuresInterfaceName+".Loads").c_str(),11,&solidLoads,
                   &solidLoadStride,&solidLoadCap);
    int solidLoadsize = solidLoadCap*solidLoadStride;
    //for(int i = 0;i < solidLoadsize;i++){
    //  std::cout << std::setprecision(15) << "solidLoads(" << i << ") = " << solidLoads[i] << std::endl;
    //}
    return(0); 
  };
  int TransferPressuresToStructures(fluidagent *fluidAgent,solidagent *solidAgent)
  { 
    int stride = 0, solidStride = 0, solidLoadStride = 0;
    int cap = 0, solidCap = 0, solidLoadCap = 0;
    double *pressures = NULL, *solidPressures = NULL, *solidLoads = NULL;

    //Get pressure array from fluid solver
    COM_get_array((fluidsInterfaceName+".pressure").c_str(),101,&pressures,&stride,&cap);
    int isize = cap*stride;
    //Add atmospheric pressure
    for(int i = 0;i < isize;i++){
      std::cout << "Pressure(" << i << ") = " << pressures[i] << std::endl;
    }
    //Transfer the pressures from the fluid to the solid
    transferAgent->Transfer("pressure","Pressures",true);
    transferAgent->Transfer("pressure","NodePressures",true);

    //////////////////////////Change the pressures to loads 
    //Get handles for dataitems needed
    int solidFaceLoadsHandle = COM_get_dataitem_handle(structuresInterfaceName+".FaceLoads");
    if(solidFaceLoadsHandle < 0){
      std::cout << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for FaceLoads with structure solver" << std::endl;
      return(1);
    }
    int solidPressuresHandle = COM_get_dataitem_handle(structuresInterfaceName+".Pressures");
    if(solidPressuresHandle < 0){
      std::cout << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for Pressures with structure solver" << std::endl;
      return(1);
    }

    //Account for atmospheric pressure when transferring to Elmer (we aren't doing this)
    //Get pressure array from solid solver
    COM_get_array((structuresInterfaceName+".Pressures").c_str(),11,&solidPressures,
                   &solidStride,&solidCap);
    int solidIsize = solidCap*solidStride;
    //Add atmospheric pressure (we aren't doing this - should we?)
    for(int i = 0;i < solidIsize;i++){
      //solidPressures[i] += 101325.0;
      //solidPressures[i] = 1.0;
      std::cout << std::setprecision(15) << "solidPressure(" << i << ") = " << solidPressures[i] << std::endl;
    }

    //Get handles for functions needed
    std::string funcName;
    funcName = surfUtilInterfaceName + ".compute_element_normals";
    int faceNormalsHandle = COM_get_function_handle(funcName.c_str());
    if(faceNormalsHandle < 0){
      std::cout << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for compute_element_normals function " << std::endl;
      return(1);
    }
    funcName = simpalInterfaceName + ".mul";
    int mulHandle = COM_get_function_handle(funcName.c_str());
    if(mulHandle < 0){
      std::cout << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for simpal multiply function " << std::endl;
      return(1);
    }
    funcName = simpalInterfaceName + ".neg";
    int negHandle = COM_get_function_handle(funcName.c_str());
    if(negHandle < 0){
      std::cout << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for simpal negate function " << std::endl;
      return(1);
    }

    //Execute the appropriate function calls
    //I'm gonna try not normalizing them!! 
    int normalize=1;
    //Call function to get face normals, store them in solidFaceLoads, do not
    //normalize them
    COM_call_function(faceNormalsHandle, &solidFaceLoadsHandle, &normalize);
    //Call function to get face normals, store them in solidFaceLoads, do not
    //normalize them
    COM_call_function(mulHandle, &solidFaceLoadsHandle, &solidPressuresHandle, 
                      &solidFaceLoadsHandle); 
    COM_call_function(negHandle, &solidFaceLoadsHandle, &solidFaceLoadsHandle); 
    /////////////////////Done changing pressures to loads

    //////////////////Transfer the structures face loads to structures node loads
    transferagent *structuresTransferAgent   = new transferagent("structuresTransferAgent");
    
    // Initialize the transfer module's common refinement
    structuresTransferAgent->Overlay(structuresInterfaceName,structuresInterfaceName);
 
    // Call the transfer now   
    structuresTransferAgent->Transfer("FaceLoads","Loads");

    //Get loads array from solid solver to check
    COM_get_array((structuresInterfaceName+".Loads").c_str(),11,&solidLoads,
                   &solidLoadStride,&solidLoadCap);
    int solidLoadsize = solidLoadCap*solidLoadStride;
    for(int i = 0;i < solidLoadsize;i++){
      std::cout << std::setprecision(15) << "solidLoads(" << i << ") = " << solidLoads[i] << std::endl;
    }

    //Print out the face normals from the fluid solver as a check
    int fluidFaceNormalsHandle = COM_get_dataitem_handle(fluidsInterfaceName+".normals");
    if(fluidFaceNormalsHandle < 0){
      std::cout << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for FaceNormals with fluids solver" << std::endl;
      return(1);
    }
    normalize=0;
    COM_call_function(faceNormalsHandle, &fluidFaceNormalsHandle, &normalize);
    double *fluidFaceNormals = NULL;
    COM_get_array((fluidsInterfaceName+".normals").c_str(),101,&fluidFaceNormals,&stride,&cap);
    isize = stride*cap; 

    std::cout << "stride = " << stride << std::endl;
    std::vector<double> sums (stride, 0.0);
    std::cout << "Driver: fluid face normals:" << std::endl;
    for(int i=0; i < cap; i++){
      for(int j=0; j < stride; j++){
        std::cout << fluidFaceNormals[stride*i + j] << " ";
        sums[j] += fluidFaceNormals[stride*i + j];
      }
      std::cout << std::endl;
    }
    std::cout << "sums: ";
    for(int i=0; i < sums.size(); i++)
      std::cout << sums[i] << " ";
    std::cout << std::endl;

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
    std::cout << "FSICoupling: Done with solution dump." << std::endl;
    return(0);
  }
  virtual int Initialize(std::vector<std::string> &componentInterfaceNames, 
          double finalTime, double timeStep){
 
    FunctionEntry("Initialize"); 
    //SetName("Initialize"); 
    std::stringstream outString; 
    //Masoud 
    outString << "Final Time = " << finalTime << std::endl;
    outString << "Time Step  = " << timeStep << std::endl;
    //Masoud: End
    StdOut(outString.str(),0,true);
    outString.clear();
    outString.str("");
    
    if(componentInterfaceNames.size() < 2)
      return(1);
    
    fluidsInterfaceName     = componentInterfaceNames[0];
    structuresInterfaceName = componentInterfaceNames[1];
    transferInterfaceName   = componentInterfaceNames[2];
    surfUtilInterfaceName     = componentInterfaceNames[3];
    simpalInterfaceName      = componentInterfaceNames[4];

    fluidsAgent     = new fluidagent;
    structuresAgent = new solidagent;
    transferAgent   = new transferagent(transferInterfaceName);
    
    // Initialize the fluids module
    if(runMode != 2)
      fluidsAgent->Initialize(fluidsInterfaceName);

    // Initialize the structures module
    if(runMode != 1)
      structuresAgent->Initialize(structuresInterfaceName, verblevel);

    // Initialize the transfer module's common refinement
    if(runMode == 0) {
      transferAgent->Overlay(structuresInterfaceName,fluidsInterfaceName);
      //      TestTransfer();
    }
    //    exit(1);

    //JK: Setting the verbosity with the modules
    int *fluidsVerb = NULL;
    //int *solidsVerb = NULL;
    std::string fluidsVerbName(fluidsInterfaceName + ".verbosity");
    //std::string solidsVerbName(structuresInterfaceName + ".verbosity");
    COM_get_array(fluidsVerbName.c_str(),fluidsAgent->PaneID(),&fluidsVerb);
    //COM_get_array(solidsVerbName.c_str(),structuresAgent->PaneID(),&solidsVerb);
   
    *fluidsVerb = verblevel;
    //*solidsVerb = verblevel;
    //JK: End verbosity

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
    std::stringstream outString;
    outString << std::endl << std::endl 
              << "***************************************** " << std::endl;
    outString << "       Starting Stepping in Time          " << std::endl;
    outString << "***************************************** " << std::endl << std::endl;
    outString << "ElmerFoamDriver:Run: Summary of the simulation " << std::endl;
    outString << "     Simulation Type       = ElmerFoamFSI" << std::endl;
    outString << "     Simulation final time = " << simulationFinalTime << std::endl;
    outString << "     Simulation time step  = " << simulationTimeStep << std::endl;
    outString << std::endl;
    StdOut(outString.str(),0,true);
    outString.clear();
    outString.str("");
    while(simulationTime < simulationFinalTime){
      
      // if(!(time%screenInterval) && (innerCount == 0)){
      // Write some stuff to screen
      /* WK
      std::cout << "ElmerFoamDriver:Run: System timestep " << ++systemStep 
                << " @ Time = " << simulationTime << std::endl;*/
      outString << "ElmerFoamDriver:Run: System timestep " << ++systemStep
                << " @ Time = " << simulationTime << std::endl;
      // }
          
      if(!runMode){
        // Transfer displacements @ Tn to fluids
       /* WK
        std::cout << "ElmerFoamDriver:Run: Transferring displacements from structures to fluids @ time(" 
                  << simulationTime << ")" << std::endl; */
        outString << "ElmerFoamDriver:Run: Transferring displacements from structures to fluids @ time("
                  << simulationTime << ")" << std::endl; 
        TransferDisplacementsToFluid(structuresAgent,fluidsAgent);
      }
      if(runMode < 2){
        fluidsAgent->InitializeTimeStep(simulationTime);
        // Step fluids to get loads @ T(n+1)
        /* WK
        std::cout << "ElmerFoamDriver:Run: Stepping fluids to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl; */
        outString << "ElmerFoamDriver:Run: Stepping fluids to time("
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        fluidsAgent->Run(simulationTime+simulationTimeStep);
      }

      if(!runMode){
        // Transfer loads @ T(n+1) to structures
        //std::cout << "FSICoupling: Transferring loads from fluids to structures @ time(" 
        //          << simulationTime+simulationTimeStep << ")" << std::endl;
        TransferLoadsToStructures(fluidsAgent,structuresAgent);
        // Transfer pressures @ T(n+1) to structures
        //std::cout << "FSICoupling: Transferring pressures from fluids to structures @ time(" 
        //          << simulationTime+simulationTimeStep << ")" << std::endl;
        //TransferPressuresToStructures(fluidsAgent,structuresAgent);
        //std::cout << "FSICoupling: Transferring pressures from fluids to structures with "
        //          << "TransferLoad function" << std::endl; 
        //int err = transferAgent->TransferLoad("pressure","Loads",true);
        //if(err == -1){
        //  std::cout << "FSICoupling: Error: Unable to transfer pressures from fluids agent" 
        //            << "to loads at structures agent." << std::endl;
        //  exit(1);
        //}
      }
      if(!(runMode==1)){
        structuresAgent->InitializeTimeStep(simulationTime);
        // Step structures to get displacements @ T(n+1)
        /* WK
        std::cout << "ElmerFoamDriver:Run: Stepping structures to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl; */
        outString << "ElmerFoamDriver:Run: Stepping structures to time("
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        structuresAgent->Run(simulationTime+simulationTimeStep);
      }
      
      // Finalize timestep and advance time T(n) --> T(n+1)
      bool converged = true;
      if(converged){
        simulationTime += simulationTimeStep;
      /* WK
        std::cout << "ElmerFoamDriver:Run: Converged at time(" 
                  << simulationTime << ")" << std::endl; */
        outString << "ElmerFoamDriver:Run: Converged at time("
                  << simulationTime << ")" << std::endl;
        if(runMode < 2)
          fluidsAgent->FinalizeTimeStep(simulationTime+simulationTimeStep);
        if(!(runMode == 1))
          structuresAgent->FinalizeTimeStep(simulationTime+simulationTimeStep);
        innerCount = 0;
      } else {
        innerCount++;
        if(innerCount > maxSubSteps){
         /* WK
          std::cout << "ElmerFoamDriver:Run: Failed to converge after "
                    << maxSubSteps << ", giving up." << std::endl; */
          outString << "ElmerFoamDriver:Run: Failed to converge after "
                    << maxSubSteps << ", giving up." << std::endl;
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
      
};
//   }
// }




COM_EXTERN_MODULE(ElmerCSC);
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

    fsicoupling fsiCoupler;
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

    std::cout << "Before Running FunctionExit" << std::endl;
    FunctionExit("Run");    
    std::cout << "After Running FunctionExit" << std::endl;
    // return 0 for success
    return(0);
  };
};
