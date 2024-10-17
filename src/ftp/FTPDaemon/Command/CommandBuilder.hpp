#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>

class FTPCommandData;
class FTP;

class CommandBuilder
{
private:
    static std::unique_ptr<CommandBuilder>  m_pInstance;
    static std::once_flag                   m_Flag;

    std::string m_strCommandLine;
    std::unordered_map<std::string, std::shared_ptr<FTPCommandData>>    m_Commands;
    std::unordered_map<std::string, std::string>                        m_Descriptions;
    
    CommandBuilder() = default;

    void deleteControlCharacter();
    void deleteFrontWhiteSpace();

    std::string getCommand();
    std::string getParameter();
    
    std::string getCommandDescription(const std::string &strCommand);
    std::shared_ptr<FTPCommandData> getCommandObject(const std::string &strCommand);
public:
    virtual ~CommandBuilder() = default;

    bool init(FTP* pFTP);

    CommandBuilder(const CommandBuilder& other)             = delete;    
    CommandBuilder& operator=(const CommandBuilder& other)  = delete;

    CommandBuilder(CommandBuilder&& other)                  = delete;
    CommandBuilder& operator=(CommandBuilder&& other)       = delete;

    std::shared_ptr<FTPCommandData> doBuild(std::string &&strCommandLine);
    std::shared_ptr<FTPCommandData> getStartCommand();

    static CommandBuilder* getInstance();
};