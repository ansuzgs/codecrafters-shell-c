#include <ctype.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])
#define MAX_TOKENS 100
#define MAX_LENGTH 256

void finish_token(char *buffer, int *buf_idx, char **tokens, int *token_count);
int tokenize(const char *line, char **tokens);
int is_executable(const char *path);
char *find_in_path(const char *command);
void fork_and_execute(char *cmd_path, int argc, char **args);
int process_exit(char *args[], int argc);
int process_echo(char *args[], int argc);
int process_type(char *args[], int argc);
int process_pwd(char *args[], int argc);
int process_cd(char *args[], int argc);

typedef enum {
    STATE_NORMAL,
    STATE_IN_SINGLE_QUOTES,
    STATE_IN_DOUBLE_QUOTES,
    STATE_ESCAPED,
} parse_state_e;

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

    char *args[MAX_TOKENS];

    while (1) {

        // Flush after every printf
        setbuf(stdout, NULL);
        printf("$ ");
        memset(input, 0, 1024);
        fgets(input, 1024, stdin);
        // Elimina el \n si lo hay
        input[strcspn(input, "\n")] = '\0';
        int token_num = tokenize(input, args);

        char *out_file = NULL;
        char *err_file = NULL;
        int saved_stdout = dup(STDOUT_FILENO);
        if (saved_stdout < 0) {
            perror("stdout");
            exit(EXIT_FAILURE);
        }

        int saved_stderr = dup(STDERR_FILENO);
        if (saved_stderr < 0) {
            perror("stdout");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < token_num; i++) {
            if (strcmp(args[i], ">") == 0 || strcmp(args[i], "1>") == 0) {
                if (i + 1 < token_num) {
                    out_file = args[i + 1];
                }
                for (int j = i; j + 2 <= token_num; j++) {
                    args[j] = args[j + 2];
                }
                token_num -= 2;

                if (out_file != NULL) {
                    char *path_copy = strdup(out_file);
                    if (!path_copy) {
                        perror("strdup");
                        return EXIT_FAILURE;
                    }
                    char *dir_path = dirname(path_copy);
                    char cmd[1024];
                    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir_path);
                    if (system(cmd) != 0) {
                        fprintf(stderr, "Error al crear directorio: %s\n",
                                dir_path);
                        free(path_copy);
                        return EXIT_FAILURE;
                    }

                    int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fd < 0) {
                        perror("open redirection");
                        free(path_copy);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    free(path_copy);
                }
                break;
            }
            if (strcmp(args[i], ">>") == 0 || strcmp(args[i], "1>>") == 0) {
                if (i + 1 < token_num) {
                    out_file = args[i + 1];
                }
                for (int j = i; j + 2 <= token_num; j++) {
                    args[j] = args[j + 2];
                }
                token_num -= 2;

                if (out_file != NULL) {
                    char *path_copy = strdup(out_file);
                    if (!path_copy) {
                        perror("strdup");
                        return EXIT_FAILURE;
                    }
                    char *dir_path = dirname(path_copy);
                    char cmd[1024];
                    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir_path);
                    if (system(cmd) != 0) {
                        fprintf(stderr, "Error al crear directorio: %s\n",
                                dir_path);
                        free(path_copy);
                        return EXIT_FAILURE;
                    }

                    int fd =
                        open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (fd < 0) {
                        perror("open redirection");
                        free(path_copy);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    free(path_copy);
                }
                break;
            }

            if (strcmp(args[i], "2>") == 0) {
                if (i + 1 < token_num) {
                    err_file = args[i + 1];
                }
                for (int j = i; j + 2 <= token_num; j++) {
                    args[j] = args[j + 2];
                }
                token_num -= 2;
                if (err_file != NULL) {
                    char *path_copy = strdup(err_file);
                    if (!path_copy) {
                        perror("strdup");
                        return EXIT_FAILURE;
                    }
                    char *dir_path = dirname(path_copy);
                    char cmd[1024];
                    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir_path);
                    if (system(cmd) != 0) {
                        fprintf(stderr, "Error al crear directorio: %s\n",
                                dir_path);
                        free(path_copy);
                        return EXIT_FAILURE;
                    }
                    int fd = open(err_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fd < 0) {
                        perror("open redirection");
                        free(path_copy);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                    free(path_copy);
                }
                break;
            }
            if (strcmp(args[i], "2>>") == 0) {
                if (i + 1 < token_num) {
                    err_file = args[i + 1];
                }
                for (int j = i; j + 2 <= token_num; j++) {
                    args[j] = args[j + 2];
                }
                token_num -= 2;
                if (err_file != NULL) {
                    char *path_copy = strdup(err_file);
                    if (!path_copy) {
                        perror("strdup");
                        return EXIT_FAILURE;
                    }
                    char *dir_path = dirname(path_copy);
                    char cmd[1024];
                    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir_path);
                    if (system(cmd) != 0) {
                        fprintf(stderr, "Error al crear directorio: %s\n",
                                dir_path);
                        free(path_copy);
                        return EXIT_FAILURE;
                    }
                    int fd =
                        open(err_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (fd < 0) {
                        perror("open redirection");
                        free(path_copy);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                    free(path_copy);
                }
                break;
            }
        }
        args[token_num] = NULL;

        // Mostrar los tokens resultantes
        /*printf("Se encontraron %d tokens:\n", token_num);*/
        /*for (int i = 0; i < token_num; i++) {*/
        /*    printf("[%d] = '%s'\n", i, args[i]);*/
        /*}*/

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
            printf("%s: command not found\n", args[0]);
        }

        for (int i = 0; i < token_num; i++) {
            free(args[i]);
        }

        if (dup2(saved_stdout, STDOUT_FILENO) < 0) {
            perror("dup2 restore");
            exit(EXIT_FAILURE);
        }
        if (dup2(saved_stderr, STDERR_FILENO) < 0) {
            perror("dup2 restore");
            exit(EXIT_FAILURE);
        }
        close(saved_stdout);
    }

    return 0;
}

