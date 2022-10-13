#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16
#define TOKEN_MAX 32
#define WHITE_SPACE " \t"
#define STANDARD_OUTPUT 0
#define MAX_PIPE 4
#define TRUE 1
#define FALSE 0

struct Pipeline
{
	int pipeSize;
	char **listOfCommand;
	int *status;
	struct Config *listOfConfig[4];
};

struct FD
{
	int fd[2];
};
struct Config
{
	int argumentSize;
	char **listOfArgument;
	char *outputFile;
	int isOutputRedirect;
};

struct Stack
{
	char *currDir;
	struct Stack *next;
};

// static void pushd(struct Stack *stack, char *str)
// {
// 	struct Stack *newElement = malloc(sizeof(struct Stack));
// 	newElement->currDir = stack->currDir;
// 	newElement->next = stack->next;

// 	stack->currDir = str;
// 	stack->next = newElement;
// }

// static void popd(struct Stack *stack)
// {
// 	if (stack->next != NULL)
// 	{
// 		stack->currDir = stack->next->currDir;
// 		stack->next = stack->next->next;
// 	}
// }

// static void dirs(struct Stack *stack)
// {
// 	struct Stack *iteration = malloc(sizeof(struct Stack));
// 	iteration = stack;
// 	while (iteration != NULL)
// 	{
// 		printf("%s\n", iteration->currDir);
// 		iteration = iteration->next;
// 	}
// }

/*
the function splits a string using a split string
input: a string, and a split string
output: array of string that is splited by the character
*/
static int redirectionOutput(char *redirectionFile)
{
	int fd1 = open(redirectionFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd1 < 0)
	{
		return EXIT_FAILURE;
	}
	dup2(fd1, STDOUT_FILENO);
	close(fd1);
	return EXIT_SUCCESS;
}
static char **splitString(char *string, char *split)
{
	char **splitList = malloc(CMDLINE_MAX);
	char *token;
	int i = 0;

	if (!strcmp(split, WHITE_SPACE))
	{
		token = strtok(string, split);
		while (token != NULL)
		{
			splitList[i] = token;
			token = strtok(NULL, split);
			i++;
		}

		return splitList;
	}

	while ((token = strsep(&string, split)) != NULL)
	{
		splitList[i] = token;
		i++;
	}
	return splitList;
}
static int getSize(char **listOfString)
{
	int size = 0;

	while (listOfString[size] != NULL)
	{
		size++;
	}

	return size;
}
/*
extract arguments and redirection files from the command and saved them to the struct
input: Config struct and a command
*/
static int parseCommand(struct Config *config, char *cmd)
{
	char *copyCmd = calloc(strlen(cmd) + 1, sizeof(char));
	strcpy(copyCmd, cmd);

	char **splitByLessThan;
	splitByLessThan = splitString(copyCmd, ">");

	int sizeSplitByLessThan = getSize(splitByLessThan);
	config->isOutputRedirect = FALSE;

	char **listOfArgument = malloc(CMDLINE_MAX);
	listOfArgument = splitString(splitByLessThan[0], WHITE_SPACE);
	int argumentSize = getSize(listOfArgument);
	if (argumentSize == 0)
	{
		fprintf(stderr, "Error: missing command\n");
		return EXIT_FAILURE;
	}
	else if (argumentSize > ARG_MAX)
	{
		fprintf(stderr, "%s\n", "Error: too many process arguments");
		fprintf(stderr, "+ completed '%s' [%d]\n",
				cmd, 1);
		return EXIT_FAILURE;
	}
	if (sizeSplitByLessThan == 2)
	{
		char **splitBySpace = malloc(CMDLINE_MAX);
		splitBySpace = splitString(splitByLessThan[1], WHITE_SPACE);
		if (getSize(splitBySpace) == 0)
		{
			fprintf(stderr, "Error: no output file\n");
			return EXIT_FAILURE;
		}
		else
		{
			config->isOutputRedirect = TRUE;
			config->outputFile = splitBySpace[0];
		}
	}
	config->listOfArgument = listOfArgument;
	config->argumentSize = getSize(config->listOfArgument);
	return EXIT_SUCCESS;
}

static int parsePipe(struct Pipeline *pipeline, char *cmd)
{
	char *copyCmd = calloc(strlen(cmd) + 1, sizeof(char));
	strcpy(copyCmd, cmd);
	char **listPipe = splitString(copyCmd, "|");

	pipeline->pipeSize = getSize(listPipe);
	struct Config *config;
	int index = 0;
	while (listPipe[index] != NULL)
	{
		config = malloc(sizeof(struct Config));
		int status = parseCommand(config, listPipe[index]);
		if (status == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}

		if (config->isOutputRedirect == TRUE)
		{
			if (index != pipeline->pipeSize - 1)
			{
				fprintf(stderr, "Error: mislocated output redirection\n");
			}
			else
			{
				// focus here - cannot be here
				int status = redirectionOutput(config->outputFile);
				if (status == EXIT_FAILURE)
				{
					fprintf(stderr, "Error: cannot open output file\n");
				}
			}
		}

		pipeline->listOfConfig[index] = config;
		index++;
	}
	pipeline->listOfCommand = listPipe;
	return EXIT_SUCCESS;
}

