CC=clang++
C=gcc -Werror
DFLAGS=-D_DEBUG -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS=-g -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing  -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings
LLVM_DIR=../llvm
BUILD=build
SOURCE=source

${BUILD}/boa.so: ${BUILD} ${BUILD}/boa.o ${BUILD}/Constraint.o ${BUILD}/log.o ${BUILD}/ConstraintGenerator.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include  -Wl,-R -Wl,'$ORIGIN' -L${LLVM_DIR}/Debug+Asserts/lib -L${LLVM_DIR}/Debug+Asserts/lib  -shared -o ${BUILD}/boa.so ${BUILD}/boa.o  ${BUILD}/Constraint.o ${BUILD}/log.o ${BUILD}/ConstraintGenerator.o -lpthread -lglpk -ldl -lm

${BUILD}/boa.o: ${SOURCE}/boa.cpp ${SOURCE}/VarLiteral.h ${SOURCE}/Pointer.h ${SOURCE}/integer.h ${SOURCE}/Buffer.h ${SOURCE}/PointerAnalyzer.h ${SOURCE}/ConstraintGenerator.h ${BUILD}/Constraint.o ${BUILD}/log.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${CFLAGS} -c -MMD -MP -MF "${BUILD}/boa.d.tmp" -MT "${BUILD}/boa.o" -MT "${BUILD}/boa.d" ${SOURCE}/boa.cpp -o ${BUILD}/boa.o
	mv -f ${BUILD}/boa.d.tmp ${BUILD}/boa.d

${BUILD}/Constraint.o : ${SOURCE}/Constraint.cpp ${SOURCE}/Constraint.h
	${CC} ${SOURCE}/Constraint.cpp ${CFLAGS} -c -o ${BUILD}/Constraint.o

${BUILD}:
	mkdir -p ${BUILD}

clean:
	rm -fr ${BUILD}
	rm -rf tests/testcases/build

TESTCASES=$(patsubst tests/testcases/%.c,tests/testcases/build/%.out,$(wildcard tests/testcases/*.c))

tests: ${BUILD}/boa.so ${TESTCASES} FORCE
	tests/testAll.sh

tests/testcases/build/%.out : tests/testcases/%.c
	mkdir -p tests/testcases/build
	${C} $< -o $@
	
FORCE:

${BUILD}/ConstraintGenerator.o : ${SOURCE}/ConstraintGenerator.cpp ${SOURCE}/ConstraintGenerator.h ${BUILD}/log.o
	${CC} ${DFLAGS} -I${LLVM_DIR}/include -I${LLVM_DIR}/tools/clang/include ${SOURCE}/ConstraintGenerator.cpp ${CFLAGS} -c -o ${BUILD}/ConstraintGenerator.o

${BUILD}/log.o : ${SOURCE}/log.cpp ${SOURCE}/log.h
	${CC} ${SOURCE}/log.cpp ${CFLAGS} -c -o ${BUILD}/log.o

