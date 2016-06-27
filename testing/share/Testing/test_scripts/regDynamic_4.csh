#!/bin/tcsh

#Enter necessary filename variables here
set OutFile = ${1}
set TmpOut = ${OutFile}_tmp.txt 
set InputDir = SimpleDynamic_4
set Outputs = ./fluid/probe_proc_2_nde_28.dat
set OutputsCheck = ./fluid/ref_4.dat
set TestName = SimpleDynamic_4:Works

#Remove old test InputDir if present
if( -d  ${InputDir}) then
  echo "removing ${InputDir} directory"
  rm -r ${InputDir}
endif

#Make InputDir directory to run test in
mkdir ${InputDir}
cd ${InputDir}

#Copy input data into InputDir
cp -r $2/share/Testing/test_data/${InputDir}/* .

#Run executable to generate output data
#ENTER YOUR COMMAND FOR RUNNING YOUR EXECUTABLE HERE
./Allclean
./AllrunParDrvSetup
cd ./fluid
mpirun -np 4 $3/elmerfoamfsiPar -v 2 test.config
cd ..

#Make sure the necesary output was generated
foreach file (${Outputs})
  if( ! -e ${file} ) then
    echo "No ${file} results file from run!"
    exit 1
  endif
end
 
#variable for test passing
@ result = 1

#diff the new output file with the saved one to check
@ i = 1
foreach file (${Outputs})
  diff ${file} $OutputsCheck[$i] 
  if($? != 0) then
    echo "${file} differs from $OutputsCheck[$i]"
    @ result = 0
  endif
  @ i += 1
end

#print test results to OutFile
printf "${TestName}=" >> ${TmpOut}
printf "$result\n" >> ${TmpOut}
cat ${TmpOut} >> ../${OutFile}
cd ..
rm -r ${InputDir}

exit 0
