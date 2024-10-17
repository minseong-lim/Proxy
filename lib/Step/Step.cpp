#ifndef __STEP_CPP__
#define __STEP_CPP__

#include "Step.hpp"
#include <tuple>

template <typename Function, typename... Args>
Step::Step(Function function, Args&&... args)
{
    m_Function = [function, args = std::make_tuple(std::forward<Args>(args)...)]
    {
        return std::apply(function, args);
    };
}

template <typename Next, typename... NextArgs>
Step Step::then(Next next, NextArgs&&... args)
{
    return Step([this, next, args = std::make_tuple(std::forward<NextArgs>(args)...)]
    {
        eStepResult eResult = execute();
        
        if (eStepResult::eContinue == eResult)
        {
            return std::apply(next, args);
        }

        return eResult;
    });
}

#endif