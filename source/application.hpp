#pragma once

#include <vector>
#include <memory>

#include "project.hpp"

class Application 
{
public:
    static void Initialize();
    static void Shutdown();
    static void DrawProject();

private:
    static Project project;
};
