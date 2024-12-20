#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

int is_executable(const char *path) {
	return access(path, X_OK) == 0;
}

char *find_in_path(const char *command) {
	char *path_env = getenv("PATH");
	if (path_env == NULL) return NULL;

	char *path_copy  = strdup(path_env);
	char *dir = strtok(path_copy, ":");

	static char full_path[1024];

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

void fork_and_execute(char *cmd_path, int argc, char **argv) {
	pid_t pid = fork();
	if (pid == 0) {
		execv(cmd_path, argv);
		perror("execv");
		exit(1);
	} else if (pid < 0) {
		perror("fork");
	} else {
		int status;
		waitpid(pid, &status, 0);
	}
}

int main() {
	// Flush after every printf
	setbuf(stdout, NULL);

	// Uncomment this block to pass the first stage
	// printf("$ ");

	// Wait for user input
	char input[100];
	while(1) {
		memset(input, 0, 100);
		printf("$ ");
		fgets(input, 100, stdin);
		input[strlen(input) - 1] = '\0';

		if (strcmp(input, "exit 0") == 0) {
			break;
		} else if (strncmp(input, "echo", 4) == 0) {
			char *p = input + 5;
			printf("%s\n", p);
		} else if (strncmp(input, "type", 4) == 0) {
			char *p = input + 5;
			char builtins[][5] = {"echo", "type", "exit", "pwd", "cd"};
			int found = 1;
			for (int i = 0; i < sizeof(builtins)/5; i++) {
				if (strncmp(p, builtins[i], strlen(builtins[i])) == 0) {
					printf("%s is a shell builtin\n", p);
					found = 0;
					break;
				}
			}
			if (found == 1) {
				char *path = find_in_path(p);
				if (path) {
					found = 0;
					printf("%s is %s\n", p, path);
				}
			}
			if (found  == 1) printf("%s: not found\n", p); 
		} else if (strncmp(input, "pwd", 3) == 0) {
			char current_path[PATH_MAX];
			if (getcwd(current_path, PATH_MAX) != NULL) {
				printf("%s\n", current_path);
			} else {
				perror("getcwd");
			}
		} else if (strncmp(input, "cd", 2) == 0) {
			if (chdir(input + 3) != 0) {
				printf("cd: %s: No such file or directory\n", input+3);
			}	       
		} else {
			/* Process the string, split the command in command and arguments */
			char *argv[10];
			int argc = 0;
			char *token = strtok(input, " ");
			while (token != NULL && argc < 10) {
				argv[argc++] = token;
				token = strtok(NULL, " ");
			}
			argv[argc] = NULL;
			/* Check if the command is in the path */
			char *cmd_path = find_in_path(argv[0]);
			/* if it is -> execute the command with the arguments */
			if (cmd_path != NULL) {
				fork_and_execute(cmd_path, argc, argv);
			} else {
				/* if not -> prinf unknown command */
				printf("%s: command not found\n", input);
			}
		}
	}
	return 0;
}
