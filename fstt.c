#include<pty.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include"ttyfunc.h"
int main(int argl, char *argv[])
{
    int succ = 0;
    int pid;
    int master, slave;
    struct winsize tsz;
    const char *rstr = "80", *cstr = "24";
    char *spawn = NULL, *arg;
    char *attach = NULL;
    char empty[] = "";
    char ctrl = 0;
    if(argl == 1)
        spawn = empty;
    for(int i = 1; i < argl; ++i)
    {
        arg = argv[i];
        if(*arg == '-')
        {
            for(const char *it = arg + 1; *it != '\0'; ++it)
            {
                switch(*it)
                {
                    case's':
                        tsz.ws_row = atoi(rstr = argv[++i]);
                        tsz.ws_col = atoi(cstr = argv[++i]);
                        break;
                    case'c':
                        ctrl = 1;
                    case'a':
                        attach = argv[++i];
                        break;
                    default:
                        fprintf(stderr, "Unrecognized option %c will be ignored.\n", *it);
                }
            }
        }
        else
            spawn = arg;
    }
    if(ctrl)
    {
        succ = openpty(&master, &slave, NULL, NULL, &tsz);
        if(succ == 0)
        {
            pid = fork();
            if(pid > 0)
                succ = 0;
            else if(pid < 0)
                perror("fork failed");
            else
            {
                setsid();
                dup2(slave, STDIN_FILENO);
                dup2(slave, STDOUT_FILENO);
                dup2(slave, STDERR_FILENO);
                succ = ioctl(slave, TIOCSCTTY);
                if(succ < 0)
                    perror("ioctl failed");
                close(slave);
                perror("execl failed");
                exit(1);
            }
        }
        else
            perror("openpty failed");
    }
    else
    {
        if(spawn != NULL)
        {
            succ = maketty(spawn, rstr, cstr);
        }
        if(attach != NULL)
        {
        }
    }
    return succ * -1;
}
