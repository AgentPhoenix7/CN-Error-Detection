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
FRAME_TEST := $(BUILD_DIR)/test_frame
FRAME_TEST_SOURCES := src/frame.c src/checksum.c src/crc.c tests/test_frame.c
FRAME_HEADER := include/frame.h
SOCKET_UTILS_TEST := $(BUILD_DIR)/test_socket_utils
SOCKET_UTILS_TEST_SOURCES := src/socket_utils.c tests/test_socket_utils.c
SOCKET_UTILS_HEADER := include/socket_utils.h
SENDER := $(BUILD_DIR)/sender
SENDER_SOURCES := src/sender.c src/socket_utils.c src/frame.c src/checksum.c src/crc.c src/error_injection.c
RECEIVER := $(BUILD_DIR)/receiver
RECEIVER_SOURCES := src/receiver.c src/socket_utils.c src/frame.c src/checksum.c src/crc.c
PYTHON_RUN := uv run --project .
EXPERIMENT_INPUT ?= test_data/multi_frame.bin
EXPERIMENT_REPEATS ?= 3

.PHONY: all test test_checksum test_crc test_error_injection test_frame test_socket_utils test_end_to_end generate_test_data experiments plots check_config_header check_crc_header clean

all: check_config_header $(CHECKSUM_TEST) $(CRC_TEST) $(ERROR_INJECTION_TEST) $(FRAME_TEST) $(SOCKET_UTILS_TEST) $(SENDER) $(RECEIVER)

test: check_config_header test_checksum test_crc test_error_injection test_frame test_socket_utils

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

test_frame: $(FRAME_TEST)
	./$(FRAME_TEST)

$(FRAME_TEST): $(FRAME_TEST_SOURCES) $(FRAME_HEADER) $(CONFIG_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FRAME_TEST_SOURCES) -o $@

test_socket_utils: $(SOCKET_UTILS_TEST)
	./$(SOCKET_UTILS_TEST)

$(SOCKET_UTILS_TEST): $(SOCKET_UTILS_TEST_SOURCES) $(SOCKET_UTILS_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOCKET_UTILS_TEST_SOURCES) -o $@

$(SENDER): $(SENDER_SOURCES) $(FRAME_HEADER) $(SOCKET_UTILS_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SENDER_SOURCES) -o $@

$(RECEIVER): $(RECEIVER_SOURCES) $(FRAME_HEADER) $(SOCKET_UTILS_HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RECEIVER_SOURCES) -o $@

test_end_to_end: all
	$(PYTHON_RUN) tools/run_experiments.py $(EXPERIMENT_INPUT) --schemes checksum crc8 crc10 crc16 crc32 --patterns none --repeats 1 --results /tmp/assignment1-e2e.csv

generate_test_data:
	$(PYTHON_RUN) tools/generate_test_data.py

experiments: all
	$(PYTHON_RUN) tools/run_experiments.py $(EXPERIMENT_INPUT) --repeats $(EXPERIMENT_REPEATS)

plots:
	$(PYTHON_RUN) tools/plot_results.py results/experiments.csv results/plots

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
