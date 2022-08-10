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
#include<termios.h>
#include<unistd.h>
#include"ttyfunc.h"

int attach_tty(const char *name)
{
    int succ = 0;
    struct winsize tsz;
    struct termios old, curr;
    unsigned width, height;
    char lfbuf[2704], *lfarr;
    char path[2601];
    int pipefd;
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
            strcpy(path, PIPEPATH);
            path[sizeof(PIPEPATH) - 1] = '/';
            strcpy(path + sizeof(PIPEPATH), name);
            pipefd = open(path, O_RDONLY);
            memset(lfarr, '\n', height - 1);
            lfarr[height - 1] = '\0';
            fputs(lfarr, stdout);
            if(lfarr != lfbuf)
                free(lfarr);
            printf("\033\133%uA", height - 1);
        }
        else
            succ = -1;
    }
    else
        perror("ioctl TIOCGWINSZ failed");
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return succ;
}
