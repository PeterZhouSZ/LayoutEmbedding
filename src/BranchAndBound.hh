#pragma once

#include <Embedding.hh>

struct BranchAndBoundSettings
{
    double optimality_gap = 0.03;
};

void branch_and_bound(Embedding& _em, const BranchAndBoundSettings& _settings = BranchAndBoundSettings());
