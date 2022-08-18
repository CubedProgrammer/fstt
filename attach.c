// This file is part of fstt.
// Copyright (C) 2022, github.com/CubedProgrammer, owner of said account.

// fstt is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

// fstt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with fstt. If not, see <https://www.gnu.org/licenses/>. 

#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/ioctl.h>
#include<sys/select.h>
#include<termios.h>
#include<unistd.h>
#include"ttyfunc.h"

ssize_t process_input(char *ctlmodep, char *detachp, char *buf, size_t cnt)
{
    ssize_t succcnt = cnt;
    size_t shift = 0;
    for(char *it = buf; it != buf + cnt; ++it)
    {
        if(shift)
            *it = it[shift];
        if(*ctlmodep == 1)
        {
            switch(*it)
            {
                case'D':
                case'd':
                    *detachp = 1;
                    cnt = it - buf + 1;
                    succcnt = cnt - 1;
                    break;
                default:
                    break;
            }
        }
        // 2 is Ctrl+B
        if(!*ctlmodep && *it == 2)
        {
            --succcnt, --cnt;
            ++shift, --it;
            *ctlmodep = 1;
        }
    }
    return succcnt;
}

int attach_tty(const char *name)
{
    int succ = 0;
    struct winsize tsz;
    struct termios old, curr;
    unsigned width, height;
    char lfbuf[2704], *lfarr;
    char path[2601], cbuf[8192];
    size_t bc;
    int ipipefd, opipefd, ready;
    int status;
    fd_set fds, *fdsp = &fds;
    struct timeval tv, *tvp = &tv;
    char detached, ctlmode = 0;
    tcgetattr(STDIN_FILENO, &old);
    memcpy(&curr, &old, sizeof(struct termios));
    curr.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &curr);
    succ = ioctl(STDIN_FILENO, TIOCGWINSZ, &tsz);
    if(succ == 0)
    {
        width = tsz.ws_col;
        height = tsz.ws_row;
        if(height > sizeof lfbuf)
        {
            lfarr = malloc(height);
            if(lfarr == NULL)
                perror("malloc failed");
        }
        else
            lfarr = lfbuf;
        if(lfarr != NULL)
        {
            strcpy(path, IPIPEPATH);
            path[sizeof(IPIPEPATH) - 1] = '/';
            strcpy(path + sizeof(IPIPEPATH), name);
            opipefd = open(path, O_WRONLY);
            memcpy(path, OPIPEPATH, sizeof(OPIPEPATH) - 1);
            ipipefd = open(path, O_RDONLY);
            strcpy(path, CACHEPATH);
            path[sizeof(CACHEPATH) - 1] = '/';
            strcpy(path + sizeof(CACHEPATH), name);
            if(opipefd < 0)
                perror("opening named pipe failed");
            else
            {
                memset(lfarr, '\n', height - 1);
                lfarr[height - 1] = '\0';
                fputs(lfarr, stdout);
                if(lfarr != lfbuf)
                    free(lfarr);
                printf("\033\133%uA", height - 1);
                detached = 0;
                while(!detached)
                {
                    FD_ZERO(fdsp);
                    FD_SET(ipipefd, fdsp);
                    FD_SET(STDIN_FILENO, fdsp);
                    tv.tv_sec = 240;
                    tv.tv_usec = 0;
                    ready = select(ipipefd + 1, fdsp, NULL, NULL, tvp);
                    if(ready == -1)
                        perror("select failed");
                    else if(ready)
                    {
                        if(FD_ISSET(ipipefd, fdsp))
                        {
                            bc = read(ipipefd, cbuf, sizeof cbuf);
                            if(bc != -1)
                                write(STDOUT_FILENO, cbuf, bc);
                        }
                        if(FD_ISSET(STDIN_FILENO, fdsp))
                        {
                            bc = read(STDIN_FILENO, cbuf, sizeof cbuf);
                            if(bc != -1)
                            {
                                bc = process_input(&ctlmode, &detached, cbuf, bc);
                                write(opipefd, cbuf, bc);
                            }
                        }
                    }
                    if(access(path, F_OK))
                        detached = 2;
                }
                if(detached == 1)
                    printf("Detached from session %s.\n", name);
                else
                    puts("Exited");
                close(ipipefd);
                close(opipefd);
            }
        }
        else
            succ = -1;
    }
    else
        perror("ioctl TIOCGWINSZ failed");
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return succ;
}
