#pragma once

#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>

class MyMesh;
MyMesh ParseObjFile(const std::filesystem::path& path);

class MyMesh
{
public:
    MyMesh() :
        vertices(nullptr),
        normals(nullptr),
        uvs(nullptr),
        indices(nullptr)
    {
    }

    MyMesh(const int vertex_count, const int normal_count, const int uv_count, const int triangle_count)
        : vertices(new float[vertex_count * 3]),
          normals(new float[normal_count * 3]),
          uvs(new float[uv_count * 2]),
          indices(new unsigned int[triangle_count * 3]),
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
        delete[] vertices;
        delete[] normals;
        delete[] uvs;
        delete[] indices;
    }

    MyMesh& operator=(MyMesh&& other)
    {
        if(this != &other)
        {
            if(vertices)
                delete[] vertices;
            if(normals)
                delete[] normals;
            if(uvs)
                delete[] uvs;
            if(indices)
                delete[] indices;

            vertices = other.vertices;
            normals = other.normals;
            uvs = other.uvs;
            indices = other.indices;
            m_vertex_count = other.m_vertex_count;
            m_normal_count = other.m_normal_count;
            m_uv_count = other.m_uv_count;
            m_triangle_count = other.m_triangle_count;

            other.vertices = nullptr;
            other.normals = nullptr;
            other.uvs = nullptr;
            other.indices = nullptr;
        }

        return *this;
    }

    int vertex_count() const { return m_vertex_count; }
    int normal_count() const { return m_normal_count; }
    int uv_count() const { return m_uv_count; }
    int triangle_count() const { return m_triangle_count; }
    float* get_vertices() const { return vertices; }
    float* get_normals() const { return normals; }
    float* get_uvs() const { return uvs; }
    unsigned int* get_indices() const { return indices; }

private:
    float* vertices = nullptr;
    float* normals = nullptr;
    float* uvs = nullptr;
    unsigned int* indices = nullptr;
    int m_vertex_count = 0;
    int m_normal_count = 0;
    int m_uv_count = 0;
    int m_triangle_count = 0;

    friend MyMesh ParseObjFile(const std::filesystem::path& path);
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
    }

    in.clear();
    in.seekg(0, std::ios::beg);
    in.seekp(0, std::ios::beg);

    MyMesh mesh{vertex_count, normal_count, uv_count, triangle_count};
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
            mesh.normals[normal_count * 3 + 0] = x;
            mesh.normals[normal_count * 3 + 1] = y;
            mesh.normals[normal_count * 3 + 2] = z;
            ++normal_count;
        }
        else if(line.at(0) == 'v' && line.at(1) == 't')
        {
            // parse uv
            ss.ignore(2);
            ss >> x >> y;
            mesh.uvs[uv_count * 2 + 0] = x;
            mesh.uvs[uv_count * 2 + 1] = y;
            ++uv_count;
        }
        else if(line.at(0) == 'v')
        {
            // parse vertex
            ss.ignore(1);
            ss >> x >> y >> z;
            mesh.vertices[vertex_count * 3 + 0] = x;
            mesh.vertices[vertex_count * 3 + 1] = y;
            mesh.vertices[vertex_count * 3 + 2] = z;
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

            std::string groups[] = {group1, group2, group3};
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
                mesh.indices[triangle_count * 3 + i] = vertex_index;
            }

            ++triangle_count;
        }
    }

    return mesh;
}