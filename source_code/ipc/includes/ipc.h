#ifndef IPC_H
# define IPC_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <time.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <semaphore.h>
# include <stdint.h>

# define SHM_NAME       "/telemetry_shm"
# define SEM_WRITER     "/telemetry_sem_w"
# define SEM_READER     "/telemetry_sem_r"
# define LOG_PATH       "/tmp/telemetry.log"

# define RING_SIZE      64
# define SENSOR_HZ      100
# define LOG_FLUSH_INT  10

typedef struct s_telemetry
{
	uint64_t	seq;
	int32_t		temperature;
	int32_t		pressure;
	int32_t		humidity;
	int32_t		accel_x;
	int32_t		accel_y;
	int32_t		accel_z;
	uint64_t	timestamp_ns;
	uint8_t		_pad[16];
}	t_telemetry;

typedef struct s_shm_ring
{
	volatile uint64_t	write_idx;
	volatile uint64_t	read_idx;
	volatile int		shutdown;
	uint8_t				_pad[44];
	t_telemetry			slots[RING_SIZE];
}	t_shm_ring;


int		sensor_init_shm(t_shm_ring **ring, sem_t **sem_w, sem_t **sem_r);
void	sensor_generate(t_telemetry *sample, uint64_t seq);
void	sensor_loop(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r);
void	sensor_cleanup(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r);

int		logger_attach_shm(t_shm_ring **ring, sem_t **sem_w, sem_t **sem_r);
int		logger_open_file(int *log_fd);
void	logger_loop(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r, int fd);
void	logger_cleanup(t_shm_ring *ring, sem_t *sem_w, sem_t *sem_r, int fd);

uint64_t	get_timestamp_ns(void);
void		print_telemetry(const t_telemetry *t);
void		format_log_line(char *buf, size_t len, const t_telemetry *t);

#endif
