#ifndef SYNC_H
# define SYNC_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <dirent.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <fcntl.h>
# include <pthread.h>
# include <time.h>
# include <errno.h>

/*
** O_DSYNC portability: some toolchains/kernels may not define O_DSYNC.
** Fall back to O_SYNC which is a superset (flushes data + metadata).
*/
# ifndef O_DSYNC
#  define O_DSYNC O_SYNC
# endif

# define BLOCK_SIZE      65536
# define MAX_PATH_LEN    4096
# define DEFAULT_THREADS 4
# define LOG_FILE        "logs/sync.log"
# define TEMP_SUFFIX     ".sync_tmp"

typedef struct s_task
{
	char			src[MAX_PATH_LEN];
	char			dst[MAX_PATH_LEN];
	size_t			size;
	struct s_task	*next;
}	t_task;

typedef struct s_queue
{
	t_task			*head;
	t_task			*tail;
	int				count;
	int				shutdown;
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
}	t_queue;

typedef struct s_perf
{
	struct timespec	start;
	struct timespec	end;
	long			files_copied;
	long			bytes_copied;
	pthread_mutex_t	lock;
}	t_perf;

typedef struct s_sync
{
	char			src_dir[MAX_PATH_LEN];
	char			dst_dir[MAX_PATH_LEN];
	t_queue			queue;
	pthread_t		*threads;
	int				thread_count;
	int				log_fd;
	pthread_mutex_t	log_lock;
	t_perf			perf;
}	t_sync;

/* main.c */
int		parse_args(int argc, char **argv, t_sync *sync);
int		init_sync(t_sync *sync);

/* scanner.c */
int		scan_directory(t_sync *sync, const char *src, const char *dst);
int		compare_files(const char *src, const char *dst);

/* scanner_utils.c */
int		build_path(char *buf, const char *dir, const char *name);
int		needs_copy(const char *src, const char *dst);
int		entry_is_valid(struct dirent *entry);

/* task_queue.c */
void	queue_init(t_queue *q);
int		queue_push(t_queue *q, const char *src, const char *dst, size_t sz);
t_task	*queue_pop(t_queue *q);
void	queue_shutdown(t_queue *q);
void	queue_destroy(t_queue *q);

/* thread_pool.c */
int		pool_create(t_sync *sync);
void	*worker_routine(void *arg);
void	pool_wait(t_sync *sync);
void	pool_destroy(t_sync *sync);

/* file_copy.c */
int		copy_file_blocks(const char *src, const char *dst);
int		copy_file_safe(const char *src, const char *dst);
int		ensure_parent_dir(const char *path);

/* file_copy_utils.c */
int		mkdir_recursive(const char *path);
int		copy_permissions(const char *src, const char *dst);
int		open_src_dst(const char *src, const char *dst, int fds[2]);
int		sync_directory(const char *dirpath);
int		fsync_path(int fd);

/* logger.c */
void	log_init(t_sync *sync);
void	log_action(t_sync *sync, const char *action, const char *path);
void	log_copy(t_sync *sync, const char *src, const char *dst, long bytes);
void	log_skip(t_sync *sync, const char *path);
void	log_perf(t_sync *sync, t_perf *p, int thread_count);
void	log_error(t_sync *sync, const char *msg, const char *path);
void	log_close(t_sync *sync);

/* perf.c */
void	perf_init(t_perf *p);
void	perf_start(t_perf *p);
void	perf_end(t_perf *p);
void	perf_add(t_perf *p, long bytes);
void	perf_report(t_perf *p, int thread_count);

/* error.c */
void	sync_perror(const char *prefix, const char *path);
void	sync_fatal(const char *msg);
int		sync_warn(const char *msg, const char *path);

#endif
