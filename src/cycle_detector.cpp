#include "cycle_detector.hpp"

namespace permuto {
    CycleDetector::CycleDetector() = default;
    
    bool CycleDetector::would_create_cycle(const std::string& path) const {
        return path_set_.find(path) != path_set_.end();
    }
    
    void CycleDetector::push_path(const std::string& path) {
        path_stack_.push_back(path);
        path_set_.insert(path);
    }
    
    void CycleDetector::pop_path() {
        if (!path_stack_.empty()) {
            path_set_.erase(path_stack_.back());
            path_stack_.pop_back();
        }
    }
    
    std::vector<std::string> CycleDetector::get_current_path() const {
        return path_stack_;
    }
    
    void CycleDetector::clear() {
        path_stack_.clear();
        path_set_.clear();
    }
}