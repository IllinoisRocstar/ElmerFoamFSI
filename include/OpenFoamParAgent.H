#ifndef __OPEN_FOAM_PAR_AGENT_H__
#define __OPEN_FOAM_PAR_AGENT_H__

#include "Orchestrator.H"
#include "InterfaceLayer.H"

class openfoamagentpar : public impact::orchestrator::agentbase, public SolverUtils::FEM::SolverAgent {
public:
  virtual int Initialize(const std::string interfaceName, int verblevel = 1){
    componentInterfaceNames.push_back(interfaceName);
    componentFunctionHandles[INIT] = COM_get_function_handle(std::string(interfaceName+".InitFoam").c_str());
    int fluidsInitHandle = componentFunctionHandles[INIT];
    if(fluidsInitHandle < 0){
      WriteErr("Failed to get fluids initialization handle.\n");
      return(1);
    }
    
    int fluidsStepHandle      = COM_get_function_handle(std::string(interfaceName+".StepFluid").c_str());
    if(fluidsStepHandle < 0){
      WriteErr("Failed to get fluids step handle.\n");
      return(1);
    }
    componentFunctionHandles[RUN] = fluidsStepHandle;
   
    
    WriteOut("Calling OpenFOAM Initialization.\n");

    int myArgc = 2;
    char *myArgv[2];
    myArgv[0] = const_cast<char *>("dummy");
    myArgv[1] = const_cast<char *>("-parallel");
    int verbIn = verblevel;
    COM_call_function(componentFunctionHandles[INIT], &myArgc, &myArgv, &verbIn); 
    
     
    std::vector<int> paneIDs;
    COM_get_panes(interfaceName.c_str(),paneIDs);
    if(!paneIDs.empty())
      paneID = paneIDs[0];
    
    double *nodalCoordinates;
    int stride = 0;
    int cap = 0;
    std::string coordinateName(interfaceName+".nc");
    std::string newDataItemName;
    if (!paneIDs.empty()) {
       COM_get_array(coordinateName.c_str(),paneID,&nodalCoordinates,&stride,&cap);
       int arraySize = (stride*cap);
       nodeCoordinates.resize(arraySize);
       for(int i = 0;i < arraySize;i++)
	 nodeCoordinates[i] = nodalCoordinates[i];
       newDataItemName = interfaceName+".coords";
       COM_new_dataitem(newDataItemName.c_str(),'n',COM_DOUBLE,3,"m");
       COM_set_array(newDataItemName.c_str(),paneID,&nodeCoordinates[0]);
    }

    // setting up probe
    newDataItemName = interfaceName+".setsProb";
    COM_new_dataitem(newDataItemName.c_str(),'w',COM_INT,1,"");
    newDataItemName = interfaceName+".probProcId";
    COM_new_dataitem(newDataItemName.c_str(),'w',COM_INT,1,"");
    newDataItemName = interfaceName+".probNdeId";
    COM_new_dataitem(newDataItemName.c_str(),'w',COM_INT,1,"");

    COM_window_init_done(interfaceName);
 
    if (!paneIDs.empty()) {
       if(SolverUtils::PopulateSolverAgentFromWindow(interfaceName,*this))
	 return(1);
    }
    WriteOut("OpenFOAM Initialization done!\n");
    return(0);
  };

  const std::vector<double> &Coordinates() const {return(nodeCoordinates);};
  int PaneID(){return(paneID);};
  virtual int Run(double endTime){
    COM_call_function(componentFunctionHandles[RUN]);
    return(0);
  };
  
  std::vector<double> virtual returnNodeCoords(){
    return nodeCoordinates;
  };	
  
  std::vector<double> virtual returnFaceCoords(){
		return faceCoordinates;
  };
  
private:
  int paneID;
  std::vector<double> nodeCoordinates;
  std::vector<double> faceCoordinates;
};

#endif
