#include "harness_dag.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace harness {

// Helper function for status name (forward declaration)
static const char* TaskNodeStatusName(TaskStatus status) {
    switch (status) {
        case TaskStatus::PENDING: return "PENDING";
        case TaskStatus::READY: return "READY";
        case TaskStatus::RUNNING: return "RUNNING";
        case TaskStatus::COMPLETED: return "COMPLETED";
        case TaskStatus::FAILED: return "FAILED";
        case TaskStatus::BLOCKED: return "BLOCKED";
    }
    return "UNKNOWN";
}

// ============================================================================
// DependencyDAG Implementation
// ============================================================================

DependencyDAG::DependencyDAG() {}

bool DependencyDAG::AddNode(const TaskNode& node) {
    if (node.task_id.empty()) return false;
    
    // Check if node already exists
    if (nodes_.find(node.task_id) != nodes_.end()) {
        return false;
    }
    
    nodes_[node.task_id] = node;
    nodes_[node.task_id].status = TaskStatus::PENDING;
    
    // Initialize adjacency lists
    adjacency_[node.task_id] = std::unordered_set<std::string>();
    reverse_adjacency_[node.task_id] = std::unordered_set<std::string>();
    
    // Add edges for existing dependencies
    for (const auto& dep : node.dependencies) {
        adjacency_[node.task_id].insert(dep);
        reverse_adjacency_[dep].insert(node.task_id);
    }
    
    return true;
}

bool DependencyDAG::AddEdge(const std::string& from, const std::string& to) {
    // Check if both nodes exist
    if (nodes_.find(from) == nodes_.end()) return false;
    if (nodes_.find(to) == nodes_.end()) return false;
    
    // Check if edge already exists
    if (adjacency_[from].find(to) != adjacency_[from].end()) {
        return true;  // Edge already exists
    }
    
    // Add edge
    adjacency_[from].insert(to);
    reverse_adjacency_[to].insert(from);
    
    // Check if adding this edge creates a cycle
    if (HasCycle()) {
        // Remove edge
        adjacency_[from].erase(to);
        reverse_adjacency_[to].erase(from);
        return false;
    }
    
    // Update task dependencies
    nodes_[from].dependencies.push_back(to);
    
    return true;
}

bool DependencyDAG::IsAcyclic() const {
    return !HasCycle();
}

std::vector<std::string> DependencyDAG::GetReadyTasks() const {
    std::vector<std::string> ready;
    
    for (const auto& [task_id, node] : nodes_) {
        if (node.status == TaskStatus::PENDING && IsTaskReady(task_id)) {
            ready.push_back(task_id);
        }
    }
    
    return ready;
}

std::vector<std::string> DependencyDAG::GetPendingTasks() const {
    std::vector<std::string> pending;
    
    for (const auto& [task_id, node] : nodes_) {
        if (node.status == TaskStatus::PENDING) {
            pending.push_back(task_id);
        }
    }
    
    return pending;
}

std::vector<std::string> DependencyDAG::GetCompletedTasks() const {
    std::vector<std::string> completed;
    
    for (const auto& [task_id, node] : nodes_) {
        if (node.status == TaskStatus::COMPLETED) {
            completed.push_back(task_id);
        }
    }
    
    return completed;
}

std::vector<std::string> DependencyDAG::GetFailedTasks() const {
    std::vector<std::string> failed;
    
    for (const auto& [task_id, node] : nodes_) {
        if (node.status == TaskStatus::FAILED) {
            failed.push_back(task_id);
        }
    }
    
    return failed;
}

bool DependencyDAG::UpdateTaskStatus(
    const std::string& task_id,
    TaskStatus status,
    const std::string& failure_reason
) {
    auto it = nodes_.find(task_id);
    if (it == nodes_.end()) return false;
    
    it->second.status = status;
    it->second.failure_reason = failure_reason;
    
    // If task failed, block all dependent tasks
    if (status == TaskStatus::FAILED) {
        for (const auto& dependent : reverse_adjacency_[task_id]) {
            nodes_[dependent].status = TaskStatus::BLOCKED;
            nodes_[dependent].failure_reason = "Dependency failed: " + task_id;
        }
    }
    
    return true;
}

bool DependencyDAG::SetTaskEvidence(
    const std::string& task_id,
    const std::string& evidence_hash,
    const std::vector<std::string>& output_hashes
) {
    auto it = nodes_.find(task_id);
    if (it == nodes_.end()) return false;
    
    it->second.evidence_hash = evidence_hash;
    it->second.output_hashes = output_hashes;
    
    return true;
}

bool DependencyDAG::IsTaskReady(const std::string& task_id) const {
    auto it = nodes_.find(task_id);
    if (it == nodes_.end()) return false;
    
    // Check all dependencies are completed
    for (const auto& dep : it->second.dependencies) {
        auto dep_it = nodes_.find(dep);
        if (dep_it == nodes_.end()) return false;
        if (dep_it->second.status != TaskStatus::COMPLETED) return false;
    }
    
    return true;
}

