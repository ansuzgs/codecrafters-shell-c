#include <stdio.h>
#include <string.h>

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
			if (found  == 1) printf("%s: not found\n", p); 
		} else {
			printf("%s: command not found\n", input);
		}
	}
	return 0;
}
