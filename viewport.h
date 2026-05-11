#pragma once

#include "raylib.h"

typedef float ftype;

struct MyCamera
{
    Vector3 position;
    Vector3 lookAt;
    Vector3 up;
    Matrix worldToScreenSpace;
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
    Vector4 transform; // x, y, width, height
    Image z_buffer;
    Texture2D z_tex2d;
    Image color_buffer;
    Texture2D color_tex2d;
    ftype last_fov;
    ftype last_near_z;
    ftype last_far_z;
};