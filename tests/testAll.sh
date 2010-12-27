#!/bin/bash

function run_testcase {
  tests/testRunner.py $1
  # TODO(tzafrir): Count tests, failed, run, colors...
}

for file in $(ls tests/testcases/*.c); do
  TESTNAME=`echo $file | cut -d"." -f1`
  run_testcase $TESTNAME
done

