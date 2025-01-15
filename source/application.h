#pragma once

#include <vector>
#include <memory>

#include "project.h"

class Application {
public:
    static void Initialize();
    static void Shutdown();

private:
    static std::vector<std::unique_ptr<Project>> projects;
};
