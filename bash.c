  GNU nano 9.0                                                                                                                                                                                                                                                                                                                                                                                                                                                                 main.c *                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
#include<fcntl.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>
#include <stdio.h>
void parse_command(char *input, char **args){
        int i =0;
        char *token = strtok(input, " \t\n");
        while (token != NULL)
        {
                args[i] = token;
                i++;
                token = strtok(NULL, " \t\n");
        }
        args[i] = NULL;
}
int count_pipes(char **args)
{
        int count = 0;
        for (int i = 0; args[i] != NULL; i++)
        {
                if(strcmp(args[i], "|")==0)
                {
                        count++;
                }
        }
        return count;
}
void split_commands(char **args, char *commands[][100], int num_commands)
{
        int command_index = 0;
        int argument_index = 0;
        for(int i = 0; args[i] != NULL; i++)
        {
                if(strcmp(args[i], "|") == 0)
                {
                        commands[command_index][argument_index] = NULL;
                        command_index++;
                        argument_index = 0;
                }
                else
                {
                        commands[command_index][argument_index] = args[i];
                        argument_index++;
                }
        }
        commands[command_index][argument_index] = NULL;
}
void setup_redirection(char **args)
{
        char *cmd_args[100];
        int j = 0;
        for(int i =0; args[i]!=NULL;i++)
        {
                if(strcmp(args[i], "<") == 0)
                {
                        int fd = open(args[i+1], O_RDONLY);
                        if(fd < 0)
                        {
                                perror("open");
                                return;
                        }
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                        i++;
                }
                else if (strcmp(args[i], ">") == 0)
                {
                        int fd = open( args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0)
                        {
                                perror("open");
                                return;
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        i++;
                }
                else if (strcmp(args[i], ">>") == 0)
                {
                        int fd = open( args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (fd<0)
                        {
                                perror("open");
                                return;
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        i++;
                }
                else
                {
                        cmd_args[j] = args[i];
                        j++;
                }
        }
        cmd_args[j] = NULL;
        execvp(cmd_args[0], cmd_args);
        perror("execvp");
}
void execute_pipeline(char *commands[][100], int num_commands)
{
        int pipes[99][2];
        pid_t pids[100];
        for(int i =0; i<num_commands -1; i++)
        {
                if(pipe(pipes[i])==-1)
                {
                        perror("pipe");
                        return;
                }
        }
        for (int i = 0; i<num_commands; i++)
        {
                pids[i] = fork();
                if(pids[i] == -1)
                {
                        perror("fork");
                        return;
                }
                if(pids[i] == 0)
                {
                        if(i>0)
                                dup2(pipes[i-1][0],STDIN_FILENO);
                        if(i<num_commands -1) dup2(pipes[i][1], STDOUT_FILENO);
                        for(int j = 0; j < num_commands - 1; j++)
                        {
                                close(pipes[j][0]);
                                close(pipes[j][1]);
                        }
                        setup_redirection(commands[i]);
                        return;
                }
        }
        for(int i = 0; i< num_commands - 1; i++)
        {
                close(pipes[i][0]);
                close(pipes[i][1]);
        }
        for(int i = 0; i < num_commands; i++)
        {
                waitpid(pids[i], NULL, 0);
        }
}
int main(){
        char input[1024];
        char *args[100];
        while(1){
                while(waitpid(-1, NULL, WNOHANG)>0)
                {
                }
                printf("myshell> ");
                fflush(stdout);
                fgets(input, sizeof(input), stdin);
                parse_command(input,args);
                if(args[0]==NULL) continue;
                int background = 0;
                int last = 0;
                while (args[last]!= NULL)
                {
                        last++;
                }
                if ( last > 0 && strcmp(args[last-1], "&") == 0)
                {
                        background = 1;
                        args[last-1] = NULL;
                }
                if(strcmp(args[0], "exit") == 0)
                {
                        break;
                }
                if(strcmp(args[0], "cd") == 0)
                {
                        if(args[1] == NULL)
                        {
                                printf("cd: missing argument\n");
                        }
                        else if(chdir(args[1])!=0)
                        {
                                perror("cd");
                        }
                        continue;
                }
                int pipe_count = count_pipes(args);
                if(pipe_count>0)
                {
                        int num_commands = pipe_count + 1;
                        char *commands[100][100];
                        split_commands(args, commands, num_commands);
                        execute_pipeline(commands, num_commands);
                        continue;
                }
                pid_t pid = fork();
                if(pid == 0){
                        char *cmd_args[100];
                        int j = 0;
                        for (int i = 0; args[i]!=NULL; i++)
                        {
                                if (strcmp(args[i], ">") == 0)
                                {
                                        int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                        if(fd<0)
                                        {
                                                perror("open");
                                                return 1;
                                        }
                                        dup2(fd, STDOUT_FILENO);
                                        close(fd);
                                        i++;
                                }
                                else if (strcmp(args[i], ">>") == 0)
                                {
                                        int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                                        if(fd<0)
                                        {
                                                perror("open");
                                                return 1;
                                        }
                                        dup2(fd, STDOUT_FILENO);
                                        close(fd);
                                        i++;
                                }
                                else if (strcmp(args[i], "<") == 0)
                                {
                                        int fd = open(args[i+1], O_RDONLY);
                                        if(fd<0)
                                        {
                                                perror("open");
                                                return 1;
                                        }
                                        dup2(fd, STDIN_FILENO);
                                        close(fd);
                                        i++;
                                }
                                else
                                {
                                        cmd_args[j] = args[i];
                                        j++;
                                }
                        }
                        cmd_args[j] = NULL;
                        execvp(args[0],cmd_args);
                        perror("execvp");
                        return 1;
                }
                else if(pid>0){
                        if(!background) waitpid(pid, NULL, 0);
                }
                else
                {
                        perror("fork");

                }
        }
        return 0;
}

