#include "harness_worktree.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

namespace harness {

// ============================================================================
// WorktreeManager Implementation
// ============================================================================

WorktreeManager::WorktreeManager(const std::string& repo_root)
    : repo_root_(repo_root) {
}

WorktreeManager::~WorktreeManager() {
    // Cleanup on destruction
    Cleanup("on-failure");
}

bool WorktreeManager::CreateWorktree(const WorktreeConfig& config) {
    // Check if worktree already exists
    if (worktrees_.find(config.task_id) != worktrees_.end()) {
        return false;
    }
    
    // Create worktree path
    std::string worktree_path = repo_root_ + "/harness/worktrees/" + config.task_id;
    
    // Create directory
    if (!fs::exists(worktree_path)) {
        fs::create_directories(worktree_path);
    }
    
    // Execute git worktree add
    std::string command = "cd " + repo_root_ + " && git worktree add " + 
                         worktree_path + " " + config.base_branch;
    
    int result = std::system(command.c_str());
    if (result != 0) {
        return false;
    }
    
    // Record worktree state
    WorktreeState state;
    state.task_id = config.task_id;
    state.branch_name = config.branch_name;
    state.is_active = true;
    // Assign current timestamp in readable format
    auto now = std::chrono::system_clock::now();
    auto time_t_time = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&time_t_time));
    state.created_at = std::string(buffer);
    state.last_modified = std::string(buffer);
    
    worktrees_[config.task_id] = state;
    
    return true;
}

bool WorktreeManager::RemoveWorktree(const std::string& task_id) {
    auto it = worktrees_.find(task_id);
    if (it == worktrees_.end()) {
        return false;
    }
    
    // Execute git worktree remove
    std::string command = "cd " + repo_root_ + " && git worktree remove " + 
                         it->second.worktree_path;
    
    int result = std::system(command.c_str());
    if (result != 0) {
        return false;
    }
    
    // Remove from map
    worktrees_.erase(it);
    
    return true;
}

const WorktreeState* WorktreeManager::GetWorktree(const std::string& task_id) const {
    auto it = worktrees_.find(task_id);
    if (it == worktrees_.end()) {
        return nullptr;
    }
    return &(it->second);
}

std::vector<WorktreeState> WorktreeManager::ListWorktrees() const {
    std::vector<WorktreeState> result;
    for (const auto& [task_id, state] : worktrees_) {
        result.push_back(state);
    }
    return result;
}

bool WorktreeManager::IsPathInScope(
    const std::string& task_id,
    const std::string& path
) const {
    auto it = worktrees_.find(task_id);
    if (it == worktrees_.end()) {
        return false;
    }
    
    // Check if path is within worktree
    std::string worktree_path = it->second.worktree_path;
    return path.find(worktree_path) == 0;
}

bool WorktreeManager::IsFileModified(
    const std::string& task_id,
    const std::string& file_path
) const {
    auto it = worktrees_.find(task_id);
    if (it == worktrees_.end()) {
        return false;
    }
    
    // Check if file is in modified files list
    for (const auto& modified : it->second.modified_files) {
        if (modified == file_path) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> WorktreeManager::GetModifiedFiles(
    const std::string& task_id
) const {
    auto it = worktrees_.find(task_id);
    if (it == worktrees_.end()) {
        return {};
    }
    
    return it->second.modified_files;
}

bool WorktreeManager::CommitChanges(
    const std::string& task_id,
    const std::string& message
) {
    auto it = worktrees_.find(task_id);
    if (it == worktrees_.end()) {
        return false;
    }
    
    // Execute git add and commit
    std::string command = "cd " + it->second.worktree_path + 
                         " && git add -A && git commit -m \"" + message + "\"";
    
    int result = std::system(command.c_str());
    if (result != 0) {
        return false;
    }
    
    // Update last modified
    it->second.last_modified = GetCurrentTimestamp();
    
    return true;
}

void WorktreeManager::Cleanup(const std::string& policy) {
    std::vector<std::string> to_remove;
    
    for (const auto& [task_id, state] : worktrees_) {
        bool should_remove = false;
        
        if (policy == "all") {
            should_remove = true;
        } else if (policy == "on-failure") {
            // Check if worktree has failed state
            // This would need integration with DAG
            should_remove = false;
        } else if (policy == "on-success") {
            // Check if worktree has success state
            // This would need integration with DAG
            should_remove = false;
        } else if (policy == "manual") {
            should_remove = false;
        }
        
        if (should_remove) {
            to_remove.push_back(task_id);
        }
    }
    
    for (const auto& task_id : to_remove) {
        RemoveWorktree(task_id);
    }
}

bool WorktreeManager::ValidateIsolation() const {
    // Check that no two worktrees share the same path
    std::unordered_map<std::string, std::string> paths;
    
    for (const auto& [task_id, state] : worktrees_) {
        if (paths.find(state.worktree_path) != paths.end()) {
            return false;  // Duplicate path
        }
        paths[state.worktree_path] = task_id;
    }
    
    // Check that all worktrees are within repo root
    for (const auto& [task_id, state] : worktrees_) {
        if (state.worktree_path.find(repo_root_) != 0) {
            return false;  // Worktree outside repo
        }
    }
    
    return true;
}

Statistics WorktreeManager::GetStatistics() const {
    Statistics stats;
    stats.total_worktrees = worktrees_.size();
    stats.active_worktrees = 0;
    stats.total_modified_files = 0;
    stats.isolated_tasks = 0;
    
    for (const auto& [task_id, state] : worktrees_) {
        if (state.is_active) {
            stats.active_worktrees++;
        }
        stats.total_modified_files += state.modified_files.size();
        stats.isolated_tasks++;
    }
    
    return stats;
}

// ============================================================================
// Private Methods
// ============================================================================

std::string WorktreeManager::ExecuteCommand(const std::string& command) const {
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}

bool WorktreeManager::WorktreeExists(const std::string& path) const {
    return fs::exists(path);
}

std::vector<std::string> WorktreeManager::GetGitStatus(const std::string& path) const {
    std::string command = "cd " + path + " && git status --porcelain";
    std::string output = ExecuteCommand(command);
    
    std::vector<std::string> files;
    std::istringstream stream(output);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            // Extract filename from git status output
            files.push_back(line.substr(3));  // Skip status codes
        }
    }
    
    return files;
}

std::string WorktreeManager::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&time));
    return std::string(buffer);
}

// ============================================================================
// WorktreeScopeValidator Implementation
// ============================================================================

bool WorktreeScopeValidator::ValidateMutation(
    const WorktreeConfig& config,
    const std::string& file_path
) {
    // Check if file is in prohibited paths
    for (const auto& prohibited : config.prohibited_paths) {
        if (prohibited == "*") return false;  // All paths prohibited
        if (file_path.find(prohibited) == 0) return false;
    }
    
    // If no allowed paths specified, check if not prohibited
    if (config.allowed_paths.empty()) {
        return true;
    }
    
    // Check if file is in allowed paths
    for (const auto& allowed : config.allowed_paths) {
        if (file_path.find(allowed) == 0) return true;
    }
    
    return false;
}

bool WorktreeScopeValidator::ValidateMutations(
    const WorktreeConfig& config,
    const std::vector<std::string>& file_paths
) {
    for (const auto& path : file_paths) {
        if (!ValidateMutation(config, path)) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> WorktreeScopeValidator::FilterAllowedPaths(
    const WorktreeConfig& config,
    const std::vector<std::string>& file_paths
) {
    std::vector<std::string> allowed;
    
    for (const auto& path : file_paths) {
        if (ValidateMutation(config, path)) {
            allowed.push_back(path);
        }
    }
    
    return allowed;
}

std::vector<std::string> WorktreeScopeValidator::FilterProhibitedPaths(
    const WorktreeConfig& config,
    const std::vector<std::string>& file_paths
) {
    std::vector<std::string> prohibited;
    
    for (const auto& path : file_paths) {
        if (!ValidateMutation(config, path)) {
            prohibited.push_back(path);
        }
    }
    
    return prohibited;
}

} // namespace harness
