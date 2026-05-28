#include "sync.h"

static void	print_usage(void)
{
	write(2, "Usage: filesync <source_dir> <target_dir> ", 42);
	write(2, "[thread_count]\n", 15);
}

int	parse_args(int argc, char **argv, t_sync *sync)
{
	struct stat	st;

	if (argc < 3 || argc > 4)
	{
		print_usage();
		return (-1);
	}
	strncpy(sync->src_dir, argv[1], MAX_PATH_LEN - 1);
	strncpy(sync->dst_dir, argv[2], MAX_PATH_LEN - 1);
	sync->thread_count = DEFAULT_THREADS;
	if (argc == 4)
		sync->thread_count = atoi(argv[3]);
	if (sync->thread_count < 1)
		sync->thread_count = 1;
	if (stat(sync->src_dir, &st) == -1)
	{
		sync_perror("Source directory", sync->src_dir);
		return (-1);
	}
	if (!S_ISDIR(st.st_mode))
	{
		fprintf(stderr, "filesync: %s: Not a directory\n", sync->src_dir);
		return (-1);
	}
	return (0);
}

int	init_sync(t_sync *sync)
{
	queue_init(&sync->queue);
	perf_init(&sync->perf);
	pthread_mutex_init(&sync->log_lock, NULL);
	log_init(sync);
	sync->threads = NULL;
	return (0);
}

static void	cleanup_sync(t_sync *sync)
{
	queue_destroy(&sync->queue);
	pthread_mutex_destroy(&sync->log_lock);
	pthread_mutex_destroy(&sync->perf.lock);
	log_close(sync);
}

int	main(int argc, char **argv)
{
	t_sync	sync;

	memset(&sync, 0, sizeof(t_sync));
	if (parse_args(argc, argv, &sync) == -1)
		return (1);
	init_sync(&sync);
	log_action(&sync, "START", sync.src_dir);
	perf_start(&sync.perf);
	if (pool_create(&sync) != 0)
		return (cleanup_sync(&sync), 1);
	mkdir_recursive(sync.dst_dir);
	scan_directory(&sync, sync.src_dir, sync.dst_dir);
	queue_shutdown(&sync.queue);
	pool_wait(&sync);
	perf_end(&sync.perf);
	perf_report(&sync.perf, sync.thread_count);
	log_perf(&sync, &sync.perf, sync.thread_count);
	log_action(&sync, "FINISH", sync.dst_dir);
	pool_destroy(&sync);
	cleanup_sync(&sync);
	return (0);
}
