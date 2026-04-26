#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

void shell_loop();
char* read_input();
char** parse_input(char* input);
int execute_command(char** args);
int launch_process(char** args);
int execute_pipe(char** left_args, char** right_args);
void sigint_handler(int sig);
char* trim_whitespace(char* s);
void free_args(char** args);
int is_builtin(char** args);
int run_builtin(char** args, int in_child);

int main() {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    shell_loop();
    return 0;
}

void shell_loop() {
    char* input;
    int status = 1;

    do {
        printf("> ");
        fflush(stdout);

        input = read_input();
        if (!input) {
            break; // EOF
        }

        // If interrupted and we returned empty string, just loop
        if (input[0] == '\0') {
            free(input);
            continue;
        }

        // Check for single pipe
        char* pipe_pos = strchr(input, '|');
        if (pipe_pos != NULL) {
            *pipe_pos = '\0';
            char* left_str = trim_whitespace(input);
            char* right_str = trim_whitespace(pipe_pos + 1);

            if (left_str[0] == '\0' || right_str[0] == '\0') {
                fprintf(stderr, "shell: invalid pipe usage\n");
                free(input);
                continue;
            }

            char** left_args = parse_input(left_str);
            char** right_args = parse_input(right_str);

            status = execute_pipe(left_args, right_args);

            free_args(left_args);
            free_args(right_args);
            free(input);
            continue;
        }

        // No pipe: normal command
        char** args = parse_input(input);
        status = execute_command(args);

        free_args(args);
        free(input);

    } while (status);
}

char* read_input() {
    char* input = NULL;
    size_t bufsize = 0;

    ssize_t n = getline(&input, &bufsize, stdin);
    if (n == -1) {
        if (errno == EINTR) {
            // Interrupted by signal; behave like an empty line
            free(input);
            clearerr(stdin);
            char* empty = strdup("");
            if (!empty) {
                perror("strdup");
                exit(EXIT_FAILURE);
            }
            return empty;
        }
        if (feof(stdin)) {
            free(input);
            return NULL;
        }
        perror("getline");
        free(input);
        exit(EXIT_FAILURE);
    }

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
        strcpy(tokens[position], token);
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            char** new_tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!new_tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
            tokens = new_tokens;
        }

        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

int is_builtin(char** args) {
    if (args[0] == NULL) return 0;
    return strcmp(args[0], "cd") == 0 ||
           strcmp(args[0], "exit") == 0;
}

int run_builtin(char** args, int in_child) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "Expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("shell");
            }
        }
        return 1; // continue shell
    }

    if (strcmp(args[0], "exit") == 0) {
        if (in_child) {
            _exit(0); // only child exits
        }
        return 0; // parent should exit shell loop
    }

    return 1;
}

int execute_command(char** args) {
    if (args[0] == NULL) {
        return 1; // empty command
    }

    if (is_builtin(args)) {
        return run_builtin(args, 0); // in parent
    }

    return launch_process(args);
}

int launch_process(char** args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("shell");
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("shell");
    } else {
        do {
            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("waitpid");
                break;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int execute_pipe(char** left_args, char** right_args) {
    int fd[2];
    pid_t p1, p2;

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    p1 = fork();
    if (p1 < 0) {
        perror("fork");
        return 1;
    }

    if (p1 == 0) {
        // First child: left side of pipe
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(fd[0]);
        close(fd[1]);

        if (is_builtin(left_args)) {
            run_builtin(left_args, 1);
            _exit(0);
        } else {
            execvp(left_args[0], left_args);
            perror("shell");
            _exit(EXIT_FAILURE);
        }
    }

    p2 = fork();
    if (p2 < 0) {
        perror("fork");
        return 1;
    }

    if (p2 == 0) {
        // Second child: right side of pipe
        if (dup2(fd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(fd[1]);
        close(fd[0]);

        if (is_builtin(right_args)) {
            run_builtin(right_args, 1);
            _exit(0);
        } else {
            execvp(right_args[0], right_args);
            perror("shell");
            _exit(EXIT_FAILURE);
        }
    }

    // Parent
    close(fd[0]);
    close(fd[1]);

    int status;
    if (waitpid(p1, &status, 0) == -1) {
        perror("waitpid");
    }
    if (waitpid(p2, &status, 0) == -1) {
        perror("waitpid");
    }

    return 1;
}

void sigint_handler(int sig) {
    (void)sig;
    const char msg[] = "\nUse 'exit' to leave the shell.\n> ";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

char* trim_whitespace(char* s) {
    char* end;

    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;

    if (*s == 0)
        return s;

    end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n'))
        end--;

    *(end + 1) = '\0';
    return s;
}

void free_args(char** args) {
    if (!args) return;
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}
