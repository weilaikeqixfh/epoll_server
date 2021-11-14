#include <sys/socket.h>
#include <netinet/in.h>
#include<sys/epoll.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<assert.h>
#define MAX_EVENT_NUMBER 1024
#define RECV_BUFF_SIZE 1024


  //使用ET模式的文件描述符都应该是非阻塞的。 如果 文件描述符是阻塞的，那么读或写操作将会因为没有后续的事件而一直处于阻塞状态(饥渴状态)。
  //将文件描述符设置为非阻塞的
	int setnonblocking(int fd)
	{
	 int old_option = fcntl(fd,F_GETFL);
	 int new_option = old_option | O_NONBLOCK;
	 fcntl(fd,F_SETFL,new_option);
	 return old_option;//返回文件描述符旧的状态，以便日后恢复该状态；
	}

  //addfd函数将文件描述符fd上的事件注册到epollfd指示的epoll内核事件表中，启用ET(边沿触发）模式，默认是水平触发；one_shot选择是否注册EPOLLONESHOT事件
  	void addfd(int epollfd,int fd ,bool one_shot)
	{
  //创建要监听操作的文件描述符结构体
 	epoll_event event;
	event.data.fd=fd;
	event.events=EPOLLIN|EPOLLET;//开启边沿触发模式
	
  //我们期望的是一个socket连接在任一时刻都只被一个 线程处理。 这一点可以使用epoll的EPOLLONESHOT事件实现。
	if(one_shot)
	{event.events|=EPOLLONESHOT;}	
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	setnonblocking(fd);
	}


	void reset_oneshot(int epollfd,int fd)
	{epoll_event event;
	event.data.fd=fd;
	event.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
	epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);}
	

int main(int argc,char *argv[])
{
	if (argc!=2)
	  {
    		printf("Using:./server port\nExample:./server 5005\n\n"); return -1;
  	  }
  //创建地址结构体
	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//任意IP地址；
	serveraddr.sin_port = htons(atoi(argv[1]));//指定通信端口；
  //创建监听socket文件描述符	
	int listenfd = socket(PF_INET,SOCK_STREAM,0);
	assert(listenfd!=-1);
  //绑定socket和它的地址
  	int ret=0;
	ret=bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	assert(ret!=-1);
  //创建监听队列以存放待处理的客户连接；
	ret=listen(listenfd,5);
	assert(ret!=-1);

	
  //创建epoll内核事件表
 	int epollfd=epoll_create(20);
  // 用于存储epoll事件表中就绪事件的event数组
	epoll_event events[MAX_EVENT_NUMBER];
  //主线程往epoll内核事件表中注册监听socket事件，当listen到新的客户连接时，listenfd变为就绪事件;listenfd上不能注册EPOLLONESHOT事件，否则后续的客户连接请求不能触发listenfd上的EPOLLIN事件 
	addfd(epollfd, listenfd, false);
	while(1)
	{
		//epoll_ wait 函数 如果 检测 到 事件， 就 将 所有 就绪 的 事件 从内 核 事件 表（epollfd） 中 复制 到 它的 第二个 参数 events 指向 的 数组 中。
		int ret=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
		if(ret<0)
		{printf("epoll failure\n");
		break;}
		for(int i=0;i<ret;i++)
		{int sockfd = events[i].data.fd;// 事件表中就绪的socket文件描述符
                if(sockfd == listenfd)//如果就绪的是监听套接字，监听到新用户连接
		{struct sockaddr_in client_address;
       		 socklen_t client_addrlength = sizeof(client_address);
		/* accept()返回一个新的socket文件描述符用于send()和recv() */
          	int clientfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
		//对每个非监听文件描述符设置为边沿触发模式，并且注册EPOLLSHOT事件
		addfd(epollfd,clientfd,true);}
		
		else if(events[i].events&EPOLLIN)//如果就绪的是其他套接字可读事件，epoll_wait通知主线程
		{
			char buffer[RECV_BUFF_SIZE];
			memset(buffer,0,RECV_BUFF_SIZE);
			while(1)
			{
				int ret=recv(sockfd,buffer,RECV_BUFF_SIZE-1,0);
				if(ret==0)
				{close(sockfd);
				printf("client closed the connection\n");
				break;}
				else if(ret<0)
				{if(errno==EAGAIN)
				{reset_oneshot(epollfd,sockfd);
				printf("read later\n");
				break;}
				}
				else
				{printf("end thead receiving data on fd:%d\n",sockfd);}
			}
		}
		else
		{printf("something else happen\n");}
	}
	}
	close(listenfd);
	return 0;
}




