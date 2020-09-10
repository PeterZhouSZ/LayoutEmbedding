#pragma once

#include <LayoutEmbedding/Embedding.hh>
#include <LayoutEmbedding/Visualization/RWTHColors.hh>

#include <glow-extras/glfw/GlfwContext.hh>
#include <glow-extras/viewer/view.hh>

namespace LayoutEmbedding {

/// Layout and Embedding side-by-side in a grid view
void view_embedding(const Embedding& _em);

/// Layout mesh with colorized embedded edges
void view_layout(const Embedding& _em, const bool patch_colors = true);

/// Target mesh with colorized embedded edge paths
void view_target(const Embedding& _em, const bool patch_colors = true);
void view_vertices_and_paths(const Embedding& _em);

pm::vertex_attribute<tg::pos3> make_layout_mesh_positions(const Embedding& _em);

void view_layout_mesh(const Embedding& _em);
void view_layout_mesh(const Embedding& _em, const pm::face_attribute<tg::color3>& _l_f_color);

void view_target_mesh(const Embedding& _em);
void view_target_mesh(const Embedding& _em, const pm::face_attribute<tg::color3>& _t_f_color);

void view_path(const Embedding& _em, const std::vector<pm::vertex_handle>& _path, const tg::color3& _color);
void view_path(const Embedding& _em, const VirtualPath& _path, const tg::color3& _color);
void view_path(const Embedding& _em, const Snake& _snake, const tg::color3& _color);

void view_edge(const pm::vertex_attribute<tg::pos3>& _pos, const pm::edge_handle& _e, const tg::color3& _color);
void view_vertex(const pm::vertex_attribute<tg::pos3>& _pos, const pm::vertex_handle& _v, const tg::color3& _color);
void view_vertex(const pm::vertex_attribute<tg::dpos3>& _pos, const pm::vertex_handle& _v, const tg::color3& _color);

void view_param(const pm::vertex_attribute<tg::dpos2> _param);

glow::SharedTexture2D read_texture(const fs::path &_file_path);

inline auto default_style()
{
    // Implemented inline so we can use 'auto' because the returned type is an implementation detail.
    return gv::config(gv::no_grid, gv::no_outline, gv::background_color(RWTH_WHITE), gv::ssao_power(0.5f));
}

}
