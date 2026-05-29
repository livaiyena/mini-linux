#include "sync.h"

void	perf_init(t_perf *p)
{
	memset(p, 0, sizeof(t_perf));
	pthread_mutex_init(&p->lock, NULL);
}

void	perf_start(t_perf *p)
{
	clock_gettime(CLOCK_MONOTONIC, &p->start);
}

void	perf_end(t_perf *p)
{
	clock_gettime(CLOCK_MONOTONIC, &p->end);
}

void	perf_add(t_perf *p, long bytes)
{
	pthread_mutex_lock(&p->lock);
	p->files_copied++;
	p->bytes_copied += bytes;
	pthread_mutex_unlock(&p->lock);
}

void	perf_report(t_perf *p, int thread_count)
{
	double	elapsed;
	double	mb;
	double	speed;

	elapsed = (p->end.tv_sec - p->start.tv_sec)
		+ (p->end.tv_nsec - p->start.tv_nsec) / 1e9;
	mb = p->bytes_copied / (1024.0 * 1024.0);
	speed = 0.0;
	if (elapsed > 0.0)
		speed = mb / elapsed;
	printf("\n====== Performance Report ======\n");
	printf("Threads:       %d\n", thread_count);
	printf("Files copied:  %ld\n", p->files_copied);
	printf("Bytes copied:  %ld (%.2f MB)\n", p->bytes_copied, mb);
	printf("Elapsed time:  %.4f seconds\n", elapsed);
	printf("Throughput:    %.2f MB/s\n", speed);
	printf("================================\n\n");
}
