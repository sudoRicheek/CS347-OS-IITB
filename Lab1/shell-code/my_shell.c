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

int main(int argc, char *argv[])
{
	char line[MAX_INPUT_SIZE];
	char **tokens;
	int i;

	while (1)
	{
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
		// tokens is null termianted

		
		if (!strcmp(tokens[0],"cd")) {
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
		else {
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
		for (i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}
