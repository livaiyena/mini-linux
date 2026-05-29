#include "ipc.h"

uint64_t	get_timestamp_ns(void)
{
	struct timespec	ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}

void	print_telemetry(const t_telemetry *t)
{
	printf("[SEQ:%06lu] T:%d°C P:%dhPa H:%d%% "
		"A(%d,%d,%d) @%lu ns\n",
		(unsigned long)t->seq,
		t->temperature, t->pressure, t->humidity,
		t->accel_x, t->accel_y, t->accel_z,
		(unsigned long)t->timestamp_ns);
}

void	format_log_line(char *buf, size_t len, const t_telemetry *t)
{
	snprintf(buf, len,
		"%lu,%d,%d,%d,%d,%d,%d,%lu\n",
		(unsigned long)t->seq,
		t->temperature, t->pressure, t->humidity,
		t->accel_x, t->accel_y, t->accel_z,
		(unsigned long)t->timestamp_ns);
}
