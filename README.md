BOA - Buffer Overrun Analyzer
=============================

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
