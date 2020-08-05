#ifndef __M_SOCK_H__
#define __M_SOCK_H__
#include <iostream>
#include <string>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

#define MAX_REQUEST 8192

class TcpSocket{
    private:
        int _sockfd;
    public:
        //改变sockfd
        void SetFd(int sockfd){
            _sockfd = sockfd;
        }

        //获取sockfd
        int GetFd(){
            return _sockfd;
        }

        //将套接字设置为非阻塞
        void SetNonBlock(){
            int flag = fcntl(_sockfd, F_GETFL, 0);
            fcntl(_sockfd, F_SETFL, flag | O_NONBLOCK);
        }

        //初始化
        bool SocketInit(uint16_t& port){
            //创建套接字
            _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(_sockfd < 0){
                cerr << "socket error" << endl;
                return false;
            }
           
            //将套接字设置为地址重用
            int opt = 1;
            setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            //创建sockaddr_in结构体
            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            //INADDR_ANY 0.0.0.0 
            addr.sin_addr.s_addr = INADDR_ANY; 
            socklen_t len = sizeof(addr);

            //绑定地址信息
            int ret = bind(_sockfd, (sockaddr*)&addr, len);
            if(ret < 0){
                cerr << "bind error" << endl;
                close(_sockfd);
                return false;
            }
            
            //开始监听
            int backlog = 10;
            ret = listen(_sockfd, backlog);
            if(ret < 0){
                cerr << "listen error" << endl;
                close(_sockfd);
                return false;
            }

            //在外部将套接字设置为非阻塞

            return true;
        }

        //获取已连接的客户端
        bool Accept(TcpSocket& newSock){
            sockaddr_in addr;
            socklen_t len = sizeof(addr);

            int fd = accept(_sockfd, (sockaddr*)&addr, &len);
            if(fd < 0){
                cerr << "accept error" << endl;
                return false;
            }

            //相同类型的两个对象之间没有访问限制
            newSock._sockfd = fd;

            //将用于通信的套接字设置非阻塞
            newSock.SetNonBlock();


            return true;
        }

        //探测接受，判断接受的数据中是否有/r/n/r/n
        bool RecvPeek(string& buf){
            buf.clear();
            char temp[MAX_REQUEST] = { 0 };
            int ret = recv(_sockfd, temp, MAX_REQUEST, MSG_PEEK);
            
            if(ret < 0){
                //如果缓冲区没有数据
                if(errno == EAGAIN){
                    return true;
                }

                cerr << "recv error" << endl;
                return false;
            }
            else if(ret == 0) {
                cerr << "peer shutdown" << endl;
                return false;
            }

            buf.assign(temp, ret);
            
            return true;
        }
        

        //通过探测接收获取的要接受的数据长度len，直接接收数据
        bool Recv(string& buf, int len){
            buf.clear();
            buf.resize(len);

            int rlen = 0;   //已接收的数据长度
            while(rlen < len){
                int ret = recv(_sockfd, &buf[0] + rlen, len - rlen, 0);
                if(ret < 0){
                    //缓冲区中没有数据
                    if(errno == EAGAIN){
                        return true;
                    }

                    cerr << "recv error" << endl;
                    return false;
                }
                else if(ret == 0){
                    //连接断开
                    cerr << "peer shutdown" << endl;
                    return false;
                }
                
                //更新rlen
                rlen += ret;
            }

            return true;
        }

        //发送数据
        bool Send(const string& buf){
            int len = 0;
            while(len < buf.size()){
                int ret = send(_sockfd, &buf[len], buf.size() - len, 0);
                if(ret < 0){
                    if(errno == EAGAIN){
                        usleep(1000);
                        continue;
                    }
                    cerr << "send error" << endl;
                    return false;
                }
                len += ret;
            }
            return true;
        }


        //关闭套接字
        void Close(){
            if(_sockfd >= 0){
                close(_sockfd);
                _sockfd = -1;
            }
        }
};
#endif
