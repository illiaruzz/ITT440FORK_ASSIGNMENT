#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define num_children 3
#define buffer_size 100

volatile sig_atomic_t interrupted = 0;

void interrupt_handler(int sig) {
    interrupted = 1;
}

int main(void) {
    int pipes[num_children][2];
    char message[buffer_size];
    pid_t pid;

    signal(SIGINT, interrupt_handler);

    for (int i = 0; i < num_children; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe failed");
            return 1;
        }

        pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }

        if (pid == 0) {
            char buffer[buffer_size] = {0};
            signal(SIGINT, SIG_IGN);
            close(pipes[i][1]);

            for (int j = 0; j < i; j++) {
                close(pipes[j][1]);
            }

            read(pipes[i][0], buffer, sizeof(buffer));
            close(pipes[i][0]);

            printf("[Child %d (PID %d)] Received: %s\n", i + 1, getpid(), buffer);

            exit(0);
        }
    }

    for (int i = 0; i < num_children; i++) {
        close(pipes[i][0]);
    }

    printf("Enter a message : ");
    fflush(stdout);

    if (fgets(message, sizeof(message), stdin) != NULL) {
        message[strcspn(message, "\r\n")] = '\0';

        if (interrupted) {
            printf("\nInterrupt Detected because Ctrl+C was pressed.\n");
            return 0;
        }

        for (int i = 0; i < num_children; i++) {
            write(pipes[i][1], message, strlen(message) + 1);
            close(pipes[i][1]);
        }
    }

    if (interrupted) {
        printf("\nInterrupt Detected because Ctrl+C was pressed.\n");
        return 0;
    }

    for (int i = 0; i < num_children; i++) {
        wait(NULL);
    }

    printf("\nAll child processes have completed execution.\n");

    return 0;
}
