# 设置编译器
CC := gcc
CFLAGS := -D_GUN_SOURCE -std=gnu11 -Wall -Wextra -O2 -g
LDLIBS := -lpthread -lmysqlclient -lcrypt

# 所有源文件（你可以手动添加，也可以自动发现）
SRCS := 
OBJS := $(SRCS:.c=.o)

# 最终目标程序名
TARGET := ftp_server

# 默认目标
.PHONY: all clean rebuild
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# 编译每个 .c 为 .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(TARGET) $(OBJS)

rebuild: clean all
