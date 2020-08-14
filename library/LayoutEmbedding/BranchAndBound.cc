#include "BranchAndBound.hh"

#include <LayoutEmbedding/Assert.hh>
#include <LayoutEmbedding/Connectivity.hh>
#include <LayoutEmbedding/EmbeddingState.hh>

#include <chrono>
#include <queue>

namespace LayoutEmbedding {

struct Candidate
{
    double lower_bound = std::numeric_limits<double>::infinity();
    double priority = 0.0;
    InsertionSequence insertions;

    bool operator<(const Candidate& _rhs) const
    {
        return priority > _rhs.priority;
    }
};

void branch_and_bound(Embedding& _em, const BranchAndBoundSettings& _settings)
{
    Candidate best_solution; // Initially empty
    double global_upper_bound = std::numeric_limits<double>::infinity();
    // TODO: Run heuristic algorithm to find a tighter initial upper bound.

    // Init priority queue with empty state.
    std::priority_queue<Candidate> q;
    {
        Candidate c;
        c.lower_bound = 0.0;
        c.priority = 0.0;
        c.insertions = {};
        q.push(c);
    }

    // TODO: Use glow-extras timer instead?
    const auto start_time = std::chrono::steady_clock::now();

    while (!q.empty()) {
        // Time limit
        if (_settings.time_limit > 0.0) {
            const auto current_time = std::chrono::steady_clock::now();
            const auto interval = current_time - start_time;
            using seconds_double = std::chrono::duration<double>;
            const double elapsed_seconds = std::chrono::duration_cast<seconds_double>(interval).count();
            if (elapsed_seconds >= _settings.time_limit) {
                std::cout << "Reached time limit of " << _settings.time_limit << " s. Terminating." << std::endl;
                if (std::isinf(global_upper_bound)) {
                    std::cout << "Warning: No valid solution was found within that time." << std::endl;
                }
                break;
            }
        }

        auto c = q.top();
        q.pop();

        // Early-out based on lower bound cached in c.
        double gap = 1.0 - c.lower_bound / global_upper_bound;
        if (gap <= _settings.optimality_gap) {
            continue;
        }

        // Reconstruct the embedding associated with this embedding sequence
        EmbeddingState es(_em);
        es.extend(c.insertions);
        es.compute_candidate_paths();

        if (c.lower_bound > 0) {
            LE_ASSERT(es.cost_lower_bound() == c.lower_bound);
        }

        std::cout << "|Embd|: " << c.insertions.size();
        std::cout << "    ";
        std::cout << "|Conf|: " << es.conflicting_l_edges.size();
        std::cout << "    ";
        std::cout << "|Ncnf|: " << es.non_conflicting_l_edges.size();
        std::cout << "    ";
        std::cout << "LB: " << es.cost_lower_bound();
        std::cout << "    ";
        std::cout << "UB: " << global_upper_bound;
        std::cout << "    ";
        std::cout << "gap: " << (gap * 100.0) << " %";
        std::cout << "    ";
        std::cout << "|Q|: " << q.size();
        std::cout << std::endl;

        if (es.cost_lower_bound() < global_upper_bound) {
            // Completed layout?
            if (es.conflicting_l_edges.empty()) {
                global_upper_bound = es.cost_lower_bound();
                best_solution = c;
                std::cout << "New upper bound: " << global_upper_bound << std::endl;
            }
            else {
                // Add children to the queue
                for (const auto& l_e : es.conflicting_l_edges) {
                    EmbeddingState new_es(es); // Copy
                    new_es.extend(l_e);
                    new_es.compute_candidate_paths();

                    const double new_lower_bound = new_es.cost_lower_bound();
                    const double new_gap = 1.0 - new_lower_bound / global_upper_bound;
                    if (new_gap > _settings.optimality_gap) {
                        Candidate new_c;
                        new_c.insertions = c.insertions;
                        new_c.insertions.push_back(l_e);
                        new_c.lower_bound = new_lower_bound;
                        new_c.priority = new_c.lower_bound * new_es.conflicting_l_edges.size();
                        q.push(new_c);
                    }
                }
            }
        }
    }
    std::cout << "Branch-and-bound optimization completed." << std::endl;

    // Apply the victorious embedding sequence to the input embedding

    // Edges with predefined insertion sequence
    std::set<pm::edge_index> l_e_embedded;
    for (const auto& l_ei : best_solution.insertions) {
        const auto l_e = _em.layout_mesh().edges()[l_ei];
        const auto l_he = l_e.halfedgeA();
        const auto path = _em.find_shortest_path(l_he);
        _em.embed_path(l_he, path);
        l_e_embedded.insert(l_e);
    }
    // Remaining edges
    for (const auto& l_e : _em.layout_mesh().edges()) {
        if (!l_e_embedded.count(l_e)) {
            const auto l_he = l_e.halfedgeA();
            const auto path = _em.find_shortest_path(l_he);
            _em.embed_path(l_he, path);
            l_e_embedded.insert(l_e);
        }
    }
}

}
