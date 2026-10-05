# Usage:
#   make             release build (build/release/she)
#   make asan        release + AddressSanitizer (build/release-asan/she)
#   make test        build and run tests
#   make test-asan   build and run tests with AddressSanitizer
#   make clean       delete the build directory
#   DEBUG=1 make     debug tree instead of release (build/debug)

# Compiler, target executable and version
CC          := gcc
TARGET      := she
VERSION     := 0.5.1


# Source, build and config directories
BUILD_DIR   := build
SRC_DIR     := src

RELEASE_DIR  := $(BUILD_DIR)/release
DEBUG_DIR    := $(BUILD_DIR)/debug

# Build modes (0, 1). Output trees are isolated per config
# so switching modes never reuses stale objects/binaries.
DEBUG ?= 0
ASAN  ?= 0

ifeq ($(DEBUG),0)
	CFG         := $(RELEASE_DIR)
	# Compiler flags for release
	CFLAGS      += -O2
else
	CFG         := $(DEBUG_DIR)
	# Compiler flags for debug
	CFLAGS      += -g3 -O0
endif

ifeq ($(ASAN),1)
	CFG     := $(CFG)-asan
	CFLAGS  += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif


OBJ_DIR     := $(CFG)/obj
TARGET_EXEC := $(CFG)/$(TARGET)

SRCS        := $(shell find $(SRC_DIR) -type f -name "*.c")
OBJS        := $(SRCS:%.c=$(OBJ_DIR)/%.o)

INC_DIRS    := include
INC_FLAGS   := $(addprefix -I,$(INC_DIRS))


# Test directories and files
TEST_DIR    := tests
TEST_LIB    := $(TEST_DIR)/test_lib

TEST_SRCS   := $(shell find $(TEST_DIR) -type f -name "*.c")
TEST_OBJS   := $(TEST_SRCS:%.c=$(OBJ_DIR)/%.o)

TEST_BIN_DIR := $(CFG)/test


# Compiler Flags
DEPS        := $(OBJS:%.o=%.d) $(TEST_OBJS:%.o=%.d)
DEPFLAGS    := -MMD -MP

CPPFLAGS    := $(INC_FLAGS) $(DEPFLAGS)      \
               -DSHE_VERSION=\"$(VERSION)\"  \
               -DSHE_BUILD=\"$(notdir $(CFG))\"
CFLAGS      += -Wall -Wextra
LDLIBS      += -lreadline


# Targets

all: $(TARGET_EXEC)

asan:
	$(MAKE) all ASAN=1

$(TARGET_EXEC): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


# Test Targets
$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(TEST_LIB) $(CFLAGS) -c $< -o $@


$(TEST_BIN_DIR)/cd_test: $(OBJ_DIR)/$(TEST_DIR)/cd_test.o \
							$(OBJ_DIR)/$(SRC_DIR)/builtins/cd.o \
							$(OBJ_DIR)/$(SRC_DIR)/util.o \
							$(OBJ_DIR)/$(SRC_DIR)/shell.o
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@


$(TEST_BIN_DIR)/trim_test: $(OBJ_DIR)/$(TEST_DIR)/trim_test.o \
							$(OBJ_DIR)/$(SRC_DIR)/util.o \
							$(OBJ_DIR)/$(SRC_DIR)/shell.o
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@


$(TEST_BIN_DIR)/tokenize_test: $(OBJ_DIR)/$(TEST_DIR)/tokenize_test.o \
								$(OBJ_DIR)/$(SRC_DIR)/tokenize.o \
								$(OBJ_DIR)/$(SRC_DIR)/token.o \
								$(OBJ_DIR)/$(SRC_DIR)/util.o \
								$(OBJ_DIR)/$(SRC_DIR)/str_util.o \
								$(OBJ_DIR)/$(SRC_DIR)/shell.o \
								$(OBJ_DIR)/$(TEST_LIB)/test_lib.o
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@


$(TEST_BIN_DIR)/expansion_test: $(OBJ_DIR)/$(TEST_DIR)/expansion_test.o \
								$(OBJ_DIR)/$(TEST_LIB)/test_lib.o \
								$(OBJ_DIR)/$(SRC_DIR)/expansion.o \
								$(OBJ_DIR)/$(SRC_DIR)/tokenize.o \
								$(OBJ_DIR)/$(SRC_DIR)/token.o \
								$(OBJ_DIR)/$(SRC_DIR)/util.o \
								$(OBJ_DIR)/$(SRC_DIR)/str_util.o \
								$(OBJ_DIR)/$(SRC_DIR)/shell.o
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@


$(TEST_BIN_DIR)/parser_test: $(OBJ_DIR)/$(TEST_DIR)/parser_test.o \
								$(OBJ_DIR)/$(TEST_LIB)/test_lib.o \
								$(OBJ_DIR)/$(SRC_DIR)/parser.o \
								$(OBJ_DIR)/$(SRC_DIR)/ast.o \
								$(OBJ_DIR)/$(SRC_DIR)/tokenize.o \
								$(OBJ_DIR)/$(SRC_DIR)/token.o \
								$(OBJ_DIR)/$(SRC_DIR)/util.o \
								$(OBJ_DIR)/$(SRC_DIR)/str_util.o \
								$(OBJ_DIR)/$(SRC_DIR)/shell.o
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@


test-unit: $(TEST_BIN_DIR)/cd_test \
		$(TEST_BIN_DIR)/trim_test \
		$(TEST_BIN_DIR)/tokenize_test \
		$(TEST_BIN_DIR)/expansion_test \
		$(TEST_BIN_DIR)/parser_test
	$(foreach bin,$^,./$(bin);)

test-unit-asan:
	$(MAKE) test-unit ASAN=1

test-py: $(TARGET_EXEC)
	pytest -vv $(TEST_DIR)/integration --she-path $(TARGET_EXEC)

test-py-asan:
	$(MAKE) test-py ASAN=1

test: test-unit test-py

test-asan: test-unit-asan test-py-asan

# Clean build
clean:
	rm -rf $(BUILD_DIR) .pytest_cache $(TEST_DIR)/integration/__pycache__

.PHONY: all asan clean test test-asan test-unit test-unit-asan test-py

-include $(DEPS)
