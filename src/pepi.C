/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Main for example parallel program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 
#include "ExampleProgram.H"

typedef ElmerFoamFSI::ExampleProgram::PEProgramType ProgramType;

int main(int argc,char *argv[])
{
  return(ElmerFoamFSI::ExampleProgram::Driver<ProgramType>(argc,argv));
}
