#include <iostream>
#include <string>
#include <fstream>
#include "common.h"


std::map<std::string,std::string> mapArgs;

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char hexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}

bool isHex(const std::string& str)
{
    for(std::string::const_iterator it(str.begin()); it != str.end(); ++it)
    {
        if (hexDigit(*it) < 0)
            return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}


void readconf()
{
    std::ifstream jfile("./conf/server_main.conf");
    json js;
    jfile >> js;
    std::cout << js.dump() << std::endl;
    if(!js.is_object())
    {
        return ;
    }
    json::iterator it = js.begin();
    json::iterator end = js.end();
    for (it; it !=end; ++it)
    {
        if(js[it.key()].is_string())
        {
            mapArgs[it.key()] = it.value();
        }
    }
}


bool ConfManager::isArgSet(const std::string& strArg)
{
    return mapArgs.count(strArg);
}

std::string ConfManager::getArgs(const std::string& strArg,const std::string& strDefault)
{
    if(mapArgs.count(strArg))
    {
        return mapArgs[strArg];
    }
    return strDefault;
}
void ConfManager::readConfigFile()
{
    std::ifstream jfile(path_);

    jfile >> conf_;
    if(!conf_.is_object())
    {
        conf_.clear();
        return ;
    }
    readConfJsonObj(conf_);
    conf_.clear();
}
void ConfManager::readConfJsonObj(const json js)
{
    try
    {
        auto it = js.begin();
        auto end = js.end();
        for (it; it !=end; ++it)
        {
            std::string key = it.key();
            auto val = it.value();
            if(val.is_object())
            {
                readConfJsonObj(val);
            }
            else if(val.is_string())
            {
                mapArgs[it.key()] = val;
            }
        }
    }
    catch(...)
    {
        printArg();
        conf_.clear();
    }

}
void ConfManager::printArg()
{
    for(auto &it : mapArgs)
    {
        //std::cout << it.first << "  :  " << it.second << std::endl;
    }
}

int getListenPort()
{
    return mapArgs.count("listenport") ? atoi(mapArgs["listenport"].data())  : 9000;
}
std::string getBindAddr()
{
    return mapArgs.count("bindaddr") ?  mapArgs["bindaddr"] : "0.0.0.0";
}
int getTimeOut()
{
    return mapArgs.count("timeout") ?  atoi(mapArgs["timeout"].data()) : 30;
}
bool isDaemon()
{
    return mapArgs.count("daemon") && mapArgs["daemon"] == "yes";
}
