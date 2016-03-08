///
/// @file
/// @ingroup impact_group
/// @brief Main function of the driver (in C/C++) for the OpemFoamFSIModule
/// @author Mike Campbell (mtcampbe@ilrocstar.com)
/// @author Mike Anderson (mjanderson@ilrocstar.com)
/// @date  March 1, 2015
/// 
///
#include "com.h"
#include "com_devel.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "primitive_utilities.H"
#include "SolverAgent.H"
#include "InterfaceLayer.H"
#include "OFModuleDriver.H"

COM_EXTERN_MODULE(OpenFoamFSI);

//using namespace std;

int main(int argc, char *argv[]){
  COM_init( &argc, &argv);

 
  // flags for operation modes
  // default mode of operation, runs the original OpenFOAM functionality
  bool regression = true;  
  // prescribe a solid surface displacement and run the fluid solver only
  bool prescribedDisplacement = false;  

  std::string arg;
  std::stringstream ss;

  // we read any driver specific flags passed
  // the strings are then removed from argv and argc is decremented
  // to preserve the input command line for OpenFoam
  if (argc > 1) {
    for (int i=1; i<argc; ++i) {
      ss.clear();
      ss.str("");
      ss << argv[i];
      if (ss.str() == "--driverRegression") {
        regression = true;
      } else if (ss.str() == "--driverPrescribedDisplacement") {
        regression = false;
        prescribedDisplacement = true;
      } else {
        Usage(argv[0]);
        exit(2);
      }
    }
  }

  COM_LOAD_MODULE_STATIC_DYNAMIC( OpenFoamFSI, "OFModule");

  /// Get the handle for the initialize function and call it
  int init_handle = COM_get_function_handle("OFModule.InitFoam");
  if(init_handle <= 0) { // fail
    std::cout << "OFModuleDriver:main: Could not get handle for init function." << std::endl;
    exit(-1);
  }

  // make a dummy argc/argv for OpenFOAM. No options passed from the command 
  // line will be used by the driver
  int myArgc = 1;
  char *myArgv[2];
  myArgv[0] = argv[0];
  myArgv[1] = NULL;

  COM_call_function(init_handle, &myArgc, &myArgv);

  // get information about what was registered in this window
  int numDataItems=0;
  std::string output;
  COM_get_dataitems("OFModule", &numDataItems, output);
  std::istringstream Istr(output);
  std::vector<std::string> dataItemNames;

  for (int i=0; i<numDataItems; ++i) {
    std::string name;
    Istr >> name;
    dataItemNames.push_back(name);
    std::cout << "OFModuleDriver:main: DataItem # " << i << ": " << name << std::endl;
  }

  // list of panes in this window
  int numPanes;
  int* paneList;
  COM_get_panes("OFModule", &numPanes, &paneList);
  std::cout << "OFModuleDriver:main: Number of Panes " << numPanes << std::endl;
  for (int i=0; i<numPanes; ++i) 
    std::cout << "OFModuleDriver:main: Pane ID # " << i+1 << "=" << paneList[i] << std::endl;

  // only one pane for serial runs
  int pane = paneList[0];

  double* Coord;
  COM_get_array("OFModule.nc", pane, &Coord);

  // check for expected number of nodes
  int numNodes = 0;
  COM_get_size("OFModule.nc", pane, &numNodes);

  // get connectivity tables for panes
  int numConn;
  std::string stringNames;
  COM_get_connectivities("OFModule", pane, &numConn, stringNames);
  std::istringstream ConnISS(stringNames);
  std::vector<std::string> connNames;

  for (int i=0; i<numConn; ++i) {
    std::string name;
    ConnISS >> name;
    connNames.push_back(name);
    std::cout << "OFModuleDriver:main: Connectivity Table # " << i+1 << ": " << name << std::endl;
  }

  // number of nodes per element
  char getDataItemLoc;
  COM_Type getDataItemType;
  int numElementNodes;
  std::string getDataItemUnits;
  std::string fullConnName("OFModule."+connNames[0]);
  COM_get_dataitem(fullConnName, &getDataItemLoc, &getDataItemType, 
                   &numElementNodes, &getDataItemUnits);

  std::cout << "OFModuleDriver:main: getDataItemLoc " << getDataItemLoc << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemType " << getDataItemType << std::endl;
  std::cout << "OFModuleDriver:main: numElementNodes " << numElementNodes << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemUnits " << getDataItemUnits << std::endl;

  int* Conn;
  int numElem;
  COM_get_array(fullConnName.c_str(), pane, &Conn);
  COM_get_size(fullConnName, pane, &numElem);

  // put elements into a vector so we can build the solver agent
  std::vector<unsigned int> connVector;
  for (int i=0; i<numElem; ++i) {
    for (int j=0; j<numElementNodes; ++j) {
      connVector.push_back((Conn[i*numElementNodes+j]));
    }
  }

  // get non-mesh data items
  int arrayLength;
  std::string name("OFModule.time");
  COM_get_dataitem(name, &getDataItemLoc, &getDataItemType, 
                   &arrayLength, &getDataItemUnits);
  double* time;
  COM_get_array(name.c_str(), pane, &time);

  name = "OFModule.endTime";
  COM_get_dataitem(name, &getDataItemLoc, &getDataItemType, 
                   &arrayLength, &getDataItemUnits);
  double* endTime;
  COM_get_array(name.c_str(), pane, &endTime);


  // make a solverAgent to store our data
  SolverUtils::FEM::SolverAgent myAgent;

  /// Register the Data Items

  // set up solution meta data
  myAgent.Solution().Meta().AddField("time", 's', 1, 8, "s");
  myAgent.Solution().Meta().AddField("endTime", 's', 1, 8, "s");
  //myAgent.Solution().Meta().AddField("initStatus", 's', 1, 4, "");
  //myAgent.Solution().Meta().AddField("runStatus", 's', 1, 4, "");
  //myAgent.Solution().Meta().AddField("dt", 's', 1, 8, "s");
  //myAgent.Solution().Meta().AddField("displacement", 'n', 3, 8, "m");
  //myAgent.Solution().Meta().AddField("traction", 'c', 1, 8, "N");
 
  myAgent.Mesh().nc.init(numNodes, Coord);
  // this assumes that our elements are quads...we can generalize this
  myAgent.Mesh().con.AddElements(numElem, numElementNodes, connVector);

  // get data registered on surface mesh
  name = "OFModule.pressure";
  COM_get_dataitem(name, &getDataItemLoc, &getDataItemType,
                    &arrayLength, &getDataItemUnits);

  std::cout << "OFModuleDriver:main: Pressure Get DataItem" << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemLoc: " << getDataItemLoc << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemType: " << getDataItemType << std::endl;
  std::cout << "OFModuleDriver:main: arrayLength: " << arrayLength << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemUnits: " << getDataItemUnits << std::endl;

  char myDataItemLoc;
  // translate element (e) to cell (c)
  if (getDataItemLoc == 'e' || getDataItemLoc == 'E') {
    myDataItemLoc = 'c';
  } else {
    std::cout << "OFModuleDriver:main: Unknown Data Item Location" << std::endl;
    exit(1);
  }

  int myDataItemType;
  if (getDataItemType == COM_DOUBLE) {
    myDataItemType = 8;
  } else {
    std::cout << "OFModuleDriver:main: Unknown Data Item Type" << std::endl;
    exit(1);
  }

  myAgent.Solution().Meta().AddField("surfacePressure", myDataItemLoc, arrayLength, 
                                     myDataItemType, getDataItemUnits);
  double* surfacePressure;
  COM_get_array(name.c_str(), pane, &surfacePressure);

  name = "OFModule.traction";
  COM_get_dataitem(name, &getDataItemLoc, &getDataItemType,
                    &arrayLength, &getDataItemUnits);

  std::cout << "OFModuleDriver:main: Traction Get DataItem" << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemLoc: " << getDataItemLoc << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemType: " << getDataItemType << std::endl;
  std::cout << "OFModuleDriver:main: arrayLength: " << arrayLength << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemUnits: " << getDataItemUnits << std::endl;

  // translate element (e) to cell (c)
  if (getDataItemLoc == 'e' || getDataItemLoc == 'E') {
    myDataItemLoc = 'c';
  } else {
    std::cout << "OFModuleDriver:main: Unknown Data Item Location" << std::endl;
    exit(1);
  }

  if (getDataItemType == COM_DOUBLE) {
    myDataItemType = 8;
  } else {
    std::cout << "OFModuleDriver:main: Unknown Data Item Type" << std::endl;
    exit(1);
  }

  myAgent.Solution().Meta().AddField("surfaceTraction", myDataItemLoc, arrayLength, 
                                     myDataItemType, getDataItemUnits);
  double* surfaceTraction;
  COM_get_array(name.c_str(), pane, &surfaceTraction);

  name = "OFModule.solidDisplacement";
  COM_get_dataitem(name, &getDataItemLoc, &getDataItemType,
                    &arrayLength, &getDataItemUnits);

  std::cout << "OFModuleDriver:main: Solid Displacement Get DataItem" << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemLoc: " << getDataItemLoc << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemType: " << getDataItemType << std::endl;
  std::cout << "OFModuleDriver:main: arrayLength: " << arrayLength << std::endl;
  std::cout << "OFModuleDriver:main: getDataItemUnits: " << getDataItemUnits << std::endl;

  // translate element (e) to cell (c)
  if (getDataItemLoc == 'e' || getDataItemLoc == 'E') {
    myDataItemLoc = 'c';
  } else if (getDataItemLoc == 'n' || getDataItemLoc == 'N') {
    myDataItemLoc = 'n';
  } else {
    std::cout << "OFModuleDriver:main: Unknown Data Item Location" << std::endl;
    exit(1);
  }

  if (getDataItemType == COM_DOUBLE) {
    myDataItemType = 8;
  } else {
    std::cout << "OFModuleDriver:main: Unknown Data Item Type" << std::endl;
    exit(1);
  }

  myAgent.Solution().Meta().AddField("solidDisplacement", myDataItemLoc, arrayLength, 
                                     myDataItemType, getDataItemUnits);
  double* solidDisplacement;
  COM_get_array("OFModule.solidDisplacement", pane, &solidDisplacement);

  // create buffers for the actual data
  myAgent.CreateSoln();

  unsigned int nnodes = myAgent.Mesh().nc.NNodes();
  unsigned int nelem = myAgent.Mesh().con.Nelem();

  // reset the buffers to be our own local buffers
  myAgent.Solution().SetFieldBuffer("time", time);
  myAgent.Solution().SetFieldBuffer("endTime", endTime);
  myAgent.Solution().SetFieldBuffer("surfacePressure", surfacePressure);
  myAgent.Solution().SetFieldBuffer("surfaceTraction", surfaceTraction);
  myAgent.Solution().SetFieldBuffer("solidDisplacement", solidDisplacement);

  /// Get the handle for the run function and call it
  int run_handle = COM_get_function_handle("OFModule.RunFoam");
  if(run_handle <= 0) { // fail
    std::cout << "OFModuleDriver:main: Could not get handle for run function." << std::endl;
    exit(-1);
  }

  int step_handle;
  /// Get the handle for the step function
  if (regression) {
    step_handle = COM_get_function_handle("OFModule.StepFoam");
    std::cout << "OFModuleDriver:main: StepFoam will be running shortly." << std::endl;
    if(step_handle <= 0) { // fail
      std::cout << "OFModuleDriver:main: Could not get handle for step function." << std::endl;
      exit(-1);
    }
  } else if (prescribedDisplacement) {
    step_handle = COM_get_function_handle("OFModule.StepFluid");
    std::cout << "OFModuleDriver:main: StepFluid will be running shortly." << std::endl;
    if(step_handle <= 0) { // fail
      std::cout << "OFModuleDriver:main: Could not get handle for step fluid alone function." << std::endl;
      exit(-1);
    }
  }

  /// write initial mesh data out to a VTK file
  std::ofstream Ouf;
  ss.clear();
  ss.str("");
  ss << time[0];
  std::string filename;
  filename = "fsi" + ss.str() +".vtk";
  Ouf.open(filename.c_str());
  if(!Ouf){
      std::cerr << "OFModuleDriver:main: OFModuleDriver::DumpSolution:Error: Could not open output file, "
                << filename << "." << std::endl;
      return -1;
  }
  std::cout << "OFModuleDriver:main: WriteVTKToStream time " << time[0] << std::endl;
  SolverUtils::WriteVTKToStream("OFModule", myAgent, Ouf);
  Ouf.close();

  std::cout << "OFModuleDriver:main: In Driver:" << "  time=" << time[0] << " endTime=" << endTime[0] << std::endl;
  while(time[0] <= endTime[0]) {
  //COM_call_function(run_handle);
    // step solution
    if (prescribedDisplacement) {
      double xmax = .6;
      double xmin = .26;
      double alpha = 3.141592/(2.0*(xmax-xmin));
      double beta = 2*3.141592/endTime[0];
      for(int i = 0;i < numNodes;i++){
        double xpos = Coord[i*3] - xmin;
        if(xpos > 0){
          xpos *= alpha;
          solidDisplacement[i*3] = solidDisplacement[i*3+2] = 0.0;
          solidDisplacement[i*3+1] = 0.001*std::sin(xpos)*std::sin(beta*time[0]);
        }
      }
    }
    COM_call_function(step_handle);
    /// write mesh data out to a VTK file at each timestep
    std::ofstream Ouf;
    ss.clear();
    ss.str("");
    ss << time[0];
    std::string filename;
    filename = "fsi" + ss.str() +".vtk";
    Ouf.open(filename.c_str());
    if(!Ouf){
        std::cerr << "OFModuleDriver::DumpSolution:Error: Could not open output file, "
                  << filename << "." << std::endl;
        return -1;
    }
    std::cout << "OFModuleDriver:main: WriteVTKToStream time " << time[0] << std::endl;
    SolverUtils::WriteVTKToStream("OFModule", myAgent, Ouf);
    Ouf.close();
  }

  COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSI, "OFModule");

  COM_finalize();
  return(0);
}

void Usage(char *exec) {
  std::cout << "Usage: " << std::endl;
  std::cout << exec << "<flags>" << std::endl
    << "OFModuleDriver:Usage: Where flags is either --driverRegression or --driverPrescribedDisplacement" << std::endl
    << "--driver Regresion runs a standard openFoam simulation" << std::endl
    << "--driverPrescribedMotion runs openFoam with a fluid only solver and applies" 
    << "a prescribed displacement to the solid/fluid boundary" << std::endl;
}
