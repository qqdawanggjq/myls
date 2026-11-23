#include<stdio.h>
#include<dirent.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
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
        struct stat st;
        if(opt->show_blocks||opt->long_format)
        {
            if((fstatat(dirfd(dir),entry->d_name,&st,AT_SYMLINK_FOLLOW))==-1)
            {
                perror(entry->d_name);
                continue;
            }
            if(opt->show_blocks)
            {
                printf("%lld ",((long long)st.st_blocks+1)/2);
            }
        }
        if(opt->show_inode)
        {
            printf("%lu ",(unsigned long)entry->d_ino);
        }
        if(opt->long_format)
        {
            if(S_ISDIR(st.st_mode))printf("d");
            else if(S_ISREG(st.st_mode))printf("-");
            else if(S_ISLNK(st.st_mode))printf("l");
            else printf("?");
            printf("%c",(st.st_mode&S_IRUSR)?'r':'-');
            printf("%c",(st.st_mode&S_IWUSR)?'w':'-');
            printf("%c",(st.st_mode&S_IXUSR)?'x':'-');
            printf("%c",(st.st_mode&S_IRGRP)?'r':'-');
            printf("%c",(st.st_mode&S_IWGRP)?'w':'-');
            printf("%c",(st.st_mode&S_IXGRP)?'x':'-');
            printf("%c",(st.st_mode&S_IROTH)?'r':'-');
            printf("%c",(st.st_mode&S_IWOTH)?'w':'-');
            printf("%c",(st.st_mode&S_IXGRP)?'x':'-');
            printf(" ");
            printf("lu ",(unsigned long)st.st_nlink);
            struct passwd *pw=getpwuid(st.st_uid);
            struct group *gr=getgrgid(st.st_gid);
            if(pw!=NULL)
            {
                printf("%s ",pw->pw_name);
            }
            else
            {
                printf("%u ",(unsigned int)st.st_uid);
            }
            if(gr!=NULL)
            {
                printf("%s ",gr->gr_name);
            }
            else
            {
                printf("%u ",(unsigned int)st.st_gid);
            }
            printf("%lld ",(long long)st.st_size);
            
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
