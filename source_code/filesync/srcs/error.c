#include "sync.h"

void	sync_perror(const char *prefix, const char *path)
{
	char	msg[MAX_PATH_LEN + 128];

	snprintf(msg, sizeof(msg), "filesync: %s: %s", prefix, path);
	perror(msg);
}

void	sync_fatal(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int	sync_warn(const char *msg, const char *path)
{
	fprintf(stderr, "Warning: %s: %s\n", msg, path);
	return (-1);
}
