CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Werror

BUILD_DIR := build
CHECKSUM_TEST := $(BUILD_DIR)/test_checksum
CHECKSUM_TEST_SOURCES := src/checksum.c tests/test_checksum.c
CRC_HEADER := include/crc.h

.PHONY: all test test_checksum check_crc_header clean

all: $(CHECKSUM_TEST) check_crc_header

test: test_checksum check_crc_header

test_checksum: $(CHECKSUM_TEST)
	./$(CHECKSUM_TEST)

$(CHECKSUM_TEST): $(CHECKSUM_TEST_SOURCES) include/checksum.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CHECKSUM_TEST_SOURCES) -o $@

check_crc_header: $(CRC_HEADER)
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -fsyntax-only $(CRC_HEADER)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
