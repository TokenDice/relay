#ifndef CDBPARAM_H
#define CDBPARAM_H
#include <map>
#include <vector>
#include "json.hpp"

class CDBparam
{
public:
    CDBparam();
    enum DataType
    {
        eError =0,
        eTxt =1,
        eInt =2
    };

    enum outputType
    {
        eVectInt =1,
        eVectTxt =2,
        eJson    =3
    };
public:
    void setColumnDataType(int column,DataType eDatatype);
    int getColumnDataType(int column);
    void setOutputType(int outputType);
    int getOutputType();


public:
    void setVectIntData(const std::vector<int> &vectData);
    void setVectIntData(int Data);
    void getVectIntData(std::vector<int>& vectData);

    void setVectTxtData(const std::vector<std::string> &vectData);
    void setVectTxtData(std::string Data);
    void getVectTxtData(std::vector<std::string> &vectData);

    void setJsonData(const nlohmann::json&result);
    void getJsonData(nlohmann::json &result );

public:

private:
    std::map<int,int>  _mapColumnDatatype;
    int _outputType;
    std::vector<int> _vectIntData;
    std::vector<std::string> _vectTxtData;
    nlohmann::json _retJsonData;

};

#endif // CDBPARAM_H
