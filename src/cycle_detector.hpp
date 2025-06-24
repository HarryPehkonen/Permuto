#pragma once
#include <string>
#include <vector>
#include <unordered_set>

namespace permuto {
    class CycleDetector {
    public:
        CycleDetector();
        
        // Check if adding this path would create a cycle
        bool would_create_cycle(const std::string& path) const;
        
        // Add a path to the current stack (when entering)
        void push_path(const std::string& path);
        
        // Remove a path from the current stack (when exiting)
        void pop_path();
        
        // Get the current path stack for cycle reporting
        std::vector<std::string> get_current_path() const;
        
        // Clear all tracking (for reuse)
        void clear();
        
    private:
        std::vector<std::string> path_stack_;
        std::unordered_set<std::string> path_set_;
    };
}