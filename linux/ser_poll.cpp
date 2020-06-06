#include<unistd.h>
#include<strings.h>
#include<sys/types.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<sys/wait.h>
#include<poll.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<vector>
#include<iostream>
using namespace std;

/*#define ERR_EXIT(m) \
    do\
    {\
        perror(m);\
        exit(EXIT_FAILURE);\
        
    }while(0)
*/
typedef vector<struct pollfd> PollFdList;

int main()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int listenfd;

    if((listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) < 0)
    {
        perror("socket");
        exit(-1);
    }
    struct sockaddr_in servaddr;
    // void bzero(void *s, size_t n);
    bzero(&servaddr,sizeof(servaddr));
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int on = 1;

    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)<0))
    {
        perror("setsockopt");
        exit(-1);
    }
    if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
    {
        perror("bind");
        exit(-1);
    }
    if(listen(listenfd,128)<0)
    {
        perror("listen");
        exit(-1);
    }

    struct pollfd pfd;
    pfd.fd = listenfd;
    pfd.events = POLLIN;

    PollFdList pollfds;
    pollfds.push_back(pfd);

    int nready;

    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int connfd;

    while(1)
    {
        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
        nready = poll(&*pollfds.begin(),pollfds.size(),-1);
        if(nready == -1)
        {
            if(errno == EINTR)
                continue;
            perror("poll");
            exit(-1);
        }
        if(nready == 0)  //没有事件发生
            continue;
        //处理listenfd揽客
        if(pollfds[0].revents&POLLIN)
        {    
            peerlen = sizeof(peeraddr);
            connfd = accept4(listenfd,(struct sockaddr*)&peeraddr,&peerlen,SOCK_NONBLOCK | SOCK_CLOEXEC);

            if(connfd == -1)
            {
                if (errno == EMFILE)
				{
					close(idlefd);
					idlefd = accept(listenfd, NULL, NULL);
					close(idlefd);
					idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
					continue;
				}
				else
                {
                    perror("listen");
                    exit(-1);
                }
            }
            
            pfd.fd = connfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pollfds.push_back(pfd);
            --nready;

            cout<<"ip = "<<inet_ntoa(peeraddr.sin_addr)<<" port = "<<ntohs(peeraddr.sin_port)<<endl;
            if(nready == 0)
                continue;


        }
        //处理响应事件
        for(PollFdList::iterator it = pollfds.begin()+1;it!=pollfds.end()&&nready>0;++it)
        {
            if(it->revents&&POLLIN)
            {
                --nready;
                connfd = it->fd;
                char buf[BUFSIZ];
                int ret = read(connfd,buf,sizeof(buf));
                if(ret ==-1)
                {
                    perror("listen");
                    exit(-1);
                }
                if(ret == 0)
                {
                    cout<<"client close"<<endl;
                    it = pollfds.erase(it);
                    --it;

                    close(connfd);
                    continue;
                }
                for(int i = 0;i<ret;i++)
                {
                    buf[i] = toupper(buf[i]);
                }
                cout<<buf;
                write(connfd,buf,ret);
            }
        }
    }
    return 0;
}




















