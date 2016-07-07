#!/bin/tcsh
#
#  Test script for OpenFoamFsiPar. This regression test compares
#  the results of an OpenFoam parallel run with the results from the
#  elmerfoamFSIPar Driver run.
#
#  The user must have the openFoam enviornment correctly loaded for this
#  to work.
#
#  mercury_HronTurekFsiPar.csh <output_file> <test_root> <build_binary>
#  <irad_bin>
#
#  <output_file>:  the name of a file that contains the output of the script, 
#  this is used by CTEST the populate the test results
#  <test_root>:  the root of our testing file structure
#  <build_binary>:  location of the IMPACT bin directory
#  <irad_bin>: Location of IRAD's binary directory for test results

set RESULTSFILE = ${1}
set SRCDIR = ${2}
set BINDIR = ${3}
set IRADEXE = ${4}
echo ${RESULTSFILE}
echo ${SRCDIR}
echo ${BINDIR}
echo ${IRADEXE}

# this test is developed based on its equivalen for workstations inorder to
# simplyfy debuging will keep same structure and modify accordingly where
# needed
echo "***************************************"
echo "Running HronTurekFsiPar regression test"
echo "***************************************"
set OutFile=${RESULTSFILE}
set TmpOut=${OutFile}_tmp 
set iradBinPath=${IRADEXE}


echo "***************************************"
echo "Running HronTurekFsiPar openFoam test"
echo "***************************************"
# Regression test for OpenFoamFSI test case
if( -d HronTurekFsiPar ) then
  echo "removing HronTurekFsiPar directory"
  rm -r HronTurekFsiPar
endif
mkdir HronTurekFsiPar
cd HronTurekFsiPar
cp -r ${SRCDIR}/share/Testing/test_data/HronTurekFsiPar/* .
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
./AllrunParDrvSetup
cd fluid
cat <<EOF > ./openfoam_parallel_test_batch.csh
#!/bin/tcsh
#
#PBS -V
#PBS -l nodes=1:ppn=8
#PBS -l walltime=00:30:00 
#PBS -j oe
#PBS -o openfoam_parallel_test_batch_output
#PBS -A MPINFRA

cd \${PBS_O_WORKDIR}
mpirun -np 2 -machinefile \${PBS_NODEFILE} $FoamApp -parallel 
echo "1" >> openFoamFinished
exit 0
EOF
qsub openfoam_parallel_test_batch.csh
@ i = 1
while($i <= 720)
    @ i += 1
    if( -e openFoamFinished ) then
        @ i += 721;
    else
        sleep 10;
    endif
end
sleep 20
cd ..
echo "Finished running parallel OpenFoam"
# check that everything finished 
grep "End" fluid/openfoam_parallel_test_batch_output > /dev/null
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
sleep 20
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


echo "********************************************"
echo "Running HronTurekFsiPar openFoamModule test"
echo "********************************************"
# now make a directory for the Parallel OpenFoamModule version 
if( -d HronTurekFsiParIMPACT ) then
  echo "removing HronTurekFsiIMPACT directory"
  rm -r HronTurekFsiParIMPACT
endif

mkdir HronTurekFsiParIMPACT
cd HronTurekFsiParIMPACT
cp -r ${SRCDIR}/share/Testing/test_data/HronTurekFsiPar/* .
# setup IMPACT Version
echo "Running IMPACT Parallel OpenFoam Driver"
./AllrunParDrvSetup
cd fluid
cat <<EOF > ./OFModule_parallel_test_batch.csh
#!/bin/tcsh
#
#PBS -V
#PBS -l nodes=1:ppn=8
#PBS -l walltime=00:30:00 
#PBS -j oe
#PBS -o OFModule_parallel_test_batch_output
#PBS -A MPINFRA

cd \${PBS_O_WORKDIR}
mpirun -np 2 -machinefile \${PBS_NODEFILE} ${BINDIR}/OFModuleDriverPar --parallel
echo "1" >> OFModuleFinished
exit 0
EOF
qsub OFModule_parallel_test_batch.csh
@ i = 1
while($i <= 720)
    @ i += 1
    if( -e OFModuleFinished ) then
        @ i += 721;
    else
        sleep 10;
    endif
end
sleep 20
cd ..
# check that everything finished 
grep "FsiFoam:Unload" fluid/OFModule_parallel_test_batch_output > /dev/null
if( $? != 0 ) then
  echo "Looks like OFModuleDrvPar failed to finish."
  echo "See the test results in HronTurekFsiParIMPACT for more details."
  exit 1
endif
echo "Finished running parallel OFModule"

# now compare the results of the two runs, for now just compare the
# accumulatedFulidInterfaceDisplacement, if that is the same, the
# rest is likely the same

# remove OpenFoam's header from the file
sleep 20
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
#rm ${TmpOut}


exit 0
