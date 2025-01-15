#pragma once

#include <string>

enum class ScriptWritingState {
    eNone,
    eHeading,
    eCharacter,
    eParenthetical,
    eDialogue,
    eAction
};

class Script {
public:
    Script();
    ~Script();

private:
    std::string name;
    std::string script;
};