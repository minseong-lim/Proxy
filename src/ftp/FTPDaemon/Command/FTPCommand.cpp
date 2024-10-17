#include "FTPCommand.hpp"

bool FTPCommandData::readFromServer(std::string& str)
{
    if (!m_pFTP || !m_pFTP->readFromServer(str))
    {
        return false;
    }

    return true;
}

bool FTPCommandData::readFromServer()
{
    string strResponseMessage;

    if (!m_pFTP || !m_pFTP->readFromServer(strResponseMessage))
    {
        return false;
    }

    return parseResponse(move(strResponseMessage));
}

bool FTPCommandData::parseResponse(string &&strResponseMessage)
{
    size_t nEndLineIndex = strResponseMessage.find_last_of(END_OF_LINE);

    if(nEndLineIndex == std::string::npos)
    {
       return false;
    }

    strResponseMessage = strResponseMessage.substr(0, nEndLineIndex - 1);

    std::string strCode;

    size_t nIndex = 0;

    for (nIndex = 0; nIndex < strResponseMessage.length(); nIndex++)
    {
        char ch = strResponseMessage.at(nIndex);

        if ((strCode.empty()) && (ch <= 32))
        {
            continue;
        }

        if (isdigit(ch))
        {
            strCode += ch;
        }
        else
        {
            break;
        }
    }

    if (nIndex >= strResponseMessage.length())
    {
        return false;
    }

    m_strResponseCode    = strCode;
    m_strResponseMessage = strResponseMessage.substr(nIndex + 1);

    return true;
}