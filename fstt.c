// This file is part of fstt.
// Copyright (C) 2022, github.com/CubedProgrammer, owner of said account.

// fstt is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

// fstt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with fstt. If not, see <https://www.gnu.org/licenses/>. 

#include<fcntl.h>
#include<pty.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/ioctl.h>
#include<sys/select.h>
#include<sys/wait.h>
#include<time.h>
#include<unistd.h>
#include"attach.h"
#include"ttyfunc.h"

int pipetty(int procfd, int pipefd, int pid, unsigned rcnt, unsigned ccnt)
{
    int succ = 0;
    int status, npid = 0;
    int ready;
    unsigned currcol = 0;
    char buf[8192];
    int biggerfd = procfd > pipefd ? procfd : pipefd;
    size_t bc;
    fd_set fds, *fdsp = &fds;
    struct timeval tv, *tvp = &tv;
    while(npid == 0)
    {
        FD_ZERO(fdsp);
        FD_SET(procfd, fdsp);
        FD_SET(pipefd, fdsp);
        tv.tv_sec = 300;
        tv.tv_usec = 0;
        ready = select(biggerfd + 1, fdsp, NULL, NULL, tvp);
        if(ready == -1)
            perror("select failed");
        else if(ready)
        {
            if(FD_ISSET(procfd, fdsp))
            {
                bc = read(procfd, buf, sizeof buf);
                printf("%zu bytes read from procfd", bc);
                write(pipefd, buf, bc);
            }
            if(FD_ISSET(pipefd, fdsp))
            {
                bc = read(pipefd, buf, sizeof buf);
                printf("%zu bytes read from pipefd", bc);
                write(procfd, buf, bc);
            }
        }
        npid = waitpid(pid, &status, WNOHANG);
    }
    if(npid == -1)
    {
        perror("waitpid failed");
        succ = -1;
    }
    printf("%d %d pid npid", pid, npid);
    return succ;
}

int main(int argl, char *argv[])
{
    int succ = 0;
    int pid;
    int master, slave;
    struct winsize tsz;
    const char *rstr = "80", *cstr = "24";
    const char *shell = "bash";
    char *spawn = NULL, *arg;
    char *attach = NULL;
    char empty[] = "";
    char numstr[10];
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
                    case'l':
                        list_tty();
                        break;
                    case'e':
                        shell = argv[++i];
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
            {
                int npfd = open(attach, O_RDONLY);
                close(slave);
                if(npfd > 0)
                {
                    succ = pipetty(master, npfd, pid, tsz.ws_row, tsz.ws_col);
                    close(npfd);
                    printf("%s and process %i has terminated.\n", attach, pid);
                }
                else
                    perror("open failed");
            }
            else if(pid < 0)
                perror("fork failed");
            else
            {
                close(master);
                setsid();
                dup2(slave, STDIN_FILENO);
                dup2(slave, STDOUT_FILENO);
                dup2(slave, STDERR_FILENO);
                succ = ioctl(slave, TIOCSCTTY);
                if(succ < 0)
                    perror("ioctl failed");
                succ = ioctl(slave, TIOCSWINSZ, &tsz);
                if(succ < 0)
                    perror("ioctl failed");
                close(slave);
                execlp(shell, shell, (char*)NULL);
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
            succ = maketty(spawn, rstr, cstr, shell);
            if(succ == -1)
            {
                fputs("Could not make terminal.\n", stderr);
                attach = NULL;
            }
            else if(*spawn == '\0')
            {
                sprintf(numstr, "%d", succ);
                attach = numstr;
            }
        }
        if(attach != NULL)
        {
            setvbuf(stdout, NULL, _IONBF, 0);
            succ = attach_tty(attach);
            if(succ)
                fprintf(stderr, "Could not attach terminal %s, check if it exists.\n", attach);
        }
    }
    return succ * -1;
}
