#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <regex.h>
int tee=0;
mprintf(int socket, char *str){
    tee++;
    while(tee>20);
    // printf("strlen: %d", strlen(str));
    send(socket, str, strlen(str), 0);
    tee--;
}
void set_out(int socket, struct stat sb, int flag) {
    char temp[1500];
    mprintf(socket, "=================================================\n\0");
    mprintf(socket, "File type:                \0");
    switch (sb.st_mode & S_IFMT) {
        case S_IFBLK:  mprintf(socket, "block device\n\0");            break;
        case S_IFCHR:  mprintf(socket, "character device\n\0");        break;
        case S_IFDIR:  mprintf(socket, "directory\n\0");               break;
        case S_IFIFO:  mprintf(socket, "FIFO/pipe\n\0");               break;
        case S_IFLNK:  mprintf(socket, "symlink\n\0");                 break;
        case S_IFREG:  mprintf(socket, "regular file\n\0");            break;
        case S_IFSOCK: mprintf(socket, "socket\n\0");                  break;
        default:       mprintf(socket, "unknown?\n\0");                break;
    }
    if(flag == 1) {

        sprintf(temp, "I-node number:            %ld\n\0", (long) sb.st_ino);
        mprintf(socket, temp);
        
        sprintf(temp, "Mode:                     %lo (octal)\n\0", (unsigned long) sb.st_mode);
        mprintf(socket, temp);
        
        sprintf(temp, "Link count:               %ld\n\0", (long) sb.st_nlink);
        mprintf(socket, temp);
        
        sprintf(temp, "Ownership:                UID=%ld   GID=%ld\n\0", (long) sb.st_uid, (long) sb.st_gid);
        mprintf(socket, temp);
        
        sprintf(temp, "Preferred I/O block size: %ld bytes\n\0",
                (long) sb.st_blksize);
        mprintf(socket, temp);
        
        sprintf(temp, "Blocks allocated:         %lld\n\0", (long long) sb.st_blocks);
        mprintf(socket, temp);
    }
    sprintf(temp, "File size:                %lld bytes\n\0", (long long) sb.st_size);
    mprintf(socket, temp);
    
    sprintf(temp, "Last status change:       %s\0", ctime(&sb.st_ctime));
    mprintf(socket, temp);
    
    sprintf(temp, "Last file access:         %s\0", ctime(&sb.st_atime));
    mprintf(socket, temp);

    sprintf(temp, "Last file modification:   %s\0", ctime(&sb.st_mtime));
    mprintf(socket, temp);
    mprintf(socket, "=================================================\n\0");
}

int sendall(int socket, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    printf("sending\n");
    while(total < *len) {
        n = send(socket, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

int sendFileListToSocket(int argc, char *argv[], int socket)
{
    char *arr[12] = {"Jan\0", "Feb\0", "Mar\0", "Apr\0", "May\0", "Jun\0", "Jul\0", "Aug\0", "Sep\0", "Oct\0", "Nov\0", "Dec\0"};
    char line[1500];
    if(argc == 2)
        if((strcmp(argv[1], "--longlist")) == 0) {
            DIR *dp;
            struct dirent *ep;
            dp = opendir (".");

            if(dp == NULL)
                perror("Could not open the directory\n");
            else {
                while(ep = readdir(dp)) {
                    char pathdir[] = "./";
                    strcat(pathdir, ep->d_name);

                    struct stat sb;
                    if (stat(pathdir, &sb) == -1) {
                        perror("stat");
                        exit(EXIT_FAILURE);
                    }   

                    sprintf(line,"File Name:                %s\n\0", ep->d_name);
                    // puts(line);
                    mprintf(socket, line);
                    set_out(socket, sb, 1);
                }
            }
        }
    if(argc == 4) {
        if(strcmp(argv[1], "--shortlist") == 0) {
            int day[2], month[2], year[2];
            sscanf(argv[2], "%d-%d-%d", &day[0], &month[0], &year[0]);
            sscanf(argv[3], "%d-%d-%d", &day[1], &month[1], &year[1]);
            DIR *dp;
            struct dirent *ep;
            dp = opendir (".");

            if(dp == NULL)
                perror("Could not open the directory\n");
            else {
                while(ep = readdir(dp)) {
                    char pathdir[] = "./";
                    strcat(pathdir, ep->d_name);

                    struct stat sb;
                    if (stat(pathdir, &sb) == -1) {
                        perror("stat");
                        exit(EXIT_FAILURE);
                    }
                    char hello[6][20];
                    int yeari, monthi, dayi;
                    sscanf(ctime(&sb.st_mtime), "%s %s %d %s %d", hello[0], hello[1], &dayi, hello[3], &yeari);
                    int i;
                    for(i = 0; i < 12; i++) {
                        if(strcmp(arr[i], hello[1]) == 0)
                            monthi = i + 1;
                    }
                    if((yeari > year[0]) && (yeari < year[1])) {
                        sprintf(line, "File Name:                %s\n\0", ep->d_name);
                        mprintf(socket, line);
                        set_out(socket, sb, 0);
                    }
                    else if((yeari == year[0]) && (yeari < year[1])) {
                        if(monthi > month[0]) {
                            sprintf(line, "File Name:                %s\n\0", ep->d_name);
                            mprintf(socket, line);
                            set_out(socket, sb, 0);
                        }
                        else if(monthi == month[0]) {
                            if(dayi >= day[0]) {
                                sprintf(line, "File Name:                %s\n\0", ep->d_name);
                                mprintf(socket, line);
                                set_out(socket, sb, 0);
                            }
                        }
                    }
                    else if((yeari == year[1]) && (yeari > year[0])) {
                        if(monthi < month[1]) {
                            sprintf(line, "File Name:                %s\n\0", ep->d_name);
                            mprintf(socket, line);
                            set_out(socket, sb, 0);
                        }
                        else if(monthi == month[1]) {
                            if(dayi <= day[1]) {
                                sprintf(line, "File Name:                %s\n\0", ep->d_name);
                                mprintf(socket, line);
                                set_out(socket, sb, 0);
                            }
                        }
                    }
                    else if((yeari == year[1]) && (yeari == year[0])) {
                        if((monthi > month[0]) && (monthi < month[1])) {
                            sprintf(line, "File Name:                %s\n\0", ep->d_name);
                            mprintf(socket, line);
                            set_out(socket, sb, 0);
                        }
                        else if((monthi == month[0]) && (monthi < month[1])) {
                            if(dayi >= day[0]) {
                                sprintf(line, "File Name:                %s\n\0", ep->d_name);
                                mprintf(socket, line);
                                set_out(socket, sb, 0);
                            }
                        }
                        else if((monthi == month[1]) && (monthi > month[0])) {
                            if(dayi <= day[1]) {
                                sprintf(line, "File Name:                %s\n\0", ep->d_name);
                                mprintf(socket, line);
                                set_out(socket, sb, 0);
                            }
                        }
                        else if((monthi == month[0]) && (monthi == month[1])) {
                            if((dayi >= day[0]) && (dayi <= day[1])) {
                                sprintf(line, "File Name:                %s\n\0", ep->d_name);
                                mprintf(socket, line);
                                set_out(socket, sb, 0);
                            }
                        }
                    }     
                }
            }
        }
    }
    if(argc == 3) {
        if(strcmp(argv[1], "--regex") == 0) {
            regex_t regex;
            char msgbuf[100];
            int ret;
            ret = regcomp(&regex, argv[2], 0);
            if(ret) { 
                fprintf(stderr, "Could not compile regex\n"); 
                exit(1); 
            }
            DIR *dp;
            struct dirent *ep;
            dp = opendir (".");
            if(dp == NULL)
                perror("Could not open the directory\n");
            else {
                while(ep = readdir(dp)) {
                    ret = regexec(&regex, ep->d_name, 0, NULL, 0);
                    if(!ret) {
                        char pathdir[] = "./";
                        strcat(pathdir, ep->d_name);
                        struct stat sb;
                        if(stat(pathdir, &sb) == -1) {
                            perror("stat");
                            exit(EXIT_FAILURE);
                        }   
                        sprintf(line, "File Name:                %s\n", ep->d_name);
                        mprintf(socket, line);
                        set_out(socket, sb, 0);
                    }
                    else if(ret != REG_NOMATCH) {
                        regerror(ret, &regex, msgbuf, sizeof(msgbuf));
                        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
                    }
                }
            }
        }
    }                 


    exit(EXIT_SUCCESS);
}

