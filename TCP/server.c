#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>

#define SENTENCE_SIZE 8192

//功能函数声明
static int handle_login(int fd, char* sentence);
static void handle_pass(int fd);
static int get_command(char* command, char* sentence);
static int handle_pasv(int fd, struct sockaddr_storage* addr, socklen_t* addr_len);
static int handle_port(int fd, struct sockaddr_storage* addr, socklen_t* addr_len, char* new_ip_addr);
static void handle_list(int fd, int data_fd, char* path);
static void handle_cwd(int fd, char* path, char* new_path);
static void handle_pwd(int fd);
static void handle_mkd(int fd, char* path, char* dir_path);
static void handle_rmd(int fd, char* path, char* dir_path);
static int handle_rnfr(int fd, char* origin_name);
static void handle_rnto(int fd, char* origin_name, char* new_name);
static void handle_stor(int fd, int data_fd, char* path, char* name);
static void handle_retr(int fd, int data_fd, char* path, char* name);
//功能函数声明结束

//全局变量声明 (finished)
int dir_num = -1;
int offset = 0;
//全局变量声明结束

//主函数声明 (finish)
static void handle_client(int fd);
//主函数声明结束

// 入口函数 (finished)
int main(int argc, char* argv[]) {
    int server_socket, client_socket;
    //连接端口.默认为6789
    int connect_port = 6789;
    struct sockaddr_in server_addr;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    //int status = 0;
    char cur_dir[100] = "";
    char tmp_dir[100] = "/tmp";
    //char* getcwd(char* buf, size_t size);将当前工作目录的绝对路径复制到buf中.size为buf的空间大小.
    getcwd(cur_dir,100);
    dir_num = strlen(cur_dir);
    //解析-port和-root指令
    for(int i = 0; i < argc; i++){
        if(!strcmp(argv[i], "-port")) {
            connect_port = atoi(argv[i + 1]);
        }
        if(!strcmp(argv[i], "-root")) {
            strcpy(tmp_dir, argv[i + 1]);
            tmp_dir[strlen(argv[i + 1])] = '\0';
        }
    }
    strcat(tmp_dir,"/");
    //int chdir(const char* path);将当前的工作目录改变为path
    chdir(tmp_dir);


    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    //初始化server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(connect_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &server_addr, sizeof(server_addr)) == -1) {
        perror("setsockopt error");
        exit(1);
    }
    bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    while (1) {
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

        if (fork() == 0) {
            handle_client(client_socket);
            exit(0);
        } else {
            // Clear zombie process
            int status;
            while (waitpid(-1, &status, WNOHANG) > 0);
        }
    }
    close(server_socket);
    return 0;
}

