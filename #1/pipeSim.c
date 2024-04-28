#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{

    char *myargs1[3];
    myargs1[0] = strdup ("ls");
    myargs1[1] = strdup ("-w -m1 -- '-S' ");
    myargs1[2] = strdup ("output.txt");

    int fd[2];  // man to grep
    pipe(fd); 

    int fd2[2];  // grep to file
    pipe(fd2);

    int pid1 = getpid();
    printf("I'm SHELL process, with PID: %d - Main command is: man %s | grep %s > %s\n", pid1, myargs1[0], myargs1[1], myargs1[2]);


    int rc = fork();
    if (rc < 0)
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0) // Child process for 'man' command
    {

        int pid2 = getpid();
        printf("I'm MAN process, with PID: %d - My command is: man %s\n", pid2, myargs1[0]);

        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
        close(fd[1]);

        close(fd2[0]);
        close(fd2[1]);

        char *myargs2[3];
        myargs2[0] = strdup("man"); // program: "man"
        myargs2[1] = strdup("ls");  // argument
        myargs2[2] = NULL;          // marks end of array
        execvp(myargs2[0], myargs2); // runs the man

        
    }
    else // parent process
    {
        
        close(fd[1]);
        int rc2 = fork();
        if (rc2 < 0)
        {
            fprintf(stderr, "fork failed\n");
            exit(1);
        }
        else if (rc2 == 0) // Child process for 'grep' command
        {
            int pid3 = getpid();
            printf("I'm GREP process, with PID: %d - My command is: grep %s\n", pid3, myargs1[1]);

            close(fd[1]);
            dup2(fd[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
            close(fd[0]);

            close(fd2[0]);
            int f = open(myargs1[2], O_CREAT | O_APPEND | O_WRONLY, 0666);
            dup2(f, STDOUT_FILENO);

            char *myargs3[] = {"grep", "-w", "-m1", "--", "-S", NULL};
            execvp(myargs3[0], myargs3);

        }
        else
        {
            close(fd[0]);
            close(fd[1]);
            close(fd2[1]);
            close(fd2[0]);

            wait(NULL);
            wait(NULL);
        }
    }

    int pid4 = getpid();
    printf("I'm SHELL process, with PID:%d - execution is completed, you can find the results in %s\n", pid4, myargs1[2]);
    return 0;
}


