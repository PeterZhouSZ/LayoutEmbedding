#pragma once

#include <LayoutEmbedding/Embedding.hh>

namespace LayoutEmbedding
{

/**
 * Perform loop subdivision on target mesh
 */
Embedding subdivide(
        const Embedding& _em_orig,
        const int _n_iters = 1);

/**
 * Returns a new Embedding instance, in which
 * embedded paths have been smoothed via straight
 * lines harmonic parametrizations of the two adjacent
 * patches, as described in [Praun2001].
 */
Embedding smooth_paths(
        const Embedding& _em_orig,
        const int _n_iters = 1,
        const bool _quad_flap_to_rectangle = true);

/**
 * Smooth only selected edges
 */
Embedding smooth_paths(
        const Embedding& _em_orig,
        const std::vector<pm::edge_handle>& _l_edges,
        const int _n_iters = 1,
        const bool _quad_flap_to_rectangle = true);

}
