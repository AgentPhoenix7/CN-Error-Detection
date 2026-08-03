CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Werror

BUILD_DIR := build
CONFIG_HEADER := include/config.h
CHECKSUM_TEST := $(BUILD_DIR)/test_checksum
CHECKSUM_TEST_SOURCES := src/checksum.c tests/test_checksum.c
CRC_TEST := $(BUILD_DIR)/test_crc
CRC_TEST_SOURCES := src/crc.c tests/test_crc.c
CRC_HEADER := include/crc.h
ERROR_INJECTION_TEST := $(BUILD_DIR)/test_error_injection
ERROR_INJECTION_TEST_SOURCES := src/error_injection.c tests/test_error_injection.c
ERROR_INJECTION_HEADER := include/error_injection.h

.PHONY: all test test_checksum test_crc test_error_injection check_config_header check_crc_header clean

all: check_config_header $(CHECKSUM_TEST) $(CRC_TEST) $(ERROR_INJECTION_TEST)

test: check_config_header test_checksum test_crc test_error_injection

check_config_header: $(CONFIG_HEADER)
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -fsyntax-only $(CONFIG_HEADER)

test_checksum: $(CHECKSUM_TEST)
	./$(CHECKSUM_TEST)

$(CHECKSUM_TEST): $(CHECKSUM_TEST_SOURCES) include/checksum.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CHECKSUM_TEST_SOURCES) -o $@

test_crc: $(CRC_TEST)
	./$(CRC_TEST)

$(CRC_TEST): $(CRC_TEST_SOURCES) $(CRC_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CRC_TEST_SOURCES) -o $@

check_crc_header: $(CRC_HEADER)
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -fsyntax-only $(CRC_HEADER)

test_error_injection: $(ERROR_INJECTION_TEST)
	./$(ERROR_INJECTION_TEST)

$(ERROR_INJECTION_TEST): $(ERROR_INJECTION_TEST_SOURCES) $(ERROR_INJECTION_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ERROR_INJECTION_TEST_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
