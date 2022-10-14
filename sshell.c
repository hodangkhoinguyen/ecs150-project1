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
#define MAX_PIPE 4

#define WHITE_SPACE " \t"

#define TRUE 1
#define FALSE 0

struct Pipeline
{
	int pipeSize;
	char **listOfCommand;
	int status[MAX_PIPE];
	struct Config *listOfConfig[MAX_PIPE];
};

/*
	Config is the entire command of each pipe
*/
struct Config
{
	int argumentSize;
	char **listOfArgument;
	char *outputFile;
	int isOutputRedirect;
	char *inputFile;
	int isInputRedirect;
};

/*
	FD stores the array whenever pipe() called
*/
struct FD
{
	int fd[2];
};

struct StackNode
{
	char *currDir;
	struct StackNode *next;
};

struct Stack
{
	struct StackNode *top;
	int size;
};

/*
	push a string to the stack
*/
static void pushd(struct Stack *stack, char *str)
{
	struct StackNode *newElement = malloc(sizeof(struct StackNode));
	if (stack->top == NULL)
	{
		stack->size = 0;
		newElement->currDir = str;
		newElement->next = NULL;
	}
	else
	{
		stack->size++;
		newElement->next = stack->top;
		char *copyString = calloc(strlen(str) + 1, sizeof(char));
		strcpy(copyString, str);
		newElement->currDir = copyString;
	}

	stack->top = newElement;
}

/*
	pop the current string out of the stack
*/
static int popd(struct Stack *stack)
{
	struct StackNode *iteration = stack->top;
	if (iteration->next != NULL)
	{
		iteration->currDir = iteration->next->currDir;
		iteration->next = iteration->next->next;
		chdir(iteration->currDir);
		stack->size--;
		return EXIT_SUCCESS;
	}
	fprintf(stderr, "Error: directory stack empty\n");
	return EXIT_FAILURE;
}

/*
	print out the stack
*/
static void dirs(struct Stack *stack)
{
	struct StackNode *iteration = malloc(sizeof(struct StackNode));
	iteration = stack->top;
	while (iteration != NULL)
	{
		printf("%s\n", iteration->currDir);
		iteration = iteration->next;
	}
}

/*
	redirect the standard output to the file
	return EXIT_FAILURE if failed to open the file
	return EXIT_SUCCESS if success
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

/*
	redirect the standard input to the file
	return EXIT_FAILURE if failed to open the file
	return EXIT_SUCCESS if success
*/
static int redirectionInput(char *redirectionFile)
{
	int fd1 = open(redirectionFile, O_RDONLY);
	if (fd1 < 0)
	{
		return EXIT_FAILURE;
	}
	dup2(fd1, STDIN_FILENO);
	close(fd1);
	return EXIT_SUCCESS;
}

/*
	the function splits a string using a split string
	input: a string, a split delimeter
	output: array of string that is splited by the character
*/
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

/*
	Get size of a list of strings
*/
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
	return EXIT_FAILURE if any error
	return EXIT_SUCCESS if success
*/
static int parseCommand(struct Config *config, char *cmd)
{
	// copy the cmd to avoid changing the original cmd before splitted
	char *copyCmd = calloc(strlen(cmd) + 1, sizeof(char));
	strcpy(copyCmd, cmd);

	// parse by output
	char **splitByOutput;
	splitByOutput = splitString(copyCmd, ">");
	int sizeSplitByOutput = getSize(splitByOutput);

	// by default, there is no output redirect
	config->isOutputRedirect = FALSE;
	config->isInputRedirect = FALSE;

	// parse by input
	char **splitByInput;
	strcpy(copyCmd, splitByOutput[0]);
	splitByInput = splitString(copyCmd, "<");
	int sizeSplitByInput = getSize(splitByInput);

	// parse each pipeline to the arguments
	char **listOfArgument = malloc(CMDLINE_MAX);
	listOfArgument = splitString(splitByInput[0], WHITE_SPACE);
	int argumentSize = getSize(listOfArgument);

	// check the errors of arguments
	if (argumentSize == 0)
	{
		fprintf(stderr, "Error: missing command\n");
		return EXIT_FAILURE;
	}
	else if (argumentSize > ARG_MAX)
	{
		fprintf(stderr, "%s\n", "Error: too many process arguments");
		return EXIT_FAILURE;
	}

	// if there is output file
	if (sizeSplitByOutput == 2)
	{
		char **splitBySpace = malloc(CMDLINE_MAX);
		splitBySpace = splitString(splitByOutput[1], WHITE_SPACE);

		// the output file is empty (i.e. "echo >  ")
		if (getSize(splitBySpace) == 0)
		{
			fprintf(stderr, "Error: no output file\n");
			return EXIT_FAILURE;
		}
		// if not empty, set True for the outputFile
		else
		{
			config->isOutputRedirect = TRUE;
			config->outputFile = splitBySpace[0];
		}
	}

	// if there is input file
	if (sizeSplitByInput == 2)
	{
		char **splitBySpace = malloc(CMDLINE_MAX);
		splitBySpace = splitString(splitByInput[1], WHITE_SPACE);

		// the input file is empty (i.e. "echo >  ")
		if (getSize(splitBySpace) == 0)
		{
			fprintf(stderr, "Error: no input file\n");
			return EXIT_FAILURE;
		}
		// if not empty, set True for the input file
		else
		{
			config->isInputRedirect = TRUE;
			config->inputFile = splitBySpace[0];
		}
	}

	// complete parse each pipe
	config->listOfArgument = listOfArgument;
	config->argumentSize = getSize(config->listOfArgument);
	return EXIT_SUCCESS;
}

