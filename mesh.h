#pragma once

#include "log.h"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>

class MyMesh;
MyMesh ParseObjFile(const std::filesystem::path& path);

void GetMeshTriangle(const MyMesh& mesh, const int triangle_index, float* vertices, float* uvs, float* normals);

class MyMesh
{
public:
    MyMesh() = default;

    MyMesh(const int vertex_count, const int normal_count, const int uv_count, const int triangle_count)
        : m_vertices(new float[vertex_count * 3]),
          m_normals(new float[normal_count * 3]),
          m_uvs(new float[uv_count * 2]),
          m_vertex_indices(new uint32_t[triangle_count * 3]),
          m_uv_indices(new uint32_t[triangle_count * 3]),
          m_normal_indices(new uint32_t[triangle_count * 3]),
          m_vertex_count(vertex_count),
          m_normal_count(normal_count),
          m_uv_count(uv_count),
          m_triangle_count(triangle_count)
    {
    }

    MyMesh(MyMesh&& other)
    {
        *this = std::move(other);
    }

    ~MyMesh()
    {
        FreeMemory();
    }

    MyMesh& operator=(MyMesh&& other)
    {
        if(this != &other)
        {
            FreeMemory();
            m_vertices = other.m_vertices;
            m_normals = other.m_normals;
            m_uvs = other.m_uvs;
            m_vertex_indices = other.m_vertex_indices;
            m_normal_indices = other.m_normal_indices;
            m_uv_indices = other.m_uv_indices;
            m_vertex_count = other.m_vertex_count;
            m_normal_count = other.m_normal_count;
            m_uv_count = other.m_uv_count;
            m_triangle_count = other.m_triangle_count;

            other.m_vertices = nullptr;
            other.m_normals = nullptr;
            other.m_uvs = nullptr;
            other.m_vertex_indices = nullptr;
            other.m_normal_indices = nullptr;
            other.m_uv_indices = nullptr;
            other.m_vertex_count = 0;
            other.m_normal_count = 0;
            other.m_uv_count = 0;
            other.m_triangle_count = 0;
        }

        return *this;
    }

    int vertex_count() const { return m_vertex_count; }
    int normal_count() const { return m_normal_count; }
    int uv_count() const { return m_uv_count; }
    int triangle_count() const { return m_triangle_count; }

private:
    void FreeMemory()
    {
        if(m_vertices)
            delete[] m_vertices;
        if(m_normals)
            delete[] m_normals;
        if(m_uvs)
            delete[] m_uvs;
        if(m_vertex_indices)
            delete[] m_vertex_indices;
        if(m_uv_indices)
            delete[] m_uv_indices;
        if(m_normal_indices)
            delete[] m_normal_indices;

        m_vertices = nullptr;
        m_normals = nullptr;
        m_uvs = nullptr;
        m_vertex_indices = nullptr;
        m_uv_indices = nullptr;
        m_vertex_count = 0;
        m_normal_count = 0;
        m_uv_count = 0;
        m_triangle_count = 0;
    }

    float* m_vertices = nullptr;
    float* m_normals = nullptr;
    float* m_uvs = nullptr;
    uint32_t* m_vertex_indices = nullptr;
    uint32_t* m_uv_indices = nullptr;
    uint32_t* m_normal_indices = nullptr;
    int m_vertex_count = 0;
    int m_normal_count = 0;
    int m_uv_count = 0;
    int m_triangle_count = 0;

    friend MyMesh ParseObjFile(const std::filesystem::path& path);
    friend void GetMeshTriangle(const MyMesh& mesh, const int triangle_index, float* vertices, float* uvs, float* normals);
};

