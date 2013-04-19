/**
 * Operating Sytems 2013 - Assignment 1
 * Nume: Olteanu Vasilica - Claudiu
 * Grupa: 332CA
 */

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define READ		0
#define WRITE		1

/**
 * Internal change-directory command.
 * @param dir : directory name
 */
static bool shell_cd(word_t *dir)
{
	/* execute cd */
	int rc;
	
	rc = chdir(dir->string);

	if(rc < 0) {
		printf("bash: cd: %s: No such file or directory\n", dir->string);
		return EXIT_FAILURE;
	} else
		return EXIT_SUCCESS;
}

/**
 * Internal exit/quit command.
 * @param cmd : the command
 */
static int shell_exit(const char *cmd)
{
	/* execute exit/quit */
	if ((strcmp(cmd, "exit") == 0) || (strcmp(cmd, "quit") == 0))
		return 1;
	return 0; 
}

/**
 * Concatenate parts of the word to obtain the command
 */
static char *get_word(word_t *s)
{
	int string_length = 0;
	int substring_length = 0;

	char *string = NULL;
	char *substring = NULL;

	while (s != NULL) {
		substring = strdup(s->string);

		if (substring == NULL) {
			return NULL;
		}

		if (s->expand == true) {
			char *aux = substring;
			substring = getenv(substring);

			/* prevents strlen from failing */
			if (substring == NULL) {
				substring = calloc(1, sizeof(char));
				if (substring == NULL) {
					free(aux);
					return NULL;
				}
			}

			free(aux);
		}

		substring_length = strlen(substring);

		string = realloc(string, string_length + substring_length + 1);
		if (string == NULL) {
			if (substring != NULL)
				free(substring);
			return NULL;
		}

		memset(string + string_length, 0, substring_length + 1);

		strcat(string, substring);
		string_length += substring_length;

		if (s->expand == false) {
			free(substring);
		}

		s = s->next_part;
	}

	return string;
}


/**
 * Concatenate command arguments in a NULL terminated list in order to pass
 * them directly to execv.
 * @param command: arguments list
 * @param size: number of the arguments
 */
static char **get_argv(simple_command_t *command, int *size)
{
	char **argv;
	word_t *param;

	int argc = 0;
	argv = calloc(argc + 1, sizeof(char *));
	assert(argv != NULL);

	argv[argc] = get_word(command->verb);
	assert(argv[argc] != NULL);

	argc++;

	param = command->params;
	while (param != NULL) {
		argv = realloc(argv, (argc + 1) * sizeof(char *));
		assert(argv != NULL);

		argv[argc] = get_word(param);
		assert(argv[argc] != NULL);

		param = param->next_word;
		argc++;
	}

	argv = realloc(argv, (argc + 1) * sizeof(char *));
	assert(argv != NULL);

	argv[argc] = NULL;
	*size = argc;

	return argv;
}

/** 
 * Redirect the input/output/stderr to the file and save the file descriptors.
 * @param filedes: std file descriptor 
 * @param filename: file name
 * @param mode: APPEND/TRUNCATE
 * @param file_fd: redirected files
 * @param stds_fd: std file descriptors
 */ 
static void redirect(int filedes, const char *filename, int mode, int *files_fd, int *stds_fd) {
	int rc;

	if(filename != NULL) {
		if(filedes == STDIN_FILENO)
			files_fd[0] = open(filename, O_RDWR);
		else {
			if (mode != IO_REGULAR) 
				files_fd[filedes] = open(filename, O_CREAT | O_APPEND | O_RDWR, 0644);
			else
				files_fd[filedes] = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
		}

		assert(files_fd[filedes] >= 0);

		stds_fd[filedes] = dup(filedes);
		assert(stds_fd[filedes] >= 0);

		rc = dup2(files_fd[filedes], filedes);
		assert(rc >= 0);
	} else {
		rc = dup2(stds_fd[filedes], filedes);
		assert(rc >= 0);

		if(files_fd[filedes] != -1) {
			rc = close(files_fd[filedes]);
			assert(rc >= 0);
		}

		rc = close(stds_fd[filedes]);
		assert(rc >= 0);
	}
}