/*
	extract each pipeline from the command and saved them to the struct
	input: Pipeline struct and a command
	return EXIT_FAILURE if any error
	return EXIT_SUCCESS if success
*/
static int parsePipe(struct Pipeline *pipeline, char *cmd)
{
	// copy the cmd to avoid changing the original cmd before splitted
	char *copyCmd = calloc(strlen(cmd) + 1, sizeof(char));
	strcpy(copyCmd, cmd);

	// parse the pipeline
	char **listPipe = splitString(copyCmd, "|");
	pipeline->pipeSize = getSize(listPipe);

	struct Config *config;
	int index = 0;

	// iterate each pipeline to parse the command
	while (listPipe[index] != NULL)
	{
		// parse each pipeline command
		config = malloc(sizeof(struct Config));
		int status = parseCommand(config, listPipe[index]);

		if (status == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}

		// check potential errors for output redirect
		if (config->isOutputRedirect == TRUE)
		{
			// check if the output file is in the last pipeline
			if (index != pipeline->pipeSize - 1)
			{
				fprintf(stderr, "Error: mislocated output redirection\n");
				return EXIT_FAILURE;
			}
			else
			{
				FILE *out_file = fopen(config->outputFile, "w");
				// check if the output file can be accessed
				if (out_file == NULL)
				{
					fprintf(stderr, "Error: cannot open output file\n");
					return EXIT_FAILURE;
				}
				fclose(out_file);
			}
		}

		// check potential errors for input redirect
		if (config->isInputRedirect == TRUE)
		{
			// check if the input file is in the first pipeline
			if (index != 0)
			{
				fprintf(stderr, "Error: mislocated input redirection\n");
				return EXIT_FAILURE;
			}
			else
			{
				FILE *in_file = fopen(config->inputFile, "r");
				// check if the input file can be accessed
				if (in_file == NULL)
				{
					fprintf(stderr, "Error: cannot open input file\n");
					return EXIT_FAILURE;
				}
				fclose(in_file);
			}
		}
		pipeline->listOfConfig[index] = config;
		index++;
	}

	// finish the parse pipeline
	pipeline->listOfCommand = listPipe;
	return EXIT_SUCCESS;
}

