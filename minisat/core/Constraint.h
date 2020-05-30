#pragma once

#include "minisat/mtl/Vec.h"
#include "minisat/core/SolverTypes.h"

namespace Minisat {

class Solver;

class Constraint {
public:
    virtual ~Constraint() {}

    virtual void getWatchers(Solver& solver, vec<Lit>& out_watchers) = 0;
    virtual bool propagate(Solver& solver, Lit p) = 0;
    virtual void calcReason(Solver& solver, Lit p, vec<Lit>& out_reason) = 0;
    virtual void undo(Solver& solver, Lit p) {}
};

}
