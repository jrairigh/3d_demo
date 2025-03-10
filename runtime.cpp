#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui_enums.h"
#include "raygui.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtc/epsilon.hpp"

#include <string>
#include <time.h>

typedef float ftype;

struct Vertex
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec2 uv;
};

struct Cube
{
    glm::vec3 position;
    glm::vec3 rotation_axis;
    glm::vec3 scale;
    glm::vec4 color;
    float angular_speed;
};

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

const int g_screen_width = 800;
const int g_screen_height = 450;
MyCamera g_camera;
ftype g_since_start = 0.0f;
ftype g_frame_time = 0.0f;
Texture2D g_depth_tex2D;
Image g_sprite_atlas;
Image g_depth_map;
bool g_is_rending_depth_buffer = false;
bool g_draw_triangle_edges = false;
bool g_is_viewing_performance_metrics = false;
float g_bias = 0.0f;
float g_wall_x = 0;
float g_wall_y = 0;
int g_wall_column = 0;
int g_wall_row = 0;
glm::vec2 g_ui_zone{175, 175};
Mesh* g_raylib_mesh;
Mesh g_raylib_meshes[4];
int g_mesh_index;
int g_pixels_outside_screen = 0;
int g_pixels_behind_other_pixels = 0;
int g_backfacing_triangles = 0;

void InitializeRuntime();
void InitializeCamera();
void RunGame();
void CloseGame();
void Update();
void UpdateCamera(const ftype zoom, const glm::vec2 move);
void Render();
void RenderWorld();
void RenderUI();
void DrawPerformanceMetrics();
void DrawMesh();
void DrawMyWeirdCubes();
void DrawCube(const Cube& cube);
void DrawAxis(const glm::vec4 position);
void DrawLine3d(const glm::vec4 start, const glm::vec4 end, const glm::vec4 color);
void DrawColorPixel(const int x, const int y, const ftype z, const glm::vec4 color);
void DrawTextureSampledPixel(const int x, const int y, const ftype z, const glm::vec2 uv);
void DrawPixel(const int x, const int y, const ftype z, const glm::vec4 color);
void Rasterize3dLine(const glm::vec4 start, const glm::vec4 end, const glm::vec4 color);
void Draw3dTriangle(const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only);
void DrawTriangle(const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only);
void DrawQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d, const bool edges_only);
void LogMat4(const char* name, const glm::mat4& m4);
void Log(const char* format, ...);
ftype GetSmoothedMouseWheelScroll();
glm::vec2 GetSmoothedMouseMove();
glm::mat4 TRSMatrix(const glm::vec3 position,const glm::vec3 rotation_axis,const glm::vec3 scale, const float angle);
glm::mat4 ProjectionMatrix();
glm::mat4 OrthographicProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far);
glm::mat4 PerspectiveProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far);
glm::mat4 RotationMatrix(const glm::vec3 axis, const ftype angle);
glm::mat4 LookAt(const glm::vec3 position, const glm::vec3 look_at, const glm::vec3 up);
glm::mat4 Mat4(const glm::vec4 column1, const glm::vec4 column2, const glm::vec4 column3, const glm::vec4 column4);
bool IsTopLeftOfTriangle(const glm::vec2 from, const glm::vec2 to);

int main() 
{
    InitializeRuntime();
    RunGame();
    CloseGame();
}

void InitializeRuntime()
{
    InitWindow(g_screen_width, g_screen_height, "3D Demo");
    GuiLoadStyleDefault();

    SetTraceLogLevel(LOG_DEBUG);

    InitializeCamera();
    SetTargetFPS(60);

    g_depth_map = GenImageColor(g_screen_width, g_screen_height, BLACK);
    g_sprite_atlas = LoadImage("assets/WallpaperAtlas.png");

    g_raylib_meshes[0] = GenMeshCone(1.0f, 1.0f, 20);
    g_raylib_meshes[1] = GenMeshSphere(1.0f, 16, 16);
    g_raylib_meshes[2] = GenMeshCylinder(1.0f, 1.0f, 20);
    g_raylib_meshes[3] = GenMeshTorus(0.4f, 1.0f, 16, 32);
}

