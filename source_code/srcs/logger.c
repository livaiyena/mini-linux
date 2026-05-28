#include "sync.h"

static void	get_timestamp(char *buf, size_t len)
{
	time_t		now;
	struct tm	*tm_info;

	now = time(NULL);
	tm_info = localtime(&now);
	strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

/*
** log_init — Opens the log file with O_DSYNC | O_APPEND.
** O_DSYNC ensures every write() to the log is immediately flushed
** to persistent storage, so crash diagnostics are never lost.
** O_APPEND prevents overwriting previous log entries.
*/
void	log_init(t_sync *sync)
{
	mkdir_recursive("logs");
	sync->log_fd = open(LOG_FILE,
			O_WRONLY | O_CREAT | O_APPEND | O_DSYNC, 0644);
	if (sync->log_fd == -1)
	{
		perror("log open");
		sync->log_fd = STDERR_FILENO;
	}
}

void	log_action(t_sync *sync, const char *action, const char *path)
{
	char	ts[64];
	char	line[MAX_PATH_LEN + 128];
	int		len;

	get_timestamp(ts, sizeof(ts));
	len = snprintf(line, sizeof(line), "[%s] %s: %s\n", ts, action, path);
	pthread_mutex_lock(&sync->log_lock);
	write(sync->log_fd, line, len);
	pthread_mutex_unlock(&sync->log_lock);
}

void	log_copy(t_sync *sync, const char *src, const char *dst, long bytes)
{
	char	ts[64];
	char	line[MAX_PATH_LEN * 2 + 128];
	int		len;

	get_timestamp(ts, sizeof(ts));
	len = snprintf(line, sizeof(line),
			"[%s] COPY [tid:%lu]: %s -> %s (%ld bytes)\n",
			ts, (unsigned long)pthread_self(), src, dst, bytes);
	pthread_mutex_lock(&sync->log_lock);
	write(sync->log_fd, line, len);
	pthread_mutex_unlock(&sync->log_lock);
}

void	log_error(t_sync *sync, const char *msg, const char *path)
{
	char	ts[64];
	char	line[MAX_PATH_LEN + 128];
	int		len;

	get_timestamp(ts, sizeof(ts));
	len = snprintf(line, sizeof(line),
			"[%s] ERROR: %s - %s (errno: %d)\n",
			ts, msg, path, errno);
	pthread_mutex_lock(&sync->log_lock);
	write(sync->log_fd, line, len);
	pthread_mutex_unlock(&sync->log_lock);
}

void	log_close(t_sync *sync)
{
	if (sync->log_fd > 2)
		close(sync->log_fd);
}
