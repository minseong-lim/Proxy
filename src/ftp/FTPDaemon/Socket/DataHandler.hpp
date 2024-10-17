#pragma once
#include "FTPCommon.hpp"

class DataHandler
{
public:
    static ssize_t ReadCMD (int nFileDesc, std::string  &strMessage, long nTimeout = TIME_OUT_SEC);    
    static bool    WriteCMD(int nFileDesc, std::string &&strMessage);

    static std::unique_ptr<DataChannelPacket> ReadData(int nFileDesc, long nTimeout = TIME_OUT_SEC);
    static bool WriteData(int nFileDesc, std::unique_ptr<DataChannelPacket> pPacket);
};