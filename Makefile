TARGET = main
SRC_DIR = src
INC_DIR = inc
BIN_DIR = bin
OBJ_DIR = obj

CC = gcc
CFLAGS = -Wall -Wextra -I$(INC_DIR) -O3 -fsanitize=address -fno-omit-frame-pointer -DDEBUG -march=native -flto -g

SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard *.c)
OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)

.PHONY: all run clean

all: $(BIN_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $<"

$(BIN_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@
	@echo "Built $@"

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Removed build artifacts."

run: all
	./$(BIN_DIR)/$(TARGET)
