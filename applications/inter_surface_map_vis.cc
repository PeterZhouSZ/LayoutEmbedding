#include <LayoutEmbedding/Visualization/Visualization.hh>
#include <omp.h>

using namespace LayoutEmbedding;
namespace fs = std::filesystem;

namespace
{

pm::halfedge_attribute<tg::pos2> projected_tex_coords(
        const pm::vertex_attribute<tg::pos3>& _pos,
        const tg::vec3 _right,
        const tg::vec3 _up)
{
    auto uvs = _pos.mesh().halfedges().make_attribute<tg::pos2>();
    for (auto h : _pos.mesh().halfedges())
        uvs[h] = tg::pos2(tg::dot(_pos[h.vertex_to()], _right), tg::dot(_pos[h.vertex_to()], _up));

    return uvs;
}

pm::face_attribute<bool> classify_front_back(
        const pm::vertex_attribute<tg::pos3>& _pos,
        const tg::vec3& _view_dir)
{
    // Compute field with 1 for front-facing and 0 for back-facing
    auto s = _pos.mesh().faces().make_attribute<double>();
    for (auto f : _pos.mesh().faces())
    {
        const auto n = pm::face_normal(f, _pos);
        s[f] = (tg::dot(n, _view_dir) <= 0) ? 1.0 : 0.0;
    }

    // Smooth field
    const int n_iters = 1000;
    for (int iter = 0; iter < n_iters; ++iter)
    {
        const auto s_copy = s;
        #pragma omp parallel for
        for (int i = 0; i < _pos.mesh().faces().size(); ++i)
        {
            const auto fi = _pos.mesh().faces()[i];
            double sum = 0.0;
            for (auto fj : fi.adjacent_faces())
                sum += s_copy[fj];
            s[fi] = sum / 3;
        }
    }

    // Turn into binary decision
    return s.map([] (auto val) { return val >= 0.5; });
}

void renderables_front_back(
        const pm::vertex_attribute<tg::pos3>& _pos,
        const pm::vertex_attribute<tg::pos3>& _pos_test, // Mesh to perform the orientation check on. Needs same connectivity.
        const pm::halfedge_attribute<tg::pos2>& _uvs,
        const glow::SharedTexture2D& _texture_front,
        const glow::SharedTexture2D& _texture_back,
        gv::SharedGeometricRenderable& _r_front,
        gv::SharedGeometricRenderable& _r_back,
        const tg::vec3& _view_dir = tg::vec3(1, 0, 0))
{
    pm::Mesh m_front;
    pm::Mesh m_back;
    auto pos_front = m_front.vertices().make_attribute<tg::pos3>();
    auto pos_back = m_back.vertices().make_attribute<tg::pos3>();
    auto uvs_front = m_front.halfedges().make_attribute<tg::pos2>();
    auto uvs_back = m_back.halfedges().make_attribute<tg::pos2>();
    auto v_to_front = _pos.mesh().vertices().make_attribute<pm::vertex_handle>();
    auto v_to_back = _pos.mesh().vertices().make_attribute<pm::vertex_handle>();

    auto vertex = [&_pos] (auto v_orig, auto& m, auto& pos, auto& v_map)
    {
        if (!v_map[v_orig].is_valid())
        {
            v_map[v_orig] = m.vertices().add();
            pos[v_map[v_orig]] = _pos[v_orig];
        }
        return v_map[v_orig];
    };

    auto add_face = [&] (auto f_orig, auto& m, auto& pos, auto& uv, auto& v_map)
    {
        // Add face
        std::vector<pm::vertex_handle> vs;
        for (auto v_orig : f_orig.vertices())
            vs.push_back(vertex(v_orig, m, pos, v_map));
        LE_ASSERT(vs.size() == 3);
        m.faces().add(vs);

        // Assign uvs
        for (auto h_orig : f_orig.halfedges())
        {
            const auto h_new = pm::halfedge_from_to(v_map[h_orig.vertex_from()], v_map[h_orig.vertex_to()]);
            uv[h_new] = _uvs[h_orig];
        }
    };

    const auto front_facing = classify_front_back(_pos_test, _view_dir);
    for (auto f_orig : _pos.mesh().faces())
    {
        if (front_facing[_pos_test.mesh().faces()[f_orig.idx]])
            add_face(f_orig, m_front, pos_front, uvs_front, v_to_front);
        else
            add_face(f_orig, m_back, pos_back, uvs_back, v_to_back);
    }

    _r_front = gv::make_renderable(pos_front);
    _r_back = gv::make_renderable(pos_back);

    // Flip texture due to different conventions
    configure(*_r_front, gv::textured(uvs_front, _texture_front).flip());
//    configure(*_r_back, gv::textured(uvs_back, _texture_back).flip());
    configure(*_r_back, tg::color3::white);
}

void show_ism(
        const fs::path& path_A,
        const fs::path& path_B)
{
    // Load overlay mesh with vertex positions on A and B
    pm::Mesh overlay_A;
    pm::Mesh overlay_B;
    auto pos_A = overlay_A.vertices().make_attribute<tg::pos3>();
    auto pos_B = overlay_B.vertices().make_attribute<tg::pos3>();
    pm::load(path_A, overlay_A, pos_A);
    pm::load(path_B, overlay_B, pos_B);

    const auto uvs_A = projected_tex_coords(pos_A, tg::vec3(0, 0, 2), tg::vec3(0, 2, 0));
    const auto texture_front = glow::Texture2D::createFromFile(fs::path(LE_DATA_PATH) / "textures/checkerboard.png", glow::ColorSpace::sRGB);
    const auto texture_back = glow::Texture2D::createFromFile(fs::path(LE_DATA_PATH) / "textures/checkerboard_white.png", glow::ColorSpace::sRGB);

    // Transfer uvs
    auto uvs_B = overlay_B.halfedges().make_attribute<tg::pos2>();
    for (auto h_B : overlay_B.halfedges())
    {
        const auto h_A = pm::halfedge_from_to(overlay_A.vertices()[h_B.vertex_from().idx],
                                              overlay_A.vertices()[h_B.vertex_to().idx]);
        uvs_B[h_B] = uvs_A[h_A];
    }

    {
        auto g = gv::grid();
        auto style = default_style();
        {
            auto v = gv::view();
            gv::SharedGeometricRenderable r1, r2;
            renderables_front_back(pos_A, pos_A, uvs_A, texture_front, texture_back, r1, r2);
            gv::view(r1);
            gv::view(r2);
        }
        {
            auto v = gv::view();
            gv::SharedGeometricRenderable r1, r2;
            renderables_front_back(pos_B, pos_A, uvs_B, texture_front, texture_back, r1, r2);
            gv::view(r1);
            gv::view(r2);
        }
    }
}

}

int main()
{
    register_segfault_handler();
    glow::glfw::GlfwContext ctx;

    show_ism(fs::path(LE_OUTPUT_PATH) / "inter_surface_map" / "greedy" / "AonB_overlay.obj",
             fs::path(LE_OUTPUT_PATH) / "inter_surface_map" / "greedy" / "BonA_overlay.obj");

//    show_ism(fs::path(LE_OUTPUT_PATH) / "inter_surface_map" / "bnb" / "AonB_overlay.obj",
//             fs::path(LE_OUTPUT_PATH) / "inter_surface_map" / "bnb" / "BonA_overlay.obj");
}
