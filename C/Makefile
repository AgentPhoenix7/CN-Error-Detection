CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Werror

BUILD_DIR := build
CHECKSUM_TEST := $(BUILD_DIR)/test_checksum
CHECKSUM_TEST_SOURCES := src/checksum.c tests/test_checksum.c
CRC_HEADER := include/crc.h
CRC_SOURCE := src/crc.c
CRC_OBJECT := $(BUILD_DIR)/crc.o

.PHONY: all test test_checksum check_crc_header clean

all: $(CHECKSUM_TEST) $(CRC_OBJECT)

test: test_checksum check_crc_header $(CRC_OBJECT)

test_checksum: $(CHECKSUM_TEST)
	./$(CHECKSUM_TEST)

$(CHECKSUM_TEST): $(CHECKSUM_TEST_SOURCES) include/checksum.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CHECKSUM_TEST_SOURCES) -o $@

check_crc_header: $(CRC_HEADER)
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -fsyntax-only $(CRC_HEADER)

$(CRC_OBJECT): $(CRC_SOURCE) $(CRC_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(CRC_SOURCE) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
