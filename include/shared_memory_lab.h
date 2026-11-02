#ifndef SHARED_MEMORY_LAB_H
#define SHARED_MEMORY_LAB_H

#include <sys/types.h>

#define SHM_MESSAGE_CAP 16

struct shared_block {
    int ready;
    char message[SHM_MESSAGE_CAP];
};

int create_segment(void);
void cleanup_segment(int segment_id, struct shared_block *block);
int task_fast_exchange(void);
int task_latest_value_limit(void);
int task_capacity_limit(void);

#endif