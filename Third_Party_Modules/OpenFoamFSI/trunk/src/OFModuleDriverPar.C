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
#include "COMM.H" // contains IRAD utilities for MPI
#include "Pane_communicator.h" // IMPACT's inter-process tools
 
COM_EXTERN_MODULE(OpenFoamFSIPar);
COM_EXTERN_MODULE(SimOut);

namespace COM {
   void Usage(char *exec) {
      std::cout << std::endl << std::endl << "Usage: " << std::endl;
      std::cout << exec << " <flags>" << std::endl
      << "Where flags are :" << std::endl
      << "--driver" << std::endl
      << " Regresion runs a standard openFoam simulation" << std::endl
      << "--driverPrescribedMotion" << std::endl
      << "  runs openFoam with a fluid only solver and applies a prescribed"
      << "  displacement to the solid/fluid boundary" << std::endl
      << "--parallel" << std::endl
      << "  runs the openFoam in the parallel mode" << std::endl
      << "--dryrun" << std::endl
      << "  runs module from begining to end and performs only one step"
      << "  of openFoam. Use just for testing purposes." << std::endl << std::endl;
   }

  // typedef for CommunicatorObject
  typedef IRAD::Comm::CommunicatorObject CommType;

  // parallel driver for OpenFoam
  int parallelProgram(int argc, char **argv) {

     // instantiating IRAD's communicatorObject
     // this constructor calls MPI_init internally and
     // sets up everthing with MPI
     std::cout <<"OFModuleDriver:Main: Setting up parallel communicator..." << std::endl;
     COM::CommType communicator(&argc,&argv);
     MPI_Comm masterComm = communicator.GetCommunicator();
     int rank = communicator.Rank();
     int nproc = communicator.Size();
     std::cout << "OFModuleDriver:Main:Rank #"
               << rank
               << ", I see "
               << nproc
               << " proccesses."
               << std::endl;

     //int main(int argc, char *argv[])
     COM_init( &argc, &argv);
     
     // split the communicator to a single process and the rest
     // a single process for driver jobs and the rest for 
     // computations
     COM::CommType newCommunicator;
     int color;
     if(rank == -1) color=0;
     else color=1;
     /* Not splitting for now
     communicator.Split(color, rank, newCommunicator);
     MPI_Comm newComm = newCommunicator.GetCommunicator();
     std::cout << "OFModuleDriver:Main:Rank #" << rank
               << ", Default communicator is split to -> "
               << newComm
               << std::endl;
     */
     MPI_Comm newComm = communicator.GetCommunicator();
     
     // flags for operation modes
     // default mode of operation, runs the original OpenFOAM functionality
     bool regression = true;  
     // prescribe a solid surface displacement and run the fluid solver only
     bool prescribedDisplacement = false;  
     // run in parallel mode
     bool runParallel = false;
     // run test cycle
     bool dryRun = false;
   
   
     // we read any driver specific flags passed
     // the strings are then removed from argv and argc is decremented
     // to preserve the input command line for OpenFoam
     std::string arg;
     std::stringstream ss;
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
	 } else if (ss.str() == "--parallel") {
	   runParallel = true;
	 } else if (ss.str() == "--dryrun") {
	   dryRun = true;
	 } else {
	   Usage(argv[0]);
	   exit(2);
	 }
       }
     } else {
        Usage(argv[0]);
        exit(2);
     }

     // changing default communicator and testing it
     COM_set_default_communicator(newComm);
     std::cout << "OFModuleDriver:Main:Rank #" << rank
	       << ", Default communicator for COM is now "
	       << newComm
	       << std::endl;
     MPI_Comm test_comm;
     test_comm = COM_get_default_communicator();
     if (test_comm != newComm)
	std::cout << "OFModuleDriver:Main:Rank #"
		  << rank
		  << ", default communicator is not set to " 
		  << newComm
		  << " properly."
		  << std::endl;
     
     // loading parallel openfoam module 
     // all processes load fluid module
     COM_LOAD_MODULE_STATIC_DYNAMIC( OpenFoamFSIPar, "OFModule");
     
     // loading SimIn on all processes
     COM_LOAD_MODULE_STATIC_DYNAMIC( SimOUT, "OUT");

     // getting number of processes  
     if (rank==0) {
       int* nProcReg;
       COM_get_array("OFModule.nproc", 0, &nProcReg);
       std::cout << "The communicator registered in OFModule uses "
               << *nProcReg << " prcesses" << std::endl;
     }

     // initializing the openfoam using fluid processes
     //if (color == 1) {
        // getting the rank
        //int fluidProcessRank = newCommunicator.Rank();
        
	// get the handle for the initialize function and call it
	int init_handle = COM_get_function_handle("OFModule.InitFoam");
	if(init_handle <= 0) { // fail
	  std::cout << "OFModuleDriver:Main:Rank #" << rank
		    << ", could not get handle for init function." << std::endl;
	  exit(-1);
	}
	 
	// making a dummy argc/argv for OpenFOAM. 
        // For now, no options passed from the command 
	// line will be used by the driver
	int myArgc = 1;
	char *myArgv[2];
        // passing parallel triggers openFoam to run in parallel mode
        myArgv[0] = argv[0];
        if (runParallel) {
          myArgc = 2;
          myArgv[1] = const_cast<char *>("-parallel");
        } else {
          myArgv[1] = NULL;
        }
        // setting verbose level
	int verb=3;
        // call the funciton
	COM_call_function(init_handle, &myArgc, &myArgv, &verb);	 
        // make a barrier                
        //newCommunicator.Barrier();
        
	// getting information about what was registered in this window by another process
        if (rank==0){
	   int numDataItems=0;
	   std::string output;
	   COM_get_dataitems("OFModule", &numDataItems, output);
	   std::istringstream Istr(output);
	   std::vector<std::string> dataItemNames;
	 
	   for (int i=0; i<numDataItems; ++i) {
	     std::string name;
	     Istr >> name;
	     dataItemNames.push_back(name);
	     std::cout << "Rank #" << rank
		       << ", OFModuleDriver:main: DataItem # " << i << ": " << name << std::endl;
	   }
        }

	//  how to a pane in this window registered by another proccess
        if (rank==0){
           int nPane;
           int* paneList;
           int paneRank = 1;
           COM_get_panes("OFModule", &nPane, &paneList, paneRank);
           std::cout << "Rank #" << rank << ", I see "<<nPane<<" pane in the process rank #"
                     << paneRank << std::endl;
           std::cout << "Here is the list:" << std::endl;
	   for (int i=0; i<nPane; ++i) 
	       std::cout <<"Pane ID # " << i+1 << "=" << paneList[i] << std::endl;
        }

        // List of panes for this process: each process creates a pane starting from 100
	int numPanes;
	int* paneList;
	COM_get_panes("OFModule", &numPanes, &paneList);
	std::cout << "Rank #" << rank
		  << ", OFModuleDriver:main: Number of Panes " << numPanes << std::endl;
	for (int i=0; i<numPanes; ++i) 
	  std::cout << "Rank #" << rank
		    << ", OFModuleDriver:main: Pane ID # " << i+1 << "=" << paneList[i] << std::endl;
	// only one pane per process currently
	int pane = paneList[0];

        // getting node coordinates  
	double* Coord;
	COM_get_array("OFModule.nc", pane, &Coord);
	// check for expected number of nodes
	int numNodes = 0;
	COM_get_size("OFModule.nc", pane, &numNodes);
	// get connectivity tables for panes
        // :q4: is quad element
	int numConn;
	std::string stringNames;
	COM_get_connectivities("OFModule", pane, &numConn, stringNames);
	std::istringstream ConnISS(stringNames);
	std::vector<std::string> connNames;
	for (int i=0; i<numConn; ++i) {
	  std::string name;
	  ConnISS >> name;
	  connNames.push_back(name);
	  std::cout << "Rank #" << rank
                    << ", OFModuleDriver:main: Connectivity Table # " << i+1 << ": " << name << std::endl;
	}
	// number of nodes per element
	char getDataItemLoc;
	COM_Type getDataItemType;
	int numElementNodes;
	std::string getDataItemUnits;
	std::string fullConnName("OFModule."+connNames[0]);
	COM_get_dataitem(fullConnName, &getDataItemLoc, &getDataItemType, 
			 &numElementNodes, &getDataItemUnits);
	std::cout << "Rank #" << rank
                  << ", OFModuleDriver:main: getDataItemLoc " << getDataItemLoc << std::endl;
	std::cout << "Rank #" << rank
                  << ", OFModuleDriver:main: getDataItemType " << getDataItemType << std::endl;
	std::cout << "Rank #" << rank
                  << ", OFModuleDriver:main: numElementNodes " << numElementNodes << std::endl;
	std::cout << "Rank #" << rank
                  << ", OFModuleDriver:main: getDataItemUnits " << getDataItemUnits << std::endl;

        // get connectivities        
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

	// get pressures
	name = "OFModule.pressure";
	COM_get_dataitem(name, &getDataItemLoc, &getDataItemType,
			  &arrayLength, &getDataItemUnits);
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: Pressure Get DataItem" << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemLoc: " << getDataItemLoc << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemType: " << getDataItemType << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: arrayLength: " << arrayLength << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemUnits: " << getDataItemUnits << std::endl;
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
	
        // get tractions
	name = "OFModule.traction";
	COM_get_dataitem(name, &getDataItemLoc, &getDataItemType,
			  &arrayLength, &getDataItemUnits);
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: Traction Get DataItem" << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemLoc: " << getDataItemLoc << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemType: " << getDataItemType << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: arrayLength: " << arrayLength << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemUnits: " << getDataItemUnits << std::endl;
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
        
        // get solid displacements 
	name = "OFModule.solidDisplacement";
	COM_get_dataitem(name, &getDataItemLoc, &getDataItemType,
			  &arrayLength, &getDataItemUnits);
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: Solid Displacement Get DataItem" << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemLoc: " << getDataItemLoc << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemType: " << getDataItemType << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: arrayLength: " << arrayLength << std::endl;
	std::cout << "Rank #" << rank << " ,OFModuleDriver:main: getDataItemUnits: " << getDataItemUnits << std::endl;
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
        
	/// write initial mesh data out to a VTK file
	std::ofstream Ouf;
	ss.clear();
	ss.str("");
	ss << time[0]
           << "_proc_"
           << rank;
	std::string filename;
	filename = "fsi_" + ss.str() +".vtk";
	Ouf.open(filename.c_str());
	if(!Ouf){
	    std::cerr << "OFModuleDriver:main: OFModuleDriver::DumpSolution:Error: Could not open output file, "
		      << filename << "." << std::endl;
	    return -1;
	}
	std::cout << "Rank #" << rank << ", OFModuleDriver:main: WriteVTKToStream time " << time[0] << std::endl;
	SolverUtils::WriteVTKToStream("OFModule", myAgent, Ouf);
	Ouf.close();
         
        /*
        // gathering total number of elements in root
        int numElemTot = 0;
        communicator.Reduce(numElem, numElemTot, IRAD::Comm::DTINT, IRAD::Comm::SUMOP, 0);
        std::cout << "Rank #"<<rank<<", number of elements = "<<numElem<<std::endl;
        std::cout << "Rank #"<<rank<<", total number of elements = "<<numElemTot<<std::endl;
        */ 
 
        // setting a barrier here
        std::cout << "Rank #"<<rank<<"...Reaching to barrier"<<std::endl;
        communicator.Barrier();

        // gathering all pressures in root
        std::vector<double> surfPresVec;
        std::vector<double> surfPresTotVec;
        std::vector<int> nSendAllVec;
        int gatherErr;
        for (int iElem=0; iElem<numElem; ++iElem)
            surfPresVec.push_back(surfacePressure[iElem]);
        std::cout << "Rank #"<<rank<<", surfPresVec size = "<<surfPresVec.size()<<std::endl;
        gatherErr =  communicator.Gatherv(surfPresVec, surfPresTotVec, nSendAllVec);
        if (gatherErr)
           std::cout << "Error in gathering information from processes." << std::endl;
	std::cout << "Rank #" << rank
	          << ", size of surfPresTotVec = " << surfPresTotVec.size()
	          << std::endl;
       
        // setting a barrier here
        std::cout << "Rank #"<<rank<<"...Reaching to barrier"<<std::endl;
        communicator.Barrier();

        // scattering surface displacements
        // first gathering number of nodes for each process
        std::vector<int> numNodesVec;
        std::vector<int> numNodesTotVec;
        std::vector<int> nNodesAllVec;
        numNodesVec.push_back(numNodes);
        //std::cout << "Rank #"<<rank<<", numNodesVec[rank] = "<<numNodesVec[0]<<std::endl;
        gatherErr =  communicator.Gatherv(numNodesVec, numNodesTotVec, nNodesAllVec);
        if (gatherErr)
           std::cout << "Error in gathering information from processes." << std::endl;
        if (rank == 0){
           for (int iProc=0; iProc<numNodesTotVec.size(); iProc++)
              std::cout << "Rank #"<<rank
                        <<", numNodesTotVec["<<iProc<<"] = "<<numNodesTotVec[iProc]<<std::endl;
        }
        std::vector<double> surfDispVec;
        std::vector<double> surfDispVecTot;
        std::vector<int> nRecvAllVec;
        int scatterErr;
        int numNodesTot = 0;
        if (rank == 0){
           for (int iProc=0; iProc<numNodesTotVec.size(); iProc++){
              numNodesTot+= numNodesTotVec[iProc];
              nRecvAllVec.push_back(3*numNodesTotVec[iProc]);
           }
           surfDispVecTot.resize(3*numNodesTot, -0.1);
           std::cout<<"Rank #"<<rank<<", numNodesTot = "<<numNodesTot<<std::endl;
           std::cout<<"Rank #"<<rank<<", surfDispVecTot = "<<surfDispVecTot.size()<<std::endl;
       }
       scatterErr =  communicator.Scatterv(surfDispVecTot, nRecvAllVec, surfDispVec);
       if (scatterErr)
          std::cout << "Error in scattering information to processes." << std::endl;
       std::cout << "Rank #" << rank
                 << ", size surfDispVec = " << surfDispVec.size() 
                 << ", first element of which = " << surfDispVec[0]
                 << std::endl;
               
              
        if (!dryRun) {
	   /// Get the handle for the run function
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
	    
		
	   std::cout << "Rank #" << rank 
		     << ", OFModuleDriver:main: In Driver:" << "  time=" << time[0] 
		     << " endTime=" << endTime[0] << std::endl;
	   
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
	     ss << time[0]
		<< "_proc_"
		<< rank;
	     std::string filename;
	     filename = "fsi_" + ss.str() +".vtk";
	     Ouf.open(filename.c_str());
	     if(!Ouf){
		 std::cerr << "OFModuleDriver::DumpSolution:Error: Could not open output file, "
			   << filename << "." << std::endl;
		 return -1;
	     }
	     std::cout << "Rank #" << rank 
		       << ", OFModuleDriver:main: WriteVTKToStream time " 
		       << time[0] << std::endl;
	     SolverUtils::WriteVTKToStream("OFModule", myAgent, Ouf);
	     Ouf.close();
	   }
        }
     //}
     // Setting COM to default COMM
     //COM_set_default_communicator(masterComm);

     // set a barrier
     communicator.Barrier();

     /*
     // Creating a Pane_communicator object
     COM::COM_base *rbase = COM_get_com();
     COM::Window* winObj = NULL;
     int winHndl = COM_get_window_handle("OFModule");
     winObj = rbase->get_window_object(winHndl);
     MAP::Pane_communicator *pComm = new MAP::Pane_communicator(winObj, newComm);
     std::cout << "Total number of panes = " << pComm->total_npanes() << std::endl;
     */

      
     // writing fluid window to hdf
     int OUT_set = COM_get_function_handle( "OUT.set_option");
     int OUT_write = COM_get_function_handle( "OUT.write_dataitem");
     int OUT_write_ctrl = COM_get_function_handle( "OUT.write_rocin_control_file");
     const char *win_out= "OFModule";
     std::string win_out_pre( win_out); 
     win_out_pre.append(".");
     int OUT_all = COM_get_dataitem_handle((win_out_pre+"all").c_str());
     ss.clear();
     ss.str("");
     ss << "_proc_"
        << rank;
     std::string fileOut;
     std::string fname, ctrl_fname;
     fileOut = "OFModule_window" + ss.str();
     fname = (std::string)win_out + "_0_";
     ctrl_fname = (std::string)win_out + "_in_0.txt";
     COM_call_function( OUT_set, "format", "HDF4");
     COM_call_function( OUT_write, (fileOut + ".hdf").c_str(), &OUT_all, win_out,
                     "0000");
     COM_call_function( OUT_write_ctrl, win_out, fileOut.c_str(), ctrl_fname.c_str());
     
     

     // Finalizing
     // unloading SimIn on all processes
     COM_UNLOAD_MODULE_STATIC_DYNAMIC( SimOUT, "OUT");
     // unloading openFoam module
     COM_UNLOAD_MODULE_STATIC_DYNAMIC(OpenFoamFSIPar, "OFModule");
     // finalize comm
     COM_finalize();
     // finalizing MPI
     communicator.Finalize();
     return(0);
   }
   
}

int main(int argc, char** argv) {
  //std::cout << "Calling parallelProgram ... " << std::endl;
  return(COM::parallelProgram(argc, argv));
}

