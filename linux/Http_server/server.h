#ifndef __M_SRV_H__
#define __M_SRV_H__
#include <fstream>
#include "tcpsocket.h"
#include "epoll_wait.h"
#include "threadpool.h"
#include "http.h"
#include <boost/filesystem.hpp>
#include <stdlib.h>
#include <sstream>

#define _MYROOT "./myroot"

class Server{
    private:
        TcpSocket _listenSock;  //用于监听的套接字
        ThreadPool _pool;   //线程池
        Epoll _epoll;   //epoll监控
    public:
        bool Start(uint16_t port){
            bool ret = true;

            //初始化服务端
            //创建套接字 + 绑定地址信息 + 监听
            ret = _listenSock.SocketInit(port);
            if(ret == false){
                return false;
            }

            //将监听套接字设置为非阻塞
            _listenSock.SetNonBlock();

            //初始化线程池pool
            ret = _pool.PoolInit();
            if(ret == false){
                return false;
            }
            
            //初始化epoll监控
            ret = _epoll.EpollInit();
            if(ret == false){
                return false;
            }

            //将监听描述符添加到epoll监控中
            ret = _epoll.AddSock(_listenSock);
            if(ret == false){
                return false;
            }

            //开始epoll监控
            while(1){
                //储存已就绪的套接字
                vector<TcpSocket> list;
                
                //判断是否有就绪的套接字
                if(_epoll.WaitSock(list) == false){
                    continue;
                }

                //处理就绪的socket
                for(int i = 0; i < list.size(); i++){
                    //若就绪的是监听套接字
                    if(list[i].GetFd() == _listenSock.GetFd()){
                        //获取已连接的客户端
                        TcpSocket cliSock;
                        ret = list[i].Accept(cliSock);
                        if(ret == false){
                            cliSock.Close();
                            continue;
                        }    

                        //添加到监控中
                        ret = _epoll.AddSock(cliSock);
                        if(ret == false){
                            cliSock.Close();
                            continue;
                        }
                        
                        //将通信套接字设置为非阻塞
                        cliSock.SetNonBlock();
                    }
                    //若就绪的是通信套接字
                    else{
                        //创建任务
                        ThreadTask task;
                        task.SetTask(list[i].GetFd(), ThreadHandler);

                        //将任务添加到任务中
                        _pool.AddTask(task);

                        //将该套接字从监控中移除，避免该套接字因为一直处于就绪状态而一直被处理
                        _epoll.DelSock(list[i]);
                    }
                }
            }
        
            //关闭监听套接字
            _listenSock.Close();

            return true;
        }

