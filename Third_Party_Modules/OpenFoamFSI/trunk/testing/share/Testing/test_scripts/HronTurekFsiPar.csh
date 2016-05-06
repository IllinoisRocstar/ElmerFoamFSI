#!/bin/tcsh
#
#  Test script for OpenFoamFsiPar. This regression test compares
#  the results of an OpenFoam parallel run with the results from the
#  elmerfoamFSIPar Driver run.
#
#  The user must have the openFoam enviornment correctly loaded for this
#  to work.
#
#  Usage:
#  HronTurekFsiPar.csh <output_file> <test_root> <build_binary>
#
#  <output_file>:  the name of a file that contains the output of the script, 
#  this is used by CTEST the populate the test results
#  <test_root>:  the root of our testing file structure
#  <build_binary>:  location of the IMPACT bin directory
#
echo "***************************************"
echo "Running HronTurekFsiPar regression test"
echo "***************************************"

set OutFile=$1
set TmpOut=${OutFile}_tmp.txt 
set iradBinPath=$4

# Regression test for OpenFoamFSI test case

if( -d HronTurekFsiPar ) then
  echo "removing HronTurekFsiPar directory"
  rm -r HronTurekFsiPar
endif

mkdir HronTurekFsiPar
cd HronTurekFsiPar

cp -r $2/share/Testing/test_data/HronTurekFsiPar/* .

# first run the open foam version
# check that the openfoam environment by looking for the executable
set FoamApp=`which icoFsiElasticNonLinULSolidFoam`

if( $? != 0 ) then
  echo "I can't find the icoFsiElasticNonLinULSolidFoam executable."
  echo "Check your environment"
  exit 1
endif

# run OpenFoam Version
echo "Running OpenFoam"
./AllrunPar
echo "Finished running parallel OpenFoam"

# check that everything finished 
grep "End" fluid/log.icoFsiElasticNonLinULSolidFoam > /dev/null

if( $? != 0 ) then
  echo "Looks like OpenFoam failed to finish."
  echo "See the test results in HronTurekFsi for more details."
  exit 1
endif

# now compare the results to archived OpenFOAM results, for now just compare the
# accumulatedFulidInterfaceDisplacement, if that is the same, the
# rest is likely the same
#
# We probably don't really care if the answers are different, as we haven't
# done any actual validation of the saved data set, but it is still nice
# to know that the code based changed under us.

# remove OpenFoam's header from the file
(tail -n +19 fluid/processor0/0.005/accumulatedFluidInterfaceDisplacement) > fluid/processor0/0.005/accumulatedFluidInterfaceDisplacement_resultOnly
set resultFile = "processor0/0.005/accumulatedFluidInterfaceDisplacement_resultOnly" 
# cleaning the files
sed 's/[)(]//g' archiveData/$resultFile > archiveData/$resultFile
sed 's/[)(]//g' fluid/$resultFile > fluid/$resultFile
$iradBinPath/diffdatafiles -t 1e-8 fluid/$resultFile archiveData/$resultFile
set STEST = $?

# move back to top level
cd ..
printf "ParOpenFoamFsiRegressionTest:Works=" >> ${TmpOut}
if( $STEST != 0 ) then
  echo "Results differ between OpenFoam and archived OpenFoamFsi Results"
  echo "See the test results in HronTurekFsiPar for more details."
  # communicate  test results back to the testing framework, 
  # this is not a fatal error
  printf "0\n" >> ${TmpOut}
else
  printf "1\n" >> ${TmpOut}
endif
 
# now make a directory for the Parallel OpenFoamModule version 

if( -d HronTurekFsiParIMPACT ) then
  echo "removing HronTurekFsiIMPACT directory"
  rm -r HronTurekFsiParIMPACT
endif

mkdir HronTurekFsiParIMPACT
cd HronTurekFsiParIMPACT

cp -r $2/share/Testing/test_data/HronTurekFsiPar/* .

# setup IMPACT Version
echo "Running IMPACT Parallel OpenFoam Driver"
./AllrunParDrvSetup
cd fluid
#$3/OFModuleDriverPar --driverRegression >& log.OFModuleDriver 
mpirun -np 2 $3/OFModuleDriverPar --parallel > & log.OFModuleDrvPar
echo "Done running IMPACT OpenFoam Driver"

# check that everything finished 
grep "FsiFoam:Unload" log.OFModuleDrvPar > /dev/null

if( $? != 0 ) then
  echo "Looks like OFModuleDrvPar failed to finish."
  echo "See the test results in HronTurekFsiParIMPACT for more details."
  exit 1
endif
cd ..

# now compare the results of the two runs, for now just compare the
# accumulatedFulidInterfaceDisplacement, if that is the same, the
# rest is likely the same

# remove OpenFoam's header from the file
(tail -n +19 fluid/processor0/0.005/accumulatedFluidInterfaceDisplacement) > fluid/processor0/0.005/accumulatedFluidInterfaceDisplacement_resultOnly
set resultFile = "processor0/0.005/accumulatedFluidInterfaceDisplacement_resultOnly"
sed 's/[)(]//g' archiveData/$resultFile > archiveData/$resultFile
sed 's/[)(]//g' fluid/$resultFile > fluid/$resultFile
$iradBinPath/diffdatafiles -t 1e-8 ../HronTurekFsiPar/fluid/$resultFile fluid/$resultFile
set STEST = $?

# move back to top level
cd ..
printf "ParOpenFoamModuleRegressionTest:Works=" >> ${TmpOut}
if( $STEST != 0 ) then
  echo "Results differ between parallel OpenFoam and parallel OFModuleDriver"
  echo "See the test results in HronTurekFsiParIMPACT for more details."
  # communicate  test results back to the testing framework
  printf "0\n" >> ${TmpOut}
  exit 1
else
  printf "1\n" >> ${TmpOut}
  # remove if successful
  #rm -r HronTurekFsi HronTurekFsiIMPACT
endif
 
cat ${TmpOut} >> ${OutFile}
rm ${TmpOut}

exit 0