void InitializeCamera()
{
    const ftype aspect = (ftype)g_screen_width / (ftype)g_screen_height;
    const ftype near_plane = 5.0f;
    const ftype far_plane = 100.0f;
    const ftype fov = 20.0f;
    const ftype half_screen_width = g_screen_width * 0.5f;
    const ftype half_screen_height = g_screen_height * 0.5f;

    g_camera.position = glm::vec3(0.0f, 0.0f, 15.0f);
    g_camera.lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    g_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_camera.near_plane = near_plane;
    g_camera.far_plane = far_plane;
    g_camera.fov = fov;
    g_camera.aspect = aspect;
    g_camera.zoom_speed = 250.0f;
    g_camera.rotation_speed = 50.0f;
    g_camera.is_orthographic = false;

    g_camera.worldToCameraSpace = LookAt(g_camera.position, g_camera.lookAt, g_camera.up);
    LogMat4("World To Camera", g_camera.worldToCameraSpace);

    g_camera.projectionMatrix = ProjectionMatrix();
    LogMat4("Projection", g_camera.projectionMatrix);
    
    g_camera.clipToScreenSpace = glm::mat4(
        half_screen_width, 0.0f, 0.0f, 0.0f,
        0.0f, -half_screen_height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        half_screen_width, half_screen_height, 0.0f, 1.0f
    );
    LogMat4("Clip To Screen", g_camera.clipToScreenSpace);
}

void RunGame()
{
    while (!WindowShouldClose()) 
    {
        g_since_start = (ftype)GetTime();
        g_frame_time = GetFrameTime();
        Update();
        Render();
    }
}

void CloseGame()
{
    UnloadImage(g_depth_map);
    UnloadImage(g_sprite_atlas);

    if(IsTextureReady(g_depth_tex2D))
    {
        UnloadTexture(g_depth_tex2D);
    }

    UnloadMesh(g_raylib_meshes[0]);
    UnloadMesh(g_raylib_meshes[1]);
    UnloadMesh(g_raylib_meshes[2]);
    UnloadMesh(g_raylib_meshes[3]);

    CloseWindow();
}

void Update()
{
    const ftype zoom = GetSmoothedMouseWheelScroll();
    const glm::vec2 glm_mouse_delta = GetSmoothedMouseMove();

    const bool is_zkey_pressed = IsKeyPressed(KEY_Z);
    if(is_zkey_pressed && !g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = true;
        g_depth_tex2D = LoadTextureFromImage(g_depth_map);
    }
    else if(is_zkey_pressed && g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = false;
        UnloadTexture(g_depth_tex2D);
    }

    const int pressed = IsKeyPressed(KEY_TAB) ? 1 : 0;
    g_mesh_index = (g_mesh_index + pressed) % 5;
    g_raylib_mesh = &g_raylib_meshes[g_mesh_index];

    const bool is_wkey_pressed = IsKeyPressed(KEY_W);
    if(is_wkey_pressed & !g_draw_triangle_edges)
    {
        g_draw_triangle_edges = true;
    }
    else if(is_wkey_pressed && g_draw_triangle_edges)
    {
        g_draw_triangle_edges = false;
    }

    const bool is_skey_pressed = IsKeyPressed(KEY_S);
    if(is_skey_pressed && !g_is_viewing_performance_metrics)
    {
        g_is_viewing_performance_metrics = true;
    }
    else if(is_skey_pressed && g_is_viewing_performance_metrics)
    {
        g_is_viewing_performance_metrics = false;
    }

    UpdateCamera(zoom, glm_mouse_delta);
}

