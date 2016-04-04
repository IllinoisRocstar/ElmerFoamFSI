#ifndef __OPEN_FOAM_AGENT_H__
#define __OPEN_FOAM_AGENT_H__

#include "Orchestrator.H"
#include "InterfaceLayer.H"

class openfoamagent : public impact::orchestrator::agentbase, public SolverUtils::FEM::SolverAgent {
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
    
    //WK
    std::cout << "am I here?" << std::endl;
    //WK   
    WriteOut("Calling OpenFOAM Initialization.\n");

    //WK
    std::cout << "am I here? 2" << std::endl;
    //WK   
    int argc_dupe = 1;
    char *argv[2];
    argv[0] = const_cast<char *>("test");
    argv[1] = NULL;
    int verbIn = verblevel;
    COM_call_function(componentFunctionHandles[INIT],&argc_dupe,argv,&verbIn);

    //WK
    std::cout << "am I here? 3" << std::endl;
    //WK   
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
    nodeCoordinates.resize(arraySize);
    for(int i = 0;i < arraySize;i++)
      nodeCoordinates[i] = nodalCoordinates[i];

    std::string newDataItem(interfaceName+".coords");
    COM_new_dataitem(newDataItem.c_str(),'n',COM_DOUBLE,3,"m");
    COM_set_array(newDataItem.c_str(),paneID,&nodeCoordinates[0]);

    COM_window_init_done(interfaceName);

    if(SolverUtils::PopulateSolverAgentFromWindow(interfaceName,*this))
      return(1);

    WriteOut("OpenFOAM Initialization done!\n");
    return(0);
  };
  const std::vector<double> &Coordinates() const {return(nodeCoordinates);};
  int PaneID(){return(paneID);};
  virtual int Run(double endTime){
    COM_call_function(componentFunctionHandles[RUN]);
    return(0);
  };
private:
  int paneID;
  std::vector<double> nodeCoordinates;
  std::vector<double> faceCoordinates;
};

#endif
