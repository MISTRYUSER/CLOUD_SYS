# 编译器和编译选项
CC = gcc
CFLAGS = -D_GNU_SOURCE -std=gnu11 -Wall -Wextra -O2 -g

# 项目目录结构
SRC_DIR := .
OBJ_DIR := build
BIN_DIR := bin

# 查找所有 .c 文件
SOURCES := $(shell find $(SRC_DIR) -name '*.c')
# 生成对应的 .o 文件路径
OBJECTS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SOURCES))
# 可执行文件名
TARGET := main

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lmysqlclient -lcrypt

# 编译 .c 文件为 .o 文件
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理目标
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean