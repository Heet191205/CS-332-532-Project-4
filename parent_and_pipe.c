#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define RANDOM_NUMS_COUNT 500 
#define THREAD_COUNT_PARENT 3 // Parent threads

int pipefd[2];               // Pipe for parent-child communication
pthread_mutex_t pipe_lock;   

// This function generate random number using the parent threads
void* generate_random_numbers(void* arg) {
    int numbers[RANDOM_NUMS_COUNT];
    for (int i = 0; i < RANDOM_NUMS_COUNT; i++) {
        numbers[i] = rand() % 1001; // Random numbers between 0 and 1000
    }

    pthread_mutex_lock(&pipe_lock);
    write(pipefd[1], numbers, sizeof(numbers)); 
    pthread_mutex_unlock(&pipe_lock);

    return NULL;
}

// This is the main function for the parent process
void parent_process() {
    pthread_t parent_threads[THREAD_COUNT_PARENT];

    // This create the threads 
    for (int i = 0; i < THREAD_COUNT_PARENT; i++) {
        if (pthread_create(&parent_threads[i], NULL, generate_random_numbers, NULL) != 0) {
            perror("Parent thread creation failed");
            exit(1);
        }
    }

    // This will wait untill the parent threads to complete the task
    for (int i = 0; i < THREAD_COUNT_PARENT; i++) {
        pthread_join(parent_threads[i], NULL);
    }

    close(pipefd[1]); 
}


int main() {

    // Creating a pipe
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    pthread_mutex_init(&pipe_lock, NULL); 

    printf("Testing parent process\n");
    parent_process();

    pthread_mutex_destroy(&pipe_lock); 
    printf("Parent process completed successfully\n");

    return 0;
}
