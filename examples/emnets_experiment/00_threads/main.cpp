#include <stdio.h>
#include <stdlib.h>
#include "thread.h"
#include "xtimer.h"
#include "mutex.h"
#include "msg.h"
#include "clk.h"
#include "board.h"


#define QUEUE_SIZE 8
#define NUM_WORKERS 3
#define TASK_QUEUE_SIZE 10
#define STACKSIZE (THREAD_STACKSIZE_DEFAULT)

typedef struct {
    void (*function)(void *);
    void *arg;
} task_t;
task_t task_queue[TASK_QUEUE_SIZE];
int task_queue_head = 0;
int task_queue_tail = 0;
int task_count = 0;

mutex_t queue_mutex;
kernel_pid_t worker_pids[NUM_WORKERS];
msg_t worker_queues[NUM_WORKERS][QUEUE_SIZE];
char worker_stack[NUM_WORKERS][STACKSIZE];

void task_function1(void *arg) {
    int id = *(int *)arg;
    printf("Worker %d: Executing Task 1\n", id);
    xtimer_sleep(1);
}

void task_function2(void *arg) {
    int id = *(int *)arg;
    printf("Worker %d: Executing Task 2\n", id);
    xtimer_sleep(1);
}

void enqueue_task(void (*function)(void *), void *arg) {
    mutex_lock(&queue_mutex);
    if (task_count < TASK_QUEUE_SIZE) {
        task_queue[task_queue_tail].function = function;
        task_queue[task_queue_tail].arg = arg;
        task_queue_tail = (task_queue_tail + 1) % TASK_QUEUE_SIZE;
        task_count++;
    } else {
        printf("Task queue is full!\n");
    }
    mutex_unlock(&queue_mutex);
}

task_t dequeue_task() {
    task_t task = {NULL, NULL};
    mutex_lock(&queue_mutex);
    if (task_count > 0) {
        task = task_queue[task_queue_head];
        task_queue_head = (task_queue_head + 1) % TASK_QUEUE_SIZE;
        task_count--;
    }
    mutex_unlock(&queue_mutex);
    return task;
}

void *worker_thread(void *arg) {
    (void)arg;
    while (1) {
        msg_t msg;
        msg_receive(&msg);
        task_t task = dequeue_task();
        if (task.function != NULL) {
            task.function(task.arg);
        }
    }
    return NULL;
}

void *scheduler_thread(void *arg) {
    (void)arg;
    int task_arg1 = 1;
    int task_arg2 = 2;
    while (1) {
        enqueue_task(task_function1, &task_arg1);
        xtimer_sleep(2);
        enqueue_task(task_function2, &task_arg2);
        xtimer_sleep(2);
        
        for (int i = 0; i < NUM_WORKERS; ++i) {
            msg_t msg;
            msg.content.value = 1;
            msg_send(&msg, worker_pids[i]);
        }
    }
    return NULL;
}

int main(void) {
    mutex_init(&queue_mutex);

    for (int i = 0; i < NUM_WORKERS; ++i) {
        worker_pids[i] = thread_create(worker_stack[i], sizeof(worker_stack[i]),
                                       THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                                       worker_thread, NULL, "worker");
    }

    char scheduler_stack[STACKSIZE];
    thread_create(scheduler_stack, sizeof(scheduler_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                  scheduler_thread, NULL, "scheduler");

    return 0;
}
