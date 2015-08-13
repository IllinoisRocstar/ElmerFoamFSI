#!/bin/tcsh

set OutFile=$1
set TmpOut=${OutFile}_tmp.txt

# Test the serial example program output
./bin/sep -o test_sep_out.txt CMakeCache.txt 
set STEST=`diff CMakeCache.txt test_sep_out.txt`
rm -f test_sep_out.txt
printf "ExampleProgram:Works=" >> ${TmpOut}
if( "$STEST" == "") then
  printf "1\n" >> ${TmpOut}
else
  printf "0\n" >> ${TmpOut}
endif
cat ${TmpOut} >> ${OutFile}
rm -f ${TmpOut}
exit 0
