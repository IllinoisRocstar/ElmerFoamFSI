#ifndef __ELMER_AGENT_H__
#define __ELMER_AGENT_H__


#include "Orchestrator.H"

class elmeragent : public impact::orchestrator::agentbase, public SolverUtils::FEM::SolverAgent {
  
public:
  virtual int Initialize(const std::string &interfaceName, int verblevel=1){
    int structuresInitHandle     = COM_get_function_handle(std::string(interfaceName+".Initialize").c_str());
    if(structuresInitHandle < 0){
      WriteErr("Failed to get Elmer initialization handle.\n");
      return(1);
    }
    componentFunctionHandles[INIT] = structuresInitHandle;
    
    int structuresRunHandle      = COM_get_function_handle(std::string(interfaceName+".Run").c_str());
    if(structuresRunHandle < 0){
      WriteErr("Failed to get Elmer run handle.\n");
      return(1);
    }
    componentFunctionHandles[RUN] = structuresRunHandle;
    
    int structuresFinalizeHandle      = COM_get_function_handle(std::string(interfaceName+".Finalize").c_str());
    if(structuresFinalizeHandle < 0){
      WriteErr("Failed to get structures finalize handle.\n");
      return(1);
    }
    componentFunctionHandles[FINALIZE] = structuresFinalizeHandle;
    
    int runs = 0;
    int verbIn = verblevel;
    COM_call_function(componentFunctionHandles[INIT],&runs, &verbIn);
    if(!runs){
      WriteErr("Elmer failed to initialize!\n");
      return(1);
    }
    
    std::vector<int> paneIDs;
    COM_get_panes(interfaceName.c_str(),paneIDs);
    if(!paneIDs.empty())
      paneID = paneIDs[0];
    
    double *nodalCoordinates;
    int stride = 0;
    int cap = 0;
    std::string coordinateName(interfaceName+".nc");
    COM_get_array(coordinateName.c_str(),paneID,&nodalCoordinates,&stride,&cap);
    int arraySize = (stride*cap);
    std::cout << "Stride = " << stride << " Cap = " << cap << std::endl;
    nodeCoordinates.resize(arraySize);
    for(int i = 0;i < arraySize;i++)
      nodeCoordinates[i] = nodalCoordinates[i];


    std::string newDataItem(interfaceName+".coords");
    COM_new_dataitem(newDataItem.c_str(),'n',COM_DOUBLE,3,"m");
    COM_set_array(newDataItem.c_str(),paneID,&nodeCoordinates[0]);

    COM_window_init_done(interfaceName);

    if(SolverUtils::PopulateSolverAgentFromWindow(interfaceName,*this))
      return(1);

    

    WriteOut("Elmer Initialization done!\n");
    return(0);
  };
  int virtual Run(double time){
    int runs = 0;
    COM_call_function(componentFunctionHandles[RUN],&runs,&time);
    if(!runs){
      WriteErr("Elmer failed to run!\n");
      return(1);
    }
  };
  const std::vector<double> &Coordinates() const {return(nodeCoordinates);};
  int PaneID(){return(paneID);};
  int virtual Finalize(){
    int runs = 0;
    WriteOut("Calling Elmer finalize.\n");
    COM_call_function(componentFunctionHandles[FINALIZE],&runs);
    WriteOut("Elmer finalize done.\n");
    if(!runs){
      WriteErr("Elmer failed to finalize!\n");
      return(1);
    }
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
