#pragma once
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtc/epsilon.hpp"

#include "raylib.h"
#include "raymath.h"

typedef float ftype;

struct MyCamera
{
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 up;
    glm::mat4 worldToCameraSpace;
    glm::mat4 projectionMatrix;
    glm::mat4 clipToScreenSpace;
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
    Image depth_map;
};