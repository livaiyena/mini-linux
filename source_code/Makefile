NAME        := filesync

SRCS_DIR    := srcs
OBJS_DIR    := objs
INC_DIR     := includes

CC          := cc
CFLAGS      := -Wall -Wextra -Werror -pthread
INCLUDES    := -I $(INC_DIR)
LDFLAGS     := -pthread

ifdef CROSS
  CC        := $(CROSS)gcc
endif

ifdef DEBUG
  CFLAGS    := -Wall -Wextra -g -fsanitize=address -pthread
  LDFLAGS   += -fsanitize=address
endif

SRCS        := $(wildcard $(SRCS_DIR)/*.c)
OBJS        := $(patsubst $(SRCS_DIR)/%.c, $(OBJS_DIR)/%.o, $(SRCS))
DEPS        := $(OBJS:.o=.d)
HEADERS     := $(wildcard $(INC_DIR)/*.h)

CLR_RESET   := \033[0m
CLR_GREEN   := \033[1;32m
CLR_CYAN    := \033[1;36m
CLR_YELLOW  := \033[1;33m
CLR_RED     := \033[1;31m
CLR_DIM     := \033[2m



all: $(NAME)
	@printf "$(CLR_GREEN)✓ Build complete: $(NAME)$(CLR_RESET)\n"

$(NAME): $(OBJS)
	@printf "$(CLR_CYAN)⟫ Linking $(NAME)...$(CLR_RESET)\n"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c | $(OBJS_DIR)
	@printf "$(CLR_DIM)  CC  %s$(CLR_RESET)\n" "$<"
	@$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

-include $(DEPS)

static: CFLAGS += -static
static: LDFLAGS += -static
static: re
	@printf "$(CLR_GREEN)✓ Static build complete: $(NAME)$(CLR_RESET)\n"

docker:
	@printf "$(CLR_CYAN)⟫ Building ARMv8-A binary via Docker...$(CLR_RESET)\n"
	@docker build -t filesync-aarch64 .
	@docker create --name filesync-extract filesync-aarch64 > /dev/null 2>&1
	@docker cp filesync-extract:/build/filesync ./filesync-aarch64
	@docker rm filesync-extract > /dev/null 2>&1
	@printf "$(CLR_GREEN)✓ Binary extracted: filesync-aarch64$(CLR_RESET)\n"
	@file filesync-aarch64

clean:
	@rm -rf $(OBJS_DIR)
	@printf "$(CLR_YELLOW)✗ Cleaned object files$(CLR_RESET)\n"

fclean: clean
	@rm -f $(NAME) filesync-aarch64
	@printf "$(CLR_YELLOW)✗ Cleaned binary$(CLR_RESET)\n"

re: fclean all

info:
	@printf "$(CLR_CYAN) Build Info $(CLR_RESET)\n"
	@printf "  CC       = $(CC)\n"
	@printf "  CFLAGS   = $(CFLAGS)\n"
	@printf "  LDFLAGS  = $(LDFLAGS)\n"
	@printf "  SOURCES  = $(words $(SRCS)) files\n"
	@printf "  HEADERS  = $(words $(HEADERS)) files\n"
	@printf "$(CLR_DIM)"
	@printf "  %s\n" $(SRCS)
	@printf "$(CLR_RESET)"

help:
	@printf "$(CLR_CYAN) filesync Makefile $(CLR_RESET)\n"
	@printf "  $(CLR_GREEN)make$(CLR_RESET)                          Build (native)\n"
	@printf "  $(CLR_GREEN)make CROSS=aarch64-linux-gnu-$(CLR_RESET)  Cross-compile for ARMv8-A\n"
	@printf "  $(CLR_GREEN)make static$(CLR_RESET)                    Static binary (no shared libs)\n"
	@printf "  $(CLR_GREEN)make docker$(CLR_RESET)                    Cross-compile via Docker\n"
	@printf "  $(CLR_GREEN)make DEBUG=1$(CLR_RESET)                   Debug build (ASan + symbols)\n"
	@printf "  $(CLR_GREEN)make info$(CLR_RESET)                      Show build configuration\n"
	@printf "  $(CLR_GREEN)make clean$(CLR_RESET)                     Remove object files\n"
	@printf "  $(CLR_GREEN)make fclean$(CLR_RESET)                    Remove all build artifacts\n"
	@printf "  $(CLR_GREEN)make re$(CLR_RESET)                        Full rebuild\n"

.PHONY: all clean fclean re static docker info help
