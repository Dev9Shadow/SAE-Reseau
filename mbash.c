/*
 * mbash.c - Version 0.5 
 * Compilation: gcc -Wall mbash.c -o mbash
 * Lancement: ./mbash
 * Auteurs: Romain SANTILLI et Eliot SCHMITT
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

// etats de l'automate
#define STATE_SPACE 0 
#define STATE_TOKEN 1 

// prototypes
int execute_command(char **args);
int parse_input(char *input, char **args);

// affichage du prompt 
void print_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("\033[1;36mmonbash\033[0m:%s$ ", cwd);
    } else {
        printf("\033[1;36mmonbash\033[0m$ ");
    }
    fflush(stdout);
}

// automate de parsing (memoire optimisee)
int parse_input(char *input, char **args) {
    int count = 0;
    int state = STATE_SPACE;
    char *ptr = input;

    while (*ptr != '\0' && count < MAX_ARGS - 1) {
        char c = *ptr;

        if (state == STATE_SPACE) {
            if (c != ' ' && c != '\t' && c != '\n') {
                state = STATE_TOKEN;
                args[count++] = ptr;
            }
        } 
        else if (state == STATE_TOKEN) {
            if (c == ' ' || c == '\t' || c == '\n') {
                state = STATE_SPACE;
                *ptr = '\0'; 
            }
        }
        ptr++;
    }
    
    args[count] = NULL;
    return count;
}

// gestion background
int detect_background(char **args, int count) {
    if (count > 0 && strcmp(args[count - 1], "&") == 0) {
        args[count - 1] = NULL;
        return 1;
    }
    return 0;
}

// gestion du if single-line
int handle_if(char **args) {
    int idx_then = -1, idx_else = -1, idx_fi = -1;

    // recherche des mots cles
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "then") == 0) idx_then = i;
        else if (strcmp(args[i], "else") == 0) idx_else = i;
        else if (strcmp(args[i], "fi") == 0) idx_fi = i;
    }

    if (idx_then == -1 || idx_fi == -1) {
        fprintf(stderr, "mbash: syntax error expected if...then...fi\n");
        return 1;
    }

    args[idx_then] = NULL;
    if (idx_else != -1) args[idx_else] = NULL;
    args[idx_fi] = NULL;

    int res = execute_command(&args[1]);

    if (res == 0) {
        return execute_command(&args[idx_then + 1]);
    } else if (idx_else != -1) {
        return execute_command(&args[idx_else + 1]);
    }
    return 0;
}

// execution centrale des commandes
int execute_command(char **args) {
    if (args[0] == NULL) return 0;
    if (strcmp(args[0], "if") == 0) return handle_if(args);
    if (strcmp(args[0], "exit") == 0) exit(0);
    if (strcmp(args[0], "true") == 0) return 0;
    if (strcmp(args[0], "false") == 0) return 1;
    if (strcmp(args[0], "status") == 0) return 0;

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "mbash: cd: argument attendu\n");
            return 1;
        }
        if (chdir(args[1]) != 0) {
            perror("mbash: cd");
            return 1;
        }
        return 0;
    }

    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
            return 0;
        } else {
            perror("mbash: pwd");
            return 1;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("mbash: fork failed");
        return 1;
    } else if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("mbash");
        }
        exit(EXIT_FAILURE);
    } else {
        int status;
        wait(&status);
        if (WIFEXITED(status)) return WEXITSTATUS(status);
    }
    return 1;
}

int main() {
    char input[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    int last_status = 0; 
    
    while (1) {
        print_prompt();

        if (fgets(input, MAX_CMD_LEN, stdin) == NULL) {
            printf("\nAu revoir\n");
            break; 
        }

        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        int argc = parse_input(input, args);
        if (argc == 0) continue;

        int background = detect_background(args, argc);

        if (strcmp(args[0], "status") == 0) {
             printf("Dernier status: %d\n", last_status);
             continue;
        }

        if (background) {
            pid_t pid = fork();
            if (pid == 0) {
                execute_command(args);
                exit(0);
            } else {
                printf("[1] %d\n", pid);
            }
        } else {
            last_status = execute_command(args);
        }
    }
    return 0;
}