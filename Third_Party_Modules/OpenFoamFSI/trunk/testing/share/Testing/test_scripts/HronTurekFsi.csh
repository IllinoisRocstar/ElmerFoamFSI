#!/bin/tcsh
#
#  Test script for OpenFoamFsi. This regression test compares
#  the results of an OpenFoam run with the results from the
#  IMAPCT Driver run.
#
#  The user must have the openFoam enviornment correctly loaded for this
#  to work.
#
#  Usage:
#  HronTurekFsi.csh <output_file> <test_root> <build_binary>
#
#  <output_file>:  the name of a file that contains the output of the script, 
#  this is used by CTEST the populate the test results
#  <test_root>:  the root of our testing file structure
#  <build_binary>:  location of the IMPACT bin directory
#
echo "************************************"
echo "Running HronTurekFsi regression test"
echo "************************************"

set OutFile=$1
set TmpOut=${OutFile}_tmp.txt 

# Regression test for OpenFoamFSI test case

if( -d HronTurekFsi ) then
  echo "removing HronTurekFsi directory"
  rm -r HronTurekFsi
endif

mkdir HronTurekFsi
cd HronTurekFsi

cp -r $2/share/Testing/test_data/HronTurekFsi/* .

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
./Allsetup
./Allrun
echo "Finished running OpenFoam"

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

set resultFile = "0.005/accumulatedFluidInterfaceDisplacement" 
diff fluid/$resultFile archiveData/$resultFile
set STEST = $?

# move back to top level
cd ..
printf "OpenFoamFsiRegressionTest:Works=" >> ${TmpOut}
if( $STEST != 0 ) then
  echo "Results differ between OpenFoam and archived OpenFoamFsi Results"
  echo "See the test results in HronTurekFsi for more details."
  # communicate  test results back to the testing framework, 
  # this is not a fatal error
  printf "0\n" >> ${TmpOut}
else
  printf "1\n" >> ${TmpOut}
endif
 
# now make a directory for the IMPACT version 

if( -d HronTurekFsiIMPACT ) then
  echo "removing HronTurekFsiIMPACT directory"
  rm -r HronTurekFsiIMPACT
endif

mkdir HronTurekFsiIMPACT
cd HronTurekFsiIMPACT

cp -r $2/share/Testing/test_data/HronTurekFsi/* .

# setup IMPACT Version
echo "Running IMPACT OpenFoam Driver"
./Allsetup
cd fluid
$3/OFModuleDriver --driverRegression >& log.OFModuleDriver 
echo "Done running IMPACT OpenFoam Driver"

# check that everything finished 
grep "End" log.OFModuleDriver > /dev/null

if( $? != 0 ) then
  echo "Looks like OFModuleDriver failed to finish."
  echo "See the test results in HronTurekFsiIMPACT for more details."
  exit 1
endif

# now compare the results of the two runs, for now just compare the
# accumulatedFulidInterfaceDisplacement, if that is the same, the
# rest is likely the same

set resultFile = "0.005/accumulatedFluidInterfaceDisplacement" 
diff $resultFile ../../HronTurekFsi/fluid/$resultFile
set STEST = $?

# move back to top level
cd ../..
printf "HronTurekFsiRegressionTest:Works=" >> ${TmpOut}
if( $STEST != 0 ) then
  echo "Results differ between OpenFoam and OFModuleDriver"
  echo "See the test results in HronTurekFsiIMPACT for more details."
  # communicate  test results back to the testing framework
  printf "0\n" >> ${TmpOut}
  exit 1
else
  printf "1\n" >> ${TmpOut}
  # remove if successful
  rm -r HronTurekFsi HronTurekFsiIMPACT
endif
 
cat ${TmpOut} >> ${OutFile}
rm ${TmpOut}

exit 0
