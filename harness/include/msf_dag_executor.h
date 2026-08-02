// MSF DAG Execution Engine (CE1 Phase 1.9)
// H1→H8 dependency resolution and execution ordering.
// Authority class: EXPERIMENT_INFRASTRUCTURE

#ifndef MSF_DAG_EXECUTOR_H
#define MSF_DAG_EXECUTOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <queue>
#include <memory>

#include "harness_dag.h"
#include "harness_authority.h"
#include "msf_scenario_engine.h"
#include "msf_evidence_emission.h"

namespace msf {

// ============================================================================
// MSF Task Definitions (H1-H8)
// ============================================================================

enum class MSFTaskID {
    H1_STATE_GENERATOR,
    H2_WORK_EXECUTOR,
    H3_FORK_ENGINE,
    H4_TELEMETRY_COLLECTOR,
    H5_MISSING_TELEMETRY,
    H6_ANTI_GAMING,
    H7_EVIDENCE_EMITTER,
    H8_VERIFIER
};

struct MSFTaskNode {
    MSFTaskID id;
    std::string task_id_str;
    std::vector<MSFTaskID> dependencies;
    harness::TaskStatus status = harness::TaskStatus::PENDING;
    std::string failure_reason;
    std::string evidence_hash;
    std::vector<std::string> output_hashes;
    std::string authority_envelope_hash;
    std::string authority_class;
    std::string worktree_path;
};

// ============================================================================
// MSF DAG Executor
// ============================================================================

class MSFDAGExecutor {
public:
    MSFDAGExecutor();
    
    // Build the MSF H1-H8 DAG
    bool BuildMSFDAG();
    
    // Execute the DAG (build-only mode for CE1)
    bool ExecuteDAG(bool build_only = true);
    
    // Get execution order
    std::vector<MSFTaskID> GetTopologicalOrder() const;
    
    // Get task status
    harness::TaskStatus GetTaskStatus(MSFTaskID task) const;
    
    // Get task by ID
    const MSFTaskNode* GetTask(MSFTaskID task) const;
    
    // Check if DAG is complete
    bool IsComplete() const;
    
    // Get statistics
    struct Statistics {
        size_t total_tasks = 0;
        size_t completed = 0;
        size_t failed = 0;
        size_t pending = 0;
        size_t ready = 0;
        size_t total_edges = 0;
        size_t max_depth = 0;
    };
    Statistics GetStatistics() const;
    
    // Generate execution plan for evidence
    std::string GenerateExecutionPlanJSON() const;

private:
    // H1-H8 task definitions
    void InitializeTasks();
    void InitializeDependencies();
    
    // Authority envelope creation
    harness::AuthorityEnvelope CreateH1Envelope();
    harness::AuthorityEnvelope CreateH2Envelope();
    harness::AuthorityEnvelope CreateH3Envelope();
    harness::AuthorityEnvelope CreateH4Envelope();
    harness::AuthorityEnvelope CreateH5Envelope();
    harness::AuthorityEnvelope CreateH6Envelope();
    harness::AuthorityEnvelope CreateH7Envelope();
    harness::AuthorityEnvelope CreateH8Envelope();
    
    // Task execution
    bool ExecuteTask(const MSFTaskNode& task);
    bool ValidateTaskPreconditions(const MSFTaskNode& task) const;
    
    std::unordered_map<MSFTaskID, MSFTaskNode> tasks_;
    harness::DependencyDAG dag_;
    
    // H1-H8 dependency mapping
    static const std::vector<std::pair<MSFTaskID, MSFTaskID>> kDependencies;
};

} // namespace msf

#endif // MSF_DAG_EXECUTOR_H