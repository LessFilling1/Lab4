#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
        
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    else 
    /* Child */
    if (pid == 0){
        exit(0);
    }

    /* Parent
     * Gives you time to observe the zombie using ps(1) ... */
    sleep(10);

    /* ... and after that, parent wait(2)s its child's
     * exit status, and prints a relevant message. */
    int status;
    pid = waitpid(-1, NULL, WNOHANG);
    if (WIFEXITED(status))
        fprintf(stderr, "\n [%d] Process %d exited with status %d.\n",
                (int) getpid(), pid, WEXITSTATUS(status));

    return 0;
}