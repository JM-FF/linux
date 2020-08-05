#ifndef __M_HTTP_H__
#define __M_HTTP_H__

#include "tcpsocket.h"
#include <list>
#include <sstream>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <sstream>
using namespace boost;

//HTTP请求类
class HttpRequest{
    private:
        //接收HTTP请求（首行 + 头部）
        bool RecvRequest(TcpSocket& cliSock, string& request){
            while(1){
                //试探接受数据，判断接收的数据中是否有\r\n\r\n
                string tmp;
                bool ret = cliSock.RecvPeek(tmp);
                if(ret == false){
                    return false;
                }
                size_t pos;
                //首行+头部的结束标志位 --- \r\n\r\n
                pos = tmp.find("\r\n\r\n");
                
                //没有找到结束标志
                if(pos == string::npos && tmp.size() == MAX_REQUEST){
                    return false;
                }
                //找到结束标志，直接进行接收
                else if(pos != string::npos){                
                    size_t len = pos + 4;
                    ret = cliSock.Recv(tmp, len);
                    if(ret == false){
                        return false;
                    }
                    
                    //获取request
                    request.assign(&tmp[0], pos);

                    //cout << request << endl;
                    return true;
                }
            }
        }
        
        //解析首行
        bool ParseFirstLine(string& firstLine){
            //首行中有请求方法，URL和版本协议，由空格间隔
            vector<string> lineList;
            split(lineList, firstLine, is_any_of(" "), token_compress_on);
            if(lineList.size() != 3){
                cerr << "parse first line error";
                return false;
            }
            
            _method = lineList[0];  //请求方法
            
            //解析URL，分割出请求路径和查询字符串
            size_t pos = lineList[1].find("?");
            if(pos == string::npos) {
                _path = lineList[1]; 
            }
            else if(pos != string::npos){
                _path = lineList[1].substr(0, pos);  //请求路径
                
                //解析查询字符串，key=val&ket=val
                string param = lineList[1].substr(pos + 1);
                vector<string> paramList;
                split(paramList, param, is_any_of("&"), token_compress_on);
                for(auto s : paramList){
                    pos = s.find("=");
                    if(pos == string::npos){
                        cerr << "queryString is error" << endl;
                        return false;
                    }
                    else if(pos != string::npos){
                        string key = s.substr(0, pos);
                        string val = s.substr(pos + 1);
                        _queryString[key] = val;
                    }
                }
            }
            
    
            _version = lineList[2]; //协议版本
            
            cout << "method: [" << _method << "]" << endl;
            cout << "path: [" << _path << "]" << endl;
            cout << "queryString: " << endl;
            for(auto i : _queryString){
                cout << "  " << i.first << "=" << i.second << endl;
            }
            cout << "version: [" << _version << "]"<< endl;

            return true;
        }    

    public:
        //HTTP请求：首行 + 头部
        //首行：请求方法，URL，协议版本
        //头部：key: val\r\n
        //正文

        //首行信息
        string _method;     //请求方法
        string _path;   //URL中的请求路径
        //URL中的查询字符串，在URL中以 key=val&key=val 的形式存在
        unordered_map<string, string> _queryString;    
        string _version;    //协议版本

        //头部，以 key: val 的形式存在
        unordered_map<string, string> _header;
        string _body;   //正文
        
        //解析HTTP请求
        int ParseHttp(TcpSocket& cliSock){
            //接收HTTP请求（首行+头部）
            string request;
            bool ret = RecvRequest(cliSock, request);
            if(ret == false){
                return 400;
            }

            //对请求的字符串按照\r\n的格式进行分割，得到一个requestList
            vector<string> requestList;
            split(requestList, request, is_any_of("\r\n"), token_compress_on);
           
            //for(int i = 0; i < requestList.size(); i++){
            //    cout << requestList[i] << endl;
            //}

            //requestList[0]是首行，对首行解析
            ret = ParseFirstLine(requestList[0]);
            if(ret == false){
                return 400;
            }

            //requestList[1]以后都是头部，对头部解析
            //key: val
            for(int i = 1; i < requestList.size(); i++){
                size_t pos = requestList[i].find(": ");
                if(pos == string:: npos){
                    cerr << "header is error" << endl;
                    return 400;
                }
                else if(pos != string::npos){
                   string key = requestList[i].substr(0, pos);
                   string val = requestList[i].substr(pos + 2);
                   _header[key] = val;
                }
            }

            //请求信息校验
            cout << "header: " <<endl;
            for(auto i : _header){
                cout << "  " <<i.first << ": " <<i.second << endl;
            }
            cout << endl;

            //接受正文
            auto it = _header.find("Content-Length");
            if(it != _header.end()){
                stringstream tmp;
                tmp << it->second;
                int bodyLen;
                tmp >> bodyLen;
                
                cliSock.Recv(_body, bodyLen);
            }

            return 200;
        }
};


//HTTP响应类
class HttpResponse{
    //HTTP响应：首行 + 头部
    //首行：协议版本，状态码，状态码描述
    //头部
    //正文
    
    private:
        //根据状态码获取状态描述
        string GetDescribe(){
            switch(_status){
                case 400: return "Bad Request";
                case 200: return "OK";
                case 206: return "Partial Content";
                case 404: return "Not Found";
            }
            return "UnKnow";
        }

    public:
        //首行信息
        string _version;    //协议版本
        int _status;    //状态码

        //头部，以 key: val 的形式存在
        unordered_map<string, string> _header;
        string _body;       //正文

        //设置头部
        bool SetHeader(const string& key, const string& val){
            _header[key] = val;
            return true;
        }

        //发送错误
        bool SendError(TcpSocket& cliSock){
            stringstream tmp;
            //首行
            tmp << _version << " " << _status << " " << GetDescribe() << "\r\n"; 
            
            //正文
            stringstream body;
            body << "<html><h1>" << GetDescribe() << "</h1></html>";
            _body = body.str();

            //头部
            SetHeader("Content-Length", to_string(_body.size()));
            SetHeader("Content-Type", "text/html"); 
            for(auto i : _header){
                tmp << i.first << ": " << i.second << "\r\n";
            } 
            tmp << "\r\n";
           
            //发送
            cliSock.Send(tmp.str());
            cliSock.Send(_body);

            return true;
        }

        //发送HTTP响应
        bool SendResponse(TcpSocket& cliSock){
            //先判断状态码，若状态码有问题，则SendError
            if(_status != 200){
                SendError(cliSock);
                return true;
            }
            
            //状态码正确，发送正常的响应信息
            stringstream tmp;
            //首行
            tmp << _version << " " << _status << " " << GetDescribe() << "\r\n"; 
           
            //如果用户没有添加Content-Length，需要响应类自动添加
            if(_header.find("Content-Length") == _header.end()){
                string len = to_string(_body.size());
                _header["Content-Length"] = len;
            }

            //头部
            for(auto i : _header){
                tmp << i.first << ": " << i.second << "\r\n";
            }    
            tmp << "\r\n";
            
            //发送首行+头部
            cliSock.Send(tmp.str());
            
            //发送正文
            cliSock.Send(_body);

            return true;
        }
};
#endif
