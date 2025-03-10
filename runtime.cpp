#include "viewport.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui_enums.h"
#include "raygui.h"

#include <string>
#include <time.h>

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

Viewport g_main_viewport;
ftype g_since_start = 0.0f;
ftype g_frame_time = 0.0f;
Texture2D g_depth_tex2D;
Image g_sprite_atlas;
bool g_is_rending_depth_buffer = false;
bool g_draw_triangle_edges = false;
bool g_is_viewing_performance_metrics = false;
float g_bias = 0.0f;
float g_wall_x = 0;
float g_wall_y = 0;
int g_wall_column = 0;
int g_wall_row = 0;
glm::vec2 g_ui_zone{175, 175};
int g_pixels_outside_screen = 0;
int g_pixels_behind_other_pixels = 0;
int g_backfacing_triangles = 0;

void InitializeRuntime();
void InitializeCamera(Viewport& viewport, const glm::ivec4& transform);
void RunGame();
void CloseGame();
void Update();
void UpdateCamera(Viewport& viewport, const ftype zoom, const glm::vec2 move);
void UpdateViewport(Viewport& viewport);
void Render();
void RenderWorld(Viewport& viewport);
void RenderUI();
void DrawPerformanceMetrics();
void DrawMyWeirdCubes(Viewport& viewport);
void DrawCube(Viewport& viewport, const Cube& cube);
void DrawAxis(const Viewport& viewport, const glm::vec4 position);
void DrawLine3d(const Viewport& viewport, const glm::vec4 start, const glm::vec4 end, const glm::vec4 color);
void DrawColorPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color);
void DrawTextureSampledPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec2 uv);
void DrawPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color);
void Draw3dTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only);
void DrawTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only);
void DrawQuad(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d, const bool edges_only);
void LogMat4(const char* name, const glm::mat4& m4);
void Log(const char* format, ...);
ftype GetSmoothedMouseWheelScroll();
glm::vec2 GetSmoothedMouseMove();
glm::mat4 TRSMatrix(const glm::vec3 position,const glm::vec3 rotation_axis,const glm::vec3 scale, const float angle);
glm::mat4 ProjectionMatrix(const Viewport& viewport);
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
    const int screen_width = 800;
    const int screen_height = 450;
    InitWindow(screen_width, screen_height, "3D Demo");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    GuiLoadStyleDefault();

    SetTraceLogLevel(LOG_DEBUG);

    InitializeCamera(g_main_viewport, {0, 0, screen_width, screen_height});
    SetTargetFPS(60);

    g_sprite_atlas = LoadImage("assets/WallpaperAtlas.png");
}

void InitializeCamera(Viewport& viewport, const glm::ivec4& transform)
{
    const ftype near_plane = 5.0f;
    const ftype far_plane = 100.0f;
    const ftype fov = 20.0f;
    
    MyCamera& camera = viewport.camera;
    camera.position = glm::vec3(0.0f, 0.0f, 15.0f);
    camera.lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.near_plane = near_plane;
    camera.far_plane = far_plane;
    camera.fov = fov;
    camera.zoom_speed = 250.0f;
    camera.rotation_speed = 50.0f;
    camera.is_orthographic = false;
    
    viewport.transform = transform;
    UpdateViewport(viewport);
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
    if(IsImageReady(g_main_viewport.depth_map))
    {
        UnloadImage(g_main_viewport.depth_map);
    }

    if(IsImageReady(g_sprite_atlas))
    {
        UnloadImage(g_sprite_atlas);
    }

    if(IsTextureReady(g_depth_tex2D))
    {
        UnloadTexture(g_depth_tex2D);
    }

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
        g_depth_tex2D = LoadTextureFromImage(g_main_viewport.depth_map);
    }
    else if(is_zkey_pressed && g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = false;
        UnloadTexture(g_depth_tex2D);
    }

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

    UpdateCamera(g_main_viewport, zoom, glm_mouse_delta);
}

