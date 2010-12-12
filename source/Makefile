CC=g++
#~/build/Debug+Asserts/bin/clang++
DFLAGS=-D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS= -g -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing  -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings 
LLVM_DIR=../../build
LLVM_SRC_DIR=../../llvm

boa: main.cpp constraint.cpp constraint.h
	${CC} main.cpp constraint.cpp ${CCFLAGS} -o boa

PointerAnalysis.so: PointerAnalysis.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include -I${LLVM_SRC_DIR}/include -I${LLVM_SRC_DIR}/tools/clang/include  -Wl,-R -Wl,'$ORIGIN' -L${LLVM_DIR}/Debug+Asserts/lib -L${LLVM_DIR}/Debug+Asserts/lib  -shared -o PointerAnalysis.so PointerAnalysis.o  -lpthread -ldl -lm 
	
PointerAnalysis.o: PointerAnalysis.cpp
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${CFLAGS} -I${LLVM_SRC_DIR}/include -I${LLVM_SRC_DIR}/tools/clang/include -c -MMD -MP -MF "PointerAnalysis.d.tmp" -MT "PointerAnalysis.o" -MT "PointerAnalysis.d" PointerAnalysis.cpp -o PointerAnalysis.o
	mv -f "PointerAnalysis.d.tmp" "PointerAnalysis.d"