void finish_token(char *buffer, int *buf_idx, char **tokens, int *token_count) {
    if (*buf_idx > 0) {
        // Cerrar el string
        buffer[*buf_idx] = '\0';

        // Reservar memoria para el nuevo token
        tokens[*token_count] = (char *)malloc(strlen(buffer) + 1);
        if (tokens[*token_count] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        // Copiar el contenido del buffer al token
        strcpy(tokens[*token_count], buffer);

        // Incrementar el numero de tokens y reiniciar el indice
        (*token_count)++;
        *buf_idx = 0;
    }
}

int tokenize(const char *line, char **tokens) {
    parse_state_e state = STATE_NORMAL;
    parse_state_e prev_state = STATE_NORMAL;
    char buffer[MAX_LENGTH];
    int buf_idx = 0;
    int token_count = 0;

    for (int i = 0; i < (int)strlen(line); i++) {
        char c = line[i];

        switch (state) {
        case STATE_NORMAL:
            if (c == '\\') {
                // Pasamos al estado STATE_ESCAPED
                prev_state = STATE_NORMAL;
                state = STATE_ESCAPED;
            } else if (c == '\'') {
                // Pasamos al estado comillas simples
                state = STATE_IN_SINGLE_QUOTES;
            } else if (c == '\"') {
                // Pasamos al estado comillas dobles
                state = STATE_IN_DOUBLE_QUOTES;
            } else if (isspace((unsigned char)c)) {
                finish_token(buffer, &buf_idx, tokens, &token_count);
            } else {
                // Caracter normal lo agregamos al buffer
                if (buf_idx < MAX_LENGTH - 1) {
                    buffer[buf_idx++] = c;
                }
            }
            break;

        case STATE_IN_SINGLE_QUOTES:
            if (c == '\'') {
                // Cierre de comillas simples
                state = STATE_NORMAL;
            } else {
                // Acumulamos literal
                if (buf_idx < MAX_LENGTH - 1) {
                    buffer[buf_idx++] = c;
                }
            }
            break;

        case STATE_IN_DOUBLE_QUOTES:
            if (c == '\"') {
                // Cierre de comillas dobles
                state = STATE_NORMAL;
            } else if (c == '\\') {
                // Permitir escapes dentro de comillas dobles
                prev_state = STATE_IN_DOUBLE_QUOTES;
                state = STATE_ESCAPED;
            } else {
                // Acumulamos literal
                if (buf_idx < MAX_LENGTH - 1) {
                    buffer[buf_idx++] = c;
                }
            }
            break;

        case STATE_ESCAPED:
            // Tomamos el siguiente caracter literalmente

            if (prev_state == STATE_NORMAL) {
                switch (c) {
                case 'n':
                    buffer[buf_idx++] = 'n';
                    break;
                case 't':
                    buffer[buf_idx++] = 't';
                    break;
                case ' ':
                    buffer[buf_idx++] = ' ';
                    break;
                case '\'':
                    buffer[buf_idx++] = '\'';
                    break;
                case '\"':
                    buffer[buf_idx++] = '\"';
                    break;
                default:
                    // las desconocidas => '\' + c
                    if (buf_idx < MAX_LENGTH - 2) {
                        buffer[buf_idx++] = '\\';
                        buffer[buf_idx++] = c;
                    }
                    break;
                }
            } else if (prev_state == STATE_IN_DOUBLE_QUOTES) {
                switch (c) {
                /*case 'n':*/
                /*    buffer[buf_idx++] = '\n';*/
                /*    break;*/
                case 't':
                    buffer[buf_idx++] = '\t';
                    break;
                case '\\':
                    buffer[buf_idx++] = '\\';
                    break;
                case '\"':
                    buffer[buf_idx++] = '\"';
                    break;
                default:
                    if (buf_idx < MAX_LENGTH - 2) {
                        buffer[buf_idx++] = '\\';
                        buffer[buf_idx++] = c;
                    }
                    break;
                }
                // Heuristica minima para volver a un estado anterior.
                // En un diseño mas completo se guardaria prev_state
            }
            state = prev_state;
            break;
        }
    }
    // Si quedara algo en el buffer al final, cerrar ese token
    finish_token(buffer, &buf_idx, tokens, &token_count);

    return token_count;
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
