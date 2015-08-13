/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Main for example serial program.
/// @author Mike Campbell (mtcampbe@illinois.edu)
/// @date 
/// 
#include "Driver.H"

typedef ElmerFoamFSI::DriverProgramType ProgramType;

int main(int argc,char *argv[])
{
  return(ElmerFoamFSI::Driver<ProgramType>(argc,argv));
}
