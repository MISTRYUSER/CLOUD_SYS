CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LIBS = -lpthread -lmysqlclient -lcrypt
TARGET = password_auth_client

SRCS = password_auth_client.c client_logic.c tlv.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)