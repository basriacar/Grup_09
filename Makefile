# Basit zamanlayıcı simülasyonu için Makefile
# İleride FreeRTOS entegrasyonu için genişleteceğiz.

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
LDFLAGS = 

SRC_DIR = src
SRCS    = $(SRC_DIR)/main.c \
          $(SRC_DIR)/scheduler.c \
          $(SRC_DIR)/tasks.c

OBJS    = $(SRCS:.c=.o)

TARGET  = freertos_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) giris.txt

clean:
	rm -f $(OBJS) $(TARGET)
