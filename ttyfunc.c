// This file is part of fstt.
// Copyright (C) 2022, github.com/CubedProgrammer, owner of said account.

// fstt is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

// fstt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with fstt. If not, see <https://www.gnu.org/licenses/>. 

#include<dirent.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>
#include"ttyfunc.h"

void cache_size(const char *path, const char *rstr, const char *cstr)
{
    FILE *fh = fopen(path, "w");
    if(fh == NULL)
        perror("fopen failed");
    else
    {
        fprintf(fh, "%s %s\n", rstr, cstr);
        fclose(fh);
    }
}

// Files can't have duplicate names
// Just don't have 3 and 03
unsigned first_missing_positive(unsigned arr[], unsigned n)
{
    unsigned tmp;
    for(unsigned i = 0; i < n; ++i)
    {
        while(arr[i] != i + 1 && arr[i] > 0)
        {
            if(arr[i] > n)
                arr[i] = -1;
            else
            {
                tmp = arr[i] - 1;
                arr[i] ^= arr[tmp] ^= arr[i] ^= arr[tmp];
            }
        }
    }
    unsigned first = n + 1;
    for(unsigned i = 0; i < n; ++i)
    {
        if(arr[i] != i + 1)
            first = i + 1, n = i;
    }
    return first;
}

int maketty(const char *name, const char *rstr, const char *cstr, const char *shell, unsigned *restrict ttynumptr)
{
    int succ = 0;
    int ttynum = -1;
    DIR *d = opendir(CACHEPATH);
    if(d == NULL)
    {
        perror("opendir failed");
        succ = -1;
    }
    else
    {
        char namebuf[9];
        int pid;
        if(*name == '\0')
        {
            char isnum;
            unsigned num;
            size_t numcnt = 0, numcapa = 8;
            unsigned *allnum = malloc(numcapa * sizeof(*allnum)), *new;
            struct dirent *en = readdir(d);
            while(en != NULL)
            {
                if(en->d_name[0] != '.')
                {
                    name = en->d_name;
                    isnum = 1;
                    for(const char *it = name; *it != '\0'; ++it)
                    {
                        if(*it > '9' || *it < '0')
                            isnum = 0;
                    }
                    if(isnum)
                    {
                        num = atoi(name);
                        if(numcnt == numcapa)
                        {
                            numcapa += numcapa >> 1;
                            new = malloc(numcapa * sizeof(*new));
                            if(new == NULL)
                                perror("malloc failed");
                            memcpy(new, allnum, sizeof(unsigned) * numcnt);
                            free(allnum);
                            allnum = new;
                        }
                        allnum[numcnt] = num;
                        ++numcnt;
                    }
                }
                en = readdir(d);
            }
            ttynum = first_missing_positive(allnum, numcnt);
            sprintf(namebuf, "%d", ttynum);
            name = namebuf;
            free(allnum);
        }
        closedir(d);
        char path[361], exepath[2601];
        strcpy(path, CACHEPATH);
        path[sizeof(CACHEPATH) - 1] = '/';
        strcpy(path + sizeof(CACHEPATH), name);
        cache_size(path, rstr, cstr);
        strcpy(path, IPIPEPATH);
        path[sizeof(IPIPEPATH) - 1] = '/';
        strcpy(path + sizeof(IPIPEPATH), name);
        succ += mkfifo(path, 0755);
        strcpy(path, OPIPEPATH);
        path[sizeof(OPIPEPATH) - 1] = '/';
        strcpy(path + sizeof(OPIPEPATH), name);
        succ += mkfifo(path, 0755);
        if(succ == 0)
        {
            pid = fork();
            if(pid > 0)
            {
                if(ttynum >= 0)
                    *ttynumptr = ttynum;
            }
            else if(pid < 0)
                perror("fork failed");
            else
            {
                realpath("/proc/self/exe", exepath);
                execl(exepath, exepath, "-ces", name, shell, rstr, cstr, (char*)NULL);
                perror("execl failed");
                exit(1);
            }
        }
        else
            perror("mkfifo failed");
    }
    return succ;
}

void list_tty(void)
{
    DIR *d = opendir(CACHEPATH);
    if(d != NULL)
    {
        fputs("Terminal names:", stdout);
        struct dirent *en = readdir(d);
        while(en != NULL)
        {
            if(en->d_name[0] != '.')
                printf(" %s", en->d_name);
            en = readdir(d);
        }
        closedir(d);
    }
    else if(errno != ENOENT)
        perror("opendir failed");
}
