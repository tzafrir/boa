#!/bin/bash

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
NO_COLOR="\033[0m"

function die {
	echo " !ERROR! $1";
	exit;
}

function run_testcase {
	if !(head -1 $1 | grep "BOA-TEST[[:space:]]*[[:digit:]][[:space:]]*$" > /dev/null); then die "Wrong testcase format $1"; fi
	local OVERRUN=true;
	if (head -1 $1 | grep "BOA-TEST[[:space:]]*0[[:space:]]*$" > /dev/null); then OVERRUN=false; fi
	
	local RETVAL=false;
	if ($RUN_BOA $1 &> /dev/stdout | egrep "^boa\[0\]$" > /dev/null); then RETVAL=true; fi

	printf "running $1 - "
	if ($RETVAL); then
		if !($OVERRUN); then
			echo -e "$GREEN SUCCESS! $NO_COLOR (No overruns)";
		else
			echo -e "$RED FAILURE!  $NO_COLOR (Missed possible overruns)";
		fi
	else
		if ($OVERRUN); then
			echo -e "$GREEN SUCCESS!  $NO_COLOR (Overruns detected)";
		else
			echo -e "$YELLOW Mhe...  $NO_COLOR (No overruns, but boa reported possible overruns)";
		fi		
	fi			
}

LLVM=../llvm
BOA=build/boa.so
RUN_BOA="${LLVM}/Debug+Asserts/bin/clang -cc1 -load ${BOA} -plugin boa"

for file in $(ls tests/testcases/*.c); do
	run_testcase $file
done

