#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>



sem_t mutex;
sem_t fanA_queue, fanB_queue;
sem_t captainSem;
pthread_barrier_t barrier;
pthread_barrier_t barrier2;
pthread_barrier_t barrier3;



int numA, numB;
int carCounter = 0;
int fanA=0;
int fanB=0;
bool isCaptain = false;

void spot(pthread_t tid, char team) {
    // Logic for finding a spot
    printf("Thread ID: %lu, Team: %c, I have found a spot in a car\n", (long)tid, team);
}


void drive(pthread_t tid, char team) {
    
    // Logic for driving
    printf("Thread ID: %ld, Team: %c, I am the captain and driving the car with ID %d\n", (long)tid, team, carCounter);
    
}

void* fanA_thread(void* arg) {
    pthread_t tid = pthread_self();
    
    sem_wait(&mutex);
    fanA+=1;
    printf("Thread ID: %ld, Team: %c, I am looking for a car\n", (long)tid,'A');

    if (fanA == 4) {
        for (int i = 0; i < 4; ++i) {
            sem_post(&fanA_queue);
        }   
        fanA = 0;
        isCaptain = true;
    } else if (fanA == 2 && fanB >= 2) {
        for (int i = 0; i < 2; ++i) {
            sem_post(&fanA_queue);
        } 
        for (int i = 0; i < 2; ++i) {
            sem_post(&fanB_queue);
        } 
        fanB -= 2;
        fanA = 0;
        isCaptain = true;
    } else {
        sem_post(&mutex);
    }

    sem_wait(&fanA_queue);

    pthread_barrier_wait(&barrier);

    spot(pthread_self(), 'A');

    pthread_barrier_wait(&barrier2);  // Wait for all threads to reach this point

    // Logic for captain
    sem_wait(&captainSem);
    if (isCaptain) {
        drive(pthread_self(), 'A');
        carCounter+=1;
        isCaptain = false;
        sem_post(&mutex);
    }
    sem_post(&captainSem);
    pthread_barrier_wait(&barrier3);
    return NULL;

}

void* fanB_thread(void* arg) {
    pthread_t tid = pthread_self();
    
    sem_wait(&mutex);
    fanB+=1;
    printf("Thread ID: %ld, Team: %c, I am looking for a car\n", (long)tid,'B');

    if (fanB == 4) {
        for (int i = 0; i < 4; ++i) {
            sem_post(&fanB_queue);
        }   
        fanB = 0;
        isCaptain = true;
    } else if (fanB == 2 && fanA >= 2) {
        for (int i = 0; i < 2; ++i) {
            sem_post(&fanB_queue);
        } 
        for (int i = 0; i < 2; ++i) {
            sem_post(&fanA_queue);
        } 
        fanA -= 2;
        fanB = 0;
        isCaptain = true;
    } else {
        sem_post(&mutex);
    }

    sem_wait(&fanB_queue);

    pthread_barrier_wait(&barrier);

    spot(pthread_self(), 'B');

    pthread_barrier_wait(&barrier2);  // Wait for all threads to reach this point

    // Logic for captain
    sem_wait(&captainSem);
    if (isCaptain) {
        drive(pthread_self(), 'B');
        carCounter+=1;
        isCaptain = false;
        sem_post(&mutex);
    }
    sem_post(&captainSem);
    pthread_barrier_wait(&barrier3); 
    return NULL;
}





int main(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    }

    numA = atoi(argv[1]);
    numB = atoi(argv[2]);

    // Check for validity of input arguments
    if (numA % 2 != 0 || numB % 2 != 0 || (numA + numB) % 4 != 0) {
        printf("Invalid input arguments\n");
        return 1;
    }

    // Initialize semaphores
    sem_init(&mutex, 0, 1);
    sem_init(&fanA_queue, 0, 0);
    sem_init(&fanB_queue, 0, 0);
    sem_init(&captainSem, 0, 1);
    pthread_barrier_init(&barrier, NULL, 4);  // Initialize the barrier with a count of 4
    pthread_barrier_init(&barrier2, NULL, 4);
    pthread_barrier_init(&barrier3, NULL, 4);

    

    pthread_t* threadsA = malloc(numA * sizeof(pthread_t));
    pthread_t* threadsB = malloc(numB * sizeof(pthread_t));

    // Create fan A threads
    for (int i = 0; i < numA; i++) {
        pthread_create(&threadsA[i], NULL, fanA_thread, NULL);
    }

    // Create fan B threads
    for (int i = 0; i < numB; i++) {
        pthread_create(&threadsB[i], NULL, fanB_thread, NULL);
    }

    // Wait for threads to finish (This part can be adjusted based on your termination criteria)
    for (int i = 0; i < numA; i++) {
        pthread_join(threadsA[i], NULL);
    }

    for (int i = 0; i < numB; i++) {
        pthread_join(threadsB[i], NULL);
    }

    printf("The main terminates\n");
    // Destroy semaphores
    sem_destroy(&mutex);
    sem_destroy(&fanA_queue);
    sem_destroy(&fanB_queue);
    sem_destroy(&captainSem);
    free(threadsA);
    free(threadsB);
    pthread_barrier_destroy(&barrier);  // Destroy the barrier when it's no longer needed
    pthread_barrier_destroy(&barrier2);
    pthread_barrier_destroy(&barrier3);
    return 0;
}
