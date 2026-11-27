#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include<errno.h>
#include<stdint.h>
#include<unistd.h>
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
int listori(const char *s,options *opt)
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
    for(;;)
    {
        errno=0;
        entry=readdir(dir);
        if(entry==NULL)
        {
            if(errno!=0)
            {
                perror("readdir");
                errored=1;
            }
            break;
        }
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
        if(opt->long_format||opt->show_blocks)
        {
            long long total=0;
            for(size_t i=0;i<count;i++)
            {
                total+=files[i].st.st_blocks;
            }
            printf("total %lld\n",(total+1)/2);
        }
        int color=isatty(STDOUT_FILENO);
        int size_width=1;
        if(opt->long_format)
        {
            for(size_t i=0;i<count;i++)
            {
                char size_buf[32];
                snprintf(size_buf,sizeof(size_buf),"%lld",(long long)files[i].st.st_size);
                int len=strlen(size_buf);
                if(len>size_width)
                {
                    size_width=len;
                }
            }
        }
        int link_width=1;
        if(opt->long_format)
        {
            for(size_t i=0;i<count;i++)
            {
                char link_buf[32];
                snprintf(link_buf,sizeof(link_buf),"%lu",(unsigned long)files[i].st.st_nlink);
                int len=strlen(link_buf);
                if(len>link_width)
                {
                    link_width=len;
                }
            }
        }
        int user_width=1;
        int group_width=1;
        if(opt->long_format)
        {
            for(size_t i=0;i<count;i++)
            {
                struct passwd * pw=getpwuid(files[i].st.st_uid);
                struct group * gr=getgrgid(files[i].st.st_gid);
                int len1=pw ? strlen(pw->pw_name) : 1;
                int len2=gr ? strlen(gr->gr_name) : 1;
                if(len1>user_width)
                {
                    user_width=len1;
                }
                if(len2>group_width)
                {
                    group_width=len2;
                }
            }
        }
        int inode_width=1;
        int block_width=1;
        for(size_t i=0;i<count;i++)
        {
            char inode_buf[32];
            char blocks_buf[32];
            if(opt->show_inode)
            {
                snprintf(inode_buf,sizeof(inode_buf),"%lu",(unsigned long)files[i].st.st_ino);
                int len=strlen(inode_buf);
                if(len>inode_width)
                {
                    inode_width=len;
                }
            }
            if(opt->show_blocks)
            {
                snprintf(blocks_buf,sizeof(blocks_buf),"%lld",((long long)files[i].st.st_blocks+1)/2);
                int len=strlen(blocks_buf);
                if(len>block_width)
                {
                    block_width=len;
                }
            }
        
        }
        for(size_t i=0;i<count;i++)
        {
            size_t j=(opt->reverse ? count-1-i : i);
            struct stat st=files[j].st;
            const char * name=files[j].name;
            if(opt->show_blocks)
            {
            printf("%*lld ",block_width,((long long)st.st_blocks+1)/2);
            }
            if(opt->show_inode)
            {
            printf("%*lu ",inode_width,(unsigned long)st.st_ino);
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
                printf("%*lu ",link_width,(unsigned long)st.st_nlink);
                struct passwd *pw=getpwuid(st.st_uid);
                struct group *gr=getgrgid(st.st_gid);
                if(pw!=NULL)
                {
                    printf("%-*s ",user_width,pw->pw_name);
                }
                else
                {
                    printf("%-*u ",user_width,(unsigned int)st.st_uid);
                }
                if(gr!=NULL)
                {
                    printf("%-*s ",group_width,gr->gr_name);
                }
                else
                {
                    printf("%-*u ",group_width,(unsigned int)st.st_gid);
                }
                printf("%*lld ",size_width,(long long)st.st_size);
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
            if(color)
            {
                if(S_ISDIR(st.st_mode))
                {
                    printf("\033[1;34m");
                }
                else if(S_ISLNK(st.st_mode))
                {
                    printf("\033[1;36m");
                }
                else if(st.st_mode&(S_IXUSR|S_IXGRP|S_IXOTH))
                {
                    printf("\033[1;32m");
                }
            }
            printf("%s",name);
            if(color)
            {
                printf("\033[0m");
            }
            printf("\n");
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
            size_t slash=(s[len-1]!='/');
            if(n>SIZE_MAX-slash-1||len>SIZE_MAX-slash-n-1)
            {
                fprintf(stderr,"path too long\n");
                failed=1;
                break;
            }
            char * path=malloc(len+n+slash+1);
            if(path==NULL)
            {
                perror("malloc");
                failed=1;
                break;
            }
            snprintf(path,len+n+slash+1,"%s%s%s",s,slash?"/":"",name);
            printf("\n");
            if(listori(path,opt)!=0)
            {
                failed=1;
            }
            free(path);
        }
    }
    for(size_t i=0;i<count;i++)
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
    return listori(path,&opt);
}
