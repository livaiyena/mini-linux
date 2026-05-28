#include "sync.h"

void	log_skip(t_sync *sync, const char *path)
{
	char	ts[64];
	char	line[MAX_PATH_LEN + 128];
	int		len;
	time_t	now;

	now = time(NULL);
	strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
	len = snprintf(line, sizeof(line),
			"[%s] SKIP (up-to-date): %s\n", ts, path);
	pthread_mutex_lock(&sync->log_lock);
	write(sync->log_fd, line, len);
	pthread_mutex_unlock(&sync->log_lock);
}

void	log_perf(t_sync *sync, t_perf *p, int thread_count)
{
	char	ts[64];
	char	line[512];
	int		len;
	double	elapsed;
	time_t	now;

	now = time(NULL);
	strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
	elapsed = (p->end.tv_sec - p->start.tv_sec)
		+ (p->end.tv_nsec - p->start.tv_nsec) / 1e9;
	len = snprintf(line, sizeof(line),
			"[%s] PERF: threads=%d files=%ld bytes=%ld "
			"elapsed=%.4fs throughput=%.2fMB/s\n",
			ts, thread_count, p->files_copied, p->bytes_copied,
			elapsed, p->bytes_copied / (1024.0 * 1024.0) / elapsed);
	pthread_mutex_lock(&sync->log_lock);
	write(sync->log_fd, line, len);
	pthread_mutex_unlock(&sync->log_lock);
}
