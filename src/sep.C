/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Main for example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 
#include "ExampleProgram.H"

typedef ElmerFoamFSI::ExampleProgram::SEProgramType ProgramType;

int main(int argc,char *argv[])
{
  return(ElmerFoamFSI::ExampleProgram::Driver<ProgramType>(argc,argv));
}
