#include "thread_pool.h"

int task_queue_init(task_queue_t *queue) {
    bzero(queue, sizeof(task_queue_t));
    return 0;
}

// 尾插
int en_queue(task_queue_t *queue, int netfd) {
    node_t *node = (node_t *)malloc(sizeof(node_t));
    node->next = NULL;
    node->netfd = netfd;
    if (queue->size == 0) {
        queue->front = node;
        queue->rear = node;
    } else {
        queue->rear->next = node;
        queue->rear = node;
    }
    ++queue->size;
    return 0;
}

// 头删
int de_queue(task_queue_t *queue) {
    node_t *node = queue->front;
    if (queue->size == 0) return -1;
    else if (queue->size == 1) {
        queue->rear = NULL;
        queue->front = NULL;
    } else {
        queue->front = queue->front->next;
    }
    free(node);
    --queue->size;
    return 0;
}

int print_queue(task_queue_t *queue) {
    node_t *node = queue->front;
    while (node != NULL) {
        printf("%d", node->netfd);
        if (node->next != NULL) {
            printf("-->");
        }
        node = node->next;
    }
    printf("\n");
    return 0;
}

