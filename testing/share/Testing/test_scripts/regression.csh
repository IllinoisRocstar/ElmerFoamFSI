#!/bin/tcsh

#Enter necessary filename variables here
set OutFile=$1
set TmpOut=${OutFile}_tmp.txt 
set InputDir=SimpleStatic
set Outputs=prb2.dat
set OutputsCheck=prb2_ref.dat
set TestName=SimpleStaticRegressionTest:Works

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
cd fluid
$3/elmerfoamfsi $2/share/Testing/test_data/${InputDir}/fluid/test.config

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
#(This uses our own speical diff (diffdatafiles) that
#can compare numbers with a tolerance. See documentation
#for more information on how to use it.)
@ i = 1
foreach file (${Outputs})
  diff ${file} ../$OutputsCheck[$i] 
  if($? != 0) then
    echo "${file} differs from $OutputsCheck[$i]"
    @ result = 0
  endif
  @ i += 1
end
cd ..

#print test results to OutFile
printf "${TestName}=" >> ${TmpOut}
printf "$result\n" >> ${TmpOut}
cat ${TmpOut} >> ../${OutFile}
cd ..
rm -r ${InputDir}

exit 0
