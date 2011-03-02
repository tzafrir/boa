BOA - Buffer Overrun Analyzer
=============================

Prerequisites
-------------

Boa requires the installation of libglpk, llvm, libllvm and clang.
On a Ubuntu 10.10 system, those can be installed by running:
    sudo apt-get install clang libglpk-dev

(clang's dependencies include llvm and libllvm).

To run unit tests, the `googletest` testing suite is also required,
see instructions in the bottom of this document.

In order to compile simply run
    $ make

In order to test run
    $ make tests

In order to run
    $ ./boa tests/testcases/simple1.c  # (or any other c source file)

Or -
    $ ./boa
for brief help
    
You may also want to add -
    complete -W "-blame -log -glpk" -c ./boa
At the end of your ~/.bashrc to get tab completion of boa flags

How To Install Google Test
==========================

In boa/.., run:

    wget http://googletest.googlecode.com/files/gtest-1.5.0.tar.gz
    tar zxfv gtest-1.5.0.tar.gz
    cd gtest-1.5.0
    ./configure && make
