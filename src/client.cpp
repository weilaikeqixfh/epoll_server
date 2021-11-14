#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>

int main(int argc,char *argv[])
{
	if(argc!=3)
	{
		printf("using :./client ip port\nExample:./client 127.0.1 5005\n\n");
		return -1;
	}
	//第一步：创建客户端socket；
	int sockfd;
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("sockfd");
		return -1;
	}
	//第二步：向服务器发起连接请求
	
	struct sockaddr_in myserveraddr;
	int port=atoi(argv[2]);
	bzero(&myserveraddr,sizeof(myserveraddr));
	myserveraddr.sin_family=AF_INET;
	myserveraddr.sin_port=htons(port);
	inet_pton(AF_INET,argv[1],&myserveraddr.sin_addr);
	if(connect(sockfd,(struct sockaddr *)&myserveraddr,sizeof(myserveraddr))!=0)
	{
		perror("connect");
		close(sockfd);
		return -1;
	}
	//第三步：与服务器通信，发送一个报文等待回复，然后等待下一个报文
	char buffer[1024];
	for(int i=0;i<1;i++)
   {
	bzero(&buffer,sizeof(buffer));
	int iret;
	int j=12;
	sprintf(buffer,"%d",j);
	if((iret=send(sockfd,buffer,strlen(buffer),0))<=0)
	{
		perror("send");
	//	break;
	}
	printf("发送:%s\n",buffer);	
	bzero(&buffer,sizeof(buffer));
	if((iret=recv(sockfd,buffer,strlen(buffer),0))<=0)
	{
		perror("recv");
	}
	printf("接收:%s\n",buffer);
   }
//第四步：关闭socket，释放资源。
close(sockfd);
}
