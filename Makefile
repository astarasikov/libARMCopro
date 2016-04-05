CC=clang
CFLAGS=-std=c99 -O2 -Wall -Wextra -Wpedantic
CFLAGS_COVERAGE=--coverage
CFLAGS_SANITIZERS=-fsanitize=undefined -fsanitize=address

ifeq ($(WITH_COVERAGE),1)
	CFLAGS+=$(CFLAGS_COVERAGE)
endif

ifeq ($(WITH_SANITIZERS),1)
	CFLAGS+=$(CFLAGS_SANITIZERS)
endif

OUT_DIR=out
COVERAGE_DIR=$(OUT_DIR)/coverage
SCAN_BUILD_DIR=$(OUT_DIR)/scan_build

GCOV_TOOL=llvm-cov gcov

APP_NAME=arm_mcr

SCAN_BUILD_OPTS=\
	-enable-checker alpha.core.BoolAssignment \
	-enable-checker alpha.core.CallAndMessageUnInitRefArg \
	-enable-checker alpha.core.CastSize \
	-enable-checker alpha.core.CastToStruct \
	-enable-checker alpha.core.FixedAddr \
	-enable-checker alpha.core.IdenticalExpr \
	-enable-checker alpha.core.PointerArithm \
	-enable-checker alpha.core.PointerSub \
	-enable-checker alpha.core.SizeofPtr \
	-enable-checker alpha.core.TestAfterDivZero \
	-enable-checker alpha.cplusplus.VirtualCall \
	-enable-checker alpha.deadcode.UnreachableCode \
	-enable-checker alpha.security.ArrayBoundV2 \
	-enable-checker alpha.security.MallocOverflow \
	-enable-checker alpha.security.ReturnPtrRange \
	-enable-checker alpha.security.taint.TaintPropagation \
	-enable-checker alpha.unix.Chroot \
	-enable-checker alpha.unix.PthreadLock \
	-enable-checker alpha.unix.SimpleStream \
	-enable-checker alpha.unix.Stream \
	-enable-checker alpha.unix.cstring.BufferOverlap \
	-enable-checker alpha.unix.cstring.NotNullTerminated \
	-enable-checker alpha.unix.cstring.OutOfBounds \
	-enable-checker security.FloatLoopCounter \
	-enable-checker security.insecureAPI.rand \
	-enable-checker security.insecureAPI.strcpy


APP_NAME:
	$(CC) -o $(APP_NAME) $(CFLAGS) $(APP_NAME).c

coverage:
	make clean
	make $(APP_NAME) WITH_COVERAGE=1
	mkdir -p $(COVERAGE_DIR)
	./$(APP_NAME)
	$(GCOV_TOOL) *.c
	gcovr --gcov-executable="$(GCOV_TOOL)" --html --html-details -o $(COVERAGE_DIR)/index.html -r . -g

full_test:
	make $(APP_NAME) WITH_SANITIZERS=1
	./$(APP_NAME) fulltest

scan_build:
	make clean
	mkdir -p $(SCAN_BUILD_DIR)
	scan-build $(SCAN_BUILD_OPTS) -o $(SCAN_BUILD_DIR) make

clean:
	rm $(APP_NAME) || true
	rm -rf $(OUT_DIR) || true
	rm *.gcno *.gcda *.gcov || true
