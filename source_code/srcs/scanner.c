#include "sync.h"

static int	handle_subdir(t_sync *sync, const char *s, const char *d)
{
	struct stat	st;

	if (stat(d, &st) == -1)
	{
		if (mkdir_recursive(d) == -1)
		{
			sync_perror("mkdir", d);
			return (-1);
		}
		log_action(sync, "MKDIR", d);
	}
	return (scan_directory(sync, s, d));
}

static int	handle_file(t_sync *sync, const char *s, const char *d)
{
	struct stat	st;

	if (needs_copy(s, d))
	{
		if (stat(s, &st) == 0)
			queue_push(&sync->queue, s, d, st.st_size);
		else
			sync_perror("stat", s);
	}
	else
		log_skip(sync, s);
	return (0);
}

static int	process_entry(t_sync *sync, struct dirent *ent,
		const char *src, const char *dst)
{
	char		src_path[MAX_PATH_LEN];
	char		dst_path[MAX_PATH_LEN];

	if (!entry_is_valid(ent))
		return (0);
	build_path(src_path, src, ent->d_name);
	build_path(dst_path, dst, ent->d_name);
	if (ent->d_type == DT_DIR)
		return (handle_subdir(sync, src_path, dst_path));
	if (ent->d_type == DT_REG)
		return (handle_file(sync, src_path, dst_path));
	return (0);
}

int	scan_directory(t_sync *sync, const char *src, const char *dst)
{
	DIR				*dir;
	struct dirent	*ent;

	dir = opendir(src);
	if (!dir)
	{
		sync_perror("opendir", src);
		return (-1);
	}
	ent = readdir(dir);
	while (ent)
	{
		process_entry(sync, ent, src, dst);
		ent = readdir(dir);
	}
	closedir(dir);
	log_action(sync, "SCANNED", src);
	return (0);
}

int	compare_files(const char *src, const char *dst)
{
	struct stat	st_s;
	struct stat	st_d;

	if (stat(src, &st_s) == -1)
		return (-1);
	if (stat(dst, &st_d) == -1)
		return (1);
	if (st_s.st_size != st_d.st_size)
		return (1);
	if (st_s.st_mtime > st_d.st_mtime)
		return (1);
	return (0);
}
