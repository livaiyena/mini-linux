#include "sync.h"

void	queue_init(t_queue *q)
{
	q->head = NULL;
	q->tail = NULL;
	q->count = 0;
	q->shutdown = 0;
	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->cond, NULL);
}

int	queue_push(t_queue *q, const char *src, const char *dst, size_t sz)
{
	t_task	*task;

	task = malloc(sizeof(t_task));
	if (!task)
		return (-1);
	strncpy(task->src, src, MAX_PATH_LEN - 1);
	task->src[MAX_PATH_LEN - 1] = '\0';
	strncpy(task->dst, dst, MAX_PATH_LEN - 1);
	task->dst[MAX_PATH_LEN - 1] = '\0';
	task->size = sz;
	task->next = NULL;
	pthread_mutex_lock(&q->lock);
	if (q->tail)
		q->tail->next = task;
	else
		q->head = task;
	q->tail = task;
	q->count++;
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->lock);
	return (0);
}

t_task	*queue_pop(t_queue *q)
{
	t_task	*task;

	pthread_mutex_lock(&q->lock);
	while (!q->head && !q->shutdown)
		pthread_cond_wait(&q->cond, &q->lock);
	if (q->shutdown && !q->head)
	{
		pthread_mutex_unlock(&q->lock);
		return (NULL);
	}
	task = q->head;
	q->head = task->next;
	if (!q->head)
		q->tail = NULL;
	q->count--;
	pthread_mutex_unlock(&q->lock);
	return (task);
}

void	queue_shutdown(t_queue *q)
{
	pthread_mutex_lock(&q->lock);
	q->shutdown = 1;
	pthread_cond_broadcast(&q->cond);
	pthread_mutex_unlock(&q->lock);
}

void	queue_destroy(t_queue *q)
{
	t_task	*tmp;

	while (q->head)
	{
		tmp = q->head;
		q->head = tmp->next;
		free(tmp);
	}
	pthread_mutex_destroy(&q->lock);
	pthread_cond_destroy(&q->cond);
}