void UpdateCamera(Viewport& viewport, const ftype zoom, const glm::vec2 move)
{
    MyCamera& camera = viewport.camera;
    static ftype last_fov = camera.fov;
    static ftype last_near = camera.near_plane;
    static ftype last_far = camera.far_plane;

    // Min fov at 20 for now so fps doesn't drop too much
    camera.fov += -zoom * camera.zoom_speed;
    camera.fov = glm::clamp<float>(camera.fov, 5, 180);
    
    const ftype length = glm::length(camera.position);
    const ftype rotation_speed = camera.rotation_speed * length;
    
    const glm::vec3 forward = glm::normalize(camera.lookAt - camera.position);
    const glm::vec3 right = glm::cross(forward, camera.up);
    const glm::vec3 move_delta = right * move.x + camera.up * move.y;
    camera.position += rotation_speed * g_frame_time * move_delta;
    camera.position = length * glm::normalize(camera.position);
    camera.up = glm::cross(right, forward);
    camera.worldToCameraSpace = LookAt(camera.position, camera.lookAt, camera.up);

    bool do_update_projection_matrix = false;
    do_update_projection_matrix = IsKeyPressed(KEY_SPACE);
    camera.is_orthographic = do_update_projection_matrix ? !camera.is_orthographic : camera.is_orthographic;

    do_update_projection_matrix = last_fov != camera.fov || do_update_projection_matrix;
    last_fov = do_update_projection_matrix ? camera.fov : last_fov;

    do_update_projection_matrix = last_near != camera.near_plane || do_update_projection_matrix;
    last_near = do_update_projection_matrix ? camera.near_plane : last_near;

    do_update_projection_matrix = last_far != camera.far_plane || do_update_projection_matrix;
    last_far = do_update_projection_matrix ? camera.far_plane : last_far;

    if(do_update_projection_matrix || IsWindowResized())
    {
        UpdateViewport(viewport);
    }
}

void UpdateViewport(Viewport& viewport)
{
    viewport.transform.z = GetScreenWidth();
    viewport.transform.w = GetScreenHeight();
    const int width = viewport.transform.z;
    const int height = viewport.transform.w;
    MyCamera& camera = viewport.camera;
    const ftype aspect = (ftype)width / (ftype)height;
    const ftype half_screen_width = width * 0.5f;
    const ftype half_screen_height = height * 0.5f;

    camera.aspect = aspect;

    camera.worldToCameraSpace = LookAt(camera.position, camera.lookAt, camera.up);
    LogMat4("World To Camera", camera.worldToCameraSpace);

    camera.projectionMatrix = ProjectionMatrix(viewport);
    LogMat4("Projection", camera.projectionMatrix);
    
    camera.clipToScreenSpace = glm::mat4(
        half_screen_width, 0.0f, 0.0f, 0.0f,
        0.0f, -half_screen_height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        half_screen_width, half_screen_height, 0.0f, 1.0f
    );
    LogMat4("Clip To Screen", camera.clipToScreenSpace);

    if(IsImageReady(viewport.depth_map))
    {
        UnloadImage(viewport.depth_map);
    }

    viewport.depth_map = GenImageColor(width, height, WHITE);
}

void Render()
{
    g_backfacing_triangles = 0;
    g_pixels_outside_screen = 0;
    g_pixels_behind_other_pixels = 0;
    BeginDrawing();
    ClearBackground(BLACK);
    RenderWorld(g_main_viewport);
    RenderUI();
    EndDrawing();
    //Log("Pixels outside screen: %d", g_pixels_outside_screen);
    //Log("Pixels behind other pixels: %d", g_pixels_behind_other_pixels);
}

