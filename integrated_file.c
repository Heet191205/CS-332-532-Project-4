#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#define RANDOM_NUMS_COUNT 500 // Numbers per parent thread
#define THREAD_COUNT_PARENT 3 // Parent threads
#define THREAD_COUNT_CHILD 10 // Child threads
#define READ_COUNT 150        // Numbers per child thread

int pipefd[2];               // Pipe for parent-child communication
pthread_mutex_t pipe_lock;   

// This function generate random number using the parent threads
void* generate_random_numbers(void* arg) {
    int numbers[RANDOM_NUMS_COUNT];
    for (int i = 0; i < RANDOM_NUMS_COUNT; i++) {
        numbers[i] = rand() % 1001; // Random numbers between 0 and 1000
    }

    // Lock the pipe before writing
    pthread_mutex_lock(&pipe_lock);
    write(pipefd[1], numbers, sizeof(numbers)); // Write to pipe
    pthread_mutex_unlock(&pipe_lock);

    return NULL;
}

// This is the main function for the parent process
void parent_process() {
    pthread_t parent_threads[THREAD_COUNT_PARENT];

    // This will wait untill the parent threads to complete the task
    for (int i = 0; i < THREAD_COUNT_PARENT; i++) {
        if (pthread_create(&parent_threads[i], NULL, generate_random_numbers, NULL) != 0) {
            perror("Parent thread creation failed");
            exit(1);
        }
    }

    // Wait for parent threads to complete
    for (int i = 0; i < THREAD_COUNT_PARENT; i++) {
        pthread_join(parent_threads[i], NULL);
    }

    close(pipefd[1]); // Close write end of pipe
}

// Function read numbers and calculate sum with the use of threads
void* calculate_sum(void* arg) {
    int numbers[READ_COUNT];
    read(pipefd[0], numbers, sizeof(numbers)); // Read from pipe

    // Calculate sum of numbers
    int sum = 0;
    for (int i = 0; i < READ_COUNT; i++) {
        sum += numbers[i];
    }

    int* result = malloc(sizeof(int)); // Store result 
    *result = sum;
    return result;
}

// Main function for the child process
void child_process() {
    pthread_t child_threads[THREAD_COUNT_CHILD];
    int total_sum = 0;

    // Create child threads to calculate sums
    for (int i = 0; i < THREAD_COUNT_CHILD; i++) {
        if (pthread_create(&child_threads[i], NULL, calculate_sum, NULL) != 0) {
            perror("Child thread creation failed");
            exit(1);
        }
    }

    // Collect results from child threads
    for (int i = 0; i < THREAD_COUNT_CHILD; i++) {
        int* result;
        pthread_join(child_threads[i], (void**)&result);
        total_sum += *result;
        free(result);
    }

    // Calculate average and write to file
    float average = total_sum / (float)(THREAD_COUNT_CHILD * READ_COUNT);
    FILE* file = fopen("output.txt", "w");
    if (file) {
        fprintf(file, "Average: %.2f\n", average);
        fclose(file);
    } else {
        perror("File write failed");
    }

    close(pipefd[0]); // Close read end of pipe
}

// Main function for the execution
int main() {
    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    pthread_mutex_init(&pipe_lock, NULL); // Initializing the mutex

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    // If pid is 0 then run the child process else run the parent process
    if (pid == 0) { 
        close(pipefd[1]); 
        printf("Running child process...\n");
        child_process();
        printf("Child process completed. Check output.txt for the result.\n");
    } else { 
        close(pipefd[0]); 
        printf("Running parent process...\n");
        parent_process();
        printf("Parent process completed.\n");

        wait(NULL); // Waiting for child process to finish
    }

    pthread_mutex_destroy(&pipe_lock); // Destroy the mutex at the end
    return 0;
}