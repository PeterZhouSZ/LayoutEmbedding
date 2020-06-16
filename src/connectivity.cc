#include "connectivity.hh"

#include <cassert>

bool adjacent(const pm::vertex_handle& _v0, const pm::vertex_handle& _v1)
{
    assert(_v0.mesh == _v1.mesh);
    const auto he = pm::halfedge_from_to(_v0, _v1);
    return he.is_valid();
}

bool incident(const pm::vertex_handle& _v, const pm::edge_handle& _e)
{
    assert(_v.mesh == _e.mesh);
    return (_e.vertexA() == _v) || (_e.vertexB() == _v);
}

bool incident(const polymesh::edge_handle& _e, const polymesh::vertex_handle& _v)
{
    return incident(_v, _e);
}