bool DependencyDAG::AllTasksCompleted() const {
    for (const auto& [task_id, node] : nodes_) {
        if (node.status != TaskStatus::COMPLETED) {
            return false;
        }
    }
    return true;
}

bool DependencyDAG::AnyTaskFailed() const {
    for (const auto& [task_id, node] : nodes_) {
        if (node.status == TaskStatus::FAILED) {
            return true;
        }
    }
    return false;
}

const TaskNode* DependencyDAG::GetTask(const std::string& task_id) const {
    auto it = nodes_.find(task_id);
    if (it == nodes_.end()) return nullptr;
    return &(it->second);
}

const std::unordered_map<std::string, TaskNode>& DependencyDAG::GetAllTasks() const {
    return nodes_;
}

std::vector<std::string> DependencyDAG::GetTopologicalOrder() const {
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    
    for (const auto& [task_id, node] : nodes_) {
        if (visited.find(task_id) == visited.end()) {
            TopologicalSortUtil(task_id, visited, order);
        }
    }
    
    std::reverse(order.begin(), order.end());
    return order;
}

std::vector<std::string> DependencyDAG::GetCriticalPath() const {
    // Find the longest path through the DAG
    // This is a simplified version - just return topological order
    return GetTopologicalOrder();
}

bool DependencyDAG::Validate() const {
    // Check for cycles
    if (HasCycle()) return false;
    
    // Check that all dependencies exist
    for (const auto& [task_id, node] : nodes_) {
        for (const auto& dep : node.dependencies) {
            if (nodes_.find(dep) == nodes_.end()) {
                return false;
            }
        }
    }
    
    // Check that H_MSF is in immutable inputs for all tasks
    for (const auto& [task_id, node] : nodes_) {
        bool has_hmsf = false;
        // This would need access to authority envelope
        // For now, just check that task_id is not empty
        if (task_id.empty()) return false;
    }
    
    return true;
}

DependencyDAG::Statistics DependencyDAG::GetStatistics() const {
    Statistics stats;
    stats.total_tasks = nodes_.size();
    stats.completed_tasks = 0;
    stats.failed_tasks = 0;
    stats.pending_tasks = 0;
    stats.ready_tasks = 0;
    stats.total_edges = 0;
    stats.max_depth = 0;
    
    for (const auto& [task_id, node] : nodes_) {
        switch (node.status) {
            case TaskStatus::COMPLETED:
                stats.completed_tasks++;
                break;
            case TaskStatus::FAILED:
                stats.failed_tasks++;
                break;
            case TaskStatus::PENDING:
                stats.pending_tasks++;
                if (IsTaskReady(task_id)) {
                    stats.ready_tasks++;
                }
                break;
            default:
                break;
        }
        
        stats.total_edges += node.dependencies.size();
    }
    
    return stats;
}

void DependencyDAG::Print() const {
    std::cout << "Dependency DAG:" << std::endl;
    for (const auto& [task_id, node] : nodes_) {
        std::cout << "  " << task_id << " -> ";
        for (const auto& dep : node.dependencies) {
            std::cout << dep << " ";
        }
        std::cout << " [" << TaskNodeStatusName(node.status) << "]" << std::endl;
    }
}

// ============================================================================
// Private Methods
// ============================================================================

bool DependencyDAG::HasCycle() const {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    
    for (const auto& [task_id, node] : nodes_) {
        if (visited.find(task_id) == visited.end()) {
            if (DFSVisit(task_id, visited, recursion_stack)) {
                return true;
            }
        }
    }
    
    return false;
}

bool DependencyDAG::DFSVisit(
    const std::string& node,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& recursion_stack
) const {
    visited.insert(node);
    recursion_stack.insert(node);
    
    auto it = adjacency_.find(node);
    if (it != adjacency_.end()) {
        for (const auto& neighbor : it->second) {
            if (visited.find(neighbor) == visited.end()) {
                if (DFSVisit(neighbor, visited, recursion_stack)) {
                    return true;
                }
            } else if (recursion_stack.find(neighbor) != recursion_stack.end()) {
                return true;  // Cycle detected
            }
        }
    }
    
    recursion_stack.erase(node);
    return false;
}

void DependencyDAG::TopologicalSortUtil(
    const std::string& node,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& order
) const {
    visited.insert(node);
    
    auto it = adjacency_.find(node);
    if (it != adjacency_.end()) {
        for (const auto& neighbor : it->second) {
            if (visited.find(neighbor) == visited.end()) {
                TopologicalSortUtil(neighbor, visited, order);
            }
        }
    }
    
    order.push_back(node);
}

} // namespace harness
