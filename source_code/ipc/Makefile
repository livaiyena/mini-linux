SENSOR      := sensor_reader
LOGGER      := logger_app

SRCS_DIR    := srcs
OBJS_DIR    := objs
INC_DIR     := includes

CC          := cc
CFLAGS      := -Wall -Wextra -Werror
INCLUDES    := -I $(INC_DIR)
LDFLAGS     := -lrt -lpthread

ifdef CROSS
  CC        := $(CROSS)gcc
endif

ifdef DEBUG
  CFLAGS    := -Wall -Wextra -g -fsanitize=address
  LDFLAGS   += -fsanitize=address
endif

COMMON_SRC  := $(SRCS_DIR)/ipc_utils.c
SENSOR_SRC  := $(SRCS_DIR)/sensor_reader.c $(COMMON_SRC)
LOGGER_SRC  := $(SRCS_DIR)/logger_app.c $(COMMON_SRC)

SENSOR_OBJS := $(patsubst $(SRCS_DIR)/%.c, $(OBJS_DIR)/%_sensor.o, $(SENSOR_SRC))
LOGGER_OBJS := $(patsubst $(SRCS_DIR)/%.c, $(OBJS_DIR)/%_logger.o, $(LOGGER_SRC))

CLR_RESET   := \033[0m
CLR_GREEN   := \033[1;32m
CLR_CYAN    := \033[1;36m
CLR_YELLOW  := \033[1;33m
CLR_DIM     := \033[2m



all: $(SENSOR) $(LOGGER)
	@printf "$(CLR_GREEN)✓ Build complete: $(SENSOR) + $(LOGGER)$(CLR_RESET)\n"

$(SENSOR): $(SENSOR_OBJS)
	@printf "$(CLR_CYAN)⟫ Linking $(SENSOR)...$(CLR_RESET)\n"
	@$(CC) $(CFLAGS) $(SENSOR_OBJS) -o $(SENSOR) $(LDFLAGS)

$(LOGGER): $(LOGGER_OBJS)
	@printf "$(CLR_CYAN)⟫ Linking $(LOGGER)...$(CLR_RESET)\n"
	@$(CC) $(CFLAGS) $(LOGGER_OBJS) -o $(LOGGER) $(LDFLAGS)

$(OBJS_DIR)/%_sensor.o: $(SRCS_DIR)/%.c | $(OBJS_DIR)
	@printf "$(CLR_DIM)  CC  %s$(CLR_RESET)\n" "$<"
	@$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJS_DIR)/%_logger.o: $(SRCS_DIR)/%.c | $(OBJS_DIR)
	@printf "$(CLR_DIM)  CC  %s$(CLR_RESET)\n" "$<"
	@$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

-include $(OBJS_DIR)/*.d

static: CFLAGS += -static
static: LDFLAGS += -static
static: re
	@printf "$(CLR_GREEN)✓ Static build complete$(CLR_RESET)\n"

docker:
	@printf "$(CLR_CYAN)⟫ Building ARMv8-A IPC binaries via Docker...$(CLR_RESET)\n"
	@docker build -t ipc-aarch64 .
	@docker create --name ipc-extract ipc-aarch64 > /dev/null 2>&1
	@docker cp ipc-extract:/build/sensor_reader ./sensor_reader-aarch64
	@docker cp ipc-extract:/build/logger_app ./logger_app-aarch64
	@docker rm ipc-extract > /dev/null 2>&1
	@printf "$(CLR_GREEN)✓ Binaries extracted:$(CLR_RESET)\n"
	@file sensor_reader-aarch64
	@file logger_app-aarch64

clean:
	@rm -rf $(OBJS_DIR)
	@printf "$(CLR_YELLOW)✗ Cleaned object files$(CLR_RESET)\n"

fclean: clean
	@rm -f $(SENSOR) $(LOGGER) sensor_reader-aarch64 logger_app-aarch64
	@printf "$(CLR_YELLOW)✗ Cleaned binaries$(CLR_RESET)\n"

re: fclean all

help:
	@printf "$(CLR_CYAN) IPC Telemetry Makefile $(CLR_RESET)\n"
	@printf "  $(CLR_GREEN)make$(CLR_RESET)                          Build both binaries\n"
	@printf "  $(CLR_GREEN)make CROSS=aarch64-linux-gnu-$(CLR_RESET)  Cross-compile for ARMv8-A\n"
	@printf "  $(CLR_GREEN)make static$(CLR_RESET)                    Static binaries\n"
	@printf "  $(CLR_GREEN)make docker$(CLR_RESET)                    Cross-compile via Docker\n"
	@printf "  $(CLR_GREEN)make DEBUG=1$(CLR_RESET)                   Debug build (ASan)\n"
	@printf "  $(CLR_GREEN)make clean$(CLR_RESET)                     Remove object files\n"
	@printf "  $(CLR_GREEN)make fclean$(CLR_RESET)                    Remove all build artifacts\n"
	@printf "  $(CLR_GREEN)make re$(CLR_RESET)                        Full rebuild\n"

.PHONY: all clean fclean re static docker help
