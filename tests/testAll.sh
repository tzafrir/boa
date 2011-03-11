#!/bin/bash

GREEN="\033[0;32m"
RED="\033[0;31m"
NO_COLOR="\033[0m"

tested=0
passed=0

for arg in $@; do
  if [ "$arg" == "-v" ]; then
    verbose=true
  else
    flags="$flags $arg"
  fi
done

function run_testcase {
  (( tested++ ))
  tests/testRunner.py $blame $flags $1
  return $?
}

for file in $(ls tests/testcases/*.c); do
  TESTNAME=`echo $file | cut -d"." -f1`
  run_testcase $TESTNAME
  if [ "$?" == "0" ]; then
    (( passed++ ))
    if [ $verbose ]; then
      echo -e "$GREEN""Test Passed: $TESTNAME""$NO_COLOR"
    fi
  else
    echo -e "$RED""Test Failed: $TESTNAME""$NO_COLOR"
  fi
done

echo "---"
echo -e "All Tests:"
echo -e "  $tested tests run"
echo -e "  $GREEN$passed tests passed$NO_COLOR"
if [ "$tested" != "$passed" ]; then
  echo -e "  $RED$(( $tested - $passed )) tests failed$NO_COLOR"
fi
