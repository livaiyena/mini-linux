#include "sync.h"

int	build_path(char *buf, const char *dir, const char *name)
{
	int	len;

	len = snprintf(buf, MAX_PATH_LEN, "%s/%s", dir, name);
	if (len >= MAX_PATH_LEN)
	{
		write(2, "Path too long\n", 14);
		return (-1);
	}
	return (0);
}

int	needs_copy(const char *src, const char *dst)
{
	struct stat	st_s;
	struct stat	st_d;

	if (stat(src, &st_s) == -1)
		return (0);
	if (stat(dst, &st_d) == -1)
		return (1);
	if (st_s.st_size != st_d.st_size)
		return (1);
	if (st_s.st_mtime > st_d.st_mtime)
		return (1);
	return (0);
}

int	entry_is_valid(struct dirent *entry)
{
	if (strcmp(entry->d_name, ".") == 0)
		return (0);
	if (strcmp(entry->d_name, "..") == 0)
		return (0);
	return (1);
}