/*
	execute the commands that are not built-in
*/
static void otherCommand(struct Config *config, char *firstArg, char **args)
{
	// redirect the output file if needed
	if (config->isOutputRedirect == TRUE)
	{
		redirectionOutput(config->outputFile);
	}

	// redirect the output file if needed
	if (config->isInputRedirect == TRUE)
	{
		redirectionInput(config->inputFile);
	}

	int status = execvp(firstArg, args);
	if (status < 0)
	{
		fprintf(stderr, "%s\n", "Error: command not found");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

/*
	run the command that only has one pipeline
*/
static void runOnePipeline(struct Config *config, struct Stack *stack, char *cmd)
{
	pid_t pid;
	int status = 0;

	char **args = config->listOfArgument;
	char *firstArg = args[0];
	// run built-in commands
	if (!strcmp(firstArg, "pwd"))
	{
		char cwd[CMDLINE_MAX];
		getcwd(cwd, CMDLINE_MAX);
		printf("%s\n", cwd);
	}
	else if (!strcmp(firstArg, "cd"))
	{
		int chdir_status = chdir(args[1]);
		if (chdir_status == -1)
		{
			fprintf(stderr, "%s\n", "Error: cannot cd into directory");
			fprintf(stderr, "+ completed '%s' [%d]\n",
					cmd, 1);
			return;
		}
	}
	else if (!strcmp(firstArg, "exit"))
	{
		fprintf(stderr, "Bye...\n");
		fprintf(stderr, "+ completed '%s' [%d]\n",
				cmd, status);

		exit(0);
	}
	else if (!strcmp(firstArg, "pushd"))
	{
		int chdir_status = chdir(args[1]);
		if (chdir_status == -1)
		{
			fprintf(stderr, "%s\n", "Error: no such directory");
			fprintf(stderr, "+ completed '%s' [%d]\n",
					cmd, 1);

			return;
		}
		char cwd[CMDLINE_MAX];
		getcwd(cwd, CMDLINE_MAX);
		pushd(stack, cwd);
	}
	else if (!strcmp(firstArg, "dirs"))
	{
		dirs(stack);
	}
	else if (!strcmp(firstArg, "popd"))
	{
		status = popd(stack);
		if (status == EXIT_FAILURE)
		{
			fprintf(stderr, "+ completed '%s' [%d]\n",
					cmd, 1);
			return;
		}
	}

	// run other command that needs fork()
	else
	{
		pid = fork();

		// Child
		if (pid == 0)
		{
			otherCommand(config, firstArg, args);
		}
		// Parent
		else if (pid > 0)
		{
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

/*
	run the entire command
*/
static void runCommand(char *cmd, struct Stack *stack)
{
	// parse pipeline
	struct Pipeline pipeline;
	memset(&pipeline, 0, sizeof(struct Pipeline));
	int parseStatus = parsePipe(&pipeline, cmd);
	if (parseStatus == EXIT_FAILURE)
	{
		return;
	}

	int pipeSize = pipeline.pipeSize;

	if (pipeSize == 0)
	{
		return;
	}

	// only 1 pipeline, then execute runOnePipeline to check if it's built-in command
	if (pipeSize == 1)
	{
		runOnePipeline(pipeline.listOfConfig[0], stack, pipeline.listOfCommand[0]);
		return;
	}

	// if pipelines are 2 or more, needs to set up the pipeline
	pid_t p[pipeSize];
	struct FD pipeFD[pipeSize - 1];
	memset(&pipeFD, 0, sizeof(struct FD) * pipeSize);

	// create enough pipeline
	for (int i = 0; i < pipeSize - 1; i++)
	{
		pipe(pipeFD[i].fd);
	}

	// fork and assign appropriate read and write ends for each pipeline
	for (int i = 0; i < pipeSize; i++)
	{
		p[i] = fork();

		// child process
		if (p[i] == 0)
		{
			char **args = pipeline.listOfConfig[i]->listOfArgument;
			char *firstArg = args[0];

			// the first pipeline
			if (i == 0)
			{
				dup2(pipeFD[i].fd[1], STDOUT_FILENO);
			}
			// the last pipeline
			else if (i == pipeSize - 1)
			{
				dup2(pipeFD[i - 1].fd[0], STDIN_FILENO);
			}
			// any pipeline in the middle
			else
			{
				dup2(pipeFD[i - 1].fd[0], STDIN_FILENO);
				dup2(pipeFD[i].fd[1], STDOUT_FILENO);
			}

			// close all the pipes for each child
			for (int i = 0; i < pipeSize - 1; i++)
			{

				close(pipeFD[i].fd[0]);
				close(pipeFD[i].fd[1]);
			}

			// execute the command after assign the pipes
			otherCommand(pipeline.listOfConfig[i], firstArg, args);

			exit(0);
		}
	}

	// close all the pipes for the parent
	for (int i = 0; i < pipeSize - 1; i++)
	{
		close(pipeFD[i].fd[0]);
		close(pipeFD[i].fd[1]);
	}

	// the parent waits for all children finish processing
	for (int i = 0; i < pipeSize; i++)
	{
		waitpid(p[i], &pipeline.status[i], 0);
	}

	// print the status for each child process
	fprintf(stderr, "+ completed '%s' ", cmd);
	for (int i = 0; i < pipeSize; i++)
	{
		fprintf(stderr, "[%d]", WEXITSTATUS(pipeline.status[i]));
	}
	fprintf(stderr, "\n");
}

int main(void)
{
	// set up the stack for directories
	char cmd[CMDLINE_MAX];
	struct Stack stack;
	memset(&stack, 0, sizeof(struct Stack));
	char cwd[CMDLINE_MAX];
	getcwd(cwd, CMDLINE_MAX);
	pushd(&stack, cwd);

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

		runCommand(cmd, &stack);
	}

	return EXIT_SUCCESS;
}