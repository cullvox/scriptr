#pragma once

#include <memory>
#include <unordered_map>

#include "node.hpp"

class Project {
public:
    Project();
    ~Project();

private:
    std::unordered_map<uint32_t, std::unique_ptr<Node>> nodes;
};