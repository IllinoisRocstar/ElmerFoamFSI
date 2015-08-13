///
/// @file
/// @ingroup impact_group
/// @brief Main function of the driver (in C/C++) for the IcoFoamModule
/// @author Jessica Kress (jkress@ilrocstar.com)
/// @date May 1, 2014
/// 
///
#include "com.h"
#include "com_devel.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>

COM_EXTERN_MODULE( IcoFoamModule);

using namespace std;

int main(int argc, char *argv[]){
  COM_init( &argc, &argv);

  std::cout << "After COM_init" << std::endl;

  COM_LOAD_MODULE_STATIC_DYNAMIC( IcoFoamModule, "Window1");

  /// Get the handle for the module function and call it
  int icoFoamModuleFunction_handle = COM_get_function_handle("Window1.IcoFoamModuleFunction");
  bool func = (icoFoamModuleFunction_handle > 0);
  std::cout << "func = " << func << std::endl;
  if(func)
    COM_call_function(icoFoamModuleFunction_handle,&argc,argv);

  COM_UNLOAD_MODULE_STATIC_DYNAMIC( IcoFoamModule, "Window1");

  COM_finalize();
  std::cout << "After COM_finalize" << std::endl;

}
