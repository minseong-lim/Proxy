#pragma once
#include <string>
#include <cstring>
#include <tuple>
#include <memory>
#include <set>
#include <stack>
#include <vector>
#include <future>
#include <functional>
#include <tree_sitter/api.h>

using Variable          = std::pair<std::string, std::string>;
using ExpansionLocation = std::tuple<u_int32_t, u_int32_t, std::string>;

class ShellParser
{
private:
    std::unique_ptr<TSParser, std::function<void(TSParser*)>> m_pParser;
    std::unique_ptr<TSTree,   std::function<void(TSTree*)>>   m_pTree;

    std::vector<std::future<std::string>>   m_CommandFutures;
    std::stack<ExpansionLocation>           m_Expansions;
    std::vector<Variable>                   m_Variables;
    std::vector<std::string>                m_Parameters;    
    std::string                             m_strScript;
    TSNode                                  m_Root;    
    
    void parseScript();
    std::string getByteString(const TSNode& node);

    ShellParser* init();
    ShellParser* parseVariable();
    std::set<std::string> parseCommand();
public:
    ShellParser()           = default;
    virtual ~ShellParser()  = default;
    std::set<std::string> parse(std::string strScript, std::string strParameters = std::string());
};

///////////////////////////////// Define template function.../////////////////////////////////////

template<typename Function>
void runCallback(const TSNode& node, Function&& function)
{
    function(node);
}

template<typename Function, typename... Callbacks>
void runCallback(const TSNode& node, Function&& function, Callbacks&&... callbacks)
{
    function(node);
    runCallback(node, std::forward<Callbacks>(callbacks)...);
}

/// @brief Node Traverse Function...
template <typename... Callbacks>
std::function<void(const TSNode&, Callbacks&&...)>
traverseNode = [](const TSNode& node, Callbacks&&... callbacks)
{
    if (ts_node_is_null(node))//ts_node_has_error(node))
    {
        return;
    }

    runCallback(node, std::forward<Callbacks>(callbacks)...);
    
    TSTreeCursor cursor = ts_tree_cursor_new(node);

    if (!ts_tree_cursor_goto_first_child(&cursor))
    {
        ts_tree_cursor_delete(&cursor);
        return;
    }

    while (true)
    {        
        traverseNode<Callbacks...>(ts_tree_cursor_current_node(&cursor), std::forward<Callbacks>(callbacks)...);

        if (!ts_tree_cursor_goto_next_sibling(&cursor))
        {
            break;
        }        
    }

    ts_tree_cursor_delete(&cursor);
};