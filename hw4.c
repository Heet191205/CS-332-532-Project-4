#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#define THREAD_COUNT_CHILD 10 // Child threads
#define READ_COUNT 150        // Numbers per child thread

int pipefd[2]; // Pipe for parent-child communication
pthread_mutex_t pipe_lock; 

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

int main() {
    // Initialize the pipe
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    // Populate the pipe with test data (1500 random numbers)
    int test_data[THREAD_COUNT_CHILD * READ_COUNT];
    for (int i = 0; i < THREAD_COUNT_CHILD * READ_COUNT; i++) {
        test_data[i] = rand() % 1001; // Random numbers between 0 and 1000
    }

    // Write test data to the pipe
    pthread_mutex_init(&pipe_lock, NULL);
    write(pipefd[1], test_data, sizeof(test_data));
    close(pipefd[1]); // Close write end of the pipe

    // Run the child process function
    printf("Running child process...\n");
    child_process();

    // Cleanup
    pthread_mutex_destroy(&pipe_lock);
    printf("Child process completed. Check output.txt for the result.\n");

    return 0;
}