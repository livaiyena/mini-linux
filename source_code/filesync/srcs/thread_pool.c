#include "sync.h"

/*
** process_task — Uses copy_file_safe() for atomic, power-resilient
** file copying. The safe copy function handles:
**   1. Writing to a temp file with O_DSYNC
**   2. fdatasync() to flush data to physical storage
**   3. Atomic rename() to the final destination
**   4. Parent directory sync for directory entry durability
** Permissions are applied to the temp file before rename.
*/
static void	process_task(t_sync *sync, t_task *task)
{
	int	ret;

	ensure_parent_dir(task->dst);
	ret = copy_file_safe(task->src, task->dst);
	if (ret >= 0)
	{
		perf_add(&sync->perf, ret);
		log_copy(sync, task->src, task->dst, ret);
	}
	else
		log_error(sync, "Copy failed", task->src);
}

void	*worker_routine(void *arg)
{
	t_sync	*sync;
	t_task	*task;

	sync = (t_sync *)arg;
	while (1)
	{
		task = queue_pop(&sync->queue);
		if (!task)
			break ;
		process_task(sync, task);
		free(task);
	}
	return (NULL);
}

int	pool_create(t_sync *sync)
{
	int	i;

	sync->threads = malloc(sizeof(pthread_t) * sync->thread_count);
	if (!sync->threads)
	{
		sync_perror("malloc", "thread pool");
		return (-1);
	}
	i = 0;
	while (i < sync->thread_count)
	{
		if (pthread_create(&sync->threads[i], NULL, worker_routine, sync))
		{
			sync_perror("pthread_create", "worker");
			return (-1);
		}
		i++;
	}
	log_action(sync, "POOL_CREATED", "threads started");
	return (0);
}

void	pool_wait(t_sync *sync)
{
	int	i;

	i = 0;
	while (i < sync->thread_count)
	{
		pthread_join(sync->threads[i], NULL);
		i++;
	}
}

void	pool_destroy(t_sync *sync)
{
	if (sync->threads)
	{
		free(sync->threads);
		sync->threads = NULL;
	}
}
