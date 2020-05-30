#include "minisat/core/Constraint.h"
#include "minisat/core/Solver.h"

#include <vector>
#include <algorithm>

namespace Minisat {

class ActiveVerticesConnected : public Constraint {
public:
    ActiveVerticesConnected(const std::vector<Lit> &lits, const std::vector<std::pair<int, int>> &edges);
    virtual ~ActiveVerticesConnected() = default;

    void getWatchers(Solver& solver, vec<Lit>& out_watchers) override;
    bool propagate(Solver& solver, Lit p) override;
    void calcReason(Solver& solver, Lit p, vec<Lit>& out_reason) override;
    void undo(Solver& solver, Lit p) override;

private:
    enum NodeState {
        kUndecided, kActive, kInactive
    };
    const int kUnvisited = -1;

    void loadState(Solver& solver);
    int buildTree(int v, int parent, int cluster_id);

    std::vector<Lit> lits_;
    std::vector<std::vector<int>> adj_;
    std::vector<NodeState> state_;
    std::vector<int> rank_, lowlink_, subtree_active_count_, cluster_id_, parent_;
    int next_rank_;
};

}
