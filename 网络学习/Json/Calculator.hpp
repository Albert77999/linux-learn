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