void RenderWorld(Viewport& viewport)
{
    if(g_is_rending_depth_buffer)
    {
        DrawTexture(g_depth_tex2D, 0, 0, WHITE);
        return;
    }

    ImageClearBackground(&viewport.depth_map, WHITE);

    DrawMyWeirdCubes(viewport);

    DrawAxis(viewport, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void RenderUI()
{
    MyCamera& camera = g_main_viewport.camera;
    std::string state = camera.is_orthographic ? "Orthographic" : "Perspective";
    state = g_is_rending_depth_buffer ? "Depth Buffer" : state;
    DrawText(state.c_str(), 10, 10, 20, YELLOW);

    GuiSlider({35, 30, 100, 20}, "Col", TextFormat("%d", g_wall_column), &g_wall_x, 0, 7);
    GuiSlider({35, 50, 100, 20}, "Row", TextFormat("%d", g_wall_row), &g_wall_y, 0, 14);
    g_wall_column = (int)glm::round(g_wall_x);
    g_wall_row = (int)glm::round(g_wall_y);
    
    GuiSlider({35, 70, 100, 20}, "Near", TextFormat("%0.1f", camera.near_plane), &camera.near_plane, 0.1f, 10.0f);
    GuiSlider({35, 90, 100, 20}, "Far", TextFormat("%0.1f", camera.far_plane), &camera.far_plane, 10.1f, 20.0f);
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

void DrawMyWeirdCubes(Viewport& viewport)
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
    
    DrawCube(viewport, cubes[0]);
    DrawCube(viewport, cubes[1]);
    DrawCube(viewport, cubes[2]);
    DrawCube(viewport, cubes[3]);
    DrawCube(viewport, cubes[4]);
    DrawCube(viewport, cubes[5]);
    DrawCube(viewport, cubes[6]);
}

void DrawCube(Viewport& viewport, const Cube& cube)
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
    DrawQuad(viewport, cube_vertices[0], cube_vertices[1], cube_vertices[2], cube_vertices[3], g_draw_triangle_edges);
    
    // front face
    DrawQuad(viewport, cube_vertices[7], cube_vertices[6], cube_vertices[5], cube_vertices[4], g_draw_triangle_edges);
    
    // top face
    DrawQuad(viewport, cube_vertices[1], cube_vertices[5], cube_vertices[6], cube_vertices[2], g_draw_triangle_edges);
    
    // bottom face
    DrawQuad(viewport, cube_vertices[0], cube_vertices[3], cube_vertices[7], cube_vertices[4], g_draw_triangle_edges);
    
    // left face
    DrawQuad(viewport, cube_vertices[0], cube_vertices[4], cube_vertices[5], cube_vertices[1], g_draw_triangle_edges);
    
    // right face
    DrawQuad(viewport, cube_vertices[2], cube_vertices[6], cube_vertices[7], cube_vertices[3], g_draw_triangle_edges);
}

void DrawAxis(const Viewport& viewport, const glm::vec4 position)
{
    const glm::vec4 x_axis = position + glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 y_axis = position + glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    const glm::vec4 z_axis = position + glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    DrawLine3d(viewport, position, x_axis, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    DrawLine3d(viewport, position, y_axis, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    DrawLine3d(viewport, position, z_axis, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

void DrawLine3d(const Viewport& viewport, const glm::vec4 start, const glm::vec4 end, const glm::vec4 color)
{
    const MyCamera& camera = viewport.camera;
    const glm::mat4 objectToClipSpace = camera.clipToScreenSpace * camera.projectionMatrix * camera.worldToCameraSpace;

    glm::vec4 clippedStart = objectToClipSpace * start;
    clippedStart /= clippedStart.w;

    glm::vec4 clippedEnd = objectToClipSpace * end;
    clippedEnd /= clippedEnd.w;

    const Color raylib_color = ColorFromNormalized({color.x, color.y, color.z, color.w});
    DrawLine((int)clippedStart.x, (int)clippedStart.y, (int)clippedEnd.x, (int)clippedEnd.y, raylib_color);
}

void DrawColorPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color)
{
    DrawPixel(viewport, x, y, z, color);
}

void DrawTextureSampledPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec2 uv)
{
    glm::mat2 uv_matrix{
        g_sprite_atlas.width, 0,
        0, g_sprite_atlas.height
    };

    // affine texture mapping (creates the wobbly textures characteristic of PS1 games)
    const glm::ivec2 texcoords = uv_matrix * uv;
    const Vector4 color = ColorNormalize(GetImageColor(g_sprite_atlas, texcoords.x % g_sprite_atlas.width, texcoords.y % g_sprite_atlas.height));

    DrawPixel(viewport, x, y, z, {color.x, color.y, color.z, color.w});
}

void DrawPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color)
{
    const int screen_width = viewport.transform.z;
    const int screen_height = viewport.transform.w;
    const bool is_outside_z_bounds = z < -1 || z > 1;
    const bool is_outside_screen_bounds = x < 0 || x >= screen_width || y < 0 || y >= screen_height;
    if(is_outside_screen_bounds)
    {
        ++g_pixels_outside_screen;
        return;
    }

    const float depth = ColorNormalize(GetImageColor(viewport.depth_map, x, y)).z;
    if(depth < z)
    {
        ++g_pixels_behind_other_pixels;
        return;
    }

    ImageDrawPixel(&viewport.depth_map, x, y, ColorFromNormalized({z, z, z, 1.0f}));
    DrawPixel(x, y, ColorFromNormalized({color.r, color.g, color.b, color.a}));
}

/*
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
*/

void Draw3dTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only)
{
    const MyCamera& camera = viewport.camera;
    const glm::vec4 a_cam = camera.worldToCameraSpace * a.position;
    const glm::vec4 b_cam = camera.worldToCameraSpace * b.position;
    const glm::vec4 c_cam = camera.worldToCameraSpace * c.position;

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

    const glm::mat4 cameraToScreenSpace = camera.clipToScreenSpace * camera.projectionMatrix;

    glm::vec4 a_screen = cameraToScreenSpace * a_cam;
    a_screen /= a_screen.w;

    glm::vec4 b_screen = cameraToScreenSpace * b_cam;
    b_screen /= b_screen.w;

    glm::vec4 c_screen = cameraToScreenSpace * c_cam;
    c_screen /= c_screen.w;

    const Vertex a1 = {a_screen, a.color, a.uv};
    const Vertex b1 = {b_screen, b.color, b.uv};
    const Vertex c1 = {c_screen, c.color, c.uv};
    DrawTriangle(viewport, a1, b1, c1, edges_only);
}

void DrawTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const bool edges_only)
{
    const int screen_width = viewport.transform.z;
    const int screen_height = viewport.transform.w;
    auto min_x = (int)glm::round(glm::min(a.position.x, glm::min(b.position.x, c.position.x)));
    auto max_x = (int)glm::round(glm::max(a.position.x, glm::max(b.position.x, c.position.x)));
    auto min_y = (int)glm::round(glm::min(a.position.y, glm::min(b.position.y, c.position.y)));
    auto max_y = (int)glm::round(glm::max(a.position.y, glm::max(b.position.y, c.position.y)));
    min_x = glm::clamp<int>(min_x, 0, screen_width - 1);
    max_x = glm::clamp<int>(max_x, 0, screen_width - 1);
    min_y = glm::clamp<int>(min_y, 0, screen_height - 1);
    max_y = glm::clamp<int>(max_y, 0, screen_height - 1);
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
            DrawColorPixel(viewport, x, y, z, white);
        }
        else
        {
            DrawTextureSampledPixel(viewport, x, y, z, uv);
        }
    }
}

void DrawQuad(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d, const bool edges_only)
{
    Draw3dTriangle(viewport, a, b, c, edges_only);
    Draw3dTriangle(viewport, a, c, d, edges_only);
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

glm::mat4 ProjectionMatrix(const Viewport& viewport)
{
    const MyCamera& camera = viewport.camera;
    return camera.is_orthographic 
        ? OrthographicProjectionMatrix(camera.fov, camera.aspect, camera.near_plane, camera.far_plane) 
        : PerspectiveProjectionMatrix(camera.fov, camera.aspect, camera.near_plane, camera.far_plane);
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