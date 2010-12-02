CC=g++
DFLAGS=-D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS=-g -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing  -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings
LLVM_DIR=../../prj/llvm

clang_example.so: CreateConstraints.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include  -Wl,-R -Wl,'$ORIGIN' -L${LLVM_DIR}/Debug+Asserts/lib -L${LLVM_DIR}/Debug+Asserts/lib  -shared -o CreateConstraints.so CreateConstraints.o  -lpthread -ldl -lm 
	
CreateConstraints.o: CreateConstraints.cpp
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${CFLAGS} -c -MMD -MP -MF "CreateConstraints.d.tmp" -MT "CreateConstraints.o" -MT "CreateConstraints.d" CreateConstraints.cpp -o CreateConstraints.o
	mv -f "CreateConstraints.d.tmp" "CreateConstraints.d"