void UpdateCamera(const ftype zoom, const glm::vec2 move)
{
    static ftype last_fov = g_camera.fov;
    static ftype last_near = g_camera.near_plane;
    static ftype last_far = g_camera.far_plane;

    // Min fov at 20 for now so fps doesn't drop too much
    g_camera.fov += -zoom * g_camera.zoom_speed;
    g_camera.fov = glm::clamp<float>(g_camera.fov, 5, 180);
    
    const ftype length = glm::length(g_camera.position);
    const ftype rotation_speed = g_camera.rotation_speed * length;
    
    const glm::vec3 forward = glm::normalize(g_camera.lookAt - g_camera.position);
    const glm::vec3 right = glm::cross(forward, g_camera.up);
    const glm::vec3 move_delta = right * move.x + g_camera.up * move.y;
    g_camera.position += rotation_speed * g_frame_time * move_delta;
    g_camera.position = length * glm::normalize(g_camera.position);
    g_camera.up = glm::cross(right, forward);
    g_camera.worldToCameraSpace = LookAt(g_camera.position, g_camera.lookAt, g_camera.up);

    bool do_update_projection_matrix = false;
    do_update_projection_matrix = IsKeyPressed(KEY_SPACE);
    g_camera.is_orthographic = do_update_projection_matrix ? !g_camera.is_orthographic : g_camera.is_orthographic;

    do_update_projection_matrix = last_fov != g_camera.fov || do_update_projection_matrix;
    last_fov = do_update_projection_matrix ? g_camera.fov : last_fov;

    do_update_projection_matrix = last_near != g_camera.near_plane || do_update_projection_matrix;
    last_near = do_update_projection_matrix ? g_camera.near_plane : last_near;

    do_update_projection_matrix = last_far != g_camera.far_plane || do_update_projection_matrix;
    last_far = do_update_projection_matrix ? g_camera.far_plane : last_far;

    if(do_update_projection_matrix)
    {
        g_camera.projectionMatrix = ProjectionMatrix();
        LogMat4("Projection", g_camera.projectionMatrix);
    }
}

void Render()
{
    g_backfacing_triangles = 0;
    g_pixels_outside_screen = 0;
    g_pixels_behind_other_pixels = 0;
    BeginDrawing();
    ClearBackground(BLACK);
    RenderWorld();
    RenderUI();
    EndDrawing();
    Log("Pixels outside screen: %d", g_pixels_outside_screen);
    //Log("Pixels behind other pixels: %d", g_pixels_behind_other_pixels);
}

