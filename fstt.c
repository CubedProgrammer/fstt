// This file is part of fstt.
// Copyright (C) 2022, github.com/CubedProgrammer, owner of said account.

// fstt is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

// fstt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with fstt. If not, see <https://www.gnu.org/licenses/>. 

#include<fcntl.h>
#include<pty.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/ioctl.h>
#include<sys/select.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<time.h>
#include<unistd.h>
#include"attach.h"
#include"ttyfunc.h"

int mktmpdir(void)
{
    int succ = 0;
    const char *dirfmt = TMPPATH;
    char dirpath[2601];
    struct stat fdat;
    stat("/proc/self", &fdat);
    snprintf(dirpath, sizeof(dirpath), dirfmt, fdat.st_uid);
    if(access(dirpath, F_OK))
    {
        succ = mkdir(dirpath, 0755);
        if(succ)
            perror("Making temporary directory failed");
        else
        {
            snprintf(dirpath, sizeof(dirpath), CACHEPATH, fdat.st_uid);
            succ = mkdir(dirpath, 0755);
            snprintf(dirpath, sizeof(dirpath), IPIPEPATH, fdat.st_uid);
            succ += mkdir(dirpath, 0755);
            snprintf(dirpath, sizeof(dirpath), OPIPEPATH, fdat.st_uid);
            succ += mkdir(dirpath, 0755);
            if(succ)
                perror("Making temporary subdirectories failed");
        }
    }
    return succ;
}

int pipetty(int procfd, int ipipefd, int opipefd, int logfd, int pid, unsigned rcnt, unsigned ccnt)
{
    int succ = 0;
    int status, npid = 0;
    int ready, loopcnt = 0;
    unsigned currcol = 0;
    char buf[8192];
    int biggerfd = ipipefd > procfd ? ipipefd : procfd;
    size_t bc;
    fd_set fds, *fdsp = &fds;
    struct timeval tv, *tvp = &tv;
    while(npid == 0)
    {
        FD_ZERO(fdsp);
        FD_SET(procfd, fdsp);
        FD_SET(ipipefd, fdsp);
        tv.tv_sec = 300;
        tv.tv_usec = 0;
        ready = select(biggerfd + 1, fdsp, NULL, NULL, tvp);
        npid = waitpid(pid, &status, WNOHANG);
        if(npid == pid)
            continue;
        if(ready == -1)
            perror("select failed");
        else if(ready)
        {
            if(FD_ISSET(procfd, fdsp))
            {
                bc = read(procfd, buf, sizeof buf);
                if(bc == -1)
                    perror("read procfd failed");
                else
                {
                    if(logfd >= 0)
                        write(logfd, buf, bc);
                    write(opipefd, buf, bc);
                }
            }
            if(FD_ISSET(ipipefd, fdsp))
            {
                bc = read(ipipefd, buf, sizeof buf);
                if(bc == -1)
                    perror("read ipipefd failed");
                else
                    write(procfd, buf, bc);
            }
        }
    }
    if(npid != pid)
    {
        perror("waitpid failed");
        succ = -1;
    }
    return succ;
}

int main(int argl, char *argv[])
{
    int succ = mktmpdir();
    int pid;
    int master, slave;
    unsigned ttynum;
    int inpfd, onpfd;
    int logfd = -1;
    struct winsize tsz;
    const char *cstr = "80", *rstr = "24";
    const char *shell = getenv("SHELL"), *logfile = NULL;
    char *spawn = NULL, *arg;
    char *attach = NULL;
    char empty[] = "";
    char numstr[10];
    char path[2601];
    char ctrl = 0;
    signal(SIGINT, SIG_IGN);
    if(shell == NULL)
        shell = "bash";
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
                    case'-':
                        arg += 2;
                        it += strlen(arg);
                        if(strcmp(arg, "help") == 0)
                        {
                            puts("Version 0.2");
                            printf("%s [OPTIONS...] [TERMINAL NAME]\n", *argv);
                            puts("-s ROWS COLUMNS for the size of the spawned pseudoterminal.");
                            puts("-L to list information about all running pseudoterminals.");
                            puts("-l to list names of all running pseudoterminals.");
                            puts("-e SHELL to set the shell to run.");
                            puts("-d LOGFILE will log the output of the terminal to specified file.");
                            puts("-c SLAVE to create and control a terminal. DO NOT USE.");
                            puts("-a NAME to attach a terminal with specified name.");
                        }
                        else
                            fprintf(stderr, "Unrecognized long option --%s will be ignored.\n", arg);
                        spawn = NULL;
                        break;
                    case's':
                        tsz.ws_row = atoi(rstr = argv[++i]);
                        tsz.ws_col = atoi(cstr = argv[++i]);
                        break;
                    case'L':
                        list_tty(1);
                        spawn = NULL;
                        break;
                    case'l':
                        list_tty(0);
                        spawn = NULL;
                        break;
                    case'e':
                        shell = argv[++i];
                        break;
                    case'd':
                        logfile = argv[++i];
                        break;
                    case'c':
                        ctrl = 1;
                    case'a':
                        attach = argv[++i];
                        spawn = NULL;
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
                char dirpath[2601];
                struct stat fdat;
                stat("/proc/self", &fdat);
                snprintf(dirpath, sizeof(dirpath), IPIPEPATH, fdat.st_uid);
                size_t dirlen = strlen(dirpath);
                strcpy(path, dirpath);
                path[dirlen] = '/';
                strcpy(path + dirlen + 1, attach);
                inpfd = open(path, O_RDWR);
                snprintf(dirpath, sizeof(dirpath), OPIPEPATH, fdat.st_uid);
                dirlen = strlen(dirpath);
                strcpy(path, dirpath);
                path[dirlen] = '/';
                strcpy(path + dirlen + 1, attach);
                onpfd = open(path, O_RDWR);
                close(slave);
                if(inpfd > 0 && onpfd > 0)
                {
                    if(logfile != NULL)
                    {
                        logfd = open(logfile, O_WRONLY | O_CREAT, 0644);
                        if(logfd == -1)
                            perror("Opening log file failed");
                    }
                    succ = pipetty(master, inpfd, onpfd, logfd, pid, tsz.ws_row, tsz.ws_col);
                    close(inpfd);
                    close(onpfd);
                    unlink(path);
                    memcpy(path, IPIPEPATH, sizeof(IPIPEPATH) - 1);
                    unlink(path);
                    strcpy(path, CACHEPATH);
                    path[sizeof(CACHEPATH) - 1] = '/';
                    strcpy(path + sizeof(CACHEPATH), attach);
                    unlink(path);
                    if(logfd >= 0)
                        close(logfd);
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
            succ = maketty(spawn, rstr, cstr, shell, &ttynum, logfile);
            if(succ == -1)
            {
                fputs("Could not make terminal.\n", stderr);
                attach = NULL;
            }
            else if(*spawn == '\0')
            {
                sprintf(numstr, "%d", ttynum);
                attach = numstr;
            }
            else
                attach = spawn;
        }
        if(attach != NULL)
        {
            if(!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
                puts("Warning, standard input and standard output should both be interactive devices.");
            setvbuf(stdout, NULL, _IONBF, 0);
            succ = attach_tty(attach);
            if(succ)
                fprintf(stderr, "Could not attach terminal %s, check if it exists.\n", attach);
        }
    }
    return succ * -1;
}
