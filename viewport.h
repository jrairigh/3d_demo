#pragma once
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtc/epsilon.hpp"

#include "raylib.h"

typedef float ftype;

struct MyCamera
{
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 up;
    glm::mat4 worldToScreenSpace;
    ftype near_plane;
    ftype far_plane;
    ftype fov;
    ftype aspect;
    ftype zoom_speed;
    ftype rotation_speed;
    bool is_orthographic;
};

struct Viewport
{
    MyCamera camera;
    glm::ivec4 transform; // x, y, width, height
    Image z_buffer;
    Texture2D z_tex2d;
    Image color_buffer;
    Texture2D color_tex2d;
    ftype last_fov;
    ftype last_near_z;
    ftype last_far_z;
};