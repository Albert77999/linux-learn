#pragma once
#include "Log.hpp"
#include "Common.hpp"

#include <fstream>
#include <string>
#include <unordered_map>
#include <iostream>
const std::string gdictname = "dict.txt";
const std::string gpath = "./";
const std::string gsep = ": ";

class Dictionary
{
private:
    bool Split(std::string *key,std::string *value,const std::string &line)
    {
        std::string::size_type pos = line.find(gsep);
        if(pos == std::string::npos || pos == 0 || pos + gsep.size() >= line.size())
            return false;
        *key = line.substr(0,pos);
        *value = line.substr(pos + gsep.size());
        if(*key == "" || *value == "")
            return false;
        return true;
    }
    bool LoadDictionary()
    {
        std::string file = _path + _filename;
        std::ifstream in(file.c_str());
        if(!in.is_open())
        {
            LOG(LogLevel::ERROR) << "open file failed";
            return false;
        }
        //进行文件的操作
        std::string line;
        while(std::getline(in,line))
        {
            std::string key;
            std::string value;
            if(Split(&key,&value,line))
            {
                _umap[key] = value;
            }      
        }
        return true;
    }
public:
    Dictionary(const std::string path = gpath,const std::string filename = gdictname)
        :_filename(filename)
        ,_path(path)
    {
        LoadDictionary();
    }
    ~Dictionary()
    {

    }
    std::string Translate(const std::string &word) const
    {
        auto iter = _umap.find(word);
        if(iter == _umap.end()) return "None";
        return iter->second;
    }
private:
    std::unordered_map<std::string,std::string> _umap;
    std::string _filename;
    std::string _path;
};
