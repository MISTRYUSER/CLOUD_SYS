# 编译器与选项
CC := gcc
CFLAGS := -Wall -Wextra -O2 -g
LDLIBS := -lpthread -lmysqlclient -lcrypto
# 所有源文件
SRC := tlv.c main.c path.c func_cd.c func_ls.c func_db.c func_dir.c func_file.c tcp.c epoll.c thread_pool.c worker.c task_queue.c solve_command.c long_solve_command.c password_auth_server.c
OBJ := $(SRC:.c=.o)

# 最终生成的程序名
TARGET := my_program

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

# 编译每个 .c 文件为 .o 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(TARGET) $(OBJ)
