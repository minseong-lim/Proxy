#pragma once
#include <iostream>
#include <memory>

#include "hsFTP.hpp"
#include "hsfCommon/DefineLog.hpp"

using namespace std;

class FTPCommandData
{
private:
    FTP*    m_pFTP;
    string  m_strCommand;
    string  m_strParameter;
    string  m_strResponseCode;
    string  m_strResponseMessage;
    string  m_strDescription;
public:
    FTPCommandData(FTP* pFTP) : m_pFTP(pFTP) { }
    virtual ~FTPCommandData() = default;

    string getCommand()         const { return m_strCommand;         }
    string getParameter()       const { return m_strParameter;       }
    string getResponseCode()    const { return m_strResponseCode;    }
    string getResponseMessage() const { return m_strResponseMessage; }
    string getDescription()     const { return m_strDescription;     }

    string getCommandLine()     const { return m_strCommand      + BLANK + m_strParameter       + END_OF_LINE; }
    string getResponseLine()    const { return m_strResponseCode + BLANK + m_strResponseMessage + END_OF_LINE; }
    
    bool writeToServer() { return m_pFTP ? m_pFTP->writeToServer(getCommandLine())  : false; }
    bool writeToClient() { return m_pFTP ? m_pFTP->writeToClient(getResponseLine()) : false; }    
    bool readFromServer(std::string& str);
    bool readFromServer();

    bool parseResponse(string &&strResponseMessage);

    void setCommand         (const string &strCommand)         { m_strCommand           = strCommand;           }    
    void setParameter       (const string &strParameter)       { m_strParameter         = strParameter;         }
    void setDescription     (const string &strDescription)     { m_strDescription       = strDescription;       }
    void setResponseCode    (const string &strResponseCode)    { m_strResponseCode      = strResponseCode;      }
    void setResponseMessage (const string &strResponseMessage) { m_strResponseMessage   = strResponseMessage;   }
    

    void clear()  { m_strCommand.clear(); m_strParameter.clear(); m_strResponseCode.clear(); m_strResponseMessage.clear(); }
    FTP* getFTP() { return m_pFTP; }

    virtual bool processCommand() = 0;
};

template <typename ErrorStrategy, typename... Strategys>
class FTPCommand : public FTPCommandData
{
private:
    template<typename Strategy>
    bool execute()
    {
        FTPCommandData *pData = dynamic_cast<FTPCommandData*>(this);
        if (!pData)
        {
            hsfdlog(D_ERRO, "dynamic_cast from <FTPCommand> to <FTPCommandData> fail...");
            return false;
        }
        Strategy strategy;
        return strategy.execute(pData);        
    }
public:
    FTPCommand(FTP* pFTP) : FTPCommandData(pFTP) { }

    virtual ~FTPCommand() = default;

    bool processCommand()
    {
        int  nIndex = 0;
        bool eResult = true;

        // 24.09.23 LMS [ 실패 시 Index 추가 ]
        // ((eResult = execute<Strategys>()) && ...);
        ((eResult = [&nIndex, this] { nIndex++; return this->execute<Strategys>(); }()) && ...);

        if (!eResult)
        {
            hsfdlog(D_ERRO, "Process command error at index %d...", nIndex - 1);
            execute<ErrorStrategy>();
            return false;
        }

        return true;
    }
};