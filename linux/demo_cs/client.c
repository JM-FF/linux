#include<stdio.h>
#include<unistd.h>
#include <sys/socket.h>
#include<sys/types.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<strings.h>
#include<string.h>

#define SERV_PORT 6666
#define SERV_IP "127.0.0.1"

int main()
{
	int cfd;
	struct sockaddr_in serv_addr;
	//#include <strings.h> void bzero(void *s, size_t n);
	bzero(&serv_addr,sizeof(serv_addr));

	char buf[BUFSIZ];

	cfd = socket(AF_INET, SOCK_STREAM,0);

	//初始化serv_addr
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	//#include <arpa/inet.h> int inet_pton(int af, const char *src, void *dst);
	inet_pton(AF_INET,SERV_IP, &serv_addr.sin_addr.s_addr);

	// int connect(int sockfd, const struct sockaddr *addr,  socklen_t addrlen);
	connect(cfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));//建立连接

	while(1)
	{
		fgets(buf,sizeof(buf),stdin);//从终端获取输入
		write(cfd,buf,strlen(buf)); //写向服务器
		int	n = read(cfd,buf,sizeof(buf)); //从服务器读取
		write(STDOUT_FILENO,buf,n); //写向屏幕打印
	}
	close(cfd);
	return 0;
}