static void otherCommand(char *firstArg, char **args)
{
	int checkExecValue = execvp(firstArg, args);
	if (checkExecValue < 0)
	{
		fprintf(stderr, "%s\n", "Error: command not found");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

static void runOneCommand(char *cmd)
{
	pid_t pid;
	int status = 0;
	struct Config config;
	memset(&config, 0, sizeof(struct Config));

	// copy command to reserve the original command
	// https://stackoverflow.com/questions/17104953/c-strtok-split-string-into-tokens-but-keep-old-data-unaltered
	char *copyCmd = calloc(strlen(cmd) + 1, sizeof(char));
	strcpy(copyCmd, cmd);

	parseCommand(&config, copyCmd);
	free(copyCmd);

	char **args = config.listOfArgument;
	char *firstArg = args[0];

	if (!strcmp(firstArg, "pwd"))
	{
		char cwd[CMDLINE_MAX];
		getcwd(cwd, CMDLINE_MAX);
		printf("%s\n", cwd);
		free(config.listOfArgument);
	}
	else if (!strcmp(firstArg, "cd"))
	{
		int chdir_status = chdir(args[1]);
		if (chdir_status == -1)
		{
			fprintf(stderr, "%s\n", "Error: cannot cd into directory");
			fprintf(stderr, "+ completed '%s' [%d]\n",
					cmd, 1);
			free(config.listOfArgument);
			return;
		}
	}
	else if (!strcmp(cmd, "exit"))
	{
		fprintf(stderr, "Bye...\n");
		fprintf(stderr, "+ completed '%s' [%d]\n",
				cmd, status);
		free(config.listOfArgument);
		exit(0);
	}
	else
	{
		pid = fork();

		if (pid == 0)
		{
			otherCommand(firstArg, args);
			free(config.listOfArgument);
		}
		else if (pid > 0)
		{
			/* Parent */
			waitpid(-1, &status, 0);
		}
		else
		{
			perror("fork");
		}
	}
	fprintf(stderr, "+ completed '%s' [%d]\n",
			cmd, WEXITSTATUS(status));
}

static void runCommand(char *cmd)
{
	struct Pipeline pipeline;
	memset(&pipeline, 0, sizeof(struct Pipeline));
	parsePipe(&pipeline, cmd);

	int pipeSize = pipeline.pipeSize;
	if (pipeSize == 0)
	{
		return;
	}

	if (pipeSize == 1)
	{
		runOneCommand(pipeline.listOfCommand[0]);
		dup2(0, STDOUT_FILENO);
		free(pipeline.listOfCommand);
		return;
	}

	pid_t p[pipeSize];
	struct FD pipeFD[pipeSize - 1];
	memset(&pipeFD, 0, sizeof(struct FD) * pipeSize);
	for (int i = 0; i < pipeSize - 1; i++)
	{
		pipe(pipeFD[i].fd);
	}
	for (int i = 0; i < pipeSize; i++)
	{
		p[i] = fork();
		if (p[i] == 0)
		{
			// https://stackoverflow.com/questions/17104953/c-strtok-split-string-into-tokens-but-keep-old-data-unaltered
			// char *copyCmd = calloc(strlen(cmd) + 1, sizeof(char));
			// strcpy(copyCmd, cmd);
			char **args = pipeline.listOfConfig[i]->listOfArgument;
			char *firstArg = args[0];

			if (i == 0)
			{
				dup2(pipeFD[i].fd[1], STDOUT_FILENO);
			}
			else if (i == pipeSize - 1)
			{
				dup2(pipeFD[i - 1].fd[0], STDIN_FILENO);

				if (pipeline.listOfConfig[i]->isOutputRedirect == TRUE)
				{
					redirectionOutput(pipeline.listOfConfig[i]->outputFile);
				}
			}
			else
			{
				dup2(pipeFD[i - 1].fd[0], STDIN_FILENO);
				dup2(pipeFD[i].fd[1], STDOUT_FILENO);
			}
			for (int i = 0; i < pipeSize - 1; i++)
			{

				close(pipeFD[i].fd[0]);
				close(pipeFD[i].fd[1]);
			}

			otherCommand(firstArg, args);
			// free(pipeline.listOfConfig[i].listOfArgument);
			exit(0);
		}
	}
	for (int i = 0; i < pipeSize - 1; i++)
	{

		close(pipeFD[i].fd[0]);
		close(pipeFD[i].fd[1]);
	}

	for (int i = 0; i < pipeSize; i++)
	{
		if (p[i] > 0)
		{
			waitpid(p[i], &pipeline.status[i], WUNTRACED);
		}
	}
	free(pipeline.listOfCommand);
}

int main(void)
{
	char cmd[CMDLINE_MAX];

	while (1)
	{
		char *nl;

		/* Print prompt */
		printf("sshell$ ");
		fflush(stdout);

		/* Get command line */
		fgets(cmd, CMDLINE_MAX, stdin);

		/* Print command line if stdin is not provided by terminal */
		if (!isatty(STDIN_FILENO))
		{
			printf("%s", cmd);
			fflush(stdout);
		}

		/* Remove trailing newline from command line */
		nl = strchr(cmd, '\n');
		if (nl)
			*nl = '\0';

		runCommand(cmd);
	}

	return EXIT_SUCCESS;
}