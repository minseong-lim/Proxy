#include "ShellParser.hpp"
#include <algorithm>
#include <stack>
#include <functional>
#include <regex>
#include <boost/algorithm/string.hpp>

extern "C"
{
    const TSLanguage* tree_sitter_bash();
}

/// @brief Node Name
constexpr const char* COMMAND               = "command";
constexpr const char* VARIABLE_ASSIGNMENT   = "variable_assignment";
constexpr const char* EXPANSION             = "expansion";
constexpr const char* SIMPLE_EXPANSION      = "simple_expansion";

/// @brief Field Name
constexpr const char* NAME_FIELD            = "name";
constexpr const char* VALUE_FIELD           = "value";

constexpr const char* BLANK_STRING          = "\"\"";

/// @brief Regex Pattern
constexpr const char* EXPANSION_PATTERN     = "\\$(\\w+)|\\$\\{([^\\}]+)\\}";

/// @brief Define tuple index
constexpr int TUPLE_START   = 0;
constexpr int TUPLE_END     = 1;
constexpr int TUPLE_VALUE   = 2;

using CallbackFunction = std::function<void(const TSNode&)>;

ShellParser* ShellParser::init()
{
    std::unique_ptr<TSParser, std::function<void(TSParser*)>>
    pParser(ts_parser_new(), [](TSParser* parser){ ts_parser_delete(parser); });

    ts_parser_set_language(pParser.get(), tree_sitter_bash());
    m_pParser = std::move(pParser);

    m_pTree.reset();
    m_Variables.clear();
    m_CommandFutures.clear();

    memset(&m_Root, 0x00, sizeof(TSNode));
    
    while (!m_Expansions.empty())
    {
        m_Expansions.pop();
    }

    return this;
}

void ShellParser::parseScript()
{
    std::unique_ptr<TSTree, std::function<void(TSTree*)>>
    pTree(ts_parser_parse_string(m_pParser.get(), nullptr, m_strScript.c_str(), m_strScript.length()),
    [](TSTree* tree){ts_tree_delete(tree);});

    m_pTree = std::move(pTree);
    m_Root  = ts_tree_root_node(m_pTree.get());
}

std::string ShellParser::getByteString(const TSNode& node)
{
    if (ts_node_is_null(node))
    {
        return BLANK_STRING;
    }

    uint32_t nStart = ts_node_start_byte(node);
    uint32_t nEnd   = ts_node_end_byte(node);

    try
    {
        return m_strScript.substr(nStart, nEnd - nStart);
    }
    catch (const std::out_of_range& e)
    {
        return BLANK_STRING;
    }
}

std::set<std::string> ShellParser::parseCommand()
{
    parseScript();

    std::set<std::string> commands;

    auto getCommand = [this](const TSNode &node)
        {
            m_CommandFutures.emplace_back(std::async(std::launch::async, [this, node]
            {
                const char *pType = ts_node_type(node);
                
                if (!pType || (0 != strcasecmp(pType, COMMAND)))
                {
                    return std::string();
                }

                TSNode commandNameNode = ts_node_child_by_field_name(node, NAME_FIELD, strlen(NAME_FIELD));

                if (ts_node_is_null(commandNameNode))
                {
                    return std::string();
                }

                std::string strCommandName = getByteString(commandNameNode);
                boost::trim(strCommandName);

                return strCommandName;
            }));            
        };

    traverseNode<CallbackFunction>(m_Root, getCommand);

    std::for_each(m_CommandFutures.begin(), m_CommandFutures.end(),
    [&commands](auto& future)
    {        
        std::string strCommand = future.get();
        if (!strCommand.empty())
        {
            commands.insert(std::move(strCommand));
        }
    });

    return commands;
}