        //数据的处理方式
        static void ThreadHandler(int sockfd){
            //获取通信套接字
            TcpSocket cliSock;
            cliSock.SetFd(sockfd);

            HttpRequest req;   //HTTP解析类
            HttpResponse rep;   //HTTP响应类

            //解析HTTP
            int status = req.ParseHttp(cliSock);
            //若HTTP解析失败，则会返回400，直接返回错误信息
            if(status == 400){
                rep._status = status;
                
                //发送失败响应
                rep.SendError(cliSock);
                //关闭客户端
                cliSock.Close();
                
                return;
            }
            
            //解析成功HTTP请求成功
            cout << "------------------" << endl;

            //根据解析的HTTP请求，组织HTTP响应，并将其填充到HTTP响应类rep中
            HttpProcess(req, rep);
             
            //发送HTTP响应
            rep.SendResponse(cliSock);

            //采用短链接，直接关闭客户端
            cliSock.Close();
            
            return;
        }
       
        
        //根据客户端请求的功能，组织HTTP响应
        static void HttpProcess(HttpRequest& req, HttpResponse& rep){
            rep._version = req._version;    //协议版本
            
            //rep._status = 200;  //状态码
            //rep._version = req._version;    //协议版本
            //rep._body = "<html> Hello World </html>";       //正文
            //rep.SetHeader("Content-Type", "text/html");    //告诉浏览器需要将正文通过html渲染出来
            
            //上传文件(POST) --- 多进程CGI处理
            //查看文件(GET) --- 请求一个目录
            //下载文件(GET) --- 请求一个文件
           
            //获取相对路径，定义实际路径
            //查看路径是否存在 filesystem::exists(string)
            string realPath = _MYROOT + req._path; 
            if(!filesystem::exists(realPath)){
                rep._status = 404;
                return;
            }
            
            //若请求方法位POST，则为上传文件请求，进行多进程CGI处理
            //若请求方法时GET，但查询字符出不为空，说明客户端提交了数据需要服务端进行处理，也直接进行多进程CGI处理
            if((req._method == "POST") || (req._method == "GET" && req._queryString.size() != 0)){
                //多进程CGI处理
                
                //for(auto it : req._header){
                //    cout << it.first << ": "<< it.second << endl;
                //}
                //cout << "_body: "<<req._body << endl;

                //在Content-Type 中有一个 boundary，是正文_body中的重要文件分隔符
                //在正文中：
                //  first_boundary = --boundary
                //  middle_boundary = \r\n--boundary\r\n
                //  last_boundary = \r\n--boundary--\r\n
                CGIProcess(req, rep);
                rep._status = 200;  //状态码
            }
            else if(req._method == "GET" && req._queryString.size() == 0){
                //若请求方法为GET，且查询字符串为空，此时需要判断客户端请求的路径 filesystem::is_directory
                //若请求的路径是目录 --- 查看目录
                if(filesystem::is_directory(realPath)){
                    ListShow(realPath, rep._body);
                    
                    rep.SetHeader("Content-Type", "text/html");
                    rep._status = 200;  //状态码
                }
                //若请求的路径是文件 --- 文件下载
                else{
                    RangeDownload(req, rep);
                }
            }
        }

        //外部CGI处理
        static void CGIProcess(HttpRequest& req, HttpResponse& rep){
            //读写管道
            int pipeIn[2], pipeOut[2];
            if(pipe(pipeIn) < 0 || pipe(pipeOut) < 0){
                cerr << "create pipe error" << endl;
                return;
            }

            //创建子进程
            int pid = fork();
            if(pid < 0){
                return;
            }
            else if(pid == 0){
                //0读，1写

                close(pipeIn[0]);   //子进程写，关闭读
                close(pipeOut[1]);   //子进程读，关闭写
                dup2(pipeIn[1], 1);     //子进程写端重定向到标准输出1
                dup2(pipeOut[0], 0);    //子进程读端重定向到标准输入0

                //设置环境变量，将头部通过环境变量传入子进程
                setenv("METHOD", req._method.c_str(), 1);   //覆盖写入
                setenv("PATH", req._path.c_str(), 1);   //覆盖写入
                for(auto i : req._header){
                    setenv(i.first.c_str(), i.second.c_str(), 1);   //覆盖写入
                }

                string realpath = _MYROOT + req._path;
                //程序替换后，要保证HTTP请求可以在外部获得
                //使用环境变量传递头部信息
                //使用管道传递正文信息，子进程再使用管道传递响应结果
                execl(realpath.c_str(), realpath.c_str(), NULL);
                exit(0);                
            }

            close(pipeIn[1]);   //父进程读，关闭写
            close(pipeOut[0]);  //父进程写，关闭读
            
            //将正文通过管道写入
            write(pipeOut[1], &req._body[0], req._body.size());
            while(1){
                char buf[1024] = { 0 };
                int ret = read(pipeIn[0], buf, 1024);
                if(ret == 0){
                    break;
                }

                buf[ret] = '\0';
                rep._body += buf;
            }
           
            close(pipeIn[0]);       //关闭读
            close(pipeOut[1]);      //关闭写
        }


