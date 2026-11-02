#include "shared_memory_lab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

int create_segment(void) {
    return shmget(IPC_PRIVATE, sizeof(struct shared_block), 0600 | IPC_CREAT);
}

void cleanup_segment(int segment_id, struct shared_block *block) {
    if (shmdt(block) == -1) {
        perror("shmdt");
    }
    if (shmctl(segment_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
}

static void wait_for_ready(const struct shared_block *block) {
    while (!__atomic_load_n(&block->ready, __ATOMIC_ACQUIRE)) {
        usleep(1000);
    }
}

static void finish_child(struct shared_block *block) {
    int failed = fflush(stdout) == EOF;

    if (shmdt(block) == -1) {
        perror("shmdt");
        failed = 1;
    }
    /* Do not return to main and accidentally run the remaining tasks. */
    _exit(failed ? EXIT_FAILURE : EXIT_SUCCESS);
}

int task_fast_exchange(void) {
    int segment_id;
    pid_t child_pid;
    int status;
    struct shared_block *block;
    const char source[] = "shared memory is fast";
    const size_t chunk_size = sizeof(block->message) - 1;
    size_t offset;

    segment_id = create_segment();
    if (segment_id == -1) {
        perror("shmget");
        return 1;
    }

    block = shmat(segment_id, NULL, 0);
    if (block == (void *)-1) {
        perror("shmat");
        if (shmctl(segment_id, IPC_RMID, NULL) == -1) {
            perror("shmctl");
        }
        return 1;
    }
    memset(block, 0, sizeof(*block));

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        cleanup_segment(segment_id, block);
        return 1;
    }

    if (child_pid == 0) {
        char received[sizeof(source)] = {0};

        /* The 20-character message needs two transfers through 16 bytes. */
        for (offset = 0; offset < sizeof(source) - 1; offset += chunk_size) {
            size_t bytes = sizeof(source) - 1 - offset;
            if (bytes > chunk_size) {
                bytes = chunk_size;
            }
            wait_for_ready(block);
            memcpy(received + offset, block->message, bytes);
            /* Acknowledge the read before the parent reuses the buffer. */
            __atomic_store_n(&block->ready, 0, __ATOMIC_RELEASE);
        }
        printf("child fast read: %s\n", received);
        finish_child(block);
    }

    for (offset = 0; offset < sizeof(source) - 1; offset += chunk_size) {
        snprintf(block->message, sizeof(block->message), "%.*s",
                 (int)chunk_size, source + offset);
        __atomic_store_n(&block->ready, 1, __ATOMIC_RELEASE);
        while (__atomic_load_n(&block->ready, __ATOMIC_ACQUIRE)) {
            usleep(1000);
        }
    }

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("waitpid");
        cleanup_segment(segment_id, block);
        return 1;
    }

    printf("parent fast write: done\n");
    fflush(stdout);
    cleanup_segment(segment_id, block);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}

int task_latest_value_limit(void) {
    int segment_id;
    pid_t child_pid;
    int status;
    struct shared_block *block;

    segment_id = create_segment();
    if (segment_id == -1) {
        perror("shmget");
        return 1;
    }

    block = shmat(segment_id, NULL, 0);
    if (block == (void *)-1) {
        perror("shmat");
        if (shmctl(segment_id, IPC_RMID, NULL) == -1) {
            perror("shmctl");
        }
        return 1;
    }
    memset(block, 0, sizeof(*block));

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        cleanup_segment(segment_id, block);
        return 1;
    }

    if (child_pid == 0) {
        wait_for_ready(block);
        printf("child latest view: %s\n", block->message);
        finish_child(block);
    }

    snprintf(block->message, sizeof(block->message), "%s", "first update");
    snprintf(block->message, sizeof(block->message), "%s", "latest-only");
    __atomic_store_n(&block->ready, 1, __ATOMIC_RELEASE);

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("waitpid");
        cleanup_segment(segment_id, block);
        return 1;
    }

    printf("limit observed: no history in shared memory\n");
    fflush(stdout);
    cleanup_segment(segment_id, block);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}

int task_capacity_limit(void) {
    int segment_id;
    pid_t child_pid;
    int status;
    struct shared_block *block;
    const char *source = "abcdefghijklmnopqrstuvwxyz";

    segment_id = create_segment();
    if (segment_id == -1) {
        perror("shmget");
        return 1;
    }

    block = shmat(segment_id, NULL, 0);
    if (block == (void *)-1) {
        perror("shmat");
        if (shmctl(segment_id, IPC_RMID, NULL) == -1) {
            perror("shmctl");
        }
        return 1;
    }
    memset(block, 0, sizeof(*block));

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        cleanup_segment(segment_id, block);
        return 1;
    }

    if (child_pid == 0) {
        wait_for_ready(block);
        printf("child capacity bytes: %zu\n", strlen(block->message));
        finish_child(block);
    }

    /* Leave one byte for the terminating NUL; explicitly bound the format. */
    snprintf(block->message, sizeof(block->message), "%.*s",
             (int)sizeof(block->message) - 1, source);
    __atomic_store_n(&block->ready, 1, __ATOMIC_RELEASE);

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("waitpid");
        cleanup_segment(segment_id, block);
        return 1;
    }

    printf("parent source bytes: %zu\n", strlen(source));
    fflush(stdout);
    cleanup_segment(segment_id, block);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}

int main(void) {
    if (task_fast_exchange() != 0) {
        return 1;
    }

    if (task_latest_value_limit() != 0) {
        return 1;
    }

    if (task_capacity_limit() != 0) {
        return 1;
    }

    printf("all tasks done\n");
    return 0;
}
