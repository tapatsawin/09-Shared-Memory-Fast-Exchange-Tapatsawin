CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -Iinclude
BIN_DIR = bin

all: $(BIN_DIR)/shared_memory_lab

check: all
	./scripts/check.sh

grade: all
	./scripts/grade.sh

$(BIN_DIR)/shared_memory_lab: src/shared_memory_lab.c include/shared_memory_lab.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BIN_DIR)/shared_memory_lab

.PHONY: all check grade clean