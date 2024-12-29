# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tgmelin <tgmelin@student.42heilbronn.de    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/11/26 20:26:50 by tgmelin           #+#    #+#              #
#    Updated: 2024/12/29 19:04:23 by tgmelin          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	:= webserv

#################################
#			Config				#
#################################

CC = c++

#################################
#			Files				#
#################################

SRCS := 						\
src/main.cpp

LIBMLX	:= ./lib/MLX42
LIBFT 	:= ./lib/libft
HEADERS	:= -I ./include -I $(LIBMLX)/include -I $(LIBFT)
LIBS	:= $(LIBMLX)/build/libmlx42.a -ldl -lglfw -pthread -lm -Ofast -ffast-math -march=native -mtune=native -funroll-loops -framework Cocoa -framework OpenGL -framework IOKit ./lib/libft.a
OBJS	:= ${SRCS:.c=.o}

#################################
#			Flags				#
#################################

DEBUG_FLAGS		= -g -fsanitize=address -fsanitize=undefined -O0 
RELEASE_FLAGS	= -flto -fomit-frame-pointer -Ofast -fvisibility=hidden -march=native #-fstack-protector-strong
CFLAGS			= -Wall -Wextra -Werror -fcolor-diagnostics# -Wunreachable-code

#################################
#			Internals			#
#################################

CLR_RMV		:= \033[0m
RED			:= \033[1;31m
GREEN		:= \033[1;32m
YELLOW		:= \033[1;33m
BLUE		:= \033[1;34m
CYAN 		:= \033[1;36m
BOLD_BLUE	:= \033[0;34m

#################################
#			Rules				#
#################################

all: CFLAGS += $(RELEASE_FLAGS)
all: deps $(NAME)

bonus: CFLAGS += -D BONUS=1
bonus: all

debug: CFLAGS += $(DEBUG_FLAGS)
debug: deps $(NAME)

debugbonus: CFLAGS += -D BONUS=1
debugbonus: debug

re: fclean all

redebug: fclean debug

rebonus: fclean bonus

redebugbonus: fclean debugbonus

feature: CFLAGS += -D BUG=1
feature: bonus

#Compilations:
%.o: %.c
	@printf "Compiling: $(CYAN)%-20s$(CLR_RMV) " "$(notdir $<)" && \
	compileout=$$( $(CC) $(CFLAGS) -o $@ -c $< $(HEADERS) 2>&1 ); \
	if [ $$? -eq 0 ]; then \
		echo "$(GREEN)[OK]$(CLR_RMV)"; \
	else \
		echo "$(RED)[KO]$(CLR_RMV)"; \
		echo "$$compileout"; \
		exit 1; \
	fi

$(NAME): $(OBJS)
	@printf "Building:  $(CYAN)%-20s$(CLR_RMV) " $(NAME) && \
	compileout=$$( $(CC) $(OBJS) $(LIBS) $(HEADERS) $(CFLAGS) -o $(NAME) 2>&1 ); \
	if [ $$? -eq 0 ]; then \
		echo "$(GREEN)[OK]$(CLR_RMV)"; \
	else \
		echo "$(RED)[KO]$(CLR_RMV)"; \
		echo "$$compileout"; \
		exit 1; \
	fi
	

#Dependencies
deps:
	@printf "Updating submodules " && \
	compileout=$$( git submodule update --init --recursive --remote --merge 2>&1 ); \
	if [ $$? -eq 0 ]; then \
		echo "$(GREEN)[OK]$(CLR_RMV)"; \
	else \
		echo "$(RED)[KO]$(CLR_RMV)"; \
		echo "$$compileout"; \
		exit 1;\
	fi
	@printf "Building:  $(CYAN)%-20s$(CLR_RMV) " libft && \
	compileout=$$( make -C $(LIBFT) > /dev/null ); \
	if [ $$? -eq 0 ]; then \
		echo "$(GREEN)[OK]$(CLR_RMV)"; \
	else \
		echo "$(RED)[KO]$(CLR_RMV)"; \
		echo "$$compileout"; \
		exit 1; \
	fi
	@printf "Building:  $(CYAN)%-20s$(CLR_RMV) " MLX42 && \
	compileout=$$( cmake -DCMAKE_C_FLAGS="-Ofast -fomit-frame-pointer -march=native -funroll-loops" $(LIBMLX) -B $(LIBMLX)/build && make -C $(LIBMLX)/build -j4 ); \
	if [ $$? -eq 0 ]; then \
		echo "$(GREEN)[OK]$(CLR_RMV)"; \
	else \
		echo "$(RED)[KO]$(CLR_RMV)"; \
		echo "$$compileout"; \
		exit 1; \
	fi

#Cleaning
depsclean:
	@make -C $(LIBFT) clean > /dev/null
	@rm -rf $(LIBMLX)/build

depsfclean:
	@make -C $(LIBFT) fclean > /dev/null
	@rm -rf $(LIBMLX)/build

clean:
	@rm -rf $(OBJS)
	@rm -rf $(LIBMLX)/build

fclean: clean
	@rm -rf $(NAME)

.PHONY: all bonus debug debugbonus re rebonus redebugbonus $(NAME) deps depsclean depsfclean clean fclean