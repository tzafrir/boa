CC=clang++
DFLAGS=-D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS=-g -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing  -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings
LLVM_DIR=../llvm
BUILD=build
SOURCE=source

${BUILD}/boa.so: ${BUILD} ${BUILD}/boa.o ${BUILD}/constraint.o ${BUILD}/log.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include  -Wl,-R -Wl,'$ORIGIN' -L${LLVM_DIR}/Debug+Asserts/lib -L${LLVM_DIR}/Debug+Asserts/lib  -shared -o ${BUILD}/boa.so ${BUILD}/boa.o  ${BUILD}/constraint.o ${BUILD}/log.o -lpthread -lglpk -ldl -lm

${BUILD}/boa.o: ${SOURCE}/boa.cpp ${SOURCE}/varLiteral.h ${SOURCE}/pointer.h ${SOURCE}/buffer.h ${SOURCE}/PointerAnalyzer.h ${SOURCE}/ConstraintGenerator.h ${BUILD}/constraint.o ${BUILD}/log.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${CFLAGS} -c -MMD -MP -MF "${BUILD}/boa.d.tmp" -MT "${BUILD}/boa.o" -MT "${BUILD}/boa.d" ${SOURCE}/boa.cpp -o ${BUILD}/boa.o
	mv -f ${BUILD}/boa.d.tmp ${BUILD}/boa.d

${BUILD}/constraint.o : ${SOURCE}/constraint.cpp ${SOURCE}/constraint.h
	${CC} ${SOURCE}/constraint.cpp ${CFLAGS} -c -o ${BUILD}/constraint.o

${BUILD}:
	mkdir ${BUILD}

clean:
	rm -fr ${BUILD}

tests: ${BUILD}/boa.so FORCE
	tests/testAll.sh
	
FORCE:

${BUILD}/log.o : ${SOURCE}/log.cpp ${SOURCE}/log.h
	${CC} ${SOURCE}/log.cpp ${CFLAGS} -c -o ${BUILD}/log.o

