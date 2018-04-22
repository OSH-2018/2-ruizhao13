#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;

int number_pipes = 0;
int run_cmd(char *args[128] ){

    int i;
    /* 内建命令 */
    if (strcmp(args[0], "cd") == 0) {
        if (args[1])
            chdir(args[1]);
        return 0;
    }
    if (strcmp(args[0], "pwd") == 0) {
        char wd[4096];
        puts(getcwd(wd, 4096));
        return 0;
    }
    char name[128];
    char value[128];
    if(strcmp(args[0], "export") == 0) {

        for(i = 0; args[1][i] != '='; i++) {
            name[i] = args[1][i];
        }
        int k;
        for(k = 0, i++; args[1][i]; i++, k++){
            value[k] = args[1][i];
        }

        setenv(name, value,1);

    }
    if (strcmp(args[0], "exit") == 0)
        return 1;

    /* 外部命令 */
    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程 */
        execvp(args[0], args);
        /* execvp失败 */
        return 255;
    }
    /* 父进程 */
    wait(NULL);
}

int run_shell(char *args[128], int curr_pipe, int pipe_locate[128]){
    if(curr_pipe == 0)
        execlp(args[pipe_locate[0] + 1], args[pipe_locate[0] + 1], NULL);
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0){/* child process */
        /* begin to run the prior cmd of the pipe */
        close(fd[PIPE_READ]);
        dup2(fd[PIPE_WRITE], STDOUT_FILENO);
        close(fd[PIPE_WRITE]);
        run_shell(args, curr_pipe - 1, pipe_locate);
    }
    else{/* parent process */
        wait(NULL);
        /* begin to run the latter cmd of the pipe */
        close(fd[PIPE_WRITE]);
        dup2(fd[PIPE_READ], STDIN_FILENO);
        close(fd[PIPE_READ]);

        char *latter_cmd[128];

        latter_cmd[0] = args[pipe_locate[curr_pipe] + 1];
        int i;
        for (i = 0; latter_cmd[i][0] != '|' && args[pipe_locate[curr_pipe] + 1 + i]; i++){
            for (latter_cmd[i+1] = latter_cmd[i] + 1; latter_cmd[i+1] != NULL; latter_cmd[i+1]++){
                if (*latter_cmd[i+1] == '\0') {
                    latter_cmd[i+1]++;
                    break;
                }
            }
        }
        latter_cmd[i] = NULL;
        if (curr_pipe == number_pipes)
        {
            run_cmd(latter_cmd);
        }
        else{
            run_cmd(latter_cmd);
            _exit(0);
        }
    }
    return 1;
}


int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];
    while (1) {


        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);

        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        char raw_cmd[256];
        strncpy(raw_cmd, cmd, 256);
        int pipe_locate[256];
        for (i = 0; i < 256; ++i)
        {
            pipe_locate[i] = -1;
        }
        /* 拆解命令行 */
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
                if (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    args[i+1]++;
                    break;
                }
        args[i] = NULL;
        int n = i;
        /* 没有输入命令 */
        if (!args[0])
            continue;

        /*examine whether pipe*/
        number_pipes = 0;
        for(i = 0;i<n;i++){
            if(args[i][0] == '|' && args[i][1] == '\0'){
                number_pipes += 1;
                pipe_locate[number_pipes] = i;
            }
        }
        if(number_pipes == 0){
            int result = run_cmd(args);
            if(result == -1){
                return 0;
            }
        }
        else {
            int u_in, u_out;
            u_in = dup(STDIN_FILENO);
            u_out = dup(STDOUT_FILENO);
            run_shell(args, number_pipes, pipe_locate);
            dup2(u_in, STDIN_FILENO);
            dup2(u_out, STDOUT_FILENO);
            continue;
        }

    }
}