#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 200
#define NUM_ITERATIONS 500

//Shiyun Zhou
//CPSC-351-Assignment-4

// Define a mutex for thread synchronization
pthread_mutex_t stack_mutex = PTHREAD_MUTEX_INITIALIZER;

// Linked list node
typedef int value_t;
typedef struct Node {
    value_t data;
    struct Node *next;
} StackNode;

// Stack function declarations
void push(value_t v, StackNode **top);
value_t pop(StackNode **top);
int is_empty(StackNode *top);
void* testStack(void* arg);

// Shared stack pointer
StackNode *top = NULL;

int main(void) {
    pthread_t threads[NUM_THREADS];

    // Create multiple threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, testStack, NULL) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy the mutex
    pthread_mutex_destroy(&stack_mutex);

    return 0;
}

// Push function with thread safety
void push(value_t v, StackNode **top) {
    pthread_mutex_lock(&stack_mutex);

    StackNode *new_node = malloc(sizeof(StackNode));
    if (!new_node) {
        perror("Malloc failed");
        exit(1);
    }
    new_node->data = v;
    new_node->next = *top;
    *top = new_node;

    pthread_mutex_unlock(&stack_mutex);
}

// Pop function with thread safety
value_t pop(StackNode **top) {
    pthread_mutex_lock(&stack_mutex);

    if (is_empty(*top)) {
        pthread_mutex_unlock(&stack_mutex);
        return 0;  // Return a default value if stack is empty
    }

    value_t data = (*top)->data;
    StackNode *temp = *top;
    *top = (*top)->next;

    free(temp);

    pthread_mutex_unlock(&stack_mutex);
    return data;
}

// Check if stack is empty
int is_empty(StackNode *top) {
    return top == NULL;
}

// Test function for threads
void* testStack(void* arg) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        push(i, &top);
        push(i + 1, &top);
        push(i + 2, &top);
        pop(&top);
        pop(&top);
        pop(&top);
    }
    return NULL;
}
