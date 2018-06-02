#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

enum RESPONSE_TPYE { ERROR = 0,OK = 1 };

static const int HAHS_SIZE = 64;


class ConfManager
{
public:
    ConfManager(std::string path = "./conf/server_main.conf"):path_(path){}
    ~ConfManager(){};
    std::string getArgs(const std::string& strArg,const std::string& strDefault);
    bool isArgSet(const std::string& strArg);
    void readConfigFile();
    void printArg();
private:
    void readConfJsonObj(const json js);
private:
    std::string path_;
    json conf_;
    std::map<std::string,std::string> mapArgs;
};





template<typename T>
std::string HexStr(const T itbegin, const T itend, bool fSpaces=false)
{
    std::string rv;
    static const char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    rv.reserve((itend-itbegin)*3);
    for(T it = itbegin; it < itend; ++it)
    {
        unsigned char val = (unsigned char)(*it);
        if(fSpaces && it != itbegin)
            rv.push_back(' ');
        rv.push_back(hexmap[val>>4]);
        rv.push_back(hexmap[val&15]);
    }

    return rv;
}

template<typename T>
inline std::string HexStr(const T& vch, bool fSpaces=false)
{
    return HexStr(vch.begin(), vch.end(), fSpaces);
}

template < class T>
std::string makeReplyMsg(bool type,T& t)
{
    json response = json::object();

    response["code"] = type ? RESPONSE_TPYE::OK : RESPONSE_TPYE::ERROR;
    response["data"] = t;

    return  response.dump();
}



#endif
