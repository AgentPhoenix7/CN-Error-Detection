CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Werror

BUILD_DIR := build
CHECKSUM_TEST := $(BUILD_DIR)/test_checksum
CHECKSUM_TEST_SOURCES := src/checksum.c tests/test_checksum.c

.PHONY: all test test_checksum clean

all: $(CHECKSUM_TEST)

test: test_checksum

test_checksum: $(CHECKSUM_TEST)
  ./$(CHECKSUM_TEST)

$(CHECKSUM_TEST): $(CHECKSUM_TEST_SOURCES) include/checksum.h | $(BUILD_DIR)
  $(CC) $(CPPFLAGS) $(CFLAGS) $(CHECKSUM_TEST_SOURCES) -o $@

$(BUILD_DIR):
  mkdir -p $(BUILD_DIR)

clean:
  rm -rf $(BUILD_DIR)