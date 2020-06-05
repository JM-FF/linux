#include<stdio.h>
#include<unistd.h>
#include <sys/socket.h>
#include<sys/types.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<strings.h>

#define SER_PORT 6666

int main()
{
	int sock,connfd; //接收socket返回值
	struct sockaddr_in server_addr , client_addr;
	char buf[BUFSIZ];
	char client_ip[BUFSIZ];
	int len;
	//创建一个套接字
	sock = socket(AF_INET,SOCK_STREAM,0); //returns a file descriptor
	//将结构体清零
	bzero(&server_addr,sizeof(server_addr));
	//bzero(&client_addr,sizeof(client_addr));
	//对结构体初始化
	server_addr.sin_family = AF_INET;//选择IPV4
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//监听本地所有IP
	server_addr.sin_port = htons(SER_PORT);//绑定端口

	bind(sock ,(struct sockaddr *)&server_addr,sizeof(server_addr));
	listen(sock,128);
	// int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	int c_len = sizeof(client_addr);
	connfd = accept(sock,(struct sockaddr *)&client_addr,&c_len); //返回值是一个新的文件描述符
	printf("client ip: %s\t port: %d\n",
			inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,client_ip,sizeof(client_ip)),
			ntohs(client_addr.sin_port));

	while(1)
	{
		// #include <unistd.h>  ssize_t read(int fd, void *buf, size_t count);
		len = read(connfd,buf,sizeof(buf));
		for(int i = 0 ;i<sizeof(buf);i++)
		{
			// #include <ctype.h> int toupper(int c); int tolower(int c);
			buf[i] = toupper(buf[i]);
		}
		// ssize_t write(int fildes, const void *buf, size_t nbyte);
		write(connfd,buf,len);
	}
	close(connfd);
	close(sock);
	return 0;
}
