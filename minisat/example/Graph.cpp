#include "minisat/example/Graph.h"

#include <set>
#include <algorithm>

namespace Minisat {

ActiveVerticesConnected::ActiveVerticesConnected(const std::vector<Lit>& lits, const std::vector<std::pair<int, int>>& edges)
    : lits_(lits), adj_(lits.size()), state_(lits.size(), kUndecided),
      rank_(lits.size()), lowlink_(lits.size()), subtree_active_count_(lits.size()), cluster_id_(lits.size()), parent_(lits.size()) {
    for (auto& e : edges) {
        adj_[e.first].push_back(e.second);
        adj_[e.second].push_back(e.first);
    }
}

void ActiveVerticesConnected::getWatchers(Solver& solver, vec<Lit>& out_watchers) {
    std::set<Lit> lits_unique;
    for (Lit l : lits_) {
        lits_unique.insert(l);
        lits_unique.insert(~l);
    }
    for (Lit l : lits_unique) {
        out_watchers.push(l);
    }
}

bool ActiveVerticesConnected::propagate(Solver& solver, Lit p) {
    int n = lits_.size();
    solver.registerUndo(var(p), this);
    for (int i = 0; i < n; ++i) {
        if (var(lits_[i]) == var(p)) {
            lbool val = solver.value(lits_[i]);
            NodeState s;
            if (val == l_True) s = kActive;
            else if (val == l_False) s = kInactive;
            else abort();
            state_[i] = s;
        }
    }

    std::fill(rank_.begin(), rank_.end(), kUnvisited);
    std::fill(lowlink_.begin(), lowlink_.end(), kUndecided);
    std::fill(subtree_active_count_.begin(), subtree_active_count_.end(), 0);
    std::fill(cluster_id_.begin(), cluster_id_.end(), kUndecided);
    std::fill(parent_.begin(), parent_.end(), -1);
    next_rank_ = 0;

    int nonempty_cluster = -1;

    for (int i = 0; i < n; ++i) {
        if (state_[i] != kInactive && rank_[i] == kUnvisited) {
            buildTree(i, -1, i);
            int sz = subtree_active_count_[i];
            if (sz >= 1) {
                if (nonempty_cluster != -1) return false; // already disconnected
                nonempty_cluster = i;
            }
        }
    }

    if (nonempty_cluster != -1) {
        for (int v = 0; v < n; ++v) {
            if (state_[v] != kUndecided) continue;

            if (cluster_id_[v] != nonempty_cluster) {
                // nodes outside the nonempty cluster should be inactive
                if (!solver.enqueue(~lits_[v], this)) return false;
            } else {
                // check if node `v` is an articulation point
                int parent_side_count = subtree_active_count_[nonempty_cluster] - subtree_active_count_[v];
                int n_nonempty_subgraph = 0;
                for (auto w : adj_[v]) {
                    if (rank_[v] < rank_[w] && parent_[w] == v) {
                        // `w` is a child of `v`
                        if (lowlink_[w] < rank_[v]) {
                            // `w` is not separated from `v`'s parent even after removal of `v`
                            parent_side_count += subtree_active_count_[w];
                        } else {
                            if (subtree_active_count_[w] > 0) ++n_nonempty_subgraph;
                        }
                    }
                }
                if (parent_side_count > 0) ++n_nonempty_subgraph;
                if (n_nonempty_subgraph >= 2) {
                    // `v` is an articulation point
                    if (!solver.enqueue(lits_[v], this)) return false;
                }
            }
        }
    }
    return true;
}

int ActiveVerticesConnected::buildTree(int v, int parent, int cluster_id) {
    rank_[v] = next_rank_++;
    cluster_id_[v] = cluster_id;
    parent_[v] = parent;
    int lowlink = rank_[v];
    int subtree_active_count = (state_[v] == kActive ? 1 : 0);

    for (int w : adj_[v]) {
        if (w == parent || state_[w] == kInactive) continue;
        if (rank_[w] == -1) {
            // unvisited
            lowlink = std::min(lowlink, buildTree(w, v, cluster_id));
            subtree_active_count += subtree_active_count_[w];
        } else {
            lowlink = std::min(lowlink, rank_[w]);
        }
    }

    subtree_active_count_[v] = subtree_active_count;
    return lowlink_[v] = lowlink;
}

void ActiveVerticesConnected::calcReason(Solver& solver, Lit p, vec<Lit>& out_reason) {
    // TODO: compute reason precisely
    // loadState(solver);

    int n = lits_.size();
    for (int i = 0; i < n; ++i) {
        if (var(lits_[i]) == var(p)) continue;
        if (state_[i] == kActive) out_reason.push(lits_[i]);
        else if (state_[i] == kInactive) out_reason.push(~lits_[i]);
    }
    for (int i = 0; i < n; ++i) {
        if (state_[i] == kActive && solver.value(lits_[i]) != l_True) abort();
        if (state_[i] == kInactive && solver.value(lits_[i]) != l_False) abort();
    }
}

void ActiveVerticesConnected::undo(Solver& solver, Lit p) {
    for (int i = 0; i < lits_.size(); ++i) {
        if (var(lits_[i]) == var(p)) {
            state_[i] = kUndecided;
        }
    }
}

void ActiveVerticesConnected::loadState(Solver& solver) {
    for (int i = 0; i < lits_.size(); ++i) {
        lbool val = solver.value(lits_[i]);
        if (val == l_Undef) state_[i] = kUndecided;
        else if (val == l_True) state_[i] = kActive;
        else state_[i] = kInactive;
    }
}

}
