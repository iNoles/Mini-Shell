#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void shell_loop();
char* read_input();
char** parse_input(char* input);
int execute_command(char** args);
int launch_process(char** args);
void sigint_handler(int sig);

int main() {
    signal(SIGINT, sigint_handler); // Handle Ctrl+C
    shell_loop(); // Start the shell loop
    return 0;
}

void shell_loop() {
    char* input;
    char** args;
    int status = 1;

    do {
        printf("> "); // Display the prompt
        input = read_input(); // Read user input
        args = parse_input(input); // Parse input into arguments
        status = execute_command(args); // Execute the command

        free(input); // Free the input buffer
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]); // Free each token
        }
        free(args); // Free the array of tokens
    } while (status); // Continue until exit command is issued
}

char* read_input() {
    char* input = NULL;
    size_t bufsize = 0;
    getline(&input, &bufsize, stdin); // Read user input
    return input;
}

char** parse_input(char* input) {
    int bufsize = 64, position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;

    if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(input, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = malloc(strlen(token) + 1);
        if (!tokens[position]) {
            fprintf(stderr, "shell: allocation error\n");
            exit(EXIT_FAILURE);
        }
        strcpy(tokens[position], token); // Copy token to dynamically allocated memory
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

int execute_command(char** args) {
    if (args[0] == NULL) {
        // Empty command
        return 1;
    }

    if (strcmp(args[0], "exit") == 0) {
        return 0; // Exit the shell
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "Expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("shell");
            }
        }
        return 1;
    }

    return launch_process(args);
}

int launch_process(char** args) {
    pid_t pid;
    int status;

    pid = fork(); // Create a child process
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("shell");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

void sigint_handler(int sig) {
    printf("\nUse 'exit' to leave the shell.\n");
    printf("> ");
    fflush(stdout);
}
