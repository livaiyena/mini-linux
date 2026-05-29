#include "ipc.h"

static volatile int	g_running = 1;

static void	handle_signal(int sig)
{
	(void)sig;
	g_running = 0;
}

int	logger_attach_shm(t_shm_ring **ring, sem_t **sem_w, sem_t **sem_r)
{
	int	fd;

	fd = shm_open(SHM_NAME, O_RDWR, 0666);
	if (fd == -1)
	{
		perror("shm_open (logger)");
		return (-1);
	}
	*ring = mmap(NULL, sizeof(t_shm_ring),
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (*ring == MAP_FAILED)
	{
		perror("mmap (logger)");
		return (-1);
	}
	*sem_w = sem_open(SEM_WRITER, 0);
	*sem_r = sem_open(SEM_READER, 0);
	if (*sem_w == SEM_FAILED || *sem_r == SEM_FAILED)
	{
		perror("sem_open (logger)");
		return (-1);
	}
	return (0);
}

int	logger_open_file(int *log_fd)
{
	*log_fd = open(LOG_PATH,
			O_WRONLY | O_CREAT | O_APPEND | O_DSYNC, 0644);
	if (*log_fd == -1)
	{
		perror("open log file");
		return (-1);
	}
	return (0);
}

void	logger_loop(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r, int fd)
{
	t_telemetry	sample;
	uint64_t	read_seq;
	uint64_t	idx;
	char		line[256];
	int			len;
	uint64_t	count;

	read_seq = 0;
	count = 0;
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	printf("[LOGGER] Attached — waiting for telemetry data...\n");
	while (g_running)
	{
		sem_wait(sem_r);
		if (ring->shutdown && ring->write_idx <= read_seq)
			break ;
		if (!g_running)
			break ;
		__sync_synchronize();
		idx = read_seq % RING_SIZE;
		sample = ring->slots[idx];
		ring->read_idx = read_seq + 1;
		sem_post(sem_w);
		format_log_line(line, sizeof(line), &sample);
		len = strlen(line);
		write(fd, line, len);
		count++;
		if (count % LOG_FLUSH_INT == 0)
			fdatasync(fd);
		if (count % 100 == 0)
		{
			printf("[LOGGER] %lu samples logged | last seq: %lu\n",
				(unsigned long)count,
				(unsigned long)sample.seq);
		}
		read_seq++;
	}
	fdatasync(fd);
	printf("[LOGGER] Stopped — %lu samples saved to %s\n",
		(unsigned long)count, LOG_PATH);
}

void	logger_cleanup(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r, int fd)
{
	if (fd > 2)
		close(fd);
	munmap(ring, sizeof(t_shm_ring));
	sem_close(sem_w);
	sem_close(sem_r);
}

int	main(void)
{
	t_shm_ring	*ring;
	sem_t		*sem_w;
	sem_t		*sem_r;
	int			log_fd;

	if (logger_attach_shm(&ring, &sem_w, &sem_r) == -1)
		return (1);
	if (logger_open_file(&log_fd) == -1)
	{
		munmap(ring, sizeof(t_shm_ring));
		return (1);
	}
	logger_loop(ring, sem_w, sem_r, log_fd);
	logger_cleanup(ring, sem_w, sem_r, log_fd);
	return (0);
}
