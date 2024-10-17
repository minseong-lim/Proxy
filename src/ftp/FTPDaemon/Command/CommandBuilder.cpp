#include "CommandBuilder.hpp"
#include "DefineCommand.hpp"
#include "DefineDescription.hpp"
#include "hsfCommon/DefineLog.hpp"

#include <optional>
#include <algorithm>
#include <cctype>

std::unique_ptr<CommandBuilder>  CommandBuilder::m_pInstance = nullptr;
std::once_flag                   CommandBuilder::m_Flag;

bool CommandBuilder::init(FTP* pFTP)
{
    if(!pFTP)
    {
        return false;
    }

    m_Commands.clear();
    m_Descriptions.clear();

    m_Commands.emplace(PASV_COMMAND,    std::make_shared<PasvCommand>(pFTP));
    m_Commands.emplace(PORT_COMMAND,    std::make_shared<PortCommand>(pFTP));
    m_Commands.emplace(LIST_COMMAND,    std::make_shared<ListCommand>(pFTP));
    m_Commands.emplace(NLST_COMMAND,    std::make_shared<ListCommand>(pFTP));
    m_Commands.emplace(RETR_COMMAND,    std::make_shared<RetrCommand>(pFTP));
    m_Commands.emplace(STOR_COMMAND,    std::make_shared<StorCommand>(pFTP));
    m_Commands.emplace(APPE_COMMAND,    std::make_shared<StorCommand>(pFTP));
    m_Commands.emplace(STOU_COMMAND,    std::make_shared<StorCommand>(pFTP));
    m_Commands.emplace(FEAT_COMMAND,    std::make_shared<FeatCommand>(pFTP));
    m_Commands.emplace(AUTH_COMMAND,    std::make_shared<AuthCommand>(pFTP));
    m_Commands.emplace(USER_COMMAND,    std::make_shared<UserCommand>(pFTP));
    m_Commands.emplace(START_COMMAND,   std::make_shared<StartCommand>(pFTP));
    m_Commands.emplace(DEFAULT_COMMAND, std::make_shared<DefaultCommand>(pFTP));

    m_Descriptions.emplace(ABOR_COMMAND, ABOR_DESCRIPTION);
    m_Descriptions.emplace(ACCT_COMMAND, ACCT_DESCRIPTION);
    m_Descriptions.emplace(ADAT_COMMAND, ADAT_DESCRIPTION);
    m_Descriptions.emplace(ALLO_COMMAND, ALLO_DESCRIPTION);
    m_Descriptions.emplace(APPE_COMMAND, APPE_DESCRIPTION);
    m_Descriptions.emplace(AUTH_COMMAND, AUTH_DESCRIPTION);
    m_Descriptions.emplace(AVBL_COMMAND, AVBL_DESCRIPTION);
    m_Descriptions.emplace(CCC_COMMAND,  CCC_DESCRIPTION);
    m_Descriptions.emplace(CDUP_COMMAND, CDUP_DESCRIPTION);
    m_Descriptions.emplace(CONF_COMMAND, CONF_DESCRIPTION);
    m_Descriptions.emplace(CSID_COMMAND, CSID_DESCRIPTION);
    m_Descriptions.emplace(CWD_COMMAND,  CWD_DESCRIPTION);
    m_Descriptions.emplace(DELE_COMMAND, DELE_DESCRIPTION);
    m_Descriptions.emplace(DSIZ_COMMAND, DSIZ_DESCRIPTION);
    m_Descriptions.emplace(ENC_COMMAND,  ENC_DESCRIPTION);
    m_Descriptions.emplace(EPRT_COMMAND, EPRT_DESCRIPTION);
    m_Descriptions.emplace(EPSV_COMMAND, EPSV_DESCRIPTION);
    m_Descriptions.emplace(FEAT_COMMAND, FEAT_DESCRIPTION);
    m_Descriptions.emplace(HELP_COMMAND, HELP_DESCRIPTION);
    m_Descriptions.emplace(HOST_COMMAND, HOST_DESCRIPTION);
    m_Descriptions.emplace(LANG_COMMAND, LANG_DESCRIPTION);
    m_Descriptions.emplace(LIST_COMMAND, LIST_DESCRIPTION);
    m_Descriptions.emplace(LPRT_COMMAND, LPRT_DESCRIPTION);
    m_Descriptions.emplace(LPSV_COMMAND, LPSV_DESCRIPTION);
    m_Descriptions.emplace(MDTM_COMMAND, MDTM_DESCRIPTION);
    m_Descriptions.emplace(MFCT_COMMAND, MFCT_DESCRIPTION);
    m_Descriptions.emplace(MFF_COMMAND,  MFF_DESCRIPTION);
    m_Descriptions.emplace(MFMT_COMMAND, MFMT_DESCRIPTION);
    m_Descriptions.emplace(MIC_COMMAND,  MIC_DESCRIPTION);
    m_Descriptions.emplace(MKD_COMMAND,  MKD_DESCRIPTION);
    m_Descriptions.emplace(MLSD_COMMAND, MLSD_DESCRIPTION);
    m_Descriptions.emplace(MLST_COMMAND, MLST_DESCRIPTION);
    m_Descriptions.emplace(MODE_COMMAND, MODE_DESCRIPTION);
    m_Descriptions.emplace(NLST_COMMAND, NLST_DESCRIPTION);
    m_Descriptions.emplace(NOOP_COMMAND, NOOP_DESCRIPTION);
    m_Descriptions.emplace(OPTS_COMMAND, OPTS_DESCRIPTION);
    m_Descriptions.emplace(PASS_COMMAND, PASS_DESCRIPTION);
    m_Descriptions.emplace(PASV_COMMAND, PASV_DESCRIPTION);
    m_Descriptions.emplace(PBSZ_COMMAND, PBSZ_DESCRIPTION);
    m_Descriptions.emplace(PORT_COMMAND, PORT_DESCRIPTION);
    m_Descriptions.emplace(PROT_COMMAND, PROT_DESCRIPTION);
    m_Descriptions.emplace(PWD_COMMAND,  PWD_DESCRIPTION);
    m_Descriptions.emplace(QUIT_COMMAND, QUIT_DESCRIPTION);
    m_Descriptions.emplace(REIN_COMMAND, REIN_DESCRIPTION);
    m_Descriptions.emplace(REST_COMMAND, REST_DESCRIPTION);
    m_Descriptions.emplace(RETR_COMMAND, RETR_DESCRIPTION);
    m_Descriptions.emplace(RMD_COMMAND,  RMD_DESCRIPTION);
    m_Descriptions.emplace(RMDA_COMMAND, RMDA_DESCRIPTION);
    m_Descriptions.emplace(RNFR_COMMAND, RNFR_DESCRIPTION);
    m_Descriptions.emplace(RNTO_COMMAND, RNTO_DESCRIPTION);
    m_Descriptions.emplace(SITE_COMMAND, SITE_DESCRIPTION);
    m_Descriptions.emplace(SIZE_COMMAND, SIZE_DESCRIPTION);
    m_Descriptions.emplace(SMNT_COMMAND, SMNT_DESCRIPTION);
    m_Descriptions.emplace(SPSV_COMMAND, SPSV_DESCRIPTION);
    m_Descriptions.emplace(STAT_COMMAND, STAT_DESCRIPTION);
    m_Descriptions.emplace(STOR_COMMAND, STOR_DESCRIPTION);
    m_Descriptions.emplace(STOU_COMMAND, STOU_DESCRIPTION);
    m_Descriptions.emplace(STRU_COMMAND, STRU_DESCRIPTION);
    m_Descriptions.emplace(SYST_COMMAND, SYST_DESCRIPTION);
    m_Descriptions.emplace(THMB_COMMAND, THMB_DESCRIPTION);
    m_Descriptions.emplace(TYPE_COMMAND, TYPE_DESCRIPTION);
    m_Descriptions.emplace(USER_COMMAND, USER_DESCRIPTION);
    m_Descriptions.emplace(XCUP_COMMAND, XCUP_DESCRIPTION);
    m_Descriptions.emplace(XMKD_COMMAND, XMKD_DESCRIPTION);
    m_Descriptions.emplace(XPWD_COMMAND, XPWD_DESCRIPTION);
    m_Descriptions.emplace(XRCP_COMMAND, XRCP_DESCRIPTION);
    m_Descriptions.emplace(XRMD_COMMAND, XRMD_DESCRIPTION);
    m_Descriptions.emplace(XRSQ_COMMAND, XRSQ_DESCRIPTION);
    m_Descriptions.emplace(XSEM_COMMAND, XSEM_DESCRIPTION);
    m_Descriptions.emplace(XSEN_COMMAND, XSEN_DESCRIPTION);

    return true;
}

