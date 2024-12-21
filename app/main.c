#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

int tokenize_input(const char *input, char **args);
int is_executable(const char *path);
char *find_in_path(const char *command);
void fork_and_execute(char *cmd_path, int argc, char **args);
int process_exit(char *args[], int argc);
int process_echo(char *args[], int argc);
int process_type(char *args[], int argc);
int process_pwd(char *args[], int argc);
int process_cd(char *args[], int argc);

typedef struct builtin_command_t {
        const char *name;
        int (*process)(char *args[], int argc);
} builtin_command_t;

builtin_command_t builtin_commands[] = {
    {"exit", process_exit}, {"echo", process_echo}, {"type", process_type},
    {"pwd", process_pwd},   {"cd", process_cd},
};

int main() {
    char input[1024];

    char *args[20];

    while (1) {

        printf("$ ");
        memset(input, 0, 1024);
        fgets(input, 1024, stdin);
        int token_num = tokenize_input(input, args);

        int found = 0;
        int check = 0;
        for (int i = 0; i < ARRAY_SIZE(builtin_commands); i++) {
            if (strncmp(args[0], builtin_commands[i].name,
                        strlen(builtin_commands[i].name)) == 0) {
                check = builtin_commands[i].process(args, token_num);
                found = 1;
            }
        }

        if (found == 0) {
            char *cmd_path = find_in_path(args[0]);
            if (cmd_path != NULL) {
                args[token_num] = NULL;
                fork_and_execute(cmd_path, token_num, args);
                found = 1;
            }
        }

        if (found == 0 || check != 0) {
            printf("%s: incorrect command\n", args[0]);
        }

        for (int i = 0; i < token_num; i++) {
            free(args[i]);
        }
        fflush(stdin);
    }

    return 0;
}

int tokenize_input(const char *input, char **args) {
    int token_idx = 0;
    int token_num = 0;
    int in_token = 0;
    int in_quote = 0;

    for (int i = 0; i < strlen(input); i++) {
        char temp[100];

        if (input[i] == '\n') {
            // Base case, end of the input
            temp[token_idx] = '\0';
            args[token_num] = malloc(sizeof(char) * strlen(temp));
            strcpy(args[token_num], temp);
            token_num++;
            break;
        } else if (input[i] == ' ') {
            if ((in_token == 1) && (in_quote == 0)) {
                temp[token_idx] = '\0';
                token_idx = 0;
                args[token_num] = malloc(sizeof(char) * strlen(temp));
                strcpy(args[token_num], temp);
                in_token = 0;
                in_quote = 0;
                token_num++;
            } else if ((in_token == 1) && (in_quote == 1)) {
                temp[token_idx++] = input[i];
            }
        } else if (input[i] == '\'') {
            if (in_quote == 1) {
                in_quote = 0;
            } else {
                in_quote = 1;
            }
        } else {
            if (in_token == 0)
                in_token = 1;
            temp[token_idx++] = input[i];
        }
    }

    return token_num;
}

int is_executable(const char *path) { return access(path, X_OK) == 0; }

char *find_in_path(const char *command) {
    char *path_env = getenv("PATH");
    if (path_env == NULL)
        return NULL;

    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");

    static char full_path[PATH_MAX];

    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (is_executable(full_path) == 1) {
            free(path_copy);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }
    free(path_copy);
    return NULL;
}

void fork_and_execute(char *cmd_path, int argc, char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execv(cmd_path, args);
        perror("execv");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

int process_exit(char *args[], int argc) {
    if (argc == 2) {
        if (strncmp(args[1], "0", 1) == 0) {
            exit(EXIT_SUCCESS);
        } else {
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Incorrect use of exit\n");
        return 1;
    }
    return 0;
}

int process_echo(char *args[], int argc) {
    for (int i = 1; i < argc; i++) {
        if (i < argc - 1) {
            printf("%s ", args[i]);
        } else {
            printf("%s", args[i]);
        }
    }
    printf("\n");
    return 0;
}

int process_type(char *args[], int argc) {
    if (argc == 2) {
        for (int i = 0; i < ARRAY_SIZE(builtin_commands); i++) {
            if (strncmp(args[1], builtin_commands[i].name,
                        strlen(builtin_commands[i].name)) == 0) {
                printf("%s is a shell builtin\n", builtin_commands[i].name);
                return 0;
            }
        }

        char *path = find_in_path(args[1]);
        if (path != NULL) {
            printf("%s is %s\n", args[1], path);
            return 0;
        }
        printf("%s: not found\n", args[1]);
    }

    return 0;
}

int process_pwd(char *args[], int argc) {
    char current_path[PATH_MAX];
    if (getcwd(current_path, PATH_MAX) != NULL) {
        printf("%s\n", current_path);
    } else {
        perror("getcwd");
    }
    return 0;
}
int process_cd(char *args[], int argc) {
    if (strncmp(args[1], "~", 1) == 0) {
        char *home_env = getenv("HOME");
        if (home_env == NULL)
            perror("HOME");
        if (chdir(home_env) != 0)
            perror("cd HOME");
    } else if (chdir(args[1]) != 0) {
        printf("cd: %s: No such file or directory\n", args[1]);
    }
    return 0;
}