void RenderWorld()
{
    if(g_is_rending_depth_buffer)
    {
        DrawTexture(g_depth_tex2D, 0, 0, WHITE);
        return;
    }

    ImageClearBackground(&g_depth_map, WHITE);

    /*
    switch(g_mesh_index)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        DrawMesh();
        break;
        default:
        DrawMyWeirdCubes();
        break;
    }
    */

    DrawMyWeirdCubes();

    DrawAxis(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void RenderUI()
{
    std::string state = g_camera.is_orthographic ? "Orthographic" : "Perspective";
    state = g_is_rending_depth_buffer ? "Depth Buffer" : state;
    DrawText(state.c_str(), 10, 10, 20, YELLOW);

    GuiSlider({35, 30, 100, 20}, "Col", TextFormat("%d", g_wall_column), &g_wall_x, 0, 7);
    GuiSlider({35, 50, 100, 20}, "Row", TextFormat("%d", g_wall_row), &g_wall_y, 0, 14);
    g_wall_column = (int)glm::round(g_wall_x);
    g_wall_row = (int)glm::round(g_wall_y);
    
    GuiSlider({35, 70, 100, 20}, "Near", TextFormat("%0.1f", g_camera.near_plane), &g_camera.near_plane, 0.1f, 10.0f);
    GuiSlider({35, 90, 100, 20}, "Far", TextFormat("%0.1f", g_camera.far_plane), &g_camera.far_plane, 10.1f, 20.0f);
    DrawRectangleLines(0, 0, (int)g_ui_zone.x, (int)g_ui_zone.y, WHITE);

    if(g_is_viewing_performance_metrics)
    {
        DrawPerformanceMetrics();
    }
}

void DrawPerformanceMetrics()
{
    constexpr int font_size = 20;
    DrawRectangle(0, 0, (int)g_ui_zone.x, (int)g_ui_zone.y, Fade(BLACK, 0.8f));

    const int fps = GetFPS();
    DrawText(TextFormat("FPS: %d", fps), 10, 10, font_size, YELLOW);

    DrawText(TextFormat("Backfacing Triangles: %d", g_backfacing_triangles), 10, 30, font_size, YELLOW);
}

void DrawMesh()
{
    for(int i = 0; i < g_raylib_mesh->triangleCount; ++i)
    {
        if(g_raylib_mesh->indices)
        {

        }
        else
        {
            const int j = i * 9;
            const ftype x1 = (ftype)g_raylib_mesh->vertices[j];
            const ftype y1 = (ftype)g_raylib_mesh->vertices[j + 1];
            const ftype z1 = (ftype)g_raylib_mesh->vertices[j + 2];
            const ftype x2 = (ftype)g_raylib_mesh->vertices[j + 3];
            const ftype y2 = (ftype)g_raylib_mesh->vertices[j + 4];
            const ftype z2 = (ftype)g_raylib_mesh->vertices[j + 5];
            const ftype x3 = (ftype)g_raylib_mesh->vertices[j + 6];
            const ftype y3 = (ftype)g_raylib_mesh->vertices[j + 7];
            const ftype z3 = (ftype)g_raylib_mesh->vertices[j + 8];
            const Vertex a = {{x1, y1, z1, 1}, {1, 0, 0, 1}, {0, 0}};
            const Vertex b = {{x2, y2, z2, 1}, {0, 1, 0, 1}, {0, 0}};
            const Vertex c = {{x3, y3, z3, 1}, {0, 0, 1, 1}, {0, 0}};
            Draw3dTriangle(a, b, c, g_draw_triangle_edges);
        }
    }
}

void DrawMyWeirdCubes()
{
    const Cube cubes[7] = {
        {{0, 0, 0},{1,0,0},{1,1,1}, {1,1,1,1}, 0},
        {{1.5f, 0, 0},{1,0,0},{0.5f,0.5f,0.5f}, {1,0,0,1}, 1},
        {{0, 1.5f, 0},{0,1,0},{0.5f,0.5f,0.5f}, {0,1,0,1}, 1},
        {{0, 0, 1.5f},{0,0,1},{0.5f,0.5f,0.5f}, {0,0,1,1}, 1},
        {{-1.5f, 0, 0},{1,0,0},{0.5f,0.5f,0.5f}, {1,0,0,1}, 1},
        {{0, -1.5f, 0},{0,1,0},{0.5f,0.5f,0.5f}, {0,1,0,1}, 1},
        {{0, 0, -1.5f},{0,0,1},{0.5f,0.5f,0.5f}, {0,0,1,1}, 1},
    };
    
    DrawCube(cubes[0]);
    DrawCube(cubes[1]);
    DrawCube(cubes[2]);
    DrawCube(cubes[3]);
    DrawCube(cubes[4]);
    DrawCube(cubes[5]);
    DrawCube(cubes[6]);
}

void DrawCube(const Cube& cube)
{
    const glm::mat4 objectToWorldSpace = TRSMatrix(cube.position, cube.rotation_axis, cube.scale, cube.angular_speed * g_since_start);
    const ftype uv_top_left_x = 1.0f / g_sprite_atlas.width;
    const ftype uv_top_left_y = 16.0f / g_sprite_atlas.height;
    const ftype wall_width = (64.0f / g_sprite_atlas.width) - uv_top_left_x;
    const ftype wall_height = (79.0f / g_sprite_atlas.height) - uv_top_left_y;
    const ftype uv_xOffset = (65.0f / g_sprite_atlas.width) * g_wall_column + uv_top_left_x;
    const ftype uv_yOffset = (65.0f / g_sprite_atlas.height) * g_wall_row + uv_top_left_y;
    Vertex cube_vertices[8] = {
        // Position        Color          UV
        {{-1, -1, -1, 1}, {1, 0, 0, 1},   {uv_xOffset, uv_yOffset}},
        {{-1, 1, -1, 1},  {0, 1, 0, 1},   {uv_xOffset, uv_yOffset + wall_height}},
        {{1, 1, -1, 1},   {0, 0, 1, 1},   {uv_xOffset + wall_width, uv_yOffset + wall_height}},
        {{1, -1, -1, 1},  {1, 1, 0, 1},   {uv_xOffset + wall_width, uv_yOffset}},
        {{-1, -1, 1, 1},  {0, 0, 1, 1},   {uv_xOffset, uv_yOffset}},
        {{-1, 1, 1, 1},   {1, 1, 0, 1},   {uv_xOffset, uv_yOffset + wall_height}},
        {{1, 1, 1, 1},    {1, 0, 0, 1},   {uv_xOffset + wall_width, uv_yOffset + wall_height}},
        {{1, -1, 1, 1},   {0, 1, 0, 1},   {uv_xOffset + wall_width, uv_yOffset}}
    };

    cube_vertices[0].position = objectToWorldSpace * cube_vertices[0].position;
    cube_vertices[1].position = objectToWorldSpace * cube_vertices[1].position;
    cube_vertices[2].position = objectToWorldSpace * cube_vertices[2].position;
    cube_vertices[3].position = objectToWorldSpace * cube_vertices[3].position;
    cube_vertices[4].position = objectToWorldSpace * cube_vertices[4].position;
    cube_vertices[5].position = objectToWorldSpace * cube_vertices[5].position;
    cube_vertices[6].position = objectToWorldSpace * cube_vertices[6].position;
    cube_vertices[7].position = objectToWorldSpace * cube_vertices[7].position;

    // the veritices are given assuming the starting camera position in the positive z direction looking at the origin
    // the order is counter clockwise for quads camera is facing and clockwise for quads camera is not facing

    // back face
    DrawQuad(cube_vertices[0], cube_vertices[1], cube_vertices[2], cube_vertices[3], g_draw_triangle_edges);
    
    // front face
    DrawQuad(cube_vertices[7], cube_vertices[6], cube_vertices[5], cube_vertices[4], g_draw_triangle_edges);
    
    // top face
    DrawQuad(cube_vertices[1], cube_vertices[5], cube_vertices[6], cube_vertices[2], g_draw_triangle_edges);
    
    // bottom face
    DrawQuad(cube_vertices[0], cube_vertices[3], cube_vertices[7], cube_vertices[4], g_draw_triangle_edges);
    
    // left face
    DrawQuad(cube_vertices[0], cube_vertices[4], cube_vertices[5], cube_vertices[1], g_draw_triangle_edges);
    
    // right face
    DrawQuad(cube_vertices[2], cube_vertices[6], cube_vertices[7], cube_vertices[3], g_draw_triangle_edges);
}

void DrawAxis(const glm::vec4 position)
{
    const glm::vec4 x_axis = position + glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 y_axis = position + glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    const glm::vec4 z_axis = position + glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    DrawLine3d(position, x_axis, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    DrawLine3d(position, y_axis, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    DrawLine3d(position, z_axis, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

void DrawLine3d(const glm::vec4 start, const glm::vec4 end, const glm::vec4 color)
{
    const glm::mat4 objectToClipSpace = g_camera.clipToScreenSpace * g_camera.projectionMatrix * g_camera.worldToCameraSpace;

    glm::vec4 clippedStart = objectToClipSpace * start;
    clippedStart /= clippedStart.w;

    glm::vec4 clippedEnd = objectToClipSpace * end;
    clippedEnd /= clippedEnd.w;

    Rasterize3dLine(clippedStart, clippedEnd, color);
}

void DrawColorPixel(const int x, const int y, const ftype z, const glm::vec4 color)
{
    DrawPixel(x, y, z, color);
}

void DrawTextureSampledPixel(const int x, const int y, const ftype z, const glm::vec2 uv)
{
    glm::mat2 uv_matrix{
        g_sprite_atlas.width, 0,
        0, g_sprite_atlas.height
    };

    // affine texture mapping (creates the wobbly textures characteristic of PS1 games)
    const glm::ivec2 texcoords = uv_matrix * uv;
    const Vector4 color = ColorNormalize(GetImageColor(g_sprite_atlas, texcoords.x % g_sprite_atlas.width, texcoords.y % g_sprite_atlas.height));

    DrawPixel(x, y, z, {color.x, color.y, color.z, color.w});
}

void DrawPixel(const int x, const int y, const ftype z, const glm::vec4 color)
{
    const bool is_outside_z_bounds = z < -1 || z > 1;
    const bool is_outside_screen_bounds = x < 0 || x >= g_screen_width || y < 0 || y >= g_screen_height;
    if(is_outside_screen_bounds)
    {
        ++g_pixels_outside_screen;
        return;
    }

    const float depth = ColorNormalize(GetImageColor(g_depth_map, x, y)).z;
    if(depth < z)
    {
        ++g_pixels_behind_other_pixels;
        return;
    }

    ImageDrawPixel(&g_depth_map, x, y, ColorFromNormalized({z, z, z, 1.0f}));
    DrawPixel(x, y, ColorFromNormalized({color.r, color.g, color.b, color.a}));
}

void Rasterize3dLine(const glm::vec4 start, const glm::vec4 end, const glm::vec4 color)
{
    int x1 = (int)start.x;
    int y1 = (int)start.y;
    const int x2 = (int)end.x;
    const int y2 = (int)end.y;

    const Color raylib_color = ColorFromNormalized({color.r, color.g, color.b, color.a});
    DrawLine(x1, y1, x2, y2, raylib_color);
    return;
    int dx = glm::abs(x2 - x1);
    int dy = glm::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    while (true)
    {
        const Color depth = GetImageColor(g_depth_map, x1, y1);
        ImageDrawPixel(&g_depth_map, x1, y1, raylib_color);
        DrawPixel(x1, y1, raylib_color);
        if (x1 == x2 && y1 == y2)
            break;

        e2 = err;

        if (e2 > -dx)
        {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dy)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void Draw3dTriangle(const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only)
{
    const glm::vec4 a_cam = g_camera.worldToCameraSpace * a.position;
    const glm::vec4 b_cam = g_camera.worldToCameraSpace * b.position;
    const glm::vec4 c_cam = g_camera.worldToCameraSpace * c.position;

    // clip triangles facing away from camera
    const glm::vec3 a_to_b = b_cam - a_cam;
    const glm::vec3 a_to_c = c_cam - a_cam;
    const glm::vec3 normal = glm::cross(a_to_c, a_to_b);

    if(normal.z >= 0)
    {
        ++g_backfacing_triangles;
        return;
    }

    // clip triangles outside of frustrum
    //if(a_cam.z < g_camera.near_plane || b_cam.z < g_camera.near_plane || c_cam.z < g_camera.near_plane)
    //{
    //    return;
    //}
//
    //if(a_cam.z > g_camera.far_plane || b_cam.z > g_camera.far_plane || c_cam.z > g_camera.far_plane)
    //{
    //    return;
    //}

    const glm::mat4 cameraToScreenSpace = g_camera.clipToScreenSpace * g_camera.projectionMatrix;

    glm::vec4 a_screen = cameraToScreenSpace * a_cam;
    a_screen /= a_screen.w;

    glm::vec4 b_screen = cameraToScreenSpace * b_cam;
    b_screen /= b_screen.w;

    glm::vec4 c_screen = cameraToScreenSpace * c_cam;
    c_screen /= c_screen.w;

    const Vertex a1 = {a_screen, a.color, a.uv};
    const Vertex b1 = {b_screen, b.color, b.uv};
    const Vertex c1 = {c_screen, c.color, c.uv};
    DrawTriangle(a1, b1, c1, edges_only);
}

void DrawTriangle(const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only)
{
    auto min_x = (int)glm::round(glm::min(a.position.x, glm::min(b.position.x, c.position.x)));
    auto max_x = (int)glm::round(glm::max(a.position.x, glm::max(b.position.x, c.position.x)));
    auto min_y = (int)glm::round(glm::min(a.position.y, glm::min(b.position.y, c.position.y)));
    auto max_y = (int)glm::round(glm::max(a.position.y, glm::max(b.position.y, c.position.y)));
    min_x = glm::clamp<int>(min_x, 0, g_screen_width - 1);
    max_x = glm::clamp<int>(max_x, 0, g_screen_width - 1);
    min_y = glm::clamp<int>(min_y, 0, g_screen_height - 1);
    max_y = glm::clamp<int>(max_y, 0, g_screen_height - 1);
    const auto dim_x = max_x - min_x;
    const auto dim_y = max_y - min_y;
    const auto dim = dim_x * dim_y;

    // fix overlapping triangles using top-left edge rule
    // there's issue using floating point because the floating point numbers are not evenly spaced.
    // Solution is using fixed point numbers.
    const ftype bias_0 = IsTopLeftOfTriangle(a.position, b.position) ? 0.0f : -0.0001f;
    const ftype bias_1 = IsTopLeftOfTriangle(b.position, c.position) ? 0.0f : -0.0001f;
    const ftype bias_2 = IsTopLeftOfTriangle(c.position, a.position) ? 0.0f : -0.0001f;
    
    const glm::vec3 a_to_b = b.position - a.position;
    const glm::vec3 a_to_c = c.position - a.position;
    for(int i = 0; i < dim; ++i)
    {
        const int x = min_x + i % dim_x;
        const int y = min_y + i / dim_x;

        // this is actually parallelogram area, but the ratio is the same between triangles and parallelograms
        // when calculating the barycentric coordinates
        const ftype triangle_area = a_to_c.x * a_to_b.y - a_to_c.y * a_to_b.x;
        
        // calculate the barycentric coordinates
        const glm::vec2 a_to_p{x - a.position.x, y - a.position.y};
        const ftype alpha = (a_to_p.x * a_to_b.y - a_to_p.y * a_to_b.x) / triangle_area;
        const ftype beta = -(a_to_p.x * a_to_c.y - a_to_p.y * a_to_c.x) / triangle_area;
        const ftype gamma = 1 - alpha - beta;
        const bool is_outside_triangle = alpha + bias_0 < 0 || beta + bias_2 < 0 || gamma + bias_1 < 0;

        if(is_outside_triangle)
        {
            continue;
        }

        const ftype z = gamma * a.position.z + alpha * c.position.z + beta * b.position.z;
        const glm::vec4 color = gamma * a.color + alpha * c.color + beta * b.color;
        const glm::vec2 uv = gamma * a.uv + alpha * c.uv + beta * b.uv;

        const ftype epsilon = 0.01f;
        if(edges_only && (glm::epsilonEqual(alpha, 0.0f, epsilon) || glm::epsilonEqual(beta, 0.0f, epsilon) || glm::epsilonEqual(gamma, 0.0f, epsilon)))
        {
            const glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
            DrawColorPixel(x, y, z, white);
        }
        else
        {
            DrawTextureSampledPixel(x, y, z, uv);
        }
    }
}

void DrawQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d, const bool edges_only)
{
    Draw3dTriangle(a, b, c, edges_only);
    Draw3dTriangle(a, c, d, edges_only);
}

void LogMat4(const char* name, const glm::mat4& m4)
{
    Log("%s:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", name,
        m4[0][0], m4[1][0], m4[2][0], m4[3][0],
        m4[0][1], m4[1][1], m4[2][1], m4[3][1],
        m4[0][2], m4[1][2], m4[2][2], m4[3][2],
        m4[0][3], m4[1][3], m4[2][3], m4[3][3]
    );
}

void Log(const char* format, ...)
{
    char buffer[1024];
    strcpy_s(buffer, 1024, "%0.3f   ");

    va_list args;
    va_start(args, format);
    vsprintf_s(buffer + strlen(buffer), 1024 - strlen(buffer), format, args);
    va_end(args);

    TraceLog(LOG_DEBUG, buffer, g_since_start);
}

ftype GetSmoothedMouseWheelScroll()
{
    static ftype last_zoom = 0.0f;
    const ftype zoom = GetMouseWheelMove();
    const ftype avg_zoom = (zoom + last_zoom) * 0.5f;
    last_zoom = avg_zoom;
    return avg_zoom * g_frame_time;
}

glm::vec2 GetSmoothedMouseMove()
{
    const Vector2 mouse_delta = IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? GetMouseDelta() : Vector2{0.0f, 0.0f};
    const glm::vec2 glm_delta = {-mouse_delta.x, mouse_delta.y};
    const Vector2 position = GetMousePosition();
    return position.x < g_ui_zone.x && position.y < g_ui_zone.y ? glm::vec2{0.0f, 0.0f} : glm_delta;
}

glm::mat4 TRSMatrix(const glm::vec3 position, const glm::vec3 rotation_axis, const glm::vec3 scale, const float angle)
{
    return glm::translate(glm::mat4(1.0f), position) * RotationMatrix(rotation_axis, angle) * glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 ProjectionMatrix()
{
    return g_camera.is_orthographic 
        ? OrthographicProjectionMatrix(g_camera.fov, g_camera.aspect, g_camera.near_plane, g_camera.far_plane) 
        : PerspectiveProjectionMatrix(g_camera.fov, g_camera.aspect, g_camera.near_plane, g_camera.far_plane);
}

glm::mat4 OrthographicProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far)
{
    const ftype height = near * glm::tan(glm::radians(fov * 0.5f));
    const ftype width = aspect * height;
    const ftype depth = far - near;
    const glm::vec4 column1{1 / width, 0, 0, 0};
    const glm::vec4 column2{0, 1 / height, 0, 0};
    const glm::vec4 column3{0, 0, 1 / depth, 0};
    const glm::vec4 column4{0, 0, -0.5f * (far + near), 1};
    return Mat4(column1, column2, -column3, column4);
}

glm::mat4 PerspectiveProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far)
{
    const ftype depth = far - near;
    const ftype tan_fov = glm::tan(glm::radians(fov * 0.5f));
    const glm::vec4 column1{1 / (aspect * tan_fov), 0, 0, 0};
    const glm::vec4 column2{0, 1 / tan_fov, 0, 0};
    const glm::vec4 column3{0, 0, (far + near) / depth, 1};
    const glm::vec4 column4{0, 0, -2 * near * far / depth, 1};
    return Mat4(column1, column2, -column3, column4);
}

glm::mat4 RotationMatrix(const glm::vec3 axis, const ftype angle)
{
    /*
    v = (n * a)a
    w = n - v
    u = a x w
    n' = v + cos(t)w + sin(t)u
    */
   const glm::vec3 rotation_axis = glm::normalize(axis);
   const ftype cos_theta = glm::cos(angle);
   const ftype sin_theta = glm::sin(angle);
   glm::mat4 rot(1);

   {
       const glm::vec3 n = glm::vec3(1, 0, 0);
       const glm::vec3 v = glm::dot(rotation_axis, n) * rotation_axis;
       const glm::vec3 w = n - v;
       const glm::vec3 u = glm::cross(rotation_axis, w);
       rot[0][0] = v.x + cos_theta * w.x + sin_theta * u.x;
       rot[0][1] = v.y + cos_theta * w.y + sin_theta * u.y;
       rot[0][2] = v.z + cos_theta * w.z + sin_theta * u.z;
   }
   
   {
       const glm::vec3 n = glm::vec3(0, 1, 0);
       const glm::vec3 v = glm::dot(rotation_axis, n) * rotation_axis;
       const glm::vec3 w = n - v;
       const glm::vec3 u = glm::cross(rotation_axis, w);
       rot[1][0] = v.x + cos_theta * w.x + sin_theta * u.x;
       rot[1][1] = v.y + cos_theta * w.y + sin_theta * u.y;
       rot[1][2] = v.z + cos_theta * w.z + sin_theta * u.z;
   }
   
   {
       const glm::vec3 n = glm::vec3(0, 0, 1);
       const glm::vec3 v = glm::dot(rotation_axis, n) * rotation_axis;
       const glm::vec3 w = n - v;
       const glm::vec3 u = glm::cross(rotation_axis, w);
       rot[2][0] = v.x + cos_theta * w.x + sin_theta * u.x;
       rot[2][1] = v.y + cos_theta * w.y + sin_theta * u.y;
       rot[2][2] = v.z + cos_theta * w.z + sin_theta * u.z;
   }

   return rot;
}

glm::mat4 LookAt(const glm::vec3 position, const glm::vec3 look_at, const glm::vec3 up)
{
    const glm::vec3 lookat_direction = glm::normalize(look_at - position);
    const glm::vec3 right = glm::normalize(glm::cross(lookat_direction, up));
    const glm::vec3 up_direction = glm::cross(right, lookat_direction);
    const glm::vec4 column1{right.x, right.y, right.z, 0};
    const glm::vec4 column2{up_direction.x, up_direction.y, up_direction.z, 0};
    const glm::vec4 column3{lookat_direction.x, lookat_direction.y, lookat_direction.z, 0};
    const glm::vec4 column4{position.x, position.y, position.z, 1};

    return glm::inverse(Mat4(column1, column2, -column3, column4));
}

glm::mat4 Mat4(const glm::vec4 column1, const glm::vec4 column2, const glm::vec4 column3, const glm::vec4 column4)
{
    const glm::vec4& c1 = column1;
    const glm::vec4& c2 = column2;
    const glm::vec4& c3 = column3;
    const glm::vec4& c4 = column4;
    return glm::mat4{
        //       row1  row2  row3  row4
        /*Col 1*/c1.x, c1.y, c1.z, c1.w,
        /*Col 2*/c2.x, c2.y, c2.z, c2.w,
        /*Col 3*/c3.x, c3.y, c3.z, c3.w,
        /*Col 4*/c4.x, c4.y, c4.z, c4.w
    };
}

bool IsTopLeftOfTriangle(const glm::vec2 from, const glm::vec2 to)
{
    const glm::vec2 a_to_b = to - from;
    const bool is_flat_edge = a_to_b.y == 0 && a_to_b.x < 0;
    const bool is_left_edge = a_to_b.y > 0;
    return is_flat_edge || is_left_edge;
}