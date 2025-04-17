export module Profiling:CPUScope;

import Common;

export class CPUScope
{
public:
    CPUScope(std::string name, const std::source_location& location = std::source_location::current());
    ~CPUScope();

private:
    std::string name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::source_location location;
    uint8_t scopeLevel;
    int reservedScopeIndex;
};