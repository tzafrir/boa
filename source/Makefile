CC=g++
#../../prj/llvm/Debug+Asserts/bin/clang++
DFLAGS=-D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS=-g -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing  -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings 
LLVM_DIR=../../llvm

boa.so: boa.o constraint.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include  -Wl,-R -Wl,'$ORIGIN' -L${LLVM_DIR}/Debug+Asserts/lib -L${LLVM_DIR}/Debug+Asserts/lib  -shared -o boa.so boa.o  constraint.o -lpthread -lglpk -ldl -lm 
	
boa.o: boa.cpp buffer.h PointerAnalyzer.h 
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${CFLAGS} -c -MMD -MP -MF "PointerAnalysis.d.tmp" -MT "PointerAnalysis.o" -MT "PointerAnalysis.d" boa.cpp -o boa.o
	mv -f "PointerAnalysis.d.tmp" "PointerAnalysis.d"

boa: main.cpp constraint.cpp constraint.h
	${CC} main.cpp constraint.cpp ${CFLAGS} -o boa

constraint.o : constraint.cpp constraint.h
	${CC} constraint.cpp ${CFLAGS} -c