MyMesh ParseObjFile(const std::filesystem::path& path)
{
    std::fstream in{path, std::ios::in};
    std::string line;

    int vertex_count = 0;
    int normal_count = 0;
    int uv_count = 0;
    int triangle_count = 0;

    while(std::getline(in, line))
    {
        if(line.at(0) == 'v' && line.at(1) == 'n')
        {
            ++normal_count;
        }
        else if(line.at(0) == 'v' && line.at(1) == 't')
        {
            ++uv_count;
        }
        else if(line.at(0) == 'v')
        {
            ++vertex_count;
        }
        else if(line.at(0) == 'f')
        {
            ++triangle_count;
        }
        else if(line.at(0) == 's')
        {
            Log("Shading type %s", line.substr(2).c_str());
        }
        else if(line.at(0) == 'o')
        {
            Log("Parsing object %s", line.substr(2).c_str());
        }
    }

    in.clear();
    in.seekg(0, std::ios::beg);

    MyMesh mesh{vertex_count, normal_count, uv_count, triangle_count};

    Log("Mesh Details:\n"
        "Vertices: %d\n"
        "Normals: %d\n"
        "UVs: %d\n"
        "Triangles: %d\n",
        vertex_count, normal_count, uv_count, triangle_count);

    vertex_count = 0;
    normal_count = 0;
    uv_count = 0;
    triangle_count = 0;

    while(std::getline(in, line))
    {
        float x, y, z;
        std::stringstream ss;
        ss << line;
        if(line.at(0) == 'v' && line.at(1) == 'n')
        {
            // parse normal
            ss.ignore(2);
            ss >> x >> y >> z;
            mesh.m_normals[normal_count * 3 + 0] = x;
            mesh.m_normals[normal_count * 3 + 1] = y;
            mesh.m_normals[normal_count * 3 + 2] = z;
            ++normal_count;
        }
        else if(line.at(0) == 'v' && line.at(1) == 't')
        {
            // parse uv
            ss.ignore(2);
            ss >> x >> y;
            mesh.m_uvs[uv_count * 2 + 0] = x;
            mesh.m_uvs[uv_count * 2 + 1] = y;
            ++uv_count;
        }
        else if(line.at(0) == 'v')
        {
            // parse vertex
            ss.ignore(1);
            ss >> x >> y >> z;
            mesh.m_vertices[vertex_count * 3 + 0] = x;
            mesh.m_vertices[vertex_count * 3 + 1] = y;
            mesh.m_vertices[vertex_count * 3 + 2] = z;
            ++vertex_count;
        }
        else if(line.at(0) == 'f')
        {
            // parse triangle
            ss.ignore(1);
            int vertex_index;
            int uv_index;
            int normal_index;
            std::string group1, group2, group3;
            ss >> group1 >> group2 >> group3;

            std::string_view groups[] = {group1, group2, group3};
            for(int i = 0; i < 3; ++i)
            {
                std::stringstream ss1;
                ss1 << groups[i];
                ss1 >> vertex_index;
                ss1.ignore(1); // ignore the '/' character
                ss1 >> uv_index;
                ss1.ignore(1); // ignore the '/' character
                ss1 >> normal_index;
                
                --vertex_index; --uv_index; --normal_index; // OBJ format is 1-indexed
                mesh.m_vertex_indices[triangle_count * 3 + i] = vertex_index;
                mesh.m_uv_indices[triangle_count * 3 + i] = uv_index;
                mesh.m_normal_indices[triangle_count * 3 + i] = normal_index;
            }

            ++triangle_count;
        }
    }

    return mesh;
}

void GetMeshTriangle(const MyMesh& mesh, const int triangle_index, float* vertices, float* uvs, float* normals)
{
    const uint32_t v1 = mesh.m_vertex_indices[triangle_index * 3 + 0];
    const uint32_t v2 = mesh.m_vertex_indices[triangle_index * 3 + 1];
    const uint32_t v3 = mesh.m_vertex_indices[triangle_index * 3 + 2];
    const uint32_t n1 = mesh.m_normal_indices[triangle_index * 3 + 0];
    const uint32_t n2 = mesh.m_normal_indices[triangle_index * 3 + 1];
    const uint32_t n3 = mesh.m_normal_indices[triangle_index * 3 + 2];
    const uint32_t uv1 = mesh.m_uv_indices[triangle_index * 3 + 0];
    const uint32_t uv2 = mesh.m_uv_indices[triangle_index * 3 + 1];
    const uint32_t uv3 = mesh.m_uv_indices[triangle_index * 3 + 2];

    // Vertex 1
    vertices[0] = mesh.m_vertices[v1 * 3 + 0]; // x
    vertices[1] = mesh.m_vertices[v1 * 3 + 1]; // y
    vertices[2] = mesh.m_vertices[v1 * 3 + 2]; // z
    normals[0] = mesh.m_normals[n1 * 3 + 0];   // normal x
    normals[1] = mesh.m_normals[n1 * 3 + 1];   // normal y
    normals[2] = mesh.m_normals[n1 * 3 + 2];   // normal z
    uvs[0] = mesh.m_uvs[uv1 * 2 + 0];          // texcoord x
    uvs[1] = mesh.m_uvs[uv1 * 2 + 1];          // texcoord y
    
    // Vertex 2
    vertices[3] = mesh.m_vertices[v2 * 3 + 0];
    vertices[4] = mesh.m_vertices[v2 * 3 + 1];
    vertices[5] = mesh.m_vertices[v2 * 3 + 2];
    normals[3] = mesh.m_normals[n1 * 3 + 0];
    normals[4] = mesh.m_normals[n1 * 3 + 1];
    normals[5] = mesh.m_normals[n1 * 3 + 2];
    uvs[2] = mesh.m_uvs[uv2 * 2 + 0];
    uvs[3] = mesh.m_uvs[uv2 * 2 + 1];
    
    // Vertex 3
    vertices[6] = mesh.m_vertices[v3 * 3 + 0];
    vertices[7] = mesh.m_vertices[v3 * 3 + 1];
    vertices[8] = mesh.m_vertices[v3 * 3 + 2];
    normals[6] = mesh.m_normals[n1 * 3 + 0];
    normals[7] = mesh.m_normals[n1 * 3 + 1];
    normals[8] = mesh.m_normals[n1 * 3 + 2];
    uvs[4] = mesh.m_uvs[uv3 * 2 + 0];
    uvs[5] = mesh.m_uvs[uv3 * 2 + 1];
}