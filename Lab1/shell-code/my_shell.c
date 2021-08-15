#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_BG_PROCS 64

// Modes of operation in Part B
#define SINGLE_BG 1
#define SERIES_FG 2
#define PARALL_BG 3

pid_t bg_proc[MAX_BG_PROCS];

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for (i = 0; i < strlen(line); i++)
	{

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		}
		else
		{
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

void multi_handler_func(char* line) {
	uint OP_MODE = 0;
	if (strstr(line, "&&&"))
		OP_MODE = 3;
	else if (strstr(line, "&&"))
		OP_MODE = 2;
	else if (strchr(line, '&'))
		OP_MODE = 1;
	
	if (OP_MODE == 0) {
		printf("%s\n", "MULTI_HANDLER_FUNC: NOT A MULTI HANDLER CASE");
		return;
	}
	
	char *linesegment;
	char **tokens;
	if (OP_MODE == 1) { // SINGLE BACKGROUND
		linesegment = strtok(line, "&");
		tokens = tokenize(linesegment);

		// Check if anything is present after '&'
		linesegment = strtok(NULL, "&");
		if (linesegment != NULL && linesegment[0] != '\n') {
			printf("%s\n", "SHELL: Unknown args found after '&'");
			return;
		}

		// Fork and do our usual business, the only difference being, 
		// we don't wait for the child process to die in the parent
		// process.
		pid_t pid_bg_child = fork();
		if (pid_bg_child < 0) {
			perror("SHELL: FATAL FORK FAILED");
		}
		else if (pid_bg_child == 0) {
			int p = execvp(tokens[0], tokens);
			if (p == -1) {
				perror("SHELL"); // we expect execvp to provide the correct error response!
				_exit(1);
			}
		}
		else {
			// Add the child to the list of bg processes.
			for (size_t i = 0; i < MAX_BG_PROCS; i++)
				if (bg_proc[i] == -1) {
					bg_proc[i] = pid_bg_child;
					break;
				}
		}
	}


	for (size_t i = 0; tokens[i] != NULL; i++)
	{
		free(tokens[i]);
	}
	free(tokens);
}

int main(int argc, char *argv[])
{
	char line[MAX_INPUT_SIZE];
	char **tokens;
	for (size_t i = 0; i < MAX_BG_PROCS; i++)
		bg_proc[i] = -1;
	

	while (1)
	{
		// Check if background processes have finished and update bg_proc list accordingly
		for (size_t i = 0; i < MAX_BG_PROCS; i++) {
			if (bg_proc[i] != -1) {
				pid_t child_proc = waitpid(bg_proc[i], NULL, WNOHANG);
				if (child_proc == bg_proc[i]) { // meaning, bg_proc[i] has changed state 
					bg_proc[i] = -1;
					printf("%s%d %s\n", "SHELL: BACKGROUND PROCESS PID:", child_proc, "FINISHED");
				}	
			}
		}
		

		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		if (tokens[0] == NULL) {
			free(tokens[0]);
			free(tokens);
			continue;
		}
		
		//do whatever you want with the commands, here we just print them

		// for (i = 0; tokens[i] != NULL; i++)
		// {
		// 	printf("found token %s (remove this debug output later)\n", tokens[i]);
		// }
		// tokens is null terminated


		// CASES: We can currently handle:

		if (!strcmp(tokens[0], "cd")) {
			if (tokens[1] == NULL || tokens[2] != NULL) {
				printf("SHELL: Incorrect command\n");
			}
			else {
				int ch = chdir(tokens[1]);
				if (ch < 0) {
					perror("SHELL");
				}
			}
		}
		else if (strchr(line, '&')) { // Check if background execution is required
			// handles execution of multiple commands at the same time
			multi_handler_func(line); 
		}
		else {
			// plain foreground execution case
			pid_t pidchild = fork();

			// printf("%d\n",pidchild);

			if (pidchild < 0) {
				perror("SHELL: FATAL FORK FAILED");
			}
			else if (pidchild == 0)	{
				//ref: https://linuxhint.com/exec_linux_system_call_c/
				int p = execvp(tokens[0], tokens);
				if (p == -1) {
					perror("SHELL");
					// Interesting read: https://stackoverflow.com/questions/5422831/what-is-the-difference-between-using-exit-exit-in-a-conventional-linux-fo
					_exit(1);
				}
			}
			else { // k > 0 
				pid_t pidchange = waitpid(pidchild, ((void *)0), 0); // NULL is ((void *)0)
			}
		}		

		

		// Freeing the allocated memory
		for (size_t i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}