/**
 * Duplicate the stderr and redirect it to stdout
 * @param stds_fd: redirected file descriptors
 */
static void duplicate_fd(int *stds_fd) {
	int rc;
	stds_fd[STDERR_FILENO] = dup(STDERR_FILENO);
	assert(stds_fd[STDERR_FILENO] >= 0);

	rc = dup2(STDOUT_FILENO, STDERR_FILENO);
	assert(rc >= 0);
}

/**
 * Checks for redirections and make them.
 * @param s: command reference
 * @param files_fd: regular file descriptors
 * @param stds_fd: std file descriptors  
 */
static bool do_redirections(simple_command_t *s, int *files_fd, int *stds_fd) {
	bool redirected = false;
	char *filename;
	char *filename2;

	if(s->in != NULL) {
		redirected = true;
		filename = get_word(s->in);
		redirect(STDIN_FILENO, filename, s->io_flags, files_fd, stds_fd);
		free(filename);
		filename = NULL;
	}
	
	if(s->out != NULL) {
		redirected = true;
		filename = get_word(s->out);
		redirect(STDOUT_FILENO, filename, s->io_flags, files_fd, stds_fd);
		free(filename);
		filename = NULL;
	}

	if(s->err != NULL) {
		filename = get_word(s->err);
		filename2 = get_word(s->out);
		if(redirected == true && strcmp(filename, filename2) == 0) {
			duplicate_fd(stds_fd);
		} else {
			redirected = true;
			redirect(STDERR_FILENO, filename, s->io_flags, files_fd, stds_fd);
		}
		free(filename);
		free(filename2);
		filename = NULL;
		filename2 = NULL;
	}

	return redirected;
}


/**
 * Restore shell redirections to std.
 * @param s: command reference
 * @param files_fd: regular file descriptors
 * @param stds_fd: std file descriptors  
 */
static void undo_redirections(simple_command_t *s, int *files_fd, int *stds_fd) {

	if(s->in != NULL) {
		redirect(STDIN_FILENO, NULL, 0, files_fd, stds_fd);
	} 

	if(s->out != NULL) {
		redirect(STDOUT_FILENO, NULL, 0, files_fd, stds_fd);
	}

	if(s->err != NULL) {
		redirect(STDERR_FILENO, NULL, 0, files_fd, stds_fd);
	}

}

/**
 * Set the environment variable.
 * @param name: name of the variable
 * @param value: value of the variable
 */
