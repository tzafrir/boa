#!/bin/bash

function die {
	echo " !ERROR! $1";
	exit;
}

function run_testcase {
	if !(head -1 $1 | grep "BOA-TEST[[:space:]]*[[:digit:]][[:space:]]*$" > /dev/null); then die "Wrong testcase format $1"; fi 
	local OVERRUN=true;
	if (head -1 $1 | grep "BOA-TEST[[:space:]]*0[[:space:]]*$" > /dev/null); then OVERRUN=false; fi
	
	local RETVAL=false;
	if ($BOA $1 > /dev/null); then RETVAL=true; fi

	printf "running $1 - "
	if ($RETVAL); then
		if !($OVERRUN); then
			echo "SUCCESS! (No overruns)";
		else
			echo "COMPLETE FAILURE! (Overruns not found by boa)";
		fi
	else
		if ($OVERRUN); then
			echo "SUCCESS! (Overruns detected)";
		else
			echo "Mhe... (No overruns, but boa reported possible overruns)";
		fi		
	fi			
}

BOA=/tmp/boa_test_bin

g++ ../main.cpp -o $BOA || die "Compilation error"

for file in $(ls testcases/*.c); do
	run_testcase $file
done
 



