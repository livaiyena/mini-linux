#include "sync.h"

static int	try_mkdir(const char *path)
{
	struct stat	st;

	if (stat(path, &st) == 0)
	{
		if (S_ISDIR(st.st_mode))
			return (0);
		return (-1);
	}
	if (mkdir(path, 0755) == -1)
	{
		perror("mkdir");
		return (-1);
	}
	return (0);
}

int	mkdir_recursive(const char *path)
{
	char	tmp[MAX_PATH_LEN];
	char	*p;

	strncpy(tmp, path, MAX_PATH_LEN - 1);
	tmp[MAX_PATH_LEN - 1] = '\0';
	p = tmp + 1;
	while (*p)
	{
		if (*p == '/')
		{
			*p = '\0';
			try_mkdir(tmp);
			*p = '/';
		}
		p++;
	}
	return (try_mkdir(tmp));
}

int	copy_permissions(const char *src, const char *dst)
{
	struct stat	st;

	if (stat(src, &st) == -1)
		return (-1);
	if (chmod(dst, st.st_mode) == -1)
	{
		perror("chmod");
		return (-1);
	}
	return (0);
}

int	open_src_dst(const char *src, const char *dst, int fds[2])
{
	fds[0] = open(src, O_RDONLY);
	if (fds[0] == -1)
	{
		perror("open src");
		return (-1);
	}
	fds[1] = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_DSYNC, 0644);
	if (fds[1] == -1)
	{
		perror("open dst");
		close(fds[0]);
		return (-1);
	}
	return (0);
}

int	sync_directory(const char *dirpath)
{
	int	fd;

	fd = open(dirpath, O_RDONLY | O_DIRECTORY);
	if (fd == -1)
	{
		perror("open dir for sync");
		return (-1);
	}
	if (fsync(fd) == -1)
	{
		perror("fsync dir");
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

int	fsync_path(int fd)
{
	if (fsync(fd) == -1)
	{
		perror("fsync");
		return (-1);
	}
	return (0);
}
