CC := gcc
.DEFAULT_GOAL := all
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
EVALUATION_TEST := $(BUILD_DIR)/test_evaluation
EVALUATION_TEST_SOURCES := src/checksum.c src/crc.c src/error_injection.c tests/test_evaluation.c
SENDER := $(BUILD_DIR)/sender
SENDER_SOURCES := src/sender.c src/sender_errors.c src/process_metrics.c src/socket_utils.c src/frame.c src/checksum.c src/crc.c src/error_injection.c
RECEIVER := $(BUILD_DIR)/receiver
RECEIVER_SOURCES := src/receiver.c src/process_metrics.c src/socket_utils.c src/frame.c src/checksum.c src/crc.c
SENDER_RANDOM_TEST := $(BUILD_DIR)/test_sender_random
SENDER_RANDOM_TEST_SOURCES := tests/test_sender_random.c src/sender_errors.c src/error_injection.c
ALL_SOURCES := $(sort $(CHECKSUM_TEST_SOURCES) $(CRC_TEST_SOURCES) $(ERROR_INJECTION_TEST_SOURCES) $(FRAME_TEST_SOURCES) $(SOCKET_UTILS_TEST_SOURCES) $(EVALUATION_TEST_SOURCES) $(SENDER_SOURCES) $(RECEIVER_SOURCES) $(SENDER_RANDOM_TEST_SOURCES))
REVIEW_DIR := $(abspath ../.scratch/end-to-end)
PYTHON_RUN := uv run --project .
EXPERIMENT_INPUT ?= test_data/multi_frame.bin
EXPERIMENT_REPEATS ?= 3
DETECTION_INPUT ?= input_files/detection_1m.txt
DETECTION_REPEATS ?= 20
PERFORMANCE_INPUT ?= input_files/performance_8m.txt
PERFORMANCE_REPEATS ?= 10
ERROR_INTERVAL ?= 2
INFERENCE_RESULTS ?= results/experiments_mixed.csv
INFERENCE_OUTPUT_DIRECTORY ?= output_files
PLOT_INPUT ?= $(INFERENCE_RESULTS)
REPORT_TEMPLATE := report/report_template.md
REPORT_OUTPUT := report/report.md

.PHONY: all test test_checksum test_crc test_error_injection test_frame test_socket_utils test_evaluation test_sender_random test_tools test_end_to_end test_mixed_end_to_end generate_test_data experiments experiments_inference plots report results check_config_header check_crc_header clean

all: check_config_header $(CHECKSUM_TEST) $(CRC_TEST) $(ERROR_INJECTION_TEST) $(FRAME_TEST) $(SOCKET_UTILS_TEST) $(EVALUATION_TEST) $(SENDER_RANDOM_TEST) $(SENDER) $(RECEIVER)

test: check_config_header test_checksum test_crc test_error_injection test_frame test_socket_utils test_evaluation test_sender_random test_tools

check_config_header: $(CONFIG_HEADER)
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -fsyntax-only $(CONFIG_HEADER)

test_checksum: $(CHECKSUM_TEST)
	./$(CHECKSUM_TEST)

$(CHECKSUM_TEST): $(CHECKSUM_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test_crc: $(CRC_TEST)
	./$(CRC_TEST)

$(CRC_TEST): $(CRC_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

check_crc_header: $(CRC_HEADER)
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -fsyntax-only $(CRC_HEADER)

test_error_injection: $(ERROR_INJECTION_TEST)
	./$(ERROR_INJECTION_TEST)

$(ERROR_INJECTION_TEST): $(ERROR_INJECTION_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test_frame: $(FRAME_TEST)
	./$(FRAME_TEST)

$(FRAME_TEST): $(FRAME_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test_socket_utils: $(SOCKET_UTILS_TEST)
	./$(SOCKET_UTILS_TEST)

$(SOCKET_UTILS_TEST): $(SOCKET_UTILS_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test_evaluation: $(EVALUATION_TEST)
	./$(EVALUATION_TEST)

$(EVALUATION_TEST): $(EVALUATION_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test_sender_random: $(SENDER_RANDOM_TEST)
	./$(SENDER_RANDOM_TEST)

$(SENDER_RANDOM_TEST): $(SENDER_RANDOM_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(ALL_SOURCES:%.c=$(BUILD_DIR)/%.d)

test_tools: $(SENDER) $(RECEIVER)
	$(PYTHON_RUN) python -m unittest discover -s tests -p 'test_*.py' -v

$(SENDER): $(SENDER_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(RECEIVER): $(RECEIVER_SOURCES:%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test_end_to_end: all
	$(PYTHON_RUN) tools/run_experiments.py $(EXPERIMENT_INPUT) --schemes checksum crc8 crc10 crc16 crc32 --patterns none --repeats 1 --results $(REVIEW_DIR)/control.csv --output-directory $(REVIEW_DIR)/control

test_mixed_end_to_end: all
	$(PYTHON_RUN) tools/run_experiments.py $(EXPERIMENT_INPUT) --schemes checksum crc8 crc10 crc16 crc32 --patterns single --repeats 1 --error-interval 2 --results $(REVIEW_DIR)/mixed.csv --output-directory $(REVIEW_DIR)/mixed
	$(PYTHON_RUN) tests/validate_mixed_csv.py $(REVIEW_DIR)/mixed.csv

generate_test_data:
	$(PYTHON_RUN) tools/generate_test_data.py

experiments: all
	$(PYTHON_RUN) tools/run_experiments.py $(EXPERIMENT_INPUT) --repeats $(EXPERIMENT_REPEATS) --error-interval $(ERROR_INTERVAL)

experiments_inference: all generate_test_data
	@set -eu; \
	mkdir -p "$(dir $(INFERENCE_RESULTS))"; \
	campaign_stage=$$(mktemp "$(dir $(INFERENCE_RESULTS)).inference.XXXXXX.csv"); \
	trap 'rm -f "$$campaign_stage"' EXIT; \
	trap 'exit 1' HUP INT TERM; \
	$(PYTHON_RUN) tools/run_experiments.py "$(DETECTION_INPUT)" --patterns single two odd burst --repeats $(DETECTION_REPEATS) --error-interval $(ERROR_INTERVAL) --results "$$campaign_stage" --output-directory "$(INFERENCE_OUTPUT_DIRECTORY)"; \
	$(PYTHON_RUN) tools/run_experiments.py "$(PERFORMANCE_INPUT)" --patterns none --repeats $(PERFORMANCE_REPEATS) --results "$$campaign_stage" --output-directory "$(INFERENCE_OUTPUT_DIRECTORY)" --append; \
	$(PYTHON_RUN) tools/experiment_results.py "$$campaign_stage"; \
	mv -f "$$campaign_stage" "$(INFERENCE_RESULTS)"

plots:
	$(PYTHON_RUN) tools/plot_results.py $(PLOT_INPUT) results/plots

report:
	$(PYTHON_RUN) tools/generate_report.py $(PLOT_INPUT) $(REPORT_TEMPLATE) $(REPORT_OUTPUT)

results: plots report

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
