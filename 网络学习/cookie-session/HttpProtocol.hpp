#pragma once
#include "Common.hpp"
#include "Log.hpp"
#include "Cookie.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>

inline std::string LowerHeaderName(std::string name)
{
    for (char &ch : name)
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    return name;
}

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
    if(pos == std::string::npos || pos == 0)
        return false;
    if(header[pos - 1] == ' ' || header[pos - 1] == '\t')
        return false;
    *key = header.substr(0,pos);
    *value = CookieModule::Trim(header.substr(pos + headersep.size()));
    return true;
}

//对于每一个HttpRequest做反序列化
class HttpRequest
{
private:
    bool ParseReqLine()
    {
        std::stringstream ss(_req_line);
        std::string extra;
        if (!(ss >> _method >> _uri >> _version) || (ss >> extra))
            return false;
        return !_uri.empty() && _uri.front() == '/' &&
               (_version == "HTTP/1.1" || _version == "HTTP/1.0");
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
                key = LowerHeaderName(key); // HTTP 字段名大小写不敏感。
                if (key == "cookie" && _headers.count(key))
                    _headers[key] += "; " + value;
                else
                    _headers[key] = value;
            }
            else continue;
        }
    }
public:
    HttpRequest(){}
    ~HttpRequest(){}
    bool Deserialize(std::string &request_str)
    {
        _headers.clear();
        _cookies.clear();
        _args.clear();
        if(ParseOneLine(request_str,&_req_line))
        {
            if (!ParseReqLine())
                return false;
            ParseHeaders(request_str);
            _body = request_str;
            _cookies = CookieModule::Parse(Header("Cookie"));
            const auto pos = _uri.find('?');
            _path = _uri.substr(0, pos);
            if(_method == "POST")
            {
                _args = _body;
            }
            else if(pos != std::string::npos)
            {
                _args = _uri.substr(pos + 1);
            }
            return true;
        }
        return false;
    }
    std::string GetContent(const std::string path)
    {
        std::string content;
        std::ifstream in(path,std::ios::binary);
        if(!in.is_open())
            return "";
        in.seekg(0,std::ios::end);
        const auto filesize = in.tellg();
        if (filesize < 0)
            return "";
        in.seekg(0,std::ios::beg);
        content.resize(static_cast<std::size_t>(filesize));
        if (!in.read(content.data(), static_cast<std::streamsize>(filesize)))
            return "";
        return content;
    }
    void Print()
    {
        std::cout << "method: " << _method << std::endl;
        std::cout << "uri: " << _uri << std::endl;
        for(auto &kv : _headers)
        {
            std::cout << kv.first << ": "
                      << ((kv.first == "cookie" || kv.first == "authorization")
                              ? "[omitted]" : kv.second) << std::endl;
        }
        std::cout << "\r\n";
    }
    void SetUri(const std::string newuri)
    {
        _uri = newuri;
    }
    std::string Uri() { return _uri; }
    std::string Path() { return _path; }
    std::string Args() { return _args; }
    const std::string &Method() const { return _method; }
    std::string Header(const std::string &name) const
    {
        const auto it = _headers.find(LowerHeaderName(name));
        return it == _headers.end() ? "" : it->second;
    }
    std::string Cookie(const std::string &name) const
    {
        const auto it = _cookies.find(name);
        return it == _cookies.end() ? "" : it->second;
    }
    std::string Suffix()
    {
        std::string::size_type pos = _path.rfind('.');
        if(pos == std::string::npos)
            return (".html");
        else
            return _path.substr(pos);
    }
private:
    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _args;
    std::string _version;
    std::unordered_map<std::string,std::string> _headers;
    std::unordered_map<std::string,std::string> _cookies;
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
            case 401:
                return "Unauthorized";
            case 404:
                return "Not Found";
            case 301:
                return "Moved Permanently";
            case 302:
                return "Found";
            case 303:
                return "See Other";
            case 403:
                return "Forbidden";
            case 405:
                return "Method Not Allowed";
            case 413:
                return "Content Too Large";
            case 431:
                return "Request Header Fields Too Large";
            case 500:
                return "Internal Server Error";
            default:
                return std::string();
        }
    }
    std::string Suffix2Desc(const std::string &suffix)
    {
        if(suffix == ".jpg")
            return "image/jpeg";
        else if (suffix == ".css")
            return "text/css; charset=UTF-8";
        else if (suffix == ".js")
            return "text/javascript; charset=UTF-8";
        else if (suffix == ".png")
            return "image/png";
        else if (suffix == ".svg")
            return "image/svg+xml";
        else
            return "text/html; charset=UTF-8";
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
        if (req.Path().find("..") != std::string::npos)
        {
            SetCode(403);
            SetHeader("Content-Type", "text/plain; charset=UTF-8");
            SetBody("Forbidden\n");
            return;
        }
        std::string uri = defaulthomepage + req.Path();
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
            SetHeader("Content-Type","text/html; charset=UTF-8");
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
        // Set-Cookie 不能覆盖成一条，也不能用逗号合并。
        for (const auto &cookie : _set_cookies)
            *resp_str += "Set-Cookie: " + cookie + sep;
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
        if (LowerHeaderName(k) == "set-cookie")
            _set_cookies.push_back(v);
        else
            _headers[k] = v;
    }
    void SetCookie(const std::string &name, const std::string &value,
                   const CookieModule::Options &options = {})
    {
        SetHeader("Set-Cookie", CookieModule::Serialize(name, value, options));
    }
private:
    std::string _version;
    int _status_code;
    std::string _status_desc;
    std::string _content;
    //构建应答
    std::string _resp_line;
    std::unordered_map<std::string,std::string> _headers;
    std::vector<std::string> _set_cookies;
    std::string _body;
};
