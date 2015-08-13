/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

Application
    icoFoam

Description
    Transient solver for incompressible, laminar flow of Newtonian fluids.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "com.h"
#include "com_devel.hpp"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
///
/// @file
/// @ingroup icoFoamModule_group
/// @brief C++ Module
/// @author JEK
/// @date August 25, 2014
/// 
///
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>


COM_EXTERN_MODULE( IcoFoamModule);


namespace IcoFoamMod {
  
  class IcoFoamModule : public COM_Object {
  public:
    /// Default constructor.
    IcoFoamModule(){};
    
    ///
    /// @brief Destructor
    ///
    /// The destructor will destroy the externally loaded modules (if they exists)
    ///
    virtual ~IcoFoamModule()
    {
      if(!other_window_name.empty()){
        int other_window_handle = COM_get_window_handle(other_window_name.c_str());
        if(other_window_handle > 0)
          COM_UNLOAD_MODULE_STATIC_DYNAMIC(COMTESTMOD,other_window_name.c_str());
      }
      if(!fortran_window_name.empty()){
        int fortran_window_handle = COM_get_window_handle(fortran_window_name.c_str());
        if(fortran_window_handle > 0)
          COM_UNLOAD_MODULE_STATIC_DYNAMIC(COMFTESTMOD,fortran_window_name.c_str());
      }
    };  

#include "icoFoamHeader.H"

    int IcoFoamModuleFunction(int *pargc, void **pargv)
    {
      int argc = *pargc;
      char** argv = (char**)(pargv);

      icoFoamMain(argc, argv);
  
  
        return 0;
    }

    ///
    /// @brief "Loads" IcoFoamModule
    ///
    ///
    static void Load(const std::string &name){
      std::cout << "Loading IcoFoamModule with name " << name 
                << "." << std::endl;

      IcoFoamModule *module_pointer = new IcoFoamModule();
      COM_new_window(name, MPI_COMM_NULL);
      module_pointer->my_window_name = name;
      std::string global_name(name+".global");
      COM_new_dataitem(global_name.c_str(),'w',COM_VOID,1,"");
      COM_set_object(global_name.c_str(),0,module_pointer);


      std::vector<COM_Type> types(13,COM_INT);
    
      types[0] = COM_RAWDATA;
      types[2] = COM_VOID;
      COM_set_member_function( (name+".IcoFoamModuleFunction").c_str(),
                               (Member_func_ptr)(&IcoFoamModule::IcoFoamModuleFunction),
                               global_name.c_str(), "bii", &types[0]);



      COM_window_init_done(name);  
    }


    ///
    /// @brief Unloads the IcoFoamModule
    ///
    ///
    static void Unload(const std::string &name){
      std::cout << "Unloading IcoFoamModule with name " << name 
                << "." << std::endl;
      IcoFoamModule *module_pointer = NULL;
      std::string global_name(name+".global");
      COM_get_object(global_name.c_str(),0,&module_pointer);
      COM_assertion_msg( module_pointer->validate_object()==0, "Invalid object");
      delete module_pointer;
      COM_delete_window(std::string(name));
    }

  private:
    std::vector<int> dataArray;
    std::string my_window_name; /// Tracks *this* window name.
    std::string other_window_name; /// Tracks *this* external window's name
    std::string fortran_window_name; /// Tracks the fortran external window's name
  };
}
  
/// 
/// @brief C/C++ bindings to load IcoFoamModule
///
extern "C" void IcoFoamModule_load_module( const char *name) {
  IcoFoamMod::IcoFoamModule::Load(name);
}
///
/// @brief C/C++ bindings to unload IcoFoamModule
///
extern "C" void IcoFoamModule_unload_module( const char *name){
  IcoFoamMod::IcoFoamModule::Unload(name);
}





// ************************************************************************* //
