#ifndef HARNESS_DAG_H
#define HARNESS_DAG_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace harness {

// ============================================================================
// O2: Dependency DAG
//
// Machine-readable task readiness. Nodes = task IDs, edges = depends_on.
// Must be acyclic. Readiness when all dependencies satisfied.
// ============================================================================

enum class TaskStatus {
    PENDING,        // Not started
    READY,          // All dependencies satisfied
    RUNNING,        // Currently executing
    COMPLETED,      // Successfully completed
    FAILED,         // Failed
    BLOCKED         // Blocked by dependency failure
};

struct TaskNode {
    std::string task_id;
    std::vector<std::string> dependencies;
    TaskStatus status;
    std::string failure_reason;
    
    // Evidence from this task
    std::string evidence_hash;
    std::vector<std::string> output_hashes;
};

class DependencyDAG {
public:
    DependencyDAG();
    
    // Add a task node
    bool AddNode(const TaskNode& node);
    
    // Add a dependency edge (from depends on to)
    bool AddEdge(const std::string& from, const std::string& to);
    
    // Check if DAG is acyclic
    bool IsAcyclic() const;
    
    // Get all ready tasks (all dependencies satisfied)
    std::vector<std::string> GetReadyTasks() const;
    
    // Get all pending tasks
    std::vector<std::string> GetPendingTasks() const;
    
    // Get all completed tasks
    std::vector<std::string> GetCompletedTasks() const;
    
    // Get all failed tasks
    std::vector<std::string> GetFailedTasks() const;
    
    // Update task status
    bool UpdateTaskStatus(
        const std::string& task_id,
        TaskStatus status,
        const std::string& failure_reason = ""
    );
    
    // Set task evidence
    bool SetTaskEvidence(
        const std::string& task_id,
        const std::string& evidence_hash,
        const std::vector<std::string>& output_hashes
    );
    
    // Check if a task is ready (all dependencies completed)
    bool IsTaskReady(const std::string& task_id) const;
    
    // Check if all tasks are completed
    bool AllTasksCompleted() const;
    
    // Check if any task has failed
    bool AnyTaskFailed() const;
    
    // Get task by ID
    const TaskNode* GetTask(const std::string& task_id) const;
    
    // Get all tasks
    const std::unordered_map<std::string, TaskNode>& GetAllTasks() const;
    
    // Get topological order
    std::vector<std::string> GetTopologicalOrder() const;
    
    // Get critical path (longest path through DAG)
    std::vector<std::string> GetCriticalPath() const;
    
    // Validate DAG structure
    bool Validate() const;
    
    // Get DAG statistics
    struct Statistics {
        size_t total_tasks;
        size_t completed_tasks;
        size_t failed_tasks;
        size_t pending_tasks;
        size_t ready_tasks;
        size_t total_edges;
        size_t max_depth;
    };
    Statistics GetStatistics() const;
    
    // Print DAG (for debugging)
    void Print() const;

private:
    std::unordered_map<std::string, TaskNode> nodes_;
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacency_;  // task -> dependencies
    std::unordered_map<std::string, std::unordered_set<std::string>> reverse_adjacency_;  // task -> dependents
    
    // Cycle detection
    bool HasCycle() const;
    bool DFSVisit(
        const std::string& node,
        std::unordered_set<std::string>& visited,
        std::unordered_set<std::string>& recursion_stack
    ) const;
    
    // Topological sort
    void TopologicalSortUtil(
        const std::string& node,
        std::unordered_set<std::string>& visited,
        std::vector<std::string>& order
    ) const;
};

} // namespace harness

#endif // HARNESS_DAG_H
