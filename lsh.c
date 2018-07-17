#include <stdio.h>

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define LSH_RL_BUFSIZE 1024		  // *lsh_read_line
#define LSH_TOK_BUFSIZE 64		  // **lsh_split_line
#define LSH_TOK_DELIM " \t\r\n\a" // **lsh_split_Line

/*
	Add some commands to the shell itself (Built-In).
*/
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = {
	"cd",
	"help",
	"exit",
};

int (*builtin_func[])(char **) = {
	&lsh_cd,
	&lsh_help,
	&lsh_exit};

int lsh_num_builtins()
{
	return sizeof(builtin_str) / sizeof(char *);
}

/*
	The implementations of built-in functions. 
*/

int lsh_cd(char **args)
{
	if (args[1] == NULL)
	{
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	}
	else
	{
		if (chdir(args[1]) != 0)
		{
			perror("lsh");
		}
	}
	return 1;
}

int lsh_help(char **args)
{
	int i;
	printf("My own LSH.\n");
	printf("Type program names and args, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < lsh_num_builtins(); i++)
	{
		printf("  %s\n", builtin_str[i]);
	}

	printf("Use the man command for info on other programs.");
	return 1;
}

int lsh_exit(char **args)
{
	return 0;
}

char *lsh_read_line(void)
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize); // Bytes
	int c;

	if (!buffer)
	{
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null char and return
		// That means 'Ctrl+D' or simply press 'Enter'
		if (c == EOF || c == '\n')
		{
			buffer[position] = '\0';
			return buffer;
		}
		else
		{
			buffer[position] = c;
		}
		position++;

		// If we've exceeded the buffer, reallocate.
		if (position >= bufsize)
		{
			bufsize += LSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer)
			{
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

char **lsh_split_line(char *line)
{
	/*
		parse that line into a list of args 
			to "tokenize" the string 
			using space or '\r\n' as delimeters
	*/
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char *));
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL)
	{
		tokens[position] = token;
		position++;

		if (position >= bufsize)
		{
			bufsize += LSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char *));
			if (!tokens)
			{
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int lsh_execute(char **args)
{
	int i;

	/*
		All this does is check if the cmd equals each builtin, 
			if so, 		run it.
			if not,		it calls 'lsh_launch()' to launch the process.

	*/

	if (args[0] == NULL)
	{
		// An empty command was entered.
		return 1;
	}

	for (i = 0; i < lsh_num_builtins(); i++)
	{
		if (strcmp(args[0], builtin_str[i]) == 0)
		{
			return (*builtin_func[i])(args);
		}
	}

	return lsh_launch(args);
}

void lsh_loop(void)
{
	/*
	To handle cmds with three steps:
		Read
		Parse
		Execute
	*/

	char *line;
	char **args;
	int status;

	do
	{
		printf("> ");
		line = lsh_read_line();
		args = lsh_split_line(line);
		status = lsh_execute(args);

		free(line);
		free(args);
	} while (status);
}

int main(int argc, char **argv)
{
	// We gonna build our own shell

	/* 
	What does shell do?
		Initialize	--	read & exec its conf, like .bashrc
		Interpret	--	read cmds & exec them
		Terminate 	--	exec shutdown cmds & free up memory
	*/

	lsh_loop();

	return EXIT_SUCCESS;
}

int lsh_launch(char **args)
{
	// *** How shells start processes ***

	/*
		There's two system calls for processes to get started.
		
		fork()	
			
			When the fork() is called,
				the OS makes a replicate of the process 
				and starts them both running. 
			
			The original process is called the 'parent',
				and the new one is called the 'child'.
			
			It returns 0 to the child process,
				and it returns to the parent the process ID number of its child.

		exec()
			
			When you call exec,
				the OS stops your process, 
				loads up the new program, and starts that one in its place.
	*/

	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0)
	{
		// Child process (using exec)
		if (execvp(args[0], args) == -1)
		{
			perror("lsh"); // invalid command argument
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		// Error forking
		perror("lsh"); // system's error message
	}
	else
	{
		// Parent process
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status)); // exited or killed
	}

	return 1;
}
