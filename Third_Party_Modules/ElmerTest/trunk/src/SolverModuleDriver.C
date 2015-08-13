///
/// @file
/// @ingroup solverModule_group
/// @brief Main function to call the Elmer SolverModule
/// @author Jessica Kress (jkress@ilrocstar.com)
/// @date January 12, 2015
/// 
///
#include "com.h"
#include "com_devel.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <sstream>

COM_EXTERN_MODULE( SolverModule);

using namespace std;

void usage(char *exec){
  std::cout << "Usage: " << std::endl
            << exec << " timeNext timeNext ... timeFinal" << std::endl
            << "where at least timeFinal is required." << std::endl
            << "NOTES: *steady state problems will always use a timestep" << std::endl
            << "of 1.0, and should only have a timeFinal" << std::endl
            <<        "*each time must be greater than the previous time" << std::endl
            << "because the simulation begins at the previous time and runs" << std::endl
            << "to the current time" << std::endl;
   std::exit(1);
}       

int main(int argc, char *argv[]){
  COM_init( &argc, &argv);

  std::cout << "After COM_init" << std::endl;

  std::vector<double> tNext;
  stringstream ss;
  double var;
  bool isNum = false;
  std::string arg;

  std::cout << "argc = " << argc << std::endl;

  if(argc > 1){
    for(int i=1; i < argc; i++){
        ss.clear();
        ss.str("");
        ss << argv[i];
        for(int j=0; j < ss.str().size(); j++){
          if(!isdigit(ss.str()[j]) && ss.str()[j] != 'e' 
             && ss.str()[j] != 'E' && ss.str()[j] != '-'
             && ss.str()[j] != '.')
            usage(argv[0]);
        }
        ss >> var;
        tNext.push_back(var);
    }
  }      
  else
    usage(argv[0]);

  std::cout << "line " << __LINE__ << std::endl;

  for(int i=0; i < tNext.size(); i++)
    std::cout << tNext[i] << std::endl;

  for(int i=1; i < tNext.size(); i++){
    std::cout << tNext[i] << " " << tNext[i-1] << std::endl;
    if(tNext[i] <= tNext[i-1])
      usage(argv[0]);
  }

  COM_LOAD_MODULE_STATIC_DYNAMIC( SolverModule, "Window1");

  /// Get the handle for the initialize function and call it
  int init_handle = COM_get_function_handle("Window1.Initialize");
  bool init_func = (init_handle > 0);
  std::cout << "init = " << init_handle << std::endl;
  if(init_handle){
    COM_call_function(init_handle);
  }

  /// Get the handle for the run function and call it
  int run_handle = COM_get_function_handle("Window1.Run");
  bool run_func = (run_handle > 0);
  std::cout << "run = " << run_handle << std::endl;
  if(run_handle){
    for(int i=0; i < tNext.size(); i++){
      std::cout << "Calling run function from driver" << std::endl;
      COM_call_function(run_handle,&tNext[i]);
    }
  }

  /// Get the handle for the run function and call it
  int final_handle = COM_get_function_handle("Window1.Finalize");
  bool final_func = (final_handle > 0);
  std::cout << "final = " << final_handle << std::endl;
  if(final_handle){
    COM_call_function(final_handle);
  }

  COM_UNLOAD_MODULE_STATIC_DYNAMIC( SolverModule, "Window1");

  COM_finalize();
  std::cout << "After COM_finalize" << std::endl;

}