//功能函数定义
static int handle_login(int fd, char* sentence)
{
    //判断用户名是否为anonymous。若是，修改登录状态flag，给出提示，要求接受一句PASS some_password指令
    if (!strcmp(sentence, "USER anonymous")) {
        write(fd, "331 User anonymous accepted\r\n", strlen("331 User anonymous accepted\r\n"));
        return 1;
    }
        //若不是，给出提示
    else{
        write(fd, "400 Unsupported user\r\n", strlen("400 Unsupported user\r\n"));
        return 0;
    }
}
static int get_command(char* command,char* sentence)
{
    int num;
    char* blank = strchr(sentence,' ');
    if (blank) {
        num = (int)(blank - sentence);
    }
    else {
        num = strlen(sentence);
    }
    memset(command,0,6);
    memmove(command, sentence,num);
    memmove(sentence,sentence + num + 1,strlen(sentence) - num);
    command[num] = '\0';
    return num;
}
static void handle_pass(int fd) {
    write(fd, "230 Password correct\r\n", strlen("230 Password correct\r\n"));
}
static int handle_pasv(int fd, struct sockaddr_storage *addr, socklen_t *addr_len){
    int server_socket, client_socket;
    //服务端和客户端的addr
    struct sockaddr_in server_addr, client_addr;
    //新开的addr
    struct sockaddr_storage new_addr;
    size_t client_addr_len = sizeof(client_addr);
    socklen_t new_addr_len;
    //储存新接口的ip地址
    char* ip_addr;
    //储存新开的两个端口
    char port_str1[10], port_str2[10];
    //等待返回client的消息,下同
    char info_back[100] = "227 Passive mode activated: ";
    int port_addr;

    //int socket(int domain, int type, int protocol);建立一个新的socket通信端口
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    //htons();将主机字节顺序转换为网络字节顺序
    server_addr.sin_port = htons(0);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    new_addr_len = sizeof(new_addr);
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
    //int bind(int sockfd, struct sockaddr* my_addr, int addrlen);给sockfd的socket一个名称,此名称由my_addr指向一个sockaddr结构.
    bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    //int listen(int s, int backlog);等待s的socket连线.backlog指定同时能处理的最大连接要求
    //仅仅只是设置socket为listen模式,并未开始接收连线.
    listen(server_socket, 10);
    //获取端口的ip地址
    getsockname(fd, (struct sockaddr* )addr, addr_len);
    getsockname(server_socket, (struct sockaddr* )&new_addr, &new_addr_len);
    //char* inet_ntoa(struct in_addr);将ip地址从点数格式转换成无符号长整型
    ip_addr = inet_ntoa(((struct sockaddr_in* )(struct sockaddr* )addr)->sin_addr);
    //ntohs();把unsigned short类型从网络序转换到主机序
    port_addr = ntohs(((struct sockaddr_in* )(struct sockaddr* )&new_addr)->sin_port);
    //把新端口存入两个port_str内
    sprintf(port_str1,"%d", port_addr / 256);
    sprintf(port_str2,"%d", port_addr % 256);
    //把.换成,
    for (int i = 0;i < strlen(ip_addr); i++) {
        if (ip_addr[i] == '.')
            ip_addr[i] = ',';
    }
    //将新ip地址和新端口连接到返回的信息后
    strcat(ip_addr,",");
    strcat(ip_addr, port_str1);
    strcat(ip_addr,",");
    strcat(ip_addr, port_str2);
    strcat(info_back, ip_addr);
    strcat(info_back,"\r\n");
    //显示成功进入PASV模式的提示
    write(fd, info_back, strlen(info_back));
    //int accept(int s, struct sockaddr* addr, int* addrlen);接受s的socket连线.
    client_socket = accept(server_socket, (struct sockaddr *) &client_addr, (socklen_t *)&client_addr_len);
    close(server_socket);
    //返回的是accept()函数的运行结果
    return client_socket;
}
//
static int handle_port(int fd, struct sockaddr_storage *addr, socklen_t *addr_len, char *new_ip_addr){
    int flag = 0;
    int i;
    int mark, port, data_fd;
    char ip_str[100];
    memset(ip_str, '\0', 100);
    char tmp_str1[100], tmp_str2[100];

    struct sockaddr_in server_addr;
    for (i=0; i < strlen(new_ip_addr); i++, flag += (new_ip_addr[i] == ',')) {
        if (flag <= 4) {
            if (new_ip_addr[i] == ',') {
                new_ip_addr[i] = '.';
                if (flag == 4)
                    memmove(ip_str, new_ip_addr, i);
                mark = i + 1;
            }
        }
        else {
            if (new_ip_addr[i] == ',') {
                new_ip_addr[i] = '.';
                memset(tmp_str1,'\0',10);
                memset(tmp_str2,'\0',10);
                memmove(tmp_str1, new_ip_addr + mark,i - mark);
                memmove(tmp_str2, new_ip_addr + i + 1, strlen(new_ip_addr) - 1 - i);
                port = atoi(tmp_str1) * 256 + atoi(tmp_str2);
                break;
            }
        }
    }
    if ((data_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    //int inet_pton(int family, const char* strptr, void* addr_ptr) ;转换由strptr指针所指的字符串,在addrptr存放二进制结果
    if (inet_pton(AF_INET, ip_str, &server_addr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    //int connect(int fd, const struct sockaddr* name, int namelen);建立与socket的连接
    if (connect(data_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    else
    {
        printf("Connection done\n");
    }
    write(fd, "200 PORT mode activated: \r\n", strlen("200 PORT mode activated: \r\n"));
    return data_fd;
}

static void handle_stor(int fd, int data_fd, char* path, char* name){
    char sentence[SENTENCE_SIZE];
    memset(sentence,'\0', SENTENCE_SIZE);
    char filename[100];
    strcpy(filename, path);
    strcat(filename, name);
    int ret;
    FILE *fp = fopen(filename,"w");
    while (1) {
        ret = read(data_fd, sentence, SENTENCE_SIZE);
        if (ret == -1) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            break;
        }
        else if (ret)
            fwrite(sentence, sizeof(char), ret, fp);
        else
            break;
    }
    close(data_fd);
    fclose(fp);
    write(fd,"226 Transfer complete\r\n",strlen("226 Transfer complete\r\n"));
}
static void handle_retr(int fd, int data_fd, char* path, char* name){
    char sentence[SENTENCE_SIZE];
    char file_name[100];
    char info_back[100];
    strcpy(file_name, path);
    strcat(file_name, name);
    sprintf(info_back,"150 Opening BINARY mode data connection for %s\r\n", name);
    write(fd, info_back, strlen(info_back));
    FILE *fp = fopen(file_name,"r");
    fseek(fp, offset, SEEK_SET);
    while (1) {
        int tmp_i = fread(sentence, sizeof(char), SENTENCE_SIZE, fp);
        if(tmp_i)
            write(data_fd, sentence, tmp_i);
        else
            break;
    }
    offset = 0;
    close(data_fd);
    fclose(fp);
    write(fd,"226 Transfer complete\r\n",strlen("220 Transfer complete\r\n"));
}
static void handle_list(int fd, int data_fd, char* path) {
    char sentence[SENTENCE_SIZE];
    char ls_command[100];
    FILE *stream;
    write(fd, "150 Here comes the directory listing\r\n", strlen("150 Here comes the directory listing\r\n"));
    strcpy(ls_command,"ls -nN");
    stream = popen(ls_command,"r");
    while (1) {
        if(fgets(sentence, SENTENCE_SIZE, stream))
            write(data_fd, sentence, strlen(sentence));
        else
            break;
    }
    close(data_fd);
    write(fd, "226 Directory send OK\r\n", strlen("226 Directory send OK\r\n"));
}
static void handle_mkd(int fd, char* path, char* dir_path) {
    char my_dir[100];
    char output_str[200];
    int test = mkdir(dir_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (test == 0) {
        strcpy(my_dir, path);
        strcat(my_dir, dir_path);
        sprintf(output_str,"257 %s created\r\n", my_dir);
    }
    else{
        sprintf(output_str,"550 Create directory operation failed\r\n");
    }
    write(fd,output_str, strlen(output_str));
}
static void handle_cwd(int fd, char* path, char* new_path) {
    char mydir[200];
    char patten[100] = "250 Directory successfully changed.\r\n";
    char patten2[100] = "550 Failed to change directory.\r\n";
    //sprintf(mydir,"./%s/",new_path);
    sprintf(mydir, "%s",new_path);
    if (chdir(mydir)==0) {
        write(fd,patten,strlen(patten));
    }
    else {
        write(fd, patten2,strlen(patten2));
    }
    getcwd(path,100);
}
static void handle_pwd(int fd) {
    char tmp_path[100];
    char info_back[200];
    getcwd(tmp_path,100);
    sprintf(info_back,"257 \"%s\"\r\n", tmp_path);
    write(fd, info_back, strlen(info_back));
}
static void handle_rmd(int fd, char* path, char* dir_path) {
    char my_dir[100];
    char info_back[100];
    int test = rmdir(dir_path);
    if (!test) {
        strcpy(my_dir, path);
        strcat(my_dir, dir_path);
        sprintf(info_back,"250 Delete success\r\n");
    }
    else{
        sprintf(info_back,"550 Delete directory operation failed.\r\n");
    }
    write(fd, info_back, strlen(info_back));
}
static int handle_rnfr(int fd, char* origin_name) {
    return (access(origin_name, F_OK));
}
static void handle_rnto(int fd, char* origin_name, char* new_name) {
    if (rename(origin_name, new_name)==0){
        write(fd, "250 Rename successful.\r\n", strlen("250 Rename successful.\r\n"));
    }
    else{
        write(fd, "421 Rename failed.\r\n", strlen("421 Rename failed.\r\n"));
    }
}
//功能函数定义结束

//主函数定义
void handle_client(int fd) {
    struct sockaddr_storage data_addr;
    int data_fd = -1;
    socklen_t data_addr_len;
    char sentence[SENTENCE_SIZE];
    char command[6] = "test";
    char tmp_password[100] = "";
    char download_path[100] = "";
    char upload_path[100] = "";
    size_t sentence_end = 0, i, len;
    int ret, rename_flag = 0;
    char rename_str[100];
    int status = 0;
    data_addr_len = sizeof(data_addr);
    write(fd, "220 Anonymous FTP server ready.\r\n", strlen("220 Anonymous FTP server ready.\r\n"));
    while (1) {
        ret = read(fd, sentence + sentence_end, SENTENCE_SIZE - sentence_end);
        if (ret == -1) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            break;
        }
        else if (ret == 0) {
            printf("break\n");
            break;
        }
        else {
            i = sentence_end;
            sentence_end += ret;
            while(i <= sentence_end) {
                for (; i < sentence_end && sentence[i] != '\n'; ++i);
                if (i != sentence_end) {
                    len = i != 0 && sentence[i - 1] == '\r' ? i - 1 : i;
                    sentence[len] = '\0';
                    if (status == 0) {
                        status = handle_login(fd, sentence);
                    }
                    else {
                        int num = get_command(command, sentence);
                        if(status == 1) {
                            if(!strcmp(command, "PASS")) {
                                memmove(tmp_password, sentence, sentence_end -num-1);
                                handle_pass(fd);
                                status = 2;
                            }
                        }
                        else  if(!strcmp(command, "SYST")) {
                            write(fd, "215 UNIX Type: L8\r\n", strlen("215 UNIX Type: L8\r\n"));
                        }
                        else  if(!strcmp(command, "TYPE")) {
                            int str_num = strlen(sentence);
                            int flag = 0;
                            if(str_num == 1)
                                if (sentence[0] == 'I')
                                    flag = 1;
                            if(flag) {
                                write(fd, "200 Type set to I.\r\n", strlen("200 Type set to I.\r\n"));
                            }
                            else {
                                write(fd, "501 Syntax error.\r\n", strlen("501 Syntax error.\r\n"));
                            }
                        }
                        else  if(!strcmp(command, "PASV")) {
                            data_fd = handle_pasv(fd, &data_addr, &data_addr_len);
                        }
                        else  if(!strcmp(command, "PORT")) {
                            //port
                            char tmp_str[100];
                            memmove(tmp_str,sentence,sentence_end - num);
                            data_fd = handle_port(fd, &data_addr, &data_addr_len, tmp_str);
                        }
                        else if (!strcmp(command, "STOR")) {
                            struct sockaddr_in tmp_addr;
                            socklen_t tmp_addr_len = sizeof(tmp_addr);
                            int test = getsockname(data_fd, (struct sockaddr*)&tmp_addr, &tmp_addr_len);
                            if (test == 0) {
                                write(fd,"150 Prepare to send data\r\n",strlen("150 Prepare to send data\r\n"));
                                char tmp_str[100];
                                memmove(tmp_str, sentence,sentence_end-num);
                                handle_stor(fd, data_fd, upload_path, tmp_str);
                            }
                            else{
                                write(fd,"425 Link not prepared\r\n",strlen("425 Link not prepared\r\n"));
                                printf("Error stor(): %s(%d)\n", strerror(errno), errno);
                            }
                        }
                        else if (!strcmp(command, "RETR")) {
                            char tmp_str[100];
                            memmove(tmp_str, sentence,sentence_end - num);
                            handle_retr(fd, data_fd, download_path, tmp_str);
                        }
                        else if (!strcmp(command, "REST")) {
                            char tmp_str[100];
                            memmove(tmp_str, sentence, sentence_end - num);
                            offset = atoi(tmp_str);
                            write(fd, "350 Restarting. Send RETR next\r\n", strlen("350 Restarting. Send RETR next\r\n"));
                        }
                        else if (!strcmp(command, "LIST")) {
                            handle_list(fd, data_fd, download_path);
                        }
                        else if (!strcmp(command, "MKD")) {
                            char tmp_str[100];
                            memmove(tmp_str, sentence,sentence_end - num);
                            handle_mkd(fd, download_path, tmp_str);
                        }
                        else  if (!strcmp(command, "CWD")) {
                            char tmp_str[100];
                            memmove(tmp_str,sentence,sentence_end-num);
                            handle_cwd(fd, download_path, tmp_str);
                        }
                        else  if (!strcmp(command, "PWD")) {
                            handle_pwd(fd);
                        }
                        else  if (!strcmp(command, "RMD")) {
                            char tmp_str[100];
                            memmove(tmp_str,sentence,sentence_end-num);
                            handle_rmd(fd, download_path, tmp_str);
                        }
                        else  if (!strcmp(command, "RNFR")) {
                            char tmp_str[100];
                            memmove(tmp_str, sentence,sentence_end-num);
                            memmove(rename_str, sentence,sentence_end-num);
                            if (!handle_rnfr(fd, tmp_str)) {
                                write(fd, "350 Prepare for RNTO.\r\n", strlen("350 Prepare for RNTO.\r\n"));
                            }
                            else {
                                write(fd, "421 Service unavailable, control connection closed\r\n", strlen("421 Service unavailable, control connection closed\r\n"));
                            }
                            rename_flag = 1;
                        }
                        else  if (!strcmp(command, "RNTO")) {
                            if (rename_flag) {
                                char tmp_str[100];
                                memmove(tmp_str, sentence, sentence_end-num);
                                handle_rnto(fd, rename_str, tmp_str);
                            }
                            else {
                                write(fd, "503 No RNFR before.\r\n", strlen("503 No RNFR before.\r\n"));
                            }
                        }
                        else  if (!strcmp(command, "QUIT")) {
                            write(fd, "221 Bye\r\n", strlen("221 Bye\r\n"));
                        }

                    }
                    ++i;
                    if (i != sentence_end)
                        memmove(sentence, sentence + i, sentence_end - i);
                    sentence_end -= i;
                }
            }
        }
    }
    close(fd);
}
//主函数定义结束