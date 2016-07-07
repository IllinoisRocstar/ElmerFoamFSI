#!/bin/tcsh

#Enter necessary filename variables here
set RESULTSFILE = ${1}
set SRCDIR = ${2}
set BINDIR = ${3}
set IRADEXE = ${4}
echo ${RESULTSFILE}

rm -f tmpresults_1.txt
cat <<EOF > ./elmerfoamfsi1_parallel_test_batch.csh
#!/bin/tcsh
#
#PBS -V
#PBS -l nodes=2:ppn=8
#PBS -l walltime=00:30:00 
#PBS -j oe
#PBS -o elmerfoamfsi_parallel_test_batch_output1
#PBS -A MPINFRA

cd \${PBS_O_WORKDIR}
mpirun -np 4 ${BINDIR}/elmerfoamfsi_parallel_test -s ${SRCDIR} -o tmpresults_1.txt 
EOF
#mpirun -np 16 -machinefile \${PBS_NODEFILE} ./bin/elmerfoamfsi_parallel_test -o tmpresults_1.txt
qsub elmerfoamfsi1_parallel_test_batch.csh
@ i = 1
while($i <= 720)
    @ i += 1
    if( -e tmpresults_1.txt ) then
        @ i += 721;
    else
        sleep 10;
    endif
end
sleep 50
cat tmpresults_1.txt >> ${RESULTSFILE}
#rm -f tmpresults_1.txt
rm -f ./elmerfoamfsi_parallel_test_batch.csh
#rm -f elmerfoamfsi_parallel_test_batch_output
exit 0
