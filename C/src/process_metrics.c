#include "process_metrics.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Reads this executable's peak resident memory in KiB.
 *
 * Scans /proc/self/status for VmHWM, validates its positive numeric value
 * and kB unit, checks file errors, and commits the output only on success.
 */
bool process_metrics_peak_rss_kb(long *peak_rss_kb)
{
  char line[256];
  long peak = 0L;
  bool found = false;
  FILE *status;

  if (peak_rss_kb == NULL) {
    return false;
  }
  status = fopen("/proc/self/status", "r");
  if (status == NULL) {
    return false;
  }
  while (fgets(line, sizeof line, status) != NULL) {
    if (strncmp(line, "VmHWM:", 6U) == 0) {
      char unit[3];
      found = sscanf(line, "VmHWM: %ld %2s", &peak, unit) == 2 &&
        strcmp(unit, "kB") == 0 && peak > 0L;
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
