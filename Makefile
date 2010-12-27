CC=g++
DFLAGS=-D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS=-g -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing  -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings 
LLVM_DIR=../llvm
BUILD=build
SOURCE=source

${BUILD}/boa.so: ${BUILD} ${BUILD}/boa.o ${BUILD}/constraint.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include  -Wl,-R -Wl,'$ORIGIN' -L${LLVM_DIR}/Debug+Asserts/lib -L${LLVM_DIR}/Debug+Asserts/lib  -shared -o ${BUILD}/boa.so ${BUILD}/boa.o  ${BUILD}/constraint.o -lpthread -lglpk -ldl -lm 
	
${BUILD}/boa.o: ${SOURCE}/boa.cpp ${SOURCE}/buffer.h ${SOURCE}/PointerAnalyzer.h
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${CFLAGS} -c -MMD -MP -MF "${BUILD}/PointerAnalysis.d.tmp" -MT "${BUILD}/PointerAnalysis.o" -MT "${BUILD}/PointerAnalysis.d" ${SOURCE}/boa.cpp -o ${BUILD}/boa.o
	mv -f ${BUILD}/PointerAnalysis.d.tmp ${BUILD}/PointerAnalysis.d

${BUILD}/constraint.o: ${SOURCE}/constraint.cpp ${SOURCE}/constraint.h
	${CC} ${SOURCE}/constraint.cpp ${CFLAGS} -c -o ${BUILD}/constraint.o

${BUILD}:
	mkdir ${BUILD}

clean:
	rm -fr ${BUILD}

tests: ${BUILD}/boa.so
	tests/testAll.sh