static int set_var(const char *name, const char *value)
{
	int rc;
	rc = setenv(name, value, 1);
	DIE(rc < 0, "setenv error");

	if(rc >= 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	pid_t pid;
	int status, count, rc;
	int files_fd[3] = {-1, -1, -1} , stds_fd[3] = {-1, -1, -1};
	char **argv;
	bool redirected = false;

	if(shell_exit(s->verb->string))
		return SHELL_EXIT;
	
	/* redirections check */
	redirected = do_redirections(s, files_fd, stds_fd);

	/* change-directory check*/
	if(strcmp(s->verb->string, "cd") == 0) {
		rc = shell_cd(s->params);
		if(redirected == true)
			undo_redirections(s, files_fd, stds_fd);

		return rc;

	}

	/* enviroment check */
	if(s->verb->next_part != NULL) {
		return set_var(s->verb->string, s->verb->next_part->next_part->string);
	}
	
	argv = get_argv(s, &count);
    pid = fork();

    switch(pid) {
    	case - 1:
    		perror("Cannot proceed. fork() error");
			break;
		case 0:
			/* execute the command with the specific arguments */
		    rc = execvp(s->verb->string, argv);
    		printf("Execution failed for '%s'\n", s->verb->string);
    		
			/* free memory */
			for (rc = 0 ; rc < count; rc++)
				free(argv[rc]);
			free(argv);
			argv = NULL;
			free_parse_memory();
			
    		exit(EXIT_FAILURE);
			break;
		default:
			break;
    }

	wait(&status);
	if (!WIFEXITED(status))
		printf("Child %d doesn't terminated normally, with code %d\n",
			 pid, WEXITSTATUS(status));

	/* restore redirections check */
	if (redirected == true) 
		undo_redirections(s, files_fd, stds_fd);

	/* free memory */
	for (rc = 0 ; rc < count; rc++)
		free(argv[rc]);
	free(argv);
	argv = NULL;
	
	/* return child exited status */
	if(status == 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}


/**
 * Process two commands in parallel, by creating two children.
 */
static bool do_in_parallel(command_t *cmd1, command_t *cmd2, int level, command_t *father)
{
	pid_t pid1, pid2;
	int rc, status1, status2;

	pid1 = fork();

	switch(pid1) {
		case -1:
			perror("Cannot proceed. fork() error");
			break;

		case 0:
			/* execute the first child */
			rc = parse_command(cmd1, level, father);

			DIE(rc < 0, "Execute cmd1 error");
    		exit(rc);

			break;

		default:
			pid2 = fork();
			switch(pid2) {
				case -1:
					perror("Cannot proceed. fork() error");
					break;

				case 0:
					/* execute the second child */
					rc = parse_command(cmd2, level, father);

					DIE(rc < 0, "Execute cmd2 error");
					exit(rc);
					break;

				default:
					/* wait for both childs*/
					waitpid(pid1, &status1, 0);
					if (!WIFEXITED(status1))
						printf("The other process (%d) doesn't terminated normally, with code %d\n",
							 pid1, WEXITSTATUS(status1));
				
					waitpid(pid2, &status2, 0);
					if (!WIFEXITED(status2))
						printf("The other process (%d) doesn't terminated normally, with code %d\n",
							 pid1, WEXITSTATUS(status2));
					break;
			}
	}

	/* return children exited status */
	if(status1 == 0 && status2 == 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}





/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2)
 */
static bool do_on_pipe(command_t *cmd1, command_t *cmd2, int level, command_t *father)
{
	int filedes[2], rc, status1, status2;
	pid_t pid1, pid2;

	/* create a pipe */
	rc = pipe(filedes);
	assert(rc >= 0);

	pid1 = fork();

	switch(pid1) {
		case -1:
			perror("Cannot proceed. fork() error");
			break;

		case 0:
			/* redirect the output-ul to the pipe */
			close(filedes[0]);
			rc = dup2(filedes[1], STDOUT_FILENO);
			DIE(rc < 0, "pipe out error");

			/* check for simple commands/multiple commands */
			if(cmd1->op == OP_PIPE)
				rc = do_on_pipe(cmd1->cmd1, cmd1->cmd2, 5, cmd1);
			else
				rc = parse_simple(cmd1->scmd, 0,cmd1->up);

			DIE(rc < 0, "execute error");

			rc = close(filedes[1]);
			DIE(rc < 0, "closing file 1 error");
    		
    		exit(EXIT_FAILURE);

			break;

		default:
			pid2 = fork();

			switch(pid2) {
				case -1:
					perror("Cannot proceed. fork() error");
					break;

				case 0:
					/* redirect the input-ul to the pipe */
					close(filedes[1]);
					rc = dup2(filedes[0], STDIN_FILENO);
					DIE(rc < 0, "pipe in error");
					if(cmd2->op == OP_NONE) 
						rc = parse_simple(cmd2->scmd, 0,cmd2->up);
					else
						rc = do_on_pipe(cmd2->cmd1, cmd2->cmd2, 5, cmd2);

					DIE(rc < 0, "cmd2 execution error");
					rc = close(filedes[0]);
					DIE(rc < 0, "closing file 0 error");
		    		
		    		exit(EXIT_FAILURE);
		    		break;

		    	default:
		    		/* wait for the first child and close the pipe */
		    		wait(&status1);
		    		if (!WIFEXITED(status1))
						printf("The other process (%d) doesn't terminated normally, with code %d\n",
							 pid1, WEXITSTATUS(status1));
		    		
		    		rc = close(filedes[1]);
		    		DIE(rc < 0, "closing file 0 error");
		    		rc = close(filedes[0]);
					DIE(rc < 0, "closing file 0 error");

					/* wait for the second child */
					wait(&status2);
					if (!WIFEXITED(status2))
						printf("The other process (%d) doesn't terminated normally, with code %d\n",
							 pid1, WEXITSTATUS(status1));
		    		
					DIE(rc < 0, "closing file 0 error");
					break;
			}
	}

	/* return children exited status  */
	if(status1 == 0 && status2 == 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}


/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	int rc1, rc2, status;
	pid_t pid;
	if (c->op == OP_NONE) {
		/* execute a simple command */
		return parse_simple(c->scmd, 1, NULL);
	}

	switch (c->op) {
	case OP_SEQUENTIAL:
		/* execute a secquential command */
		rc1 = parse_command(c->cmd1, 2, c);
		rc2 = parse_command(c->cmd2, 2, c);

		if(rc1 == EXIT_SUCCESS || rc2 == EXIT_SUCCESS)
			return EXIT_SUCCESS;
		else
			return EXIT_FAILURE;
		break;

	case OP_PARALLEL:
		/* execute the commands simultaneously */
		return do_in_parallel(c->cmd1, c->cmd2, 3, c->up);
		break;

	case OP_CONDITIONAL_NZERO:
		/* execute the second command only if the first one
                 * returns non zero */		

		rc1 = parse_command(c->cmd1, 4, c->up);

		if(rc1 == EXIT_FAILURE) {
			rc1 = parse_command(c->cmd2, 4, c->up);
		}
		return rc1;

		break;

	case OP_CONDITIONAL_ZERO:
		/* execute the second command only if the first one
                 * returns zero */


		rc1 = parse_command(c->cmd1, 4, c->up);
		
		if (rc1 == EXIT_SUCCESS) {
			rc1 = parse_command(c->cmd2, 4, c->up);
		}
		return rc1;

		break;

	case OP_PIPE:
		/* execute 2 commands connected by a pipe */
		pid = fork();
		switch(pid) {
			case -1:
				perror("Cannot proceed. fork() error");
				break;

			case 0:
				rc1 = do_on_pipe(c->cmd1, c->cmd2, 5, c->up);
				exit(rc1);

				break;

			default:

	    		wait(&status);

	    		if (!WIFEXITED(status))
					printf("The other process (%d) doesn't terminated normally, with code %d\n",
						 pid, WEXITSTATUS(status));
				return !WIFEXITED(status);
				break;
		}
		
		break;

	default:
		assert(false);
	}

	return 0;
}

/**
 * Readline from mini-shell.
 */
char *read_line()
{
	char *instr;
	char *chunk;
	char *ret;

	int instr_length;
	int chunk_length;

	int endline = 0;

	instr = NULL;
	instr_length = 0;

	chunk = calloc(CHUNK_SIZE, sizeof(char));
	if (chunk == NULL) {
		fprintf(stderr, ERR_ALLOCATION);
		return instr;
	}

	while (!endline) {
		ret = fgets(chunk, CHUNK_SIZE, stdin);
		if (ret == NULL) {
			break;
		}

		chunk_length = strlen(chunk);
		if (chunk[chunk_length - 1] == '\n') {
			chunk[chunk_length - 1] = 0;
			endline = 1;
		}

		ret = instr;
		instr = realloc(instr, instr_length + CHUNK_SIZE + 1);
		if (instr == NULL) {
			free(ret);
			return instr;
		}
		memset(instr + instr_length, 0, CHUNK_SIZE);
		strcat(instr, chunk);
		instr_length += chunk_length;
	}

	free(chunk);

	return instr;
}
