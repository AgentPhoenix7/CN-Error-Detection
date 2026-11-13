/** @file process_metrics.h
 * @brief Measures resident memory of the executing Linux program.
 */
#ifndef PROCESS_METRICS_H
#define PROCESS_METRICS_H

#include <stdbool.h>

/**
 * @brief Reads the post-exec peak resident set size from Linux procfs.
 *
 * Unlike getrusage's inherited high-water mark, VmHWM describes this
 * executable's address space. Requires readable /proc/self/status.
 *
 * @param[out] peak_rss_kb Peak RSS in KiB; unchanged on failure.
 * @return Whether a valid peak was read.
 */
bool process_metrics_peak_rss_kb(long *peak_rss_kb);

#endif