ShellParser* ShellParser::parseVariable()
{
    parseScript();

    auto getExpansionLocation = [this](uint32_t nStart, uint32_t nEnd)
        {
            if ((nStart < 0) || (nEnd >= m_strScript.length()))
            {
                return;
            }

            std::string strExpansion;

            try
            {
                strExpansion = m_strScript.substr(nStart, nEnd - nStart);
            }
            catch (const std::out_of_range &e)
            {
                return;
            }

            std::regex pattern(EXPANSION_PATTERN);
            std::smatch match;

            if (!std::regex_search(strExpansion, match, pattern))
            {
                return;
            }

            std::string strVariable = match[1].str().empty() ? match[2].str() : match[1].str();
            std::string strValue    = BLANK_STRING;

            if (std::all_of(strVariable.begin(), strVariable.end(), ::isdigit))
            {
                try
                {
                    strValue = m_Parameters.at(std::stoi(strVariable) - 1);
                }
                catch (const std::out_of_range &e)
                {
                    //
                }
            }
            else
            {
                auto it = std::find_if(m_Variables.begin(), m_Variables.end(),
                [strVariable](const auto &pair) { return pair.first == strVariable; });

                if (it != (m_Variables.end()))
                {
                    size_t index = std::distance(m_Variables.begin(), it);
                    strValue = m_Variables[index].second;
                }
            }

            m_Expansions.push(std::make_tuple(nStart, nEnd, std::move(strValue)));
        };

    auto substituteVariable = [this]
        {
            while(!m_Expansions.empty())
            {
                ExpansionLocation tuple = m_Expansions.top();
                m_Expansions.pop();
                
                uint32_t nStart      = std::get<TUPLE_START>(tuple);
                uint32_t nEnd        = std::get<TUPLE_END>(tuple);
                std::string strValue = std::get<TUPLE_VALUE>(tuple);

                m_strScript.erase(nStart, nEnd - nStart);
                m_strScript.insert(nStart, std::move(strValue));
            }
        };
    
    // Callback
    auto getExpansion = [this, getExpansionLocation](const TSNode &node)
        {
            const char *pType = ts_node_type(node);

            if (!pType || !(!strcasecmp(pType, EXPANSION) || !strcasecmp(pType, SIMPLE_EXPANSION)))
            {
                return;
            }

            getExpansionLocation(ts_node_start_byte(node), ts_node_end_byte(node));
        };

    // Callback
    auto getVariable = [this](const TSNode &node)
        {
            auto getVariableValue = [this](std::string strValue)
            {
                if (strValue.empty())
                {
                    return std::string();
                }

                std::regex pattern(EXPANSION_PATTERN);
                std::smatch match;

                if (!std::regex_search(strValue, match, pattern))
                {
                    return std::move(strValue);
                }

                std::string strVariable = match[1].str().empty() ? match[2].str() : match[1].str();

                // Parameter...
                if (std::all_of(strVariable.begin(), strVariable.end(), ::isdigit))
                {
                    try   { return m_Parameters.at(std::stoi(strVariable) - 1); }
                    catch (const std::out_of_range &e) { return std::string();  }
                }
                    
                // Variable...
                auto it = std::find_if(m_Variables.begin(), m_Variables.end(),
                [strVariable] (const auto& pair) { return pair.first == strVariable; });

                if (it != m_Variables.end())
                {
                    size_t index = std::distance(m_Variables.begin(), it);
                    return m_Variables[index].second;
                }

                return std::string();    
            };

            const char *pType = ts_node_type(node);

            if (!pType || !(!strcasecmp(pType, VARIABLE_ASSIGNMENT)))
            {
                return;
            }

            TSNode nameNode  = ts_node_child_by_field_name(node, NAME_FIELD,  strlen(NAME_FIELD));
            TSNode valueNode = ts_node_child_by_field_name(node, VALUE_FIELD, strlen(VALUE_FIELD));

            if (ts_node_is_null(nameNode))
            {
                return;
            }

            std::string strName  = getByteString(nameNode);
            std::string strValue = getVariableValue(std::move(getByteString(valueNode)));

            auto find_it = std::find_if(m_Variables.begin(), m_Variables.end(),
            [strName](const auto &pair) { return pair.first == strName; });

            if (find_it != m_Variables.end())
            {
                size_t index             = std::distance(m_Variables.begin(), find_it);
                m_Variables[index].first = strValue;
                return;
            }

            m_Variables.emplace_back(std::make_pair(std::move(strName), std::move(strValue)));
        };

    traverseNode<CallbackFunction, CallbackFunction>(m_Root, getExpansion, getVariable);

    substituteVariable();

    return this;
}

std::set<std::string> ShellParser::parse(std::string strScript, std::string strParameters)
{
    auto parseParameter = std::async(std::launch::async, [this, strParameters]
    {
        std::istringstream ss(strParameters);
        std::string token;

        while (std::getline(ss, token, ' ') || std::getline(ss, token, '\t') || std::getline(ss, token, '\\'))
        {
            boost::trim(token);
            m_Parameters.push_back(token);
        }

        return;
    });

    m_strScript = std::move(strScript);
    parseParameter.wait();

    return init()->parseVariable()->parseCommand();
}