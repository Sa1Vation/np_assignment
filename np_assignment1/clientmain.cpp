#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Included to get the support library
#include "calcLib.h"

#define PORT 4950					//准备要链接服务端的端口号
#define MAXDATASIZE 1500			//接受来自服务端信息的数组长度


int main(int argc, char *argv[]){
  
  	/* Do magic */
	int sockfd;						//客户端的套接字
	char buf[MAXDATASIZE];			//客户端存储服务端信息的数组
	struct hostent *he;				//存储服务端的主机信息
	struct sockaddr_in server;		//存储服务端的地址信息

	int numbytes;					//存储每次接受来自服务端的字节数多少	

	//判断输入参数数量正确与否
	if (argc != 2) {
	  	printf("Error: argument number wrong. dir: %s argument num: (%d)\n",argv[0],argc);
	  	exit(1);
	}

	printf("client: getting server info\n");
	//获取服务端主机信息
	if((he=gethostbyname(argv[1]))==NULL){
		printf("Error: client cannot get server host info\n");
		exit(1);
	}

	printf("client: creating a socket\n");
	//创建客户端套接字
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error: client cannot create a socket\n");
		exit(1);
	}

	bzero(&server,sizeof(server));	//初始化服务地址信息，以免受到之前存过的东西影响
	server.sin_family=AF_INET;
	server.sin_port=htons(PORT);
	server.sin_addr=*((struct in_addr*)he->h_addr);

	printf("client: connecting to server\n");
	if(connect(sockfd,(struct sockaddr*)&server,sizeof(server))==-1){	//请求与服务端建立连接
		printf("Error: client cannot connect to server\n");  
       	exit(1);
	}

	printf("client: establish a connection to server %s:%d\n",inet_ntoa(server.sin_addr),htons(server.sin_port));

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {		//接收来自服务端的第一批信息(即支持的协议)
	  	printf("Error:client cannot receive data from server\n");
	  	exit(1);
	}
	
	buf[numbytes] = '\0';	//为了在后续调用strcmp函数时与字符串常量成功进行比较，即模拟字符串常量以"\0"作为结尾
	
	printf("server: support communicating protocols as follows:\n\"%s\"\n",buf);
	char *serverSupportProtocol;	//保存服务端支持的协议名
	bool supportServerProtocol=false;	//标记是否已经从服务端支持的协议堆中找到自己也支持的协议

	serverSupportProtocol = strtok(buf, "\n");	//分割字符串，将一个个协议分割出来
   
   	//如果找到，则返回OK.如果没找到，则返回Not OK，并终止通讯
   	while( serverSupportProtocol != NULL ) {
     	if(strcmp(serverSupportProtocol,"TEXT TCP 1.0")==0){
     		const char *response="OK";
     		send(sockfd, response, sizeof(response), 0);
     		supportServerProtocol=true;
     		printf("client: OK, achieve agreement on communicating protocol with server\n");
     		break;
     	}
      	serverSupportProtocol = strtok(NULL, "\n");
   	}
   	if(!supportServerProtocol){
   		printf("Client do not support server-side communicating protocol\n");
   		send(sockfd, "Not OK", 7, 0);
   		close(sockfd);
   		exit(1);
   	}

   	memset(buf,0,sizeof(buf));
   	if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {		//接收来自服务端的计算题目
	  	printf("Error in receiving data from server\n");
	  	close(sockfd);
	  	exit(1);
	}
	char *operation;	//保存计算类型（加减乘除）
	operation = strtok(buf, " ");
	double fresult;
	int result;
	char answer[100];	//保存自己计算出的答案
	if(operation[0]=='f'){
		double value1=atof(strtok(NULL, " "));
		double value2=atof(strtok(NULL, " "));
		if(operation[1]=='a'){
			fresult=value1+value2;
		}else if(operation[1]=='s'){
			fresult=value1-value2;
		}else if(operation[1]=='m'){
			fresult=value1*value2;
		}else{
			fresult=value1/value2;
		}
		sprintf(answer,"%8.8g",fresult);
		printf("client: receive command \"%s %f %f\" from server\nclient: send answer %s back to server\n",operation,value1,value2,answer);
	}else{
		int value1=atoi(strtok(NULL, " "));
		int value2=atoi(strtok(NULL, " "));
		if(operation[0]=='a'){
			result=value1+value2;
		}else if(operation[0]=='s'){
			result=value1-value2;
		}else if(operation[0]=='m'){
			result=value1*value2;
		}else{
			result=value1/value2;
		}
		sprintf(answer,"%d",result);
		printf("client: receive command \"%s %d %d\" from server\nclient: send answer %s back to server\n",operation,value1,value2,answer);
	}
	send(sockfd, answer, sizeof(answer), 0);						//返回答案给服务端

	memset(buf,0,sizeof(buf));										//初始化重新利用接受消息数组
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {		//接收服务端计算出的正确答案
	  	printf("Error in receiving data from server\n");
	  	close(sockfd);
	  	exit(1);
	}
	printf("server: the right answer should be %s\n",buf);

	memset(buf,0,sizeof(buf));
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {		//接受服务端返回的计算结果状态(Wrong/Right)
	  	printf("Error in receiving data from server\n");
	  	close(sockfd);
	  	exit(1);
	}
	printf("server: %s\n",buf);


	close(sockfd);	//任务完成，关闭连接
	printf("client: program shutdown.\n");


	return 0;

}
