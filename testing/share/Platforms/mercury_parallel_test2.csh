#!/bin/tcsh

#Enter necessary filename variables here
set RESULTSFILE = ${1}
set SRCDIR = ${2}
set BINDIR = ${3}
set IRADEXE = ${4}
echo ${RESULTSFILE}

rm -f tmpresults_2.txt
cat <<EOF > ./elmerfoamfsi2_parallel_test_batch.csh
#!/bin/tcsh
#
#PBS -V
#PBS -l nodes=2:ppn=8
#PBS -l walltime=00:30:00 
#PBS -j oe
#PBS -o elmerfoamfsi_parallel_test_batch_output2
#PBS -A MPINFRA

cd \${PBS_O_WORKDIR}
mpiexec -np 16 ${BINDIR}/pepi -o tmpresults_2.txt 1000000
EOF
qsub elmerfoamfsi2_parallel_test_batch.csh
@ i = 1
while($i <= 720)
    @ i += 1
    if( -e tmpresults_2.txt ) then
        @ i += 721;
    else
        sleep 10;
    endif
end
sleep 20

printf "PEPI:Runs=" >> ${RESULTSFILE}
@ err = 0
if ( -e tmpresults_2.txt ) then
   printf "1\n" >> ${RESULTSFILE}
else
   printf "0\n" >> ${RESULTSFILE}
   @ err += 1
endif
set RESULTS=`cat tmpresults_2.txt | grep 3.141592653589`
printf "PEPI:Works=" >> ${RESULTSFILE}
if ( "$RESULTS" == "") then
   printf "0\n" >> ${RESULTSFILE}
   @ err += 1
else
   printf "1\n" >> ${RESULTSFILE}
endif  
#rm -f tmpresults_2.txt
rm -f ./elmerfoamfsi_parallel_test_batch.csh
#rm -f elmerfoamfsi_parallel_test_batch_output
exit ${err}
