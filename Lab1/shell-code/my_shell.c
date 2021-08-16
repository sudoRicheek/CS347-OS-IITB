#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

// SHELL Limitations
#define MAX_BG_PROCS 64
#define MAX_FG_PROCS 64

// Modes of operation in Part B
#define SINGLE_BG 1
#define SERIES_FG 2
#define PARALL_FG 3

pid_t bg_proc[MAX_BG_PROCS];
pid_t fg_proc[MAX_FG_PROCS];

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

void destroy_tokens(char **tokens) {
	if (tokens != NULL) {
		for (size_t i = 0; tokens[i] != NULL; i++) {
			free(tokens[i]);
		}
		free(tokens);
	}
}

void multi_handler_func(char* line) {
	uint OP_MODE = 0;
	if (strstr(line, "&&&"))
		OP_MODE = PARALL_FG;
	else if (strstr(line, "&&"))
		OP_MODE = SERIES_FG;
	else if (strchr(line, '&'))
		OP_MODE = SINGLE_BG;
	
	if (OP_MODE == 0) {
		printf("%s\n", "MULTI_HANDLER_FUNC: NOT A MULTI HANDLER CASE");
		return;
	}
	
	char *linesegment = NULL; // Stores delimited commands
	char **tokens = NULL; // tokenized commands

	if (OP_MODE == SINGLE_BG) { // SINGLE BACKGROUND
		linesegment = strtok(line, "&");
		tokens = tokenize(linesegment);

		// Check if anything is present after '&'
		linesegment = strtok(NULL, "&");
		if (linesegment != NULL && linesegment[0] != '\n') {
			printf("%s\n", "SHELL: Unknown args found after '&'");
			return;
		}
		if (tokens[0] == NULL) // check for emptiness
			return;

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
				destroy_tokens(tokens); // clean child memory image if exec fails
				_exit(1);
			}
		}
		else {
			// Add the child to the list of bg processes.
			for (size_t i = 0; i < MAX_BG_PROCS; i++) {
				if (bg_proc[i] == -1) {
					bg_proc[i] = pid_bg_child;
					break;
				}
				else if (i == MAX_BG_PROCS - 1) {
					printf("SHELL: MAX LIMIT FOR BACKGROUND PROCESSES REACHED");
					waitpid(pid_bg_child, NULL, 0); // Cant facilitate background execution
				}
			}
		}
		// Free the space!
		destroy_tokens(tokens);
	}
	else if (OP_MODE == SERIES_FG) { // SERIES FOREGROUND
		linesegment = strtok(line, "&"); // separated by && 
		while (linesegment != NULL && linesegment[0] != '\n') {
			tokens = tokenize(linesegment);

			if (tokens[0] == NULL)
				continue;
			
			pid_t pid_fg_child = fork();
			if (pid_fg_child < 0) {
				perror("SHELL: FATAL FORK FAILED");
			}
			else if (pid_fg_child == 0) {
				int p = execvp(tokens[0], tokens);
				if (p == -1) {
					perror("SHELL"); // we expect execvp to provide the correct error response!
					destroy_tokens(tokens); // clean child memory image if exec fails
					_exit(1); // gracefully exit if execvp fails
				}
			}
			else {
				// In parent shell
				pid_t pidchange = waitpid(pid_fg_child, NULL, 0); // wait for child to die
				
				// DEBUG output
				if (pidchange == pid_fg_child)
					printf("%s done\n", tokens[0]);
			}
			linesegment = strtok(NULL, "&"); // get next command in sequence

			// Free the space!
			destroy_tokens(tokens); // since there are mallocs on every tokenize operation
		}	
	}
	else if (OP_MODE == PARALL_FG) { // PARALLEL FOREGROUND
		linesegment = strtok(line, "&"); // separated by '&&&'
		while (linesegment != NULL && linesegment[0] != '\n') {
			tokens = tokenize(linesegment);

			if (tokens[0] == NULL)
				continue;

			pid_t pid_pll_fg_chld = fork();
			if (pid_pll_fg_chld < 0) {
				perror("SHELL: FATAL FORK FAILED");
			}
			else if (pid_pll_fg_chld == 0) {
				int p = execvp(tokens[0], tokens);
				if (p == -1) {
					perror("SHELL"); // we expect execvp to provide the correct error response!
					destroy_tokens(tokens); // clean child memory image if exec fails
					_exit(1); // gracefully exit if execvp fails
				}
			}
			else {
				// In parent process
				for (size_t i = 0; i < MAX_FG_PROCS; i++) {
					if (fg_proc[i] == -1) {
						fg_proc[i] = pid_pll_fg_chld;
						break;
					}
					else if (i == MAX_FG_PROCS - 1) {
						printf("SHELL: MAX LIMIT FOR FOREGROUND PROCESSES REACHED\n");
						printf("SHELL: Can't facilitate parallel execution\n");
						waitpid(pid_pll_fg_chld, NULL, 0); // Cant facilitate parallel execution
					}
				}
			}			
			linesegment = strtok(NULL, "&");

			// Free the space!
			destroy_tokens(tokens); // since there are mallocs on every tokenize operation
		}		

		// Make all the runnable processes and finally
		// wait for them to die all at once!
		for (size_t i = 0; i < MAX_FG_PROCS; i++) {
			if (fg_proc[i] != -1) {
				pid_t pidchange = waitpid(fg_proc[i], NULL, 0);

				// DEBUG output
				if (pidchange == fg_proc[i]) {
					fg_proc[i] = -1;
					printf("PID:%d done\n", pidchange);
				}				
			}			
		}
	}	
}

int main(int argc, char *argv[])
{
	char line[MAX_INPUT_SIZE];
	char **tokens =  NULL;
	for (size_t i = 0; i < MAX_BG_PROCS; i++)
		bg_proc[i] = -1;
	
	for (size_t i = 0; i < MAX_FG_PROCS; i++)
		fg_proc[i] = -1;
	

	while (1)
	{
		// Check if background processes have finished -> reap and update bg_proc list accordingly
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

		// CASES: We can currently handle:
		if (!strcmp(tokens[0], "exit")) {
			for (size_t i = 0; i < MAX_BG_PROCS; i++) {
				if (bg_proc[i] != -1) {
					int k = kill(bg_proc[i], SIGKILL);
					if (k == 0)
						bg_proc[i] = -1;
					else if (k == -1)
						perror("SHELL"); // we expect kill to set the correct errno
				}
			}
			destroy_tokens(tokens);
			break;
		}
		else if (!strcmp(tokens[0], "cd")) {
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
					destroy_tokens(tokens); // clean child memory image if exec fails
					// Interesting read: https://stackoverflow.com/questions/5422831/what-is-the-difference-between-using-exit-exit-in-a-conventional-linux-fo
					_exit(1);
				}
			}
			else { // k > 0 
				pid_t pidchange = waitpid(pidchild, ((void *)0), 0); // NULL is ((void *)0)
			}
		}		

		// Freeing the allocated memory
		destroy_tokens(tokens);		
	}
	return 0;
}
