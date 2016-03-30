/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 

#ifndef __FSI_COUPLING_H__
#define __FSI_COUPLING_H__

//#include "Parameters.H"
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
                  runMode(0), writeHDF(true), writeVTK(true) {
     SetName("fsicoupling");
   };
  // JK: Making stuff for verblevel in fsicoupling functions
  void SetVerbLevel(int verb){ verblevel = verb;};
  int VerbLevel() const { return verblevel;}; 
  // JK: End verblevel
  int TransferDisplacementsToFluid(solidagent *solidAgent,fluidagent *fluidAgent)
  {
    std::stringstream outString;
    // Masoud: Checking quality of coordinate transfer
    std::vector<double> crdVecSolid1(solidAgent->Coordinates());
    std::vector<double> crdVecFluid1(fluidAgent->Coordinates());
    if ( verblevel >= 3 ) {
       outString << "Solid Coodinates are : " << std::endl;
       for (int i = 0; i < crdVecSolid1.size()/3; i++)
          outString << crdVecSolid1[i*3] << " " 
                    << crdVecSolid1[i*3+1] << " " 
                    << crdVecSolid1[i*3+2] << std::endl;
       StdOut(outString.str(),3, true);
       outString.clear();
       outString.str("");
       outString << " Fluid Coodinates are : " << std::endl;
       for (int i = 0; i < crdVecFluid1.size()/3; i++)
          outString << crdVecFluid1[i*3]   << " " 
                    << crdVecFluid1[i*3+1] << " " 
                    << crdVecFluid1[i*3+2] << std::endl;
       StdOut(outString.str(),3,true);
       outString.clear();
       outString.str("");
    }
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
    if (verblevel >= 3){
      outString << "Fluid displacements before transfer: " << std::endl;
      for (int i = 0; i < numberFluidNodes; i++)
          outString << fluidDisp1[i*3] << " " 
                    << fluidDisp1[i*3+1] << " " 
                    << fluidDisp1[i*3+2] << std::endl;
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
      outString << " Solid displacements for node 0: " << std::endl;
      outString << "Number of nodes: " << numberSolidNodes  << std::endl;
      for (int i = 0; i < numberSolidNodes; i++)
          outString << solidDisp1[i*3] << " " 
                    << solidDisp1[i*3+1] << " " 
                    << solidDisp1[i*3+2] << std::endl;
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
    }    

    transferAgent->Interpolate("Displacements","solidDisplacement");

    if (verblevel >= 3){
      COM_get_array(fluidsCoordinateName.c_str(),fluidsAgent->PaneID(),&fluidDisp2);
      COM_get_array(solidsCoordinateName.c_str(),structuresAgent->PaneID(),&solidDisp2);
      outString << "Number of fluid nodes: " << numberFluidNodes  <<std::endl;
      outString << "Fluid displacements after transfer : " << std::endl;
      for (int i = 0; i < numberFluidNodes; i++)
          outString << fluidDisp2[0*3] << " " 
                    << fluidDisp2[0*3+1] << " " 
                    << fluidDisp2[0*3+2] << std::endl;
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
  
      // Masoud: Cheking if displacements are updated properly
      std::vector<double> crdVecSolid2(solidAgent->Coordinates());
      std::vector<double> crdVecFluid2(fluidAgent->Coordinates());
      outString << "Fluid Coodinate Updates : " << std::endl;
      for (int i = 0; i < crdVecFluid2.size(); i = i + 3)
      {
          // adding some value to it
          outString << crdVecFluid2[i]   - crdVecFluid1[i]  << " " 
                    << crdVecFluid2[i+1] - crdVecFluid1[i+1]<< " " 
                    << crdVecFluid2[i+2] - crdVecFluid1[i+2]<< std::endl;
      }
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
      outString << "Solid Coodinate Updates : " << std::endl;
      for (int i = 0; i < crdVecSolid2.size(); i = i + 3)
      {
          outString << crdVecSolid2[i]   - crdVecSolid1[i]  << " " 
                    << crdVecSolid2[i+1] - crdVecSolid1[i+1]<< " " 
                    << crdVecSolid2[i+2] - crdVecSolid1[i+2]<< std::endl;
      }
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
    }
    // Masoud: End

    return(0);
  };


  int TransferLoadsToStructures(fluidagent *fluidAgent,solidagent *solidAgent)
  { 
    std::stringstream outString;
    int stride = 0;
    int cap = 0;
    double *tractions = NULL;
    COM_get_array((fluidsInterfaceName+".traction").c_str(),101,&tractions,&stride,&cap);
    int isize = cap*stride;
    outString << "Transfering loads to the structures solver." << std::endl;
    StdOut(outString.str(),2,true);
    outString.clear();
    outString.str("");
    
    if (verblevel >= 3){
      outString << "Tractions (driver): " << std::endl;
      for(int i = 0;i < isize/3;i++)
        outString << tractions[3*i + 0] << " "
                  << tractions[3*i + 1] << " "
                  << tractions[3*i + 2] << std::endl;
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
    }

    transferAgent->Transfer("traction","Loads",true);

    //Get loads array from solid solver to check
    if (verblevel >= 3){
      int solidLoadStride = 0, solidLoadCap = 0;
      double  *solidLoads = NULL;
      COM_get_array((structuresInterfaceName+".Loads").c_str(),11,&solidLoads,
                     &solidLoadStride,&solidLoadCap);
      int solidLoadsize = solidLoadCap*solidLoadStride;
      outString << "Loads transfered to the solid : " << std::endl;
      for(int i = 0;i < solidLoadsize;i++){
        outString << std::setprecision(15) << "solidLoads(" << i << ") = " << solidLoads[i] << std::endl;
      StdOut(outString.str(),3,true);
      outString.clear();
      outString.str("");
      }
    }
    return(0); 
  };


  int TransferPressuresToStructures(fluidagent *fluidAgent,solidagent *solidAgent)
  { 
    std::stringstream outString;
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
      outString << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for FaceLoads with structure solver" << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
      return(1);
    }
    int solidPressuresHandle = COM_get_dataitem_handle(structuresInterfaceName+".Pressures");
    if(solidPressuresHandle < 0){
      outString << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for Pressures with structure solver" << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
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
      outString << std::setprecision(15) << "solidPressure(" << i << ") = " << solidPressures[i] << std::endl;
    }
    StdOut(outString.str(),3,true);
    outString.clear();
    outString.str("");

    //Get handles for functions needed
    std::string funcName;
    funcName = surfUtilInterfaceName + ".compute_element_normals";
    int faceNormalsHandle = COM_get_function_handle(funcName.c_str());
    if(faceNormalsHandle < 0){
      outString << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for compute_element_normals function " << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
      return(1);
    }
    funcName = simpalInterfaceName + ".mul";
    int mulHandle = COM_get_function_handle(funcName.c_str());
    if(mulHandle < 0){
      outString << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for simpal multiply function " << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
      return(1);
    }
    funcName = simpalInterfaceName + ".neg";
    int negHandle = COM_get_function_handle(funcName.c_str());
    if(negHandle < 0){
      outString << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for simpal negate function " << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
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
      outString << "Error: (TransferPressuresToStructures)" << std::endl
                << "       No handle for FaceNormals with fluids solver" << std::endl;
      StdOut(outString.str(),0,true);
      outString.clear();
      outString.str("");
      return(1);
    }
    normalize=0;
    COM_call_function(faceNormalsHandle, &fluidFaceNormalsHandle, &normalize);
    double *fluidFaceNormals = NULL;
    COM_get_array((fluidsInterfaceName+".normals").c_str(),101,&fluidFaceNormals,&stride,&cap);
    isize = stride*cap; 

    outString << "stride = " << stride << std::endl;
    std::vector<double> sums (stride, 0.0);
    outString << "Driver: fluid face normals:" << std::endl;
    for(int i=0; i < cap; i++){
      for(int j=0; j < stride; j++){
        outString << fluidFaceNormals[stride*i + j] << " ";
        sums[j] += fluidFaceNormals[stride*i + j];
      }
      outString << std::endl;
    }  
    outString << "sums: ";
    for(int i=0; i < sums.size(); i++)
      outString << sums[i] << " ";
    outString << std::endl;
    StdOut(outString.str(),3,true);
    outString.clear();
    outString.str("");

    return(0); 
  };


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
  virtual int Initialize(std::vector<std::string> &componentInterfaceNames, 
          double finalTime, double timeStep){

    FunctionEntry("Initialize"); 
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
      fluidsAgent->Initialize(fluidsInterfaceName, verblevel);

    // Initialize the structures module
    if(runMode != 1)
      structuresAgent->Initialize(structuresInterfaceName, verblevel);

    // Initialize the transfer module's common refinement
    if(runMode == 0) {
      transferAgent->Overlay(structuresInterfaceName,fluidsInterfaceName);
      //      TestTransfer();
    }
    //    exit(1);


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
      outString << "ElmerFoamDriver:Run: System timestep " << ++systemStep 
                << " @ Time = " << simulationTime << std::endl;
      StdOut(outString.str(),1,true);
      outString.clear();
      outString.str("");

      // }
          
      if(!runMode){
        // Transfer displacements @ Tn to fluids
        outString << "ElmerFoamDriver:Run: Transferring displacements from structures to fluids @ time(" 
                  << simulationTime << ")" << std::endl;
        StdOut(outString.str(),1,true);
        outString.clear();
        outString.str("");
        TransferDisplacementsToFluid(structuresAgent,fluidsAgent);
      }
      if(runMode < 2){
        fluidsAgent->InitializeTimeStep(simulationTime);
        // Step fluids to get loads @ T(n+1)
        outString << "ElmerFoamDriver:Run: Stepping fluids to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl;
        StdOut(outString.str(),1,true);
        outString.clear();
        outString.str("");
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
        outString << "ElmerFoamDriver:Run: Stepping structures to time(" 
                  << simulationTime+simulationTimeStep << ")" << std::endl; */
        outString << "ElmerFoamDriver:Run: Stepping structures to time("
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
        outString << "ElmerFoamDriver:Run: Converged at time(" 
                  << simulationTime << ")" << std::endl; 
        outString << "ElmerFoamDriver:Run: Converged at time("
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
          outString << "ElmerFoamDriver:Run: Failed to converge after "
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

#endif
