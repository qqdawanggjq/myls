#include<stdio.h>
#include<string.h>
#include<stdlib.h>
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
typedef struct 
{
    char* name;
    struct stat st;
}fileinfo;
int cmp_name(const void * p1,const void * p2)
{
    const fileinfo * a=p1;
    const fileinfo * b=p2;
    return strcmp(a->name,b->name);
}
int cmp_time(const void * p1,const void * p2)
{
    const fileinfo * a=p1;
    const fileinfo * b=p2;
    if(a->st.st_mtim.tv_sec>b->st.st_mtim.tv_sec)
    {
        return -1;
    }
    if(a->st.st_mtim.tv_sec<b->st.st_mtim.tv_sec)
    {
        return 1;
    }
    if(a->st.st_mtim.tv_nsec>b->st.st_mtim.tv_nsec)
    {
        return -1;
    }
    if(a->st.st_mtim.tv_nsec<b->st.st_mtim.tv_nsec)
    {
        return 1;
    }
    return strcmp(a->name,b->name);
}
int list_ori(const char *s,options *opt)
{
    DIR*dir=opendir(s);
    if(dir==NULL)
    {
        perror(s);
        return 1;
    }
    struct dirent* entry;
    fileinfo* files=NULL;
    size_t count=0;
    size_t maxsave=0;
    int failed=0;
    int errored=0;
    while((entry=readdir(dir))!=NULL)
    {
        if(!opt->show_all&&entry->d_name[0]=='.')
        {
            continue;
        }
        struct stat st;
        if((fstatat(dirfd(dir),entry->d_name,&st,AT_SYMLINK_NOFOLLOW))==-1)
        {
            perror(entry->d_name);
            errored=1;
            continue;
        }
        if(count==maxsave)
        {
            size_t newsize=((maxsave==0)?8:maxsave*2);
            fileinfo* ppp=realloc(files,newsize*sizeof(*files));
            if(ppp==NULL)
            {
                perror("realloc");
                failed=1;
                break;
            }
            files=ppp;
            maxsave=newsize;
        }
        files[count].name=malloc(strlen(entry->d_name)+1);
        if(files[count].name==NULL)
        {
            perror("malloc");
            failed=1;
            break;
        }
        strcpy(files[count].name,entry->d_name);
        files[count].st=st;
        count++;
    }
    if(closedir(dir)==-1)
    {
        perror(s);
        failed=1;
    }
    if(!failed&&count>1)
    {
        if(opt->sort_time)
        {
            qsort(files,count,sizeof(*files),cmp_time);
        }
        else
        {
            qsort(files,count,sizeof(*files),cmp_name);
        }
    }
    if(!failed)
    {
        if(!failed&&opt->recursive)
        {
            printf("%s:\n",s);
        }
        for(size_t i=0;i<count;i++)
        {
            size_t j=(opt->reverse ? count-1-i : i);
            struct stat st=files[j].st;
            const char * name=files[j].name;
            if(opt->show_blocks)
            {
            printf("%lld ",((long long)st.st_blocks+1)/2);
            }
            if(opt->show_inode)
            {
            printf("%lu ",(unsigned long)st.st_ino);
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
                printf("%c",(st.st_mode&S_IXOTH)?'x':'-');
                printf(" ");
                printf("%lu ",(unsigned long)st.st_nlink);
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
                char buf[64];
                struct tm* t=localtime(&st.st_mtime);
                if(t!=NULL)
                {
                    if(strftime(buf,sizeof(buf),"%b %e %H:%M",t)>0)
                    {
                        printf("%s ",buf);
                    }
                    else
                    {
                        printf("? ");
                    }
                }
                else
                {
                    printf("? ");
                }
            }
            printf("%s\n",name);
        }
    }
    if(!failed&&opt->recursive)
    {
        for(size_t i=0;i<count;i++)
        {
            size_t j=(opt->reverse? count-1-i : i);
            const char * name=files[j].name;
            if(!S_ISDIR(files[j].st.st_mode))
            {
                continue;
            }
            if((strcmp(name,"."))==0||(strcmp(name,".."))==0)
            {
                continue;
            }
            size_t len=strlen(s);
            size_t n=strlen(name);
            int xiegang=(s[len-1]!='/');
            char * path=malloc(len+n+xiegang+1);
            if(path==NULL)
            {
                perror("malloc");
                failed=1;
                break;
            }
            snprintf(path,len+n+xiegang+1,"%s%s%s",s,xiegang?"/":"",name);
            printf("\n");
            if(list_ori(path,opt)!=0)
            {
                failed=1;
            }
            free(path);
        }
    }
    for(int i=0;i<count;i++)
    {
        free(files[i].name);
    }
    free(files);
    return failed||errored;
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
