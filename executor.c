#include "minishell.h"

int	is_builtin_command(char *cmd)
{
	char	*builtins[] = {"cd", "echo", "pwd", "export", "unset", "env",
			"exit", NULL};
	int		i;

	i = -1;
	while (builtins[++i])
	{
		if (ft_strcmp(builtins[i], cmd) == 0)
			return (1);
	}
	return (0);
}

int	execute_builtin(t_command *cmd, char **envp)
{
	char	*command;

	command = cmd->args[0];
	if (ft_strcmp(command, "cd") == 0)
		return (builtin_cd(cmd));
	else if (strcmp(command, "echo") == 0)
		return (builtin_echo(cmd));
	else if (strcmp(command, "pwd") == 0)
		return (builtin_pwd());
	else if (strcmp(command, "export") == 0)
		return (builtin_export(cmd, envp));
	else if (strcmp(command, "unset") == 0)
		return (builtin_unset(cmd));
	else if (strcmp(command, "env") == 0)
		return (builtin_env(envp));
	else if (strcmp(command, "exit") == 0)
		return (builtin_exit(cmd));
	return (1);
}

static int	setup_pipe(int pipe_fd[2])
{
	if (pipe(pipe_fd) == -1)
	{
		perror("minishell: pipe error");
		return (-1);
	}
	return (0);
}

static void	child_process(t_command *current, int prev_pipe_read,
		int pipe_fd[2], char **envp)
{
	if (prev_pipe_read != -1)
	{
		if (dup2(prev_pipe_read, STDIN_FILENO) == -1)
		{
			perror("minishell: dup2 error on input");
			exit(1);
		}
		safe_close(&prev_pipe_read);
	}
	if (current->next)
	{
		safe_close(&pipe_fd[0]);
		if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
		{
			perror("minishell: dup2 error on output");
			exit(1);
		}
		safe_close(&pipe_fd[1]);
	}
	if (setup_redirections(current) == -1)
		exit(1);
	execute_single_command(current, envp);
	fprintf(stderr, "minishell: command execution failed\n");
	exit(127);
}

void	execute_single_command(t_command *current, char **envp)
{
	char	*exec_path;

	if (current->args && current->args[0]
		&& is_builtin_command(current->args[0]))
		exit(execute_builtin(current, envp));
	else if (current->args && current->args[0])
	{
		exec_path = find_executable_path(current->args[0], envp);
		if (!exec_path)
			exec_path = ft_strdup(current->args[0]);
		if (!exec_path)
		{
			fprintf(stderr, "minishell: %s: command not found\n",
				current->args[0]);
			exit(127);
		}
		execve(exec_path, current->args, envp);
		fprintf(stderr, "minishell: %s: %s\n", current->args[0],
			strerror(errno));
		free(exec_path);
		exit(127);
	}
	exit(0);
}

static int	wait_for_children(void)
{
	pid_t	last_pid;
	int		status;

	status = 0;
	while ((last_pid = waitpid(-1, &status, 0)) > 0)
		;
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	else
		return (1);
}

static int	parent_process(int prev_pipe_read, int pipe_fd[2])
{
	if (prev_pipe_read != -1)
		close(prev_pipe_read);
	if (pipe_fd[1] != -1)
	{
		close(pipe_fd[1]);
		return (pipe_fd[0]);
	}
	if (pipe_fd[0] != -1)
		return (pipe_fd[0]);
	return (-1);
}
static void	handle_fork_error(t_command *current, int prev_pipe_read,
		int pipe_fd[2])
{
	perror("minishell: fork error");
	safe_close(&prev_pipe_read);
	if (current->next)
	{
		safe_close(&pipe_fd[0]);
		safe_close(&pipe_fd[1]);
	}
}

int	setup_command_pipe(t_command *current, int *prev_pipe_read, int pipe_fd[2])
{
	if (current->next)
	{
		if (setup_pipe(pipe_fd) == -1)
		{
			safe_close(prev_pipe_read);
			return (0);
		}
	}
	else
	{
		pipe_fd[0] = -1;
		pipe_fd[1] = -1;
	}
	return (1);
}
int	setup_all_heredocs(t_command *cmd_list)
{
	t_command		*current;
	t_redirections	*redir;
	int				heredoc_fd;

	current = cmd_list;
	while (current)
	{
		redir = current->redirections;
		while (redir)
		{
			if (redir->type == TOKEN_HEREDOC)
			{
				heredoc_fd = setup_heredoc(redir->file);
				if (heredoc_fd == -1)
					return (-1);
				redir->heredoc_fd = heredoc_fd;
			}
			redir = redir->next;
		}
		current = current->next;
	}
	return 0;
}

int	execute_command_list(t_command *cmd_list, char **envp)
{
	int				pipe_fd[2];
	pid_t			pid;
	int				prev_pipe_read;
	t_command		*current;
	t_redirections	*redir;

	if (setup_all_heredocs(cmd_list) == -1)
		return 1;
	pipe_fd[0] = -1;
	pipe_fd[1] = -1;
	prev_pipe_read = -1;
	current = cmd_list;
	while (current)
	{
		if (!setup_command_pipe(current, &prev_pipe_read, pipe_fd))
			return (1);
		expand_command_args(current, envp);
		pid = fork();
		if (pid == -1)
		{
			handle_fork_error(current, prev_pipe_read, pipe_fd);
			return (1);
		}
		if (pid == 0)
			child_process(current, prev_pipe_read, pipe_fd, envp);
		redir = current->redirections;
		while (redir)
		{
			if (redir->type == TOKEN_HEREDOC && redir->heredoc_fd >= 0)
				safe_close(&redir->heredoc_fd);
			redir = redir->next;
		}
		prev_pipe_read = parent_process(prev_pipe_read, pipe_fd);
		current = current->next;
	}
	safe_close(&prev_pipe_read);
	return (wait_for_children());
}
