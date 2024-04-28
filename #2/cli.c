#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#define MAX_LINE_LENGTH 256
#define MAX_SIZE 100
#define MAX_JOBS 100

pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t completedThreadsMutex = PTHREAD_MUTEX_INITIALIZER;
int completedThreads = 0;


struct BackgroundJob {
    pid_t pid;
    int isRunning; // 1 if running, 0 if terminated
};

struct ListenerThreadArgs {
    int pipefd;
    pthread_t tid;  //pipe that the thread will read from
};

void *commandListenerThread(void *args) {
    struct ListenerThreadArgs *listenerArgs = (struct ListenerThreadArgs *)args;

    // Acquire the mutex to ensure thread safe printing
    pthread_mutex_lock(&printMutex);

    fprintf(stdout, "---- %lu\n", (unsigned long)pthread_self());
    fflush(stdout);

    pthread_mutex_unlock(&printMutex);

    // Read and print strings from the pipe
    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(listenerArgs->pipefd, buffer, sizeof(buffer))) > 0) {

        pthread_mutex_lock(&printMutex);
        fprintf(stdout, "%s", buffer);
        fflush(stdout);
        pthread_mutex_unlock(&printMutex);
        
    }
    
    pthread_mutex_lock(&printMutex);

    fprintf(stdout, "---- %lu\n", (unsigned long)pthread_self());
    fflush(stdout);

    pthread_mutex_unlock(&printMutex);
    // Release the mutex

    // Increment the completedThreads counter
    pthread_mutex_lock(&completedThreadsMutex);
    completedThreads++;
    pthread_mutex_unlock(&completedThreadsMutex);

    pthread_exit(NULL);
}


