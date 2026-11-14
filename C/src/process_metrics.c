#include "process_metrics.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Reads this executable's peak resident memory in KiB.
 *
 * Scans /proc/self/status for VmHWM, parses a positive long with strtol and
 * checks conversion errors, the kB suffix, and file errors before committing
 * the output. Only whitespace may follow the unit.
 */
bool process_metrics_peak_rss_kb(long *peak_rss_kb)
{
  char line[256];
  long peak = 0L;
  bool found = false;

  if (peak_rss_kb == NULL) {
    return false;
  }
  FILE* status = fopen("/proc/self/status", "r");
  if (status == NULL) {
    return false;
  }
  while (fgets(line, sizeof line, status) != NULL) {
    if (strncmp(line, "VmHWM:", 6U) == 0) {
      const char *value = line + 6U;
      char *end;
      errno = 0;
      peak = strtol(value, &end, 10);
      if (errno != 0 || end == value || peak <= 0L) {
        break;
      }
      while (isspace((unsigned char)*end)) {
        end++;
      }
      if (strncmp(end, "kB", 2U) != 0) {
        break;
      }
      end += 2U;
      while (isspace((unsigned char)*end)) {
        end++;
      }
      found = *end == '\0';
      break;
    }
  }
  const bool read_error = ferror(status) != 0;
  if (fclose(status) != 0 || read_error || !found) {
    return false;
  }
  *peak_rss_kb = peak;
  return true;
}
