#include "sync.h"

static long	do_copy_loop(int fd_src, int fd_dst)
{
	char	buf[BLOCK_SIZE];
	ssize_t	bytes_read;
	long	total;

	total = 0;
	bytes_read = read(fd_src, buf, BLOCK_SIZE);
	while (bytes_read > 0)
	{
		if (write(fd_dst, buf, bytes_read) != bytes_read)
		{
			perror("write");
			return (-1);
		}
		total += bytes_read;
		bytes_read = read(fd_src, buf, BLOCK_SIZE);
	}
	if (bytes_read < 0)
	{
		perror("read");
		return (-1);
	}
	return (total);
}

int	copy_file_blocks(const char *src, const char *dst)
{
	int		fds[2];
	long	total;

	if (open_src_dst(src, dst, fds) == -1)
		return (-1);
	total = do_copy_loop(fds[0], fds[1]);
	if (total >= 0)
	{
		if (fdatasync(fds[1]) == -1)
		{
			perror("fdatasync");
			total = -1;
		}
	}
	close(fds[0]);
	close(fds[1]);
	return (total);
}

int	copy_file_safe(const char *src, const char *dst)
{
	char	tmp_path[MAX_PATH_LEN];
	int		fds[2];
	long	total;

	if (snprintf(tmp_path, MAX_PATH_LEN, "%s%s",
			dst, TEMP_SUFFIX) >= MAX_PATH_LEN)
	{
		write(2, "Path too long for temp file\n", 27);
		return (-1);
	}
	if (open_src_dst(src, tmp_path, fds) == -1)
		return (-1);
	total = do_copy_loop(fds[0], fds[1]);
	if (total >= 0)
	{
		if (fdatasync(fds[1]) == -1)
		{
			perror("fdatasync");
			total = -1;
		}
	}
	close(fds[0]);
	close(fds[1]);
	if (total < 0)
	{
		unlink(tmp_path);
		return (-1);
	}
	copy_permissions(src, tmp_path);
	if (rename(tmp_path, dst) == -1)
	{
		perror("rename");
		unlink(tmp_path);
		return (-1);
	}
	ensure_parent_dir(dst);
	return (total);
}

int	ensure_parent_dir(const char *path)
{
	char	parent[MAX_PATH_LEN];
	char	*last_slash;

	strncpy(parent, path, MAX_PATH_LEN - 1);
	parent[MAX_PATH_LEN - 1] = '\0';
	last_slash = strrchr(parent, '/');
	if (!last_slash)
		return (0);
	*last_slash = '\0';
	if (mkdir_recursive(parent) == -1)
		return (-1);
	return (sync_directory(parent));
}
