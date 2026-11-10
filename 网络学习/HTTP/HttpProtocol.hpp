#pragma once
#include "Common.hpp"
#include "Log.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>

const std::string sep = "\r\n";
const std::string headersep = ":";
const std::string blankline = sep;
const std::string defaulthomepage = "wwwroot";
const std::string page404 = "wwwroot/404.html";
const std::string firstpage = "index.html";

//把传入参数划分为一行一行的
bool ParseOneLine(std::string &in_str, std::string *out_str)
{
    std::string::size_type pos = in_str.find(sep);
    if(pos == std::string::npos)
        return false;
    *out_str = in_str.substr(0,pos);
    in_str.erase(0,pos + sep.size());
    return true;
}
//把每一行的参数(header)划分为key value的形式
bool SplitString(const std::string &header,std::string *key,std::string *value)
{
    std::string::size_type pos = header.find(headersep);
    if(pos == std::string::npos || pos <= 0)
        return false;
    if(header[pos - 1] == ' ')
        return false;
    *key = header.substr(0,pos);
    if(header[pos + 1] == ' ')
    {
        pos++;
    }
    *value = header.substr(pos + headersep.size());
    return true;
}

//对于每一个HttpRequest做反序列化
class HttpRequest
{
private:
    void ParseReqLine()
    {
        std::stringstream ss(_req_line);
        ss >> _method >> _uri >> _version;
    }
    void ParseHeaders(std::string &request_str)
    {
        std::string line;
        std::vector<std::string> req_headers;
        while(true)
        {
            bool r = ParseOneLine(request_str,&line);
            if(r && !line.empty())
            {
                req_headers.push_back(line);

            }
            else if(r && line.empty())
            {
                //说明读到了空白行，下面内容就是body了
                break;
            }
            else return;
        }
        //提取req_headers的kv
        std::string key, value;
        for(auto &header : req_headers)
        {
            if(SplitString(header,&key,&value))
            {
                _headers[key] = value;
            }
            else continue;
        }
    }
public:
    HttpRequest(){}
    ~HttpRequest(){}
    void Deserialize(std::string &request_str)
    {
        if(ParseOneLine(request_str,&_req_line))
        {
            ParseReqLine();
            ParseHeaders(request_str);
            _body = request_str;
            if(_method == "POST")
            {
                _path = _uri;
                _args = _body;
            }
            else if(_method == "GET")
            {
                _path = _uri;
                std::string::size_type pos = _uri.find("?");
                if(pos == std::string::npos || pos <= 0)
                    return;
                _path = _uri.substr(0,pos);
                _args = _uri.substr(pos + 1);
            }
        }
    }
    std::string GetContent(const std::string path)
    {
        std::string content;
        std::ifstream in(path,std::ios::binary);
        if(!in.is_open())
            return "";
        in.seekg(0,std::ios::end);
        int filesize = in.tellg();
        in.seekg(0,std::ios::beg);
        content.resize(filesize);
        in.read(content.data(),filesize);
        return content;
    }
    void Print()
    {
        std::cout << "method: " << _method << std::endl;
        std::cout << "uri: " << _uri << std::endl;
        std::cout << "args: " << _args << std::endl;
        for(auto &kv : _headers)
        {
            std::cout << kv.first << ": " << kv.second << std::endl;
        }
        std::cout << "\r\n";
        std::cout << "body: "<< _body << std::endl;
    }
    void SetUri(const std::string newuri)
    {
        _uri = newuri;
    }
    std::string Uri() { return _uri; }
    std::string Path() { return _path; }
    std::string Args() { return _args; }
    std::string Suffix()
    {
        std::string::size_type pos = _uri.rfind('.');
        if(pos == std::string::npos)
            return (".html");
        else
            return _uri.substr(pos);
    }
private:
    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _args;
    std::string _version;
    std::unordered_map<std::string,std::string> _headers;
    std::string _body;//body内部在做分化

    std::string _blank_line;
    std::string _req_line;//首行
};

//对于每一个HttpResponse做序列化
class HttpResponse
{
private:
    std::string Code2Desc(int code)
    {
        switch(code)
        {
            case 200:
                return "OK";
            case 400:
                return "Bad Request";
            case 404:
                return "Not Found";
            case 301:
                return "Moved Permanently";
            case 302:
                return "Found";
            default:
                return std::string();
        }
    }
    std::string Suffix2Desc(const std::string &suffix)
    {
        if(suffix == ".jpg")
            return "image/jpeg";
        else
            return "text/html";
    }
public:
    explicit HttpResponse()
        :_version("HTTP/1.1")
        ,_status_code(200)
        ,_status_desc("OK")
    {

    }
    ~HttpResponse()
    {

    }
    void Build(HttpRequest &req)
    {
        std::string uri = defaulthomepage + req.Uri(); 
        if(uri.back() == '/')
        {
            uri += firstpage;
        }
        std::cout << "客户端在请求" << std::endl;
        req.Print();
        std::cout << "---------------------------------" << std::endl;
        _content = req.GetContent(uri);
        if(_content.empty())
        {
            _status_code = 404;
            _content = req.GetContent(page404);
            SetHeader("Content-Type","text/html");
        }
        else
        {
            _status_code = 200;
            SetHeader("Content-Type",Suffix2Desc(req.Suffix()));
        }
        _status_desc = Code2Desc(_status_code);// 和状态码强相关
        SetHeader("Content-Length",std::to_string(_content.size()));
        _body = _content;
    }
    void Serialize(std::string *resp_str)
    {
        SetHeader("Content-Length", std::to_string(_body.size()));
        std::vector<std::string> resp_headers;
        for(auto &header : _headers)
        {
            resp_headers.push_back(header.first + headersep + " " + header.second);
        }
        _resp_line = _version + " " + std::to_string(_status_code) + " " + _status_desc + sep;
        //序列化
        *resp_str = _resp_line;
        for(auto &line : resp_headers)
        {
            *resp_str += line;
            *resp_str += sep;
        }
        *resp_str += blankline;
        *resp_str += _body;

    }
    void SetCode(int code)
    {
        _status_code = code;
        _status_desc = Code2Desc(_status_code);
    }
    void SetBody(const std::string &body)
    {
        _body = body;
    }
    void SetHeader(const std::string &k,const std::string &v)
    {
        _headers[k] = v;
    }
private:
    std::string _version;
    int _status_code;
    std::string _status_desc;
    std::string _content;
    //构建应答
    std::string _resp_line;
    std::unordered_map<std::string,std::string> _headers;
    std::string _body;
};
