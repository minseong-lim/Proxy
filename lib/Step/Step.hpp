#ifndef __STEP_HPP__
#define __STEP_HPP__

#include <functional>

enum class eStepResult
{
	eNone,
	eError,	
	eStop,
	eContinue,
	eComplete,
	eRestart
};

class Step
{
private:
    std::function<eStepResult(void)> m_Function;
public:
    template <typename Function, typename... Args>
    Step(Function function, Args&&... args);

    template <typename Next, typename... NextArgs>
    Step then(Next next, NextArgs&&... args);

    eStepResult execute() { return m_Function ? m_Function() : eStepResult::eError; }
};

#include "Step.cpp"

#endif