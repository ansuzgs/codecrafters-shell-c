#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int is_executable(const char *path) {
	return access(path, X_OK) == 0;
}

char *find_in_path(const char *command) {
	char *path_env = getenv("PATH");
	if (path_env == NULL) return NULL;

	char *path_copy  = strdup(path_env);
	char *dir = strtok(path_copy, ":");

	static char full_path[1024] = {0};

	while (dir != NULL) {
		snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
		if (is_executable(full_path) == 0) {
			free(path_copy);
			return full_path;
		}
		dir = strtok(NULL, ":");
	}

	free(path_copy);
	return NULL;
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
			char builtins[][5] = {"echo", "type", "exit"};
			int found = 1;
			for (int i = 0; i < sizeof(builtins)/5; i++) {
				if (strncmp(p, builtins[i], strlen(builtins[i])) == 0) {
					printf("%s is a shell builtin\n", p);
					found = 0;
					break;
				}
			}
			char *path = find_in_path(p);
			if (path) {
				found = 0;
				printf("%s is %s\n", p, path);
			}
			if (found  == 1) printf("%s: not found\n", p); 
		} else {
			printf("%s: command not found\n", input);
		}
	}
	return 0;
}
