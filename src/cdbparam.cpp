#include "cdbparam.h"
CDBparam::CDBparam()
{

}

void CDBparam::setColumnDataType(int column, CDBparam::DataType eDatatype)
{
    _mapColumnDatatype[column] = eDatatype;
}

int CDBparam::getColumnDataType(int column)
{
    return _mapColumnDatatype[column];
}

void CDBparam::setOutputType(int outputType)
{
    _outputType = outputType;
}

int CDBparam::getOutputType()
{
    return _outputType;
}

void CDBparam::setVectIntData(const std::vector<int> &vectData)
{
    for(unsigned int i=0;i<vectData.size();i++)
    {
        _vectIntData.push_back(vectData.at(i));
    }
}

void CDBparam::setVectIntData(int Data)
{
    _vectIntData.push_back(Data);
}

void CDBparam::getVectIntData(std::vector<int> &vectData)
{
    for(unsigned  int i =0;i<_vectIntData.size();i++)
    {
        vectData.push_back(_vectIntData.at(i));
    }
}

void CDBparam::setVectTxtData(const std::vector<std::__cxx11::string> &vectData)
{
    for(unsigned int i=0;i<vectData.size();i++)
    {
        _vectTxtData.push_back(vectData.at(i));
    }
}

void CDBparam::setVectTxtData(std::__cxx11::string Data)
{
    _vectTxtData.push_back(Data);
}

void CDBparam::setJsonData(const nlohmann::json &result)
{
    _retJsonData = result;
}

void CDBparam::getJsonData(nlohmann::json &result)
{
    result =_retJsonData;
}
