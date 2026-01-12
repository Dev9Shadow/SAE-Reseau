/*
 * mbash.c - Bash Miniature
 * Compilation: gcc -Wall mbash.c -o mbash
 * Lancement: ./mbash
 * Auteurs: Romain SANTILLI et Eliot SCHMITT
 *
 * Fonctionnalités implémentées :
 * - cd (changement de répertoire)
 * - pwd (affichage du répertoire courant)
 * - exit et Ctrl+D (quitter proprement)
 * - Exécution de commandes externes via PATH (execvp)
 * - Exécution en arrière-plan avec &
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

void print_prompt() {
    char cwd[1024];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("\033[1;36mmonbash\033[0m:%s$ ", cwd);
    } else {
        printf("\033[1;36mmonbash\033[0m$ ");
    }
    
    fflush(stdout);
}

int main() {
    char input[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    
    while (1) {
        print_prompt();

        if (fgets(input, MAX_CMD_LEN, stdin) == NULL) {
            printf("\nAu revoir!\n");
            break; 
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;

        int i = 0;
        int background = 0; 
        char *token = strtok(input, " ");
        
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (i > 0 && strcmp(args[i-1], "&") == 0) {
            background = 1;
            args[i-1] = NULL;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "mbash: cd: argument attendu\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("mbash: cd");
                }
            }
            continue;
        }

        if (strcmp(args[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("mbash: pwd");
            }
            continue; 
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("mbash: fork failed");
        } else if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                perror("mbash");
            }
            exit(EXIT_FAILURE);
        } else {
            if (!background) {
                wait(NULL);
            } else {
                printf("[1] %d\n", pid); 
            }
        }
    }
    return 0;
}