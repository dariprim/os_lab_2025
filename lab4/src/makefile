CC = gcc
LAB3_PATH = /workspaces/os_lab_2025/lab3/src
CFLAGS = -I. -I$(LAB3_PATH)
LDFLAGS = -pthread

all: zombie process_memory parallel_sum

zombie: zombie.c
	$(CC) -o zombie zombie.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

parallel_sum: parallel_sum.c sum_utils.c $(LAB3_PATH)/utils.c
	$(CC) -o parallel_sum parallel_sum.c sum_utils.c $(LAB3_PATH)/utils.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f zombie process_memory parallel_sum