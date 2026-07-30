#ifndef HARNESS_WORKTREE_H
#define HARNESS_WORKTREE_H

#include <string>
#include <vector>
#include <unordered_map>

namespace harness {

// ============================================================================
// O3: Worktree Manager - Statistics struct (must be defined before WorktreeManager)
// ============================================================================
struct Statistics {
    size_t total_worktrees;
    size_t active_worktrees;
    size_t total_modified_files;
    size_t isolated_tasks;
};

struct WorktreeConfig {
    std::string task_id;
    std::string branch_name;
    std::string base_branch;
    std::vector<std::string> allowed_paths;
    std::vector<std::string> prohibited_paths;
    bool auto_cleanup;
    std::string cleanup_policy;
};

struct WorktreeState {
    std::string task_id;
    std::string worktree_path;
    std::string branch_name;
    bool is_active;
    std::string created_at;
    std::string last_modified;
    std::vector<std::string> modified_files;
};

class WorktreeManager {
public:
    WorktreeManager(const std::string& repo_root);
    ~WorktreeManager();
    
    bool CreateWorktree(const WorktreeConfig& config);
    bool RemoveWorktree(const std::string& task_id);
    const WorktreeState* GetWorktree(const std::string& task_id) const;
    std::vector<WorktreeState> ListWorktrees() const;
    bool IsPathInScope(const std::string& task_id, const std::string& path) const;
    bool IsFileModified(const std::string& task_id, const std::string& file_path) const;
    std::vector<std::string> GetModifiedFiles(const std::string& task_id) const;
    bool CommitChanges(const std::string& task_id, const std::string& message);
    void Cleanup(const std::string& policy = "all");
    bool ValidateIsolation() const;
    Statistics GetStatistics() const;

private:
    std::string repo_root_;
    std::unordered_map<std::string, WorktreeState> worktrees_;
    
    std::string ExecuteCommand(const std::string& command) const;
    bool WorktreeExists(const std::string& path) const;
    std::vector<std::string> GetGitStatus(const std::string& path) const;
    std::string GetCurrentTimestamp() const;
};

class WorktreeScopeValidator {
public:
    static bool ValidateMutation(const WorktreeConfig& config, const std::string& file_path);
    static bool ValidateMutations(const WorktreeConfig& config, const std::vector<std::string>& file_paths);
    static std::vector<std::string> FilterAllowedPaths(const WorktreeConfig& config, const std::vector<std::string>& file_paths);
    static std::vector<std::string> FilterProhibitedPaths(const WorktreeConfig& config, const std::vector<std::string>& file_paths);
};

} // namespace harness

#endif // HARNESS_WORKTREE_H