std::string CommandBuilder::getCommandDescription(const std::string &strCommand)
{
    try
    {
        return m_Descriptions.at(strCommand);
    }
    catch (const std::out_of_range& e)
    {
        return std::string();
    }
}

std::shared_ptr<FTPCommandData> CommandBuilder::getCommandObject(const std::string &strCommand)
{
    try
    {
        return m_Commands.at(strCommand);
    }
    catch (const std::out_of_range& e)
    {
        return nullptr;
    }
}

void CommandBuilder::deleteControlCharacter()
{
    for(size_t ni = m_strCommandLine.length() - 1 ; ni >=0 ; ni--)
    {
        u_char ch = static_cast<u_char>(m_strCommandLine.at(ni));

        // 0 ~ 32 -> ASCII Control Character
        if(32 >= ch)
        {
            m_strCommandLine.erase(ni, 1);     
        }
        else
        {
            break;
        }
    }

    return;
}

void CommandBuilder::deleteFrontWhiteSpace()
{
    size_t nIndex = m_strCommandLine.find_first_not_of(" \t");

    if (nIndex != std::string::npos)
    {
        m_strCommandLine = m_strCommandLine.substr(nIndex);
    }

    return;
}

std::string CommandBuilder::getCommand()
{    
    deleteFrontWhiteSpace();

    std::string strCommand;

    size_t nIndex = m_strCommandLine.find_first_of(" \t\n");

    if(nIndex != std::string::npos)
    {
        strCommand       = m_strCommandLine.substr(0, nIndex);   
        m_strCommandLine = m_strCommandLine.substr(nIndex + 1);
    }
    else
    {
        strCommand = m_strCommandLine;
        m_strCommandLine.clear();
    }

    transform(strCommand.begin(), strCommand.end(), strCommand.begin(), ::toupper);

    return strCommand;
}

