#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <pthread.h>
#include "get_file_list.h"
#include <openssl/md5.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int commandFlagN;
int parseCommand(char c[], char **p) {
    int i = 0, j = 0, pos = 0;
    char t[100], ch;
    while(1) {
        j=0;
        ch=c[i++];
        while(ch!=' ' && ch!='\0') {
            t[j++] = ch;
            ch = c[i++];
        }
        while(ch==' '){
            ch = c[i++];
        }
        i--;
        t[j] = '\0';
        p[pos++] = strdup(t);
        if(ch=='\0')
            break;
    }
    commandFlagN = pos; // Number of command line flags that are set
    p[pos] = '\0';
    return commandFlagN;
}
int getMD5(char *filename, char *c2)
{
    unsigned char c[16];
    int i;
    FILE *inFile = fopen (filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
        return 0;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    for(i=0;i<16;i++)
                sprintf(&c2[i*2],"%02x", c[i]);
    fclose (inFile);
    return 0;
}
void sig_handler(int signo)
{
    pid_t thisprocess;
    int statet;
    thisprocess=getpid();
    if (signo == SIGINT)
        write(1, "Please use exit to exit the application.\n$> ", 43);
    else if (signo == SIGKILL)
        write(1, "Please use exit to exit the application.\n$> ", 43);
    return;
}

void stripFirst(char *arg[]) {
    int i = 1;
    char ch = arg[i];
    while(ch!='\0') {
        arg[i-1] = arg[i];
        ch = arg[++i];
    }
    arg[i] = '\0';
    puts(arg);
}

long long int get_file_size(char *path, char *file_size) {
    struct stat sb;
    long long int size;
    // char *size_buf = (char*)malloc(sizeof(char) * 100);
    if(stat(path, &sb) == -1) {
        perror("Stat Error");
        return "DisfunctionStat";
    }
    size = (long long)sb.st_size;
    sprintf(file_size, "%lld", size);
    return size;
}

int CONTROL_PORT_LISTEN_SOCKET, CONTROL_PORT_CONNECTION_SOCKET;
struct sockaddr_in control_serv_addr;
int CONTROL_PORT_NUMBER = 25135;
char control_buffer[1500];

int startControlPort() {
    printf("\n");
    char filename[100], filesize[100], fileBuf[1500], listArgsBuf[100], *listArgsBufList[10], md5[33];
    int bufi = 0, i = 0, fsizelen = 0, ret = 1, listArgs;
    long long int fileSizelld = 1500;
    struct sockaddr_in request_serv_addr;
    FILE *fp;
    if(CONTROL_PORT_LISTEN_SOCKET == 0) {
        CONTROL_PORT_LISTEN_SOCKET = socket(AF_INET, SOCK_STREAM, 0);
        if(CONTROL_PORT_LISTEN_SOCKET < 0)
            handle_error("[CONTROL PORT] ERROR WHILE CREATING A SOCKET\n");
        else
            printf("[CONTROL PORT] SOCKET ESTABLISHED SUCCESSFULLY\n");

        bzero((char *) &control_serv_addr, sizeof(control_serv_addr));
        control_serv_addr.sin_family = AF_INET; //For a remote machine
        control_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all network devices and broadcasting from default (lowest numbered device)
        control_serv_addr.sin_port = htons(CONTROL_PORT_NUMBER); // What port our control server runs on

        if(bind(CONTROL_PORT_LISTEN_SOCKET, (struct sockaddr * )&control_serv_addr, sizeof(control_serv_addr))<0)
            handle_error("[CONTROL PORT] ERROR BINDING TO SOCKET");
        else
            printf("[CONTROL PORT] SOCKET BINDED SUCCESSFULLY\n");
        if(listen(CONTROL_PORT_LISTEN_SOCKET , 50) == -1)
            handle_error("[CONTROL PORT] FAILED TO ESTABLISH LISTENING");
        printf("[CONTROL PORT] LISTENING STARTED\n" );
    }
    bzero((char *) &request_serv_addr, sizeof(request_serv_addr));
    int reqseraddlen = sizeof(request_serv_addr);
    while((CONTROL_PORT_CONNECTION_SOCKET=accept(CONTROL_PORT_LISTEN_SOCKET , (struct sockaddr*)&request_serv_addr, &reqseraddlen))<0);
    //while((CONTROL_PORT_CONNECTION_SOCKET = accept(CONTROL_PORT_LISTEN_SOCKET, NULL, NULL))<0);
    bzero(control_buffer, sizeof(control_buffer));
    recv(CONTROL_PORT_CONNECTION_SOCKET, control_buffer, sizeof(control_buffer), 0); // Getting the message

    // puts(control_buffer);
    
    printf("[CONTROL PORT] RECEIVED REQUEST FROM %s\n", inet_ntoa(request_serv_addr.sin_addr)); // Right now, assume it's a message of fixed length 1500, I don't think this would be flexible code :/ 
    if(control_buffer[0] == 'd') {
        //printf("\nteehee?\n");
        if(control_buffer[1] == 'a') {
            // printf("\nteehee!\n");
            // getting filename
            for(i=0;i<100;i++) // Fetching the filename to get it's size
                filename[i] = control_buffer[i+2];
            
            bzero(control_buffer, 1500); // clear buffer after getting the filename
            control_buffer[0] = 'd';
            control_buffer[1] = 'r';
            control_buffer[2] = '\0';
            
            fileSizelld = get_file_size(filename, filesize);
            puts(&filesize);
            fsizelen = strlen(filesize);
            // printf("filesize:len %s:%d\n", filesize, fsizelen);
            for(i=0;i<fsizelen;i++)
                control_buffer[i+2] = filesize[i];
            control_buffer[i+2] = '\0';
            i++;
            getMD5(filename, md5);
            printf("hash: %s %s\n",md5 ,filename);
            for(i=0;i<32;i++)
                control_buffer[i+102] = md5[i];
            control_buffer[i+102] = '\0';
            ret = send(CONTROL_PORT_CONNECTION_SOCKET, control_buffer, 1500, 0); // sending crafted response to the ask request.
            if(!ret)
                handle_error("Error while sending request response");
            // Send the file.
            fp = fopen(filename, "rb");
            bzero(fileBuf, 1500);
            while(fread(fileBuf, 1, 1500, fp)) {
                if(fileSizelld > 1500) {
                    ret = send(CONTROL_PORT_CONNECTION_SOCKET, fileBuf, 1500, 0);
                    fileSizelld -= 1500;
                }
                else {
                    // Sending the last chunck after remaining file size is calculated
                    ret = send(CONTROL_PORT_CONNECTION_SOCKET, fileBuf, fileSizelld, 0);
                    break;
                }
                printf("sent packet %c\n", fileBuf[0]);
                if(ret < 0)
                    handle_error("Error sending file");
                else if(ret == 0)
                    handle_error("Client hung");
            }
            close(CONTROL_PORT_CONNECTION_SOCKET);
        }
    }
    else if(control_buffer[0] == 'u') {
        for(i=0;i<100;i++) // Fetching the filename to get it's size
            filename[i] = control_buffer[i+1];
        printf("Upload request for %s form %s:%d\n", filename, inet_ntoa(request_serv_addr.sin_addr), request_serv_addr.sin_port);
        // Reply with our download request on the control port
        startDownloadTCP(inet_ntoa(request_serv_addr.sin_addr), CONTROL_PORT_NUMBER, filename);
    }
    else if(control_buffer[0] == 'f') {
        for(i=0;i<100;i++)
            listArgsBuf[i] = control_buffer[i+1];
        printf("File List request: %s \n", listArgsBuf);
        ret = 1;
        listArgs = parseCommand(listArgsBuf, listArgsBufList);
        // Start writing the response from get_file_list call directly to the open connection.
        sendFileListToSocket(listArgs, listArgsBufList, CONTROL_PORT_CONNECTION_SOCKET);
    }
    close(CONTROL_PORT_CONNECTION_SOCKET);
}
int startUploadTCP(char *server_ip, int server_port, char *filename) {
    int uploadPid = fork();
    if(uploadPid != 0)
        return uploadPid;
    // As the spec suggests, just fire the right upload request and the other side shall continue with a call to startDownloadTCP
    char buf[1500];
    struct sockaddr_in upload_serv_addr;
    int mySock = -1, ret;

    bzero(buf, 1500);
    bzero((char*) &upload_serv_addr, sizeof(upload_serv_addr));
    // Setting up specs to connect to the server on that port to send the upload request
    upload_serv_addr.sin_family = AF_INET;
    upload_serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    upload_serv_addr.sin_port = htons(server_port);
    // Acquire a socket to send this upload request on
    while(mySock < 0)
        mySock = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to the control port
    while(connect(mySock, (struct sockaddr *)&upload_serv_addr, sizeof(upload_serv_addr))<0);

    printf("Sending upload request packet\n");

    // Ctaft the upload request packet
    buf[0] = 'u';
    int i;
    for(i=0;i<100;i++){
        if(filename[i] == '\0')
            break;
        buf[i+1] = filename[i];
    }
    while(i<100)
        buf[i++] = '\0';   

    ret = send(mySock, buf, sizeof(buf), 0);
    if(ret == 0)
        handle_error("Error sending upload request, please retry.");
    
    exit(0); // Exit killing the child process
}
int startDownloadTCP(char *server_ip, int server_port, char *filename) {
    int downloadPid = fork();
    int ret, chunkSize = 1500;
    long long int fileSize;
    char fileSizeStr[1500];
    FILE *downloadedFile;
    if(downloadPid != 0)
        return downloadPid;
    printf("Downloading %s from %s:%d\n", filename, server_ip, server_port);

    struct sockaddr_in download_serv_addr;
    char buf[1500], temp;
    int bufi = 0, i;
    bzero((char *) &download_serv_addr, sizeof(download_serv_addr));
    int mySock = -1;

    while(mySock < 0) 
        mySock = socket(AF_INET, SOCK_STREAM, 0); // Attain my Socket to download on.

    printf("Download, acquired socket\n");

    // Setting up specks to connect to that server on that port for download.
    download_serv_addr.sin_family = AF_INET;
    download_serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    download_serv_addr.sin_port = htons(server_port);

    // Connect to the control port
    while(connect(mySock, (struct sockaddr *)&download_serv_addr, sizeof(download_serv_addr))<0);
    printf("Sending donwload request packet\n");

    // Craft download request pack, read spec
    buf[bufi++] = 'd'; // download
    buf[bufi++] = 'a'; // ask
    for(i=0;i<100;i++){
        if(filename[i] == '\0')
            break;
        buf[bufi++] = filename[i];
    }
    while(bufi<102)
        buf[bufi++] = '\0';

    ret = send(mySock, buf, sizeof(buf), 0);
    puts(buf);

    if(ret == 0)
        handle_error("Error sending download request, port may not be running server");

    ret = recv(mySock, buf, sizeof(buf), 0);
    if(ret == 0)
        handle_error("Error getting download request response from other end");

    printf("response: %s %d\n", buf, ret);
    // Check if response is fine
    if(control_buffer[0] == 'd' ||
            control_buffer[1] == 'r') {
        close(mySock);
        handle_error("Download request response invalid error");
    }

    // Writing it to a file
    downloadedFile = fopen(filename, "wb");

    // Getting the filesize and MD5 hash
    for(i=2;i<102;i++) {
        fileSizeStr[i-2] = buf[i];
    }

    for(i=102;i<133;i++) {
        // md5[1-102] = buf[i];
    }
    fileSize = 0;
    i = 0;
    while(fileSizeStr[i]!='\0') {
        fileSize *= 10;
        fileSize += fileSizeStr[i++]-'0';
    }
    printf("received len %lld\n", fileSize);
    ret = 1;
    bzero(buf, 1500);
    while(1) {
        if(!ret)
            break;
        bzero(buf, 1500);
        chunkSize = (fileSize < chunkSize) ? fileSize : chunkSize;
        if(chunkSize < 1500) {
            recv(mySock, buf, chunkSize, 0);
            fwrite(buf, chunkSize, 1, downloadedFile);
            printf("wrote file last!!\n");
            break;
        }
        ret = recv(mySock, buf, chunkSize, 0);
        fwrite(buf, chunkSize, 1, downloadedFile);
        printf("wrote file\n");
    }
    fclose(downloadedFile);
    if(ret == 0 && chunkSize != fileSize)
        handle_error("host port killed connection, retry download!");
    close(mySock);
    
    exit(0); // Exit killing the child process
}
int startFileListRequest(char* server_ip, int server_port, char *command){
    int fileListPid = fork();
    if(fileListPid != 0)
        return fileListPid;
    // Send the request for the file list
    char buf[1500];
    struct sockaddr_in serv_addr;
    int mySock = -1, ret;

    bzero(buf, 1500);
    bzero((char*) &serv_addr, sizeof(serv_addr));
    // Setting up specs to connect to the server on that port to send the upload request
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(server_port);
    // Acquire a socket to send this request on
    while(mySock < 0)
        mySock = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to the control port
    while(connect(mySock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0);

    printf("\nSending file list request packet\n");

    // Craft the file list request packet
    buf[0] = 'f';
    int i;
    for(i=0;i<100;i++){
        if(command[i] == '\0')
            break;
        buf[i+1] = command[i];
    }
    i++;
    while(i<100)
        buf[i++] = '\0';   

    ret = send(mySock, buf, sizeof(buf), 0);
    if(ret == 0)
        handle_error("Error sending file list request, please retry.");
    ret = 1;
    // Just start printing out whatever is sent by the other side
    while(ret>0) {
        ret = recv(mySock, buf, sizeof(buf), 0);
        printf("%s", buf);
    }
    close(mySock);
    exit(0); // Exit killing the child process
}

int main(int argc, char *argv[]) {
    char command[1000];
    int control_port_pid, status;
    char *in, *out;
    char *args[1000];
    char *args2[1000];
    char *connIP = "10.4.3.205\0";
    control_port_pid = fork();
    while(control_port_pid == 0)
        startControlPort();

    while(1) {
        signal (SIGTTOU, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        if (signal(SIGINT, sig_handler) == SIG_ERR)
        {
            write(2,"\ncan't catch SIGINT\n",20);
            continue;
        }
        if (signal(SIGQUIT, sig_handler) == SIG_ERR)
        {
            write(2,"\ncan't catch SIGKILL\n",21);
            continue;
        }
        printf("$> ");

        gets(command);
        parseCommand(command, args);
        if(!strcmp(args[0], "exit")) {
            signal(SIGQUIT, SIG_IGN);
            kill(control_port_pid ,SIGQUIT);
            close(CONTROL_PORT_LISTEN_SOCKET);
            break;
        }
        else if(!strcmp(args[0], "FileDownload")) {
            // Start Downlod sends request from the server and gets back the file and writes to that file
            // Params: (IP_addr, Port, filename)
            // Note, filename verification of size is sent back by the server.
            if(!startDownloadTCP(connIP, CONTROL_PORT_NUMBER, args[1])) // potential fork bomb vuln here! PS: trust me, I bombed myself :P
                //TODO: this 127.0.0.1 needs to be changed to the connections IP address, figure out how to do that
                break;
        }
        else if(!strcmp(args[0], "FileUpload")) {
            startUploadTCP(connIP, CONTROL_PORT_NUMBER, args[1]);
            //TODO: Change this hard coded ip
        }
        else if(!strcmp(args[0], "FileList")) {
            startFileListRequest(connIP, CONTROL_PORT_NUMBER, command);
            //TODO: Change this hard coded ip
        }
        else if(!strcmp(args[0],  "Connect")) {
            connIP = strdup(args[1]);
            printf("Connected to: %s\n", connIP);
        }
    }
}
