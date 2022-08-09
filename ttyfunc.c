#include<dirent.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>
#define PIPEPATH "/tmp/fixed_size_terminal_terminal_named_pipes"

int maketty(const char *name, const char *rstr, const char *cstr)
{
    int succ = 0;
    DIR *d = opendir(PIPEPATH);
    if(d == NULL)
    {
        if(errno != ENOENT)
            perror("opendir failed");
        else
        {
            succ = mkdir(PIPEPATH, 0755);
            if(succ == 0)
                d = opendir(PIPEPATH);
            else
                perror("mkdir failed");
        }
    }
    if(d != NULL)
    {
        char namebuf[9];
        int pid;
        if(*name == '\0')
        {
            char isnum;
            unsigned highnum = 0, num;
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
                        if(highnum < num)
                            highnum = num;
                    }
                }
                en = readdir(d);
            }
            ++highnum;
            sprintf(namebuf, "%u", highnum);
            name = namebuf;
        }
        closedir(d);
        char path[361], exepath[2601];
        strcpy(path, PIPEPATH);
        path[sizeof(PIPEPATH) - 1] = '/';
        strcat(path, name);
        succ = mkfifo(path, 0755);
        if(succ == 0)
        {
            pid = fork();
            if(pid > 0)
                succ = 0;
            else if(pid < 0)
                perror("fork failed");
            else
            {
                realpath("/proc/self/exe", exepath);
                execl(exepath, exepath, "-cs", path, rstr, cstr, (char*)NULL);
                perror("execl failed");
                exit(1);
            }
        }
        else
            perror("mkfifo failed");
    }
    else
        succ = -1;
    return succ;
}

void list_tty(void)
{
    DIR *d = opendir(PIPEPATH);
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
