#ifndef __M_EPOLL_H__
#define __M_EPOLL_H__
#include "tcpsocket.h"
#include <vector>
#include <sys/epoll.h>

#define MAX_EPOLL 1024

class Epoll{
    private:
        int _epfd;
    public:
        //构造函数
        Epoll():_epfd(-1){}

        //初始化
        bool EpollInit(){
            _epfd = epoll_create(MAX_EPOLL);
            if(_epfd < 0){
                cerr << "epoll_create error" << endl;
                return false;
            }

            return true;
        }

        //添加文件描述符
        bool AddSock(TcpSocket& sock){
            //获取文件描述符
            int fd = sock.GetFd();

            //创建epoll_event事件结构体
            epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fd;
            
            int ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev);
            if(ret < 0){
                cerr << "add fd error" << endl;
                return false;
            }

            return true;
        }
        
        //删除文件描述符
        bool DelSock(TcpSocket& sock){
            //获取文件描述符
            int fd = sock.GetFd();

            int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
            if(ret < 0){
                cerr << "del fd error" << endl;
                return false;
            }

            return true;
        }


        //获取就绪的文件描述符
        bool WaitSock(vector<TcpSocket>& list, int timeout = 2000){
            //创建epoll_event结构体数组
            epoll_event evs[MAX_EPOLL];

            int nfds = epoll_wait(_epfd, evs, MAX_EPOLL, timeout);
            if(nfds < 0){
                cerr << "epoll_wait error" << endl;
                return false;
            }
            else if(nfds == 0){
                cerr << "epoll wait timeout" << endl;
                return false;
            }

            for(int i = 0; i < nfds; i++){
                int fd = evs[i].data.fd;

                TcpSocket sock;
                sock.SetFd(fd);

                list.push_back(sock);
            }

            return true;
        }
};
#endif
