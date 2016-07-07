#!/bin/tcsh

#Enter necessary filename variables here
set RESULTSFILE = ${1}
set SRCDIR = ${2}
set BINDIR = ${3}
set IRADEXE = ${4}
echo ${RESULTSFILE}
echo ${SRCDIR}
echo ${BINDIR}
echo ${IRADEXE}

# Intertnal variables
set OutFile = ${RESULTSFILE}
set TmpOut = ${OutFile}_tmp.txt
set InputDir = HronTurekFSI3_136
set prbFile = probe_proc_70_nde_6.dat
set Outputs = ./fluid/${prbFile}
set OutputsCheck = ./fluid/ref_136.dat
set TestName = HronTurekFSI3_136:Works

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
./Allclean
./AllrunParDrvSetup
cd ./fluid
cat <<EOF > ./regHTFSI3_parallel_test_batch.csh
#!/bin/tcsh
#
#PBS -V
#PBS -l nodes=compute-0-31:ppn=8+compute-0-26:ppn=8+compute-0-25:ppn=8+compute-0-24:ppn=8+compute-0-23:ppn=8+compute-0-21:ppn=8+compute-0-20:ppn=8+compute-0-19:ppn=8+compute-0-18:ppn=8+compute-0-17:ppn=8+compute-0-16:ppn=8+compute-0-13:ppn=8+compute-0-12:ppn=8+compute-0-11:ppn=8+compute-0-10:ppn=8+compute-0-5:ppn=8+compute-0-3:ppn=8
#PBS -l walltime=00:03:30 
#PBS -j oe
#PBS -o regHTFSI3_parallel_test_batch_output
#PBS -A MPINFRA
cd \${PBS_O_WORKDIR}
cat \${PBS_NODEFILE} > chosenNodes.txt
mpirun -np 136 -machinefile \${PBS_NODEFILE} ${BINDIR}/elmerfoamfsiPar -v 2 test.config
exit 0
EOF
qsub regHTFSI3_parallel_test_batch.csh
@ i = 1
while($i <= 720)
    @ i += 1
    if( -e ${prbFile} ) then
        set res = `cat ${prbFile} | grep 0.001`
        if ( "${res}" == "" ) then
            echo "Wait iteration $i";
        else
            @ i += 721;
        endif
    else
        sleep 10;
    endif
end
sleep 100
#mpirun -np 4 $3/elmerfoamfsiPar -v 2 test.config
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
#(This uses our own speical diff (diffdatafiles) that
#can compare numbers with a tolerance. See documentation
#for more information on how to use it.)
@ i = 1
foreach file (${Outputs})
  #$4/diffdatafiles ${file} $OutputsCheck[$i] -t 1.0e-10 -n
  set res = `diff ${file} $OutputsCheck[$i]`
  printf "${res}" >> diffres.txt
  if ( "${res}" == "" ) then
    echo "Test is passed!"
  else
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
#rm -r ${InputDir}

exit 0

