#pragma once
#include "Protocol.hpp"


class Calculator
{
public:
    Response Execute(const Request &req)
    {
        int result = 0;
        int code = 0;
        int x = req.X();
        int y = req.Y();
        char oper = req.Oper();
        switch(oper)
        {
            case '+':
                result = x + y;
                break;
            case '-':
                result = x - y;
                break;
            case '*':
                result = x * y;
                break;
            case '/':
                if(y != 0)
                    result = x / y;
                else
                    code = 1;
                break;
            case '%':
                if(y != 0)
                    result = x % y;
                else
                    code = 2;
                break;
            default:
                code = 3;
                break;
        }
        Response resp(result,code);
        return resp;
    }
};

Calculator cal;
std::string HandlerRequest(std::string &inbuffer)
{
    std::string request_str;
    std::string result_str;
    while(Decode(inbuffer,&request_str))
    {
        std::string resp_str;
        //一定拿到了一个完整报文
        //1.反序列化
        if(request_str.empty())//
            break;
        Request req;
        if(!req.Deserialize(request_str))
            break;
        //2.业务处理
        Response resp = cal.Execute(req);
        //3.序列化
        resp.Serialize(resp_str);
        //4.添加长度字段 协议
        Encode(resp_str);
        //5.添加所有的应答
        result_str += resp_str;
    }
    return result_str.empty() ? "" : result_str;
}