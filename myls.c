#include<stdio.h>
#include<dirent.h>
typedef struct 
{
    int show_all;
    int long_format;
    int recursive;
    int sort_time;
    int reverse;
    int show_inode;
    int show_blocks;
}options;
int list_ori(const char *s,options *opt)
{
    DIR*dir=opendir(s);
    if(dir==NULL)
    {
        perror(s);
        return 1;
    }
    struct dirent* entry;
    while((entry=readdir(dir))!=NULL)
    {
        if(!opt->show_all&&entry->d_name[0]=='.')
        {
            continue;
        }
        if(opt->show_inode)
        {
            printf("%lu ",(unsigned long)entry->d_ino);
        }
        printf("%s\n",entry->d_name);
    }
    closedir(dir);
    return 0;
}
int main(int argc,char* argv[])
{
    options opt={0};
    const char* path=".";
    for(int i=1;i<argc;i++)
    {
        if(argv[i][0]!='-')
        {
            path=argv[i];
            continue;
        }
        for(int j=1;argv[i][j]!='\0';j++)
        {
            if(argv[i][j]=='a')
            {
                opt.show_all=1;
            }
            else if(argv[i][j]=='l')
            {
                opt.long_format=1;
            }
            else if(argv[i][j]=='R')
            {
                opt.recursive=1;
            }
            else if(argv[i][j]=='t')
            {
                opt.sort_time=1;
            }
            else if(argv[i][j]=='r')
            {
                opt.reverse=1;
            }
            else if(argv[i][j]=='i')
            {
                opt.show_inode=1;
            }
            else if(argv[i][j]=='s')
            {
                opt.show_blocks=1;
            }
        }
    }
    return list_ori(path,&opt);
}
