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
#include<time.h>
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
unsigned first_missing_nonnega(unsigned arr[], unsigned n)
{
    unsigned tmp;
    for(unsigned i = 0; i < n; ++i)
    {
        while(arr[i] != i && arr[i] < n)
        {
            if(arr[i] >= n)
                arr[i] = -1;
            else
            {
                tmp = arr[i];
                arr[i] ^= arr[tmp] ^= arr[i] ^= arr[tmp];
            }
        }
    }
    unsigned first = n;
    for(unsigned i = 0; i < n; ++i)
    {
        if(arr[i] != i)
            first = i, n = i;
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
            ttynum = first_missing_nonnega(allnum, numcnt);
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

void list_tty(char l)
{
    char **names, **tmpname;
    size_t cnt, capa;
    DIR *d = opendir(CACHEPATH);
    if(d != NULL)
    {
        if(l)
        {
            capa = 18;
            names = malloc(capa * sizeof(*names));
            cnt = 0;
        }
        else
            fputs("Terminal names:", stdout);
        struct dirent *en = readdir(d);
        while(en != NULL)
        {
            if(en->d_name[0] != '.')
            {
                if(l)
                {
                    if(cnt == capa)
                    {
                        capa += capa >> 1;
                        tmpname = malloc(capa * sizeof(*tmpname));
                        if(tmpname == NULL)
                            perror("malloc failed");
                        else
                        {
                            memcpy(tmpname, names, cnt * sizeof(char*));
                            free(names);
                            names = tmpname;
                        }
                    }
                    names[cnt] = strdup(en->d_name);
                    ++cnt;
                }
                else
                    printf(" %s", en->d_name);
            }
            en = readdir(d);
        }
        closedir(d);
        if(l)
        {
            char pathbuf[2601];
            struct stat cachedat;
            time_t currtime = time(NULL), thentime;
            unsigned days, hours, minutes;
            int lncnt, colcnt;
            FILE *fh;
            strcpy(pathbuf, CACHEPATH);
            pathbuf[sizeof(CACHEPATH) - 1] = '/';
            for(size_t i = 0; i < cnt; ++i)
            {
                strcpy(pathbuf + sizeof(CACHEPATH), names[i]);
                stat(pathbuf, &cachedat);
                thentime = cachedat.st_ctime;
                thentime = currtime - thentime;
                thentime /= 60;
                fh = fopen(pathbuf, "r");
                fscanf(fh, "%d %d", &lncnt, &colcnt);
                fclose(fh);
                days = thentime / 1440;
                hours = thentime % 1440 / 60;
                minutes = thentime % 60;
                if(days == 0)
                    printf("%s %d rows %d columns, %u:%02u ago.\n", names[i], lncnt, colcnt, hours, minutes);
                else
                    printf("%s %d rows %d columns, %u days and %u:%02u ago.\n", names[i], lncnt, colcnt, days, hours, minutes);
                free(names[i]);
            }
            free(names);
        }
        else
            putchar('\n');
    }
    else if(errno != ENOENT)
        perror("opendir failed");
}