/*
Function that will execute the command that is written in command.txt
It takes the parameters from command.txt 
Handeles the command according to it being background command or not 
Handeles the command according to it has output redirectioning or not 
if it doesn't have that it cretaes a thread in shell process that prints to the console

*/
void executeCommand( char *filename, char *redirection, char *background,char *options, char *input,char *command,struct BackgroundJob *backgroundJobs, int *numJobs,int *maxJobs, pthread_t *listenerThreads, int *numThreads) {

    if(strcmp(command, "wait") == 0){

        for (int i = 0; i < *numJobs; ++i) {
            if (backgroundJobs[i].isRunning) {
                waitpid(backgroundJobs[i].pid, NULL, 0);
                backgroundJobs[i].isRunning = 0;
            }
        }

        // Wait for the listener threads to finish
        for (int i = 0; i < *numThreads; ++i) {
            if(*numJobs==completedThreads)
            {
                pthread_join(listenerThreads[i], NULL);
            }
            
        }

    } 
    
    
    pid_t pid;
    int pipefd[2];

    //If no output redirectioning create a pipe between shell and command process

    if (strcmp(redirection, ">") != 0) {
        if (pipe(pipefd) == -1) {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid = fork();

    if (pid == -1) {

        perror("Error forking process");
        exit(EXIT_FAILURE);

    } else if (pid == 0) {
        // Child process

        if (strcmp(redirection, "-") != 0) {
            

            if (strcmp(redirection , ">")==0) {
                // Output redirection
               
               
                close(STDOUT_FILENO);

                int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("Error opening file for redirection");
                    exit(EXIT_FAILURE);
                }

                dup2(fd, STDOUT_FILENO);
                

            } else if (strcmp(redirection , "<")==0) {

                //redirect the output to  the pipe

                close(pipefd[0]);

                dup2(pipefd[1], STDOUT_FILENO);

                close(pipefd[1]);
                
                // Input redirection
                close(STDIN_FILENO);

                int fd = open(filename, O_RDONLY);
                if (fd < 0) {
                    perror("Error opening file for redirection");
                    exit(EXIT_FAILURE);
                }

                dup2(fd, STDIN_FILENO);
                
            } 
        }
        else{

            //redirect the output to  the pipe

            close(pipefd[0]);

            dup2(pipefd[1], STDOUT_FILENO);

            close(pipefd[1]);

        }

        char *args[5];
        int i = 0;
        

        args[i++] = strdup(command);

        if (options[0]!= '\0') {

            args[i++] = strdup(options);
            args[i++] = strdup("--");
        }

        if (input[0] != '\0') {
            args[i++] = strdup(input);
        }

        
        args[i] = NULL;  
        execvp(args[0], args);

        // If execvp fails
        perror("Error executing command");
        exit(EXIT_FAILURE);

    } else {

        // Parent process

       
        if (strcmp(redirection, ">") != 0) {

            // Close the write end of the pipe in the parent process
            close(pipefd[1]);


            // Create a new thread for the listener
            pthread_t listenerThread;
            struct ListenerThreadArgs listenerArgs;
            listenerArgs.pipefd = pipefd[0];  //The pipe that thread will read result of the execution of the command
            listenerArgs.tid = listenerThread;

            if (pthread_create(&listenerThread, NULL, commandListenerThread, (void *)&listenerArgs) != 0) {
                perror("Error creating listener thread");
                exit(EXIT_FAILURE);
            }

             // Store the listener thread ID
            if (*numThreads == MAX_JOBS) {
                perror("Too many listener threads");
                exit(EXIT_FAILURE);
            }
            listenerThreads[*numThreads] = listenerArgs.tid;
            (*numThreads)++;
        }




        if (strcmp(background, "n") == 0) {
            waitpid(pid, NULL, 0);
        } else {

           if (*numJobs == *maxJobs) {
                // Expand the background job array
                *maxJobs *= 2;
                backgroundJobs = realloc(backgroundJobs, (*maxJobs) * sizeof(struct BackgroundJob));
                if (backgroundJobs == NULL) {
                    perror("Error reallocating memory");
                    exit(EXIT_FAILURE);
                }
            }
            backgroundJobs[*numJobs].pid = pid;
            backgroundJobs[*numJobs].isRunning = 1;
            (*numJobs)++;
        }

        


    }    
      
}





void waitForBackgroundJobs(struct BackgroundJob *backgroundJobs, int numJobs, pthread_t *listenerThreads, int numThreads) {
   
   //Wait for background jobs to finish
    for (int i = 0; i < numJobs; ++i) {
        if (backgroundJobs[i].isRunning) {
            waitpid(backgroundJobs[i].pid, NULL, 0);
            backgroundJobs[i].isRunning = 0;
        }
    }

    // Wait for the listener threads to finish
    for (int i = 0; i < numThreads; ++i) {
        if(numJobs==completedThreads)
        {
            pthread_join(listenerThreads[i], NULL);
        }
    }
}


/*
After reading each command from command.txt fle parse each command to its components
*/

void parseCommand( FILE *outputFile,char *line, char *filename, char *redirection, char *background,char *options, char *input,char *command) {
    
    
    char *token=NULL;
    char *saveptr=NULL;

    int count=0;
    int count1=0;
   

    
    fprintf(outputFile,"----------\n");

    
    token = strtok_r((char *)line, " ", &saveptr);

    if (token != NULL) {
        fprintf(outputFile, "Command : %s\n", token);
        strcat(command,token);
    }

    
    while ((token = strtok_r(NULL, " ", &saveptr)) != NULL) {

        if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0) {
            
            count1++;
            strcpy(redirection,token);

        } else if (strcmp(token, "&") == 0) {
            
            count++;
            strcpy(background,"y");

        } else if (strstr(token,"-")!=NULL) {
            
            strcpy(options,token);

        } else if (count1 !=0 && strstr(token,".txt")!=NULL)  {
            
            strcpy(filename,token);
        }
        else{

            strcpy(input,token);

        }
    }


    if(count1==0){

        strcpy(redirection,"-");
    }
    if(count==0){

        strcpy(background,"n");
    }
    

    fprintf(outputFile,"Input: %s\n", input);
    fprintf(outputFile,"Options: %s\n", options);
    fprintf(outputFile,"Redirection : %s\n", redirection);
    fprintf(outputFile,"Background Job : %s\n",background);
    fprintf(outputFile,"----------\n");
    fflush(outputFile);

}

int main() {

    FILE *inputFile = fopen("commands.txt", "r");
      if (inputFile == NULL) {
        perror("Error opening files");
        exit(EXIT_FAILURE);
    }

    
    FILE *outputFile = fopen("parse.txt","w");
    if (outputFile == NULL) {
        perror("Error opening parse.txt for writing");
        return 1;
    }
    
  

    // Initialize background job array
    struct BackgroundJob backgroundJobs[MAX_JOBS];
    int numJobs = 0;
    int maxJobs=MAX_JOBS;
    //// Initialize ListenerThreads array
    pthread_t listenerThreads[MAX_JOBS];
    int numThreads = 0;

    char line[MAX_LINE_LENGTH];
    

    
    while (fgets(line, sizeof(line), inputFile) != NULL) {
        line[strcspn(line, "\n")] = '\0';


        char filename[MAX_SIZE]= "";
        char redirection[MAX_SIZE]= "";
        char background[MAX_SIZE]= "";
        char options[MAX_SIZE]= "";
        char input[MAX_SIZE]= "";
        char command[MAX_SIZE]= "";

        parseCommand(outputFile,line,&filename, &redirection, &background, &options, &input,&command);

        executeCommand(filename, redirection, background, options, input,command,backgroundJobs, &numJobs,&maxJobs,listenerThreads, &numThreads);
        
        
    }

    waitForBackgroundJobs(backgroundJobs, numJobs, listenerThreads, numThreads);


    fclose(inputFile);
    fclose(outputFile);
    

    return 0;
}


