#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


#include <calcLib.h>

#define PORT 4950   //服务器端口号
#define MAXSZ 1500  //输入缓存数组的最大长度  

#define BACKLOG 1   //允许多少个客户端同时连接服务端


int main(int argc, char *argv[]){
  int sock_fd, new_fd;  //前者是监听套接字，后者是专门与客户端打交道的套接字
  struct sockaddr_in server;  //服务端的地址结构体
  struct sockaddr_in client;  //客户端的地址结构体
  socklen_t addrlen;          //地址长度
  int clientCount=0;          //当前客户端是第几个连上的，在服务端终端上来区分标记不同客户端的信息

  if((sock_fd=socket(AF_INET,SOCK_STREAM,0))==-1){  //创建监听套接字
    perror("server: socket");
    printf("server: bind socket error\n");
    exit(1);
  }

  printf("server: setting socket options\n");  
  int opt=SO_REUSEADDR;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt)) == -1) {    //设置监听套接字选项
    printf("server: set socket error\n");
    perror("setsockopt");
    exit(1);
  }

  bzero(&server,sizeof(server));    //初始化服务端地址结构体
  server.sin_family = AF_INET;      
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = htonl(INADDR_ANY); 

  printf("server: binding socket\n");     
  if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {    //将监听套接字与本地地址绑定
    perror("server: bind");
    printf("server: bind socket error\n");
    exit(1);
  }

  printf("server: listening to socekt\n");
  if (listen(sock_fd, BACKLOG) == -1) {     //启动监听套接字
    perror("listen");
    printf("server:fail to listen to the socket\n");
    exit(1);
  }
  addrlen=sizeof(client);

  printf("server: waiting for connection...\n");
  char msg[MAXSZ];    //作为接受客户端信息的存储数组，并多次重复使用
  
  int readSize;

  //主while循环，一次只能与一个客户端对接，但在不同时间可以处理不同客户端
  while(1) {  
    printf("-------------client[%d]-------------\n",clientCount);
    if((new_fd=accept(sock_fd,(struct sockaddr*)&client,&addrlen))==-1){    //监听接收来自客户端的连接请求 
      perror("accept");
      printf("server: accept connection error\n");
      continue;
    }
    printf("server: establish a connection from client %s:%d\n",inet_ntoa(client.sin_addr),htons(client.sin_port));
 
    printf("server: Sending support communication protocols \n");
    if (send(new_fd, "TEXT TCP 1.0", 13, 0) == -1){   //发送服务端支持的协议
      perror("send");
      printf("server: send error\n");
      shutdown(new_fd, SHUT_RDWR);
      close(new_fd);
      continue;   //当发送不了时，放弃当前客户端并等待下一个客户端 
    }
    //内循环直至与客户端通讯结束或意外发生
    while(1){
      readSize=recv(new_fd,&msg,MAXSZ,0);   //接受来自客户端的信息
      printf("client: send status \"%s\"\n",msg);   

      msg[readSize]='\0';   //为了在后续调用strcmp函数时与字符串常量成功进行比较，即模拟字符串常量以"\0"作为结尾
      if(strcmp(msg,"OK")!=0){    //如果客户端不支持服务端的协议，则结束对话
        printf("error:client does not support server\'s communication protocols\n");
        shutdown(new_fd, SHUT_RDWR);
        close(new_fd);
        break;
      }

      initCalcLib();    //在调用外教提供的指令生成包前必须调用他提供的初始化函数
      char operationCommand[MAXSZ];   //待返回客户端的计算题目字符串
      char *operation=randomType();   //随机计算题的字符串
      double fresult=0;   //如果计算题是浮点计算，则将服务端计算结果存储在这里
      int result=0;       //如果计算题是整数计算，则将服务端计算结果存储在这里
      if(operation[0]=='f'){
        double value1=randomFloat(),value2=randomFloat();
        if(operation[1]=='a'){
          fresult=value1+value2;
        }else if(operation[1]=='s'){
          fresult=value1-value2;
        }else if(operation[1]=='m'){
          fresult=value1*value2;
        }else{
          fresult=value1/value2;
        }
        sprintf(operationCommand,"%s %f %f",operation,value1,value2);
      }else{
        int value1=randomInt(),value2=randomInt();
        if(operation[0]=='a'){
          result=value1+value2;
        }else if(operation[0]=='s'){
          result=value1-value2;
        }else if(operation[0]=='m'){
          result=value1*value2;
        }else{
          result=value1/value2;
        }
        sprintf(operationCommand,"%s %d %d",operation,value1,value2);
      }
      printf("server: send \"%s\" to client\n",operationCommand);

      if (send(new_fd, operationCommand, MAXSZ, 0) == -1){      //发送计算题给客户端
        perror("send");
        printf("server: send error\n");
        shutdown(new_fd, SHUT_RDWR);
        close(new_fd);
        continue; //leave loop execution, go back to the while, main accept() loop. 
      }

      memset(msg,0,MAXSZ);    //重新初始化接受客户端的消息容器，来重复使用
      readSize=recv(new_fd,&msg,MAXSZ,0);   //阻塞接收客户端的消息
      char resultString[MAXSZ];   //存储服务端的计算结果

      printf("client: my answer to \"%s\" is: %s\n",operationCommand,msg);
      //将之前的计算数值转化为字符串
      if(operation[0]=='f'){
        sprintf(resultString,"%8.8g",fresult);
      }else{
        sprintf(resultString,"%d",result);
      }

      printf("server: the right answer should be %s\n",resultString);
      send(new_fd,resultString,sizeof(resultString),0);   //将服务端的计算结果返回给客户端

      //比较服务端和客户端的计算结果并根据此返回OK还是ERROR给客户端
      if(strcmp(msg,resultString)==0){
        printf("server: right answer\n");
        memset(resultString,0,sizeof(resultString));  //初始化返回数组，重新利用
        sprintf(resultString,"OK");
      }else{
        printf("server: wrong answer\n");
        memset(resultString,0,sizeof(resultString));  //初始化返回数组，重新利用
        sprintf(resultString,"ERROR");
      }
      send(new_fd,resultString,sizeof(resultString),0);

      sleep(1);
      close(new_fd);
      printf("server: mission has done. stop connection.\n");
      break;
    }
    clientCount++;    //将所处理过的客户端数量加一，以计数
  }

  return 0;
}