#include "ipc.h"

static volatile int	g_running = 1;

static void	handle_signal(int sig)
{
	(void)sig;
	g_running = 0;
}

int	sensor_init_shm(t_shm_ring **ring, sem_t **sem_w, sem_t **sem_r)
{
	int	fd;

	shm_unlink(SHM_NAME);
	fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	if (fd == -1)
	{
		perror("shm_open");
		return (-1);
	}
	if (ftruncate(fd, sizeof(t_shm_ring)) == -1)
	{
		perror("ftruncate");
		close(fd);
		return (-1);
	}
	*ring = mmap(NULL, sizeof(t_shm_ring),
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (*ring == MAP_FAILED)
	{
		perror("mmap");
		return (-1);
	}
	memset(*ring, 0, sizeof(t_shm_ring));
	sem_unlink(SEM_WRITER);
	sem_unlink(SEM_READER);
	*sem_w = sem_open(SEM_WRITER, O_CREAT, 0666, RING_SIZE);
	*sem_r = sem_open(SEM_READER, O_CREAT, 0666, 0);
	if (*sem_w == SEM_FAILED || *sem_r == SEM_FAILED)
	{
		perror("sem_open");
		return (-1);
	}
	return (0);
}

void	sensor_generate(t_telemetry *sample, uint64_t seq)
{
	sample->seq = seq;
	sample->temperature = 2000 + (rand() % 1000);
	sample->pressure = 100000 + (rand() % 5000);
	sample->humidity = 4000 + (rand() % 2000);
	sample->accel_x = -1000 + (rand() % 2001);
	sample->accel_y = -1000 + (rand() % 2001);
	sample->accel_z = 9000 + (rand() % 2000);
	sample->timestamp_ns = get_timestamp_ns();
}

void	sensor_loop(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r)
{
	t_telemetry		sample;
	uint64_t		seq;
	struct timespec	delay;
	uint64_t		idx;

	seq = 0;
	delay.tv_sec = 0;
	delay.tv_nsec = 1000000000L / SENSOR_HZ;
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	printf("[SENSOR] Started — writing at %d Hz\n", SENSOR_HZ);
	while (g_running)
	{
		sem_wait(sem_w);
		if (!g_running)
			break ;
		sensor_generate(&sample, seq);
		idx = seq % RING_SIZE;
		ring->slots[idx] = sample;
		__sync_synchronize();
		ring->write_idx = seq + 1;
		sem_post(sem_r);
		if (seq % 100 == 0)
			print_telemetry(&sample);
		seq++;
		nanosleep(&delay, NULL);
	}
	ring->shutdown = 1;
	__sync_synchronize();
	sem_post(sem_r);
	printf("[SENSOR] Stopped — %lu samples written\n",
		(unsigned long)seq);
}

void	sensor_cleanup(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r)
{
	munmap(ring, sizeof(t_shm_ring));
	sem_close(sem_w);
	sem_close(sem_r);
	shm_unlink(SHM_NAME);
	sem_unlink(SEM_WRITER);
	sem_unlink(SEM_READER);
}

int	main(void)
{
	t_shm_ring	*ring;
	sem_t		*sem_w;
	sem_t		*sem_r;

	srand((unsigned int)time(NULL));
	if (sensor_init_shm(&ring, &sem_w, &sem_r) == -1)
		return (1);
	sensor_loop(ring, sem_w, sem_r);
	sensor_cleanup(ring, sem_w, sem_r);
	return (0);
}
