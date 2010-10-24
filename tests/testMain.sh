#!/bin/bash

g++ ../main.cpp -o /tmp/boa_test_bin && test "`/tmp/boa_test_bin`" = "Figuring out code reviews" && echo SUCCESS && exit
echo FAILURE
