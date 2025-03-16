NAME = minishell

LIBFT_DIR = libft
GNL_DIR = get_next_line
LIBFT = $(LIBFT_DIR)/libft.a
GNL = $(GNL_DIR)/gnl.a
CC = cc
CFLAGS = -Wall -Wextra -Werror -I$(LIBFT_DIR) -I$(GNL_DIR) 
LIBS = $(LIBFT) $(GNL) 
INCLUDES = -I/opt/homebrew/opt/readline/include
LDFLAGS = -L/opt/homebrew/opt/readline/lib -lreadline -lhistory

SRCS =	main.c \
		utils.c \
		signals.c \
		tokenizer.c \
		token_utils.c \
		token_types.c \
		lst_functions.c \
		commands.c \
		executor.c \
		builtins.c \
		memory.c \
		env_expanions.c \
		redirections.c

OBJS = $(SRCS:.c=.o)

all: $(LIBFT) $(GNL) $(NAME)

$(GNL):
	@make -C $(GNL_DIR)

$(LIBFT):
	@make -C $(LIBFT_DIR) all bonus

$(NAME): $(OBJS) $(LIBFT)
	$(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)
	@if [ -f $(LIBFT) ]; then make -C $(LIBFT_DIR) clean; fi
	@if [ -f $(GNL) ]; then make -C $(GNL_DIR) clean; fi
fclean: clean
	rm -f $(NAME)
	@if [ -f $(LIBFT) ]; then make -C $(LIBFT_DIR) fclean; fi
	@if [ -f $(GNL) ]; then make -C $(GNL_DIR) fclean; fi

re: fclean all

.PHONY: all clean fclean re
