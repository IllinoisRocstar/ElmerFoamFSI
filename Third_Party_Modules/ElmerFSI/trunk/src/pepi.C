/// 
/// @file
/// @ingroup elmermoduledriver_group
/// @brief Main for example parallel program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 
#include "ExampleProgram.H"

typedef ElmerModuleDriver::ExampleProgram::PEProgramType ProgramType;

int main(int argc,char *argv[])
{
  return(ElmerModuleDriver::ExampleProgram::Driver<ProgramType>(argc,argv));
}