std::string CommandBuilder::getParameter()
{
    deleteFrontWhiteSpace();

    std::string strParameter;
    strParameter = m_strCommandLine.substr(0);

    m_strCommandLine.clear();    

    return strParameter;
}

std::shared_ptr<FTPCommandData> CommandBuilder::doBuild(std::string &&strCommandLine)
{    
    m_strCommandLine = strCommandLine;

    if(m_strCommandLine.empty())
    {
        hsfdlog(D_ERRO, "Get empty request");
        return nullptr;
    }

    deleteControlCharacter();

    std::string strCommand = getCommand();

    if(strCommand.empty())
    {
        hsfdlog(D_ERRO, "Get empty command");
        return nullptr;
    }

    std::string strParameter = getParameter();

    std::shared_ptr<FTPCommandData> pCommand = getCommandObject(strCommand);

    if(!pCommand)
    {
        pCommand = getCommandObject(DEFAULT_COMMAND);
    }

    if(!pCommand)
    {
        hsfdlog(D_ERRO, "Invalid command");
        return nullptr;
    }

    pCommand->setCommand(strCommand);
    pCommand->setParameter(strParameter);
    pCommand->setDescription(getCommandDescription(strCommand));

    hsfdlog(D_INFO, "Get from client [ %s ] command", strCommand.c_str());

    return pCommand;
}

std::shared_ptr<FTPCommandData> CommandBuilder::getStartCommand()
{
    return getCommandObject(START_COMMAND);
}

CommandBuilder* CommandBuilder::getInstance()
{
    std::call_once(m_Flag, []
    {
        std::unique_ptr<CommandBuilder> pInstance(new CommandBuilder);
        m_pInstance = move(pInstance);
    });

    return m_pInstance.get();
}