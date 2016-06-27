/// 
/// @file
/// @ingroup elmerfoamfsi_group
/// @brief Main for example parallel program.
/// @author Masoud Safdari (msafdari@illinois.edu)
/// @date 
/// 
#include "Driver.H"

typedef ElmerFoamFSI::PEProgramType ParProgramType;

int main(int argc,char *argv[])
{
  return(ElmerFoamFSI::Driver<ParProgramType>(argc,argv));
}
