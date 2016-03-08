#!/bin/tcsh

rm -f tmpresults_1.txt
cat <<EOF > ./elmerfoamfsi_parallel_test_batch.csh
#!/bin/tcsh
#
#PBS -l nodes=2:ppn=8
#PBS -l walltime=00:30:00 
#PBS -j oe
#PBS -o elmerfoamfsi_parallel_test_batch_output

cd \${PBS_O_WORKDIR}
mpirun -np 16 ./bin/elmerfoamfsi_parallel_test -o tmpresults_1.txt
EOF
#mpirun -np 16 -machinefile \${PBS_NODEFILE} ./bin/elmerfoamfsi_parallel_test -o tmpresults_1.txt
qsub elmerfoamfsi_parallel_test_batch.csh
@ i = 1
while($i <= 720)
    @ i += 1
    if( -e tmpresults_1.txt ) then
        @ i += 721;
    else
        sleep 10;
    endif
end
cat tmpresults_1.txt >> $1
rm -f tmpresults_1.txt
rm -f ./elmerfoamfsi_parallel_test_batch.csh
rm -f elmerfoamfsi_parallel_test_batch_output
exit 0