        //目录展示
        static void ListShow(string& path, string& body){
            //请求一个.. ---> 获取上层目录
            
            //./myroot
            //第一次传入 ./myroot/，此时需要获取/
            //第二次传入 ./myroot/textdir/，此时需要获取/textdir/
            string root = _MYROOT;
            //请求路径中是否在_ROOT中
            size_t pos = path.find(root);
            if(pos == string::npos){
                return; 
            }
            string dirPath = path.substr(root.size());

            stringstream tmp;
            tmp << "<html> <head> <style> *{ margin : 0 }  .main-window{ height : 100%;width : 80%; margin : 0 auto;}";
            tmp << ".upload{ position : relative; height : 20%; width : 100%; background-color : #ddd; text-align : center}";
            tmp << ".listshow{ position : relative; height : 80%; width : 100%; background-color : #ddd;} </style> </head>";
            tmp << " <body> <div class = 'main-window'> <div class = 'upload'>";
            tmp << "<form action = '/upload' method = 'POST' enctype = 'multipart/form-data'>";
            tmp << "<div class ='upload-btn'> <input type= 'file' name ='fileupload'>";
            tmp << "<input type='submit' name = 'submit'></div></form>";
            tmp << "</div> <hr />";
            tmp << "<div class = 'listshow'><ol>";

            //获取每一个文件信息
            filesystem::directory_iterator begin(path);
            filesystem::directory_iterator end;
            for(; begin != end; begin++){
                string pathName = begin->path().string();       //带路径的文件名
                string fileName = begin->path().filename().string();    //文件名
                string reqPath = dirPath + fileName;
                
                //如果该请求是一个目录
                if(filesystem::is_directory(pathName)){
                    tmp << "<li><strong> <a href = '" << reqPath << "'>" << fileName << "/"<< "</a></strong><br/>"; 
                    tmp << "<small> filetype : directory </small></li>";
                }
                //如果请求的是一个普通文件
                else{
                    int64_t mtime = filesystem::last_write_time(pathName);
                    int64_t ssize = filesystem::file_size(pathName);

                    tmp << "<li><strong><a href='"<< reqPath << "'>" << fileName << "</a></strong><br/>";
                    tmp << "<small> modified: " << mtime  <<  "<br/>";
                    tmp << "application-ostream- " << ssize / 1024 << "bytes" << "<br/> </small></li>";
                }
            }
            
            tmp <<"</ol> </div> <hr /> </div> </body> </html>";
            body = tmp.str();
        }

    
        //下载
        static bool RangeDownload(HttpRequest& req, HttpResponse& rep){
            string realPath = _MYROOT + req._path;
            int64_t data_len = filesystem::file_size(realPath);
            int64_t last_mtime = filesystem::last_write_time(realPath);
            string etag = to_string(data_len) + to_string(last_mtime);

            //通过判断Range来确定是进行普通下载，还是断点续传版本的下载
            auto it = req._header.find("Range");
           
            //普通下载
            if(it == req._header.end()){
                Download(realPath, 0, data_len, rep._body);
                rep._status = 200;  //状态码
            }
            //断点续传
            else{
                string range = it->second;

                string unit = "bytes=";
                size_t pos = range.find(unit);
                if(pos == string::npos){
                    return false;
                }

                pos += unit.size();
                size_t pos2 = range.find("-", pos);
                if(pos2 == string::npos){
                    return false;
                }

                string start = range.substr(pos, pos2 - pos);
                string end = range.substr(pos2 + 1);
                int64_t dig_start, dig_end;
                dig_start = atoi(&start[0]);
                if(end.size() == 0){
                    dig_end = data_len - 1;
                }
                else{
                    dig_end = atoi(&end[0]);    
                }

                int64_t range_len = dig_end - dig_start + 1;
                Download(realPath, dig_start, range_len, rep._body);
                
                stringstream tmp;
                tmp << "bytes" << dig_start << "-" << dig_end << "/" << data_len;
                rep.SetHeader("Content-Range", tmp.str());
                rep._status = 206;  //状态码
            }
           
            rep.SetHeader("Content-Type", "application/octet-stream");
            rep.SetHeader("Accept-Ranges", "bytes");
            rep.SetHeader("ETag", etag);

            return true;
        }

        static bool Download(string& path, int64_t start, int64_t len, string& body){
            body.resize(len);

            ifstream file(path);
            if(!file.is_open()){
                cerr << "open file erroe" << endl;
                return false;
            }

            file.seekg(start, ios::beg);
            file.read(&body[0], len);
            if(!file.good()){
                cerr << "read file data error" << endl;
                return false;
            }

            file.close();
            
            return true;
        }
    
};
#endif
