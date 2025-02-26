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

#include <string>
#include <time.h>

typedef float ftype;

struct Vertex
{
    glm::vec4 position;
    glm::vec4 color;
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
Texture2D g_depth_texture;
Image g_depth_map;
bool g_is_rending_depth_buffer = false;

void InitializeRuntime();
void InitializeCamera();
void RunGame();
void CloseGame();
void Update();
void UpdateCamera(const ftype zoom, const glm::vec2 move);
void Render();
void RenderWorld();
void RenderUI();
void RenderCube(const Cube& cube);
void DrawAxis(const glm::vec4 position);
void DrawLine3d(const glm::vec4 start, const glm::vec4 end, const glm::vec4 color);
void DrawPixel(const int x, const int y, const ftype z, const glm::vec4 color);
void Rasterize3dLine(const glm::vec4 start, const glm::vec4 end, const glm::vec4 color);
void Draw3dTriangle(const Vertex& a, const Vertex& b, const Vertex& c);
void DrawTriangle(const Vertex& a, const Vertex& b, const Vertex& c);
void LogMat4(const char* name, const glm::mat4& m4);
void Log(const char* format, ...);
ftype GetSmoothedMouseWheelScroll();
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
}

void InitializeCamera()
{
    const ftype aspect = (ftype)g_screen_width / (ftype)g_screen_height;
    const ftype near_plane = 5.0f;
    const ftype far_plane = 100.0f;
    const ftype fov = 20.0f;
    const ftype half_screen_width = g_screen_width * 0.5f;
    const ftype half_screen_height = g_screen_height * 0.5f;

    g_camera.position = glm::vec3(0.0f, 0.0f, -15.0f);
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
    CloseWindow();
}

void Update()
{
    const ftype zoom = GetSmoothedMouseWheelScroll();
    const Vector2 mouse_delta = IsMouseButtonDown(MOUSE_LEFT_BUTTON)
        ? GetMouseDelta() : Vector2{0.0f, 0.0f};
    const glm::vec2 glm_mouse_delta = {-mouse_delta.x, mouse_delta.y};

    const bool is_zkey_pressed = IsKeyPressed(KEY_Z);
    if(is_zkey_pressed && !g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = true;
        g_depth_texture = LoadTextureFromImage(g_depth_map);
    }
    else if(is_zkey_pressed && g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = false;
        UnloadTexture(g_depth_texture);
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
    g_camera.fov = glm::clamp<float>(g_camera.fov, 20, 180);
    
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
    do_update_projection_matrix = false;//IsKeyPressed(KEY_SPACE);
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
    BeginDrawing();
    ClearBackground(BLACK);
    RenderWorld();
    RenderUI();
    EndDrawing();
}

void RenderWorld()
{
    if(g_is_rending_depth_buffer)
    {
        DrawTexture(g_depth_texture, 0, 0, WHITE);
        return;
    }

    ImageClearBackground(&g_depth_map, WHITE);

    const Cube cubes[7] = {
        {{0, 0, 0},{1,0,0},{1,1,1}, {1,1,1,1}, 0},
        {{1.5f, 0, 0},{1,0,0},{0.5f,0.5f,0.5f}, {1,0,0,1}, 1},
        {{0, 1.5f, 0},{0,1,0},{0.5f,0.5f,0.5f}, {0,1,0,1}, 1},
        {{0, 0, 1.5f},{0,0,1},{0.5f,0.5f,0.5f}, {0,0,1,1}, 1},
        {{-1.5f, 0, 0},{1,0,0},{0.5f,0.5f,0.5f}, {1,0,0,1}, 1},
        {{0, -1.5f, 0},{0,1,0},{0.5f,0.5f,0.5f}, {0,1,0,1}, 1},
        {{0, 0, -1.5f},{0,0,1},{0.5f,0.5f,0.5f}, {0,0,1,1}, 1},
    };
    
    RenderCube(cubes[0]);
    RenderCube(cubes[1]);
    RenderCube(cubes[2]);
    RenderCube(cubes[3]);
    RenderCube(cubes[4]);
    RenderCube(cubes[5]);
    RenderCube(cubes[6]);
    
    DrawAxis(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void RenderUI()
{
    DrawFPS(10, 10);

    std::string state = g_camera.is_orthographic ? "Orthographic" : "Perspective";
    state = g_is_rending_depth_buffer ? "Depth Buffer" : state;
    DrawText(state.c_str(), 10, 30, 20, YELLOW);

    GuiSlider({35, 70, 100, 20}, "Near", TextFormat("%0.1f", g_camera.near_plane), &g_camera.near_plane, 0.1f, 10.0f);
    GuiSlider({35, 90, 100, 20}, "Far", TextFormat("%0.1f", g_camera.far_plane), &g_camera.far_plane, 10.1f, 20.0f);
}

void RenderCube(const Cube& cube)
{
    const glm::mat4 objectToWorldSpace = TRSMatrix(cube.position, cube.rotation_axis, cube.scale, cube.angular_speed * g_since_start);
    Vertex cube_vertices[8] = {
        {{-1, -1, -1, 1}, {1, 0, 0, 1}},
        {{-1, 1, -1, 1}, {0, 1, 0, 1}},
        {{1, 1, -1, 1}, {0, 0, 1, 1}},
        {{1, -1, -1, 1}, {1, 1, 0, 1}},
        {{-1, -1, 1, 1}, {0, 0, 1, 1}},
        {{-1, 1, 1, 1}, {1, 1, 0, 1}},
        {{1, 1, 1, 1}, {1, 0, 0, 1}},
        {{1, -1, 1, 1}, {0, 1, 0, 1}}
    };

    cube_vertices[0].position = objectToWorldSpace * cube_vertices[0].position;
    cube_vertices[1].position = objectToWorldSpace * cube_vertices[1].position;
    cube_vertices[2].position = objectToWorldSpace * cube_vertices[2].position;
    cube_vertices[3].position = objectToWorldSpace * cube_vertices[3].position;
    cube_vertices[4].position = objectToWorldSpace * cube_vertices[4].position;
    cube_vertices[5].position = objectToWorldSpace * cube_vertices[5].position;
    cube_vertices[6].position = objectToWorldSpace * cube_vertices[6].position;
    cube_vertices[7].position = objectToWorldSpace * cube_vertices[7].position;

    Draw3dTriangle(cube_vertices[2], cube_vertices[0], cube_vertices[1]);
    Draw3dTriangle(cube_vertices[3], cube_vertices[0], cube_vertices[2]);

    Draw3dTriangle(cube_vertices[6], cube_vertices[5], cube_vertices[4]);
    Draw3dTriangle(cube_vertices[7], cube_vertices[6], cube_vertices[4]);

    Draw3dTriangle(cube_vertices[6], cube_vertices[1], cube_vertices[5]);
    Draw3dTriangle(cube_vertices[1], cube_vertices[6], cube_vertices[2]);

    Draw3dTriangle(cube_vertices[7], cube_vertices[4], cube_vertices[0]);
    Draw3dTriangle(cube_vertices[7], cube_vertices[0], cube_vertices[3]);

    Draw3dTriangle(cube_vertices[6], cube_vertices[7], cube_vertices[2]);
    Draw3dTriangle(cube_vertices[7], cube_vertices[3], cube_vertices[2]);

    Draw3dTriangle(cube_vertices[5], cube_vertices[1], cube_vertices[0]);
    Draw3dTriangle(cube_vertices[5], cube_vertices[0], cube_vertices[4]);
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

void DrawPixel(const int x, const int y, const ftype z, const glm::vec4 color)
{
    const bool is_outside_z_bounds = z < -1 || z > 1;
    const bool is_outside_screen_bounds = x < 0 || x >= g_screen_width || y < 0 || y >= g_screen_height;
    if(is_outside_screen_bounds)
    {
        return;
    }

    const bool is_not_visible = ColorNormalize(GetImageColor(g_depth_map, x, y)).z < z;
    if(is_not_visible)
    {
        return;
    }

    ImageDrawPixel(&g_depth_map, x, y, ColorFromNormalized({z, z, z, 1.0f}));

    const Color raylib_color = ColorFromNormalized({color.r, color.g, color.b, color.a});
    DrawPixel(x, y, raylib_color);
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

void Draw3dTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
{
    const glm::mat4 objectToScreenSpace = g_camera.clipToScreenSpace * g_camera.projectionMatrix * g_camera.worldToCameraSpace;

    glm::vec4 a_screen = objectToScreenSpace * a.position;
    a_screen /= a_screen.w;

    glm::vec4 b_screen = objectToScreenSpace * b.position;
    b_screen /= b_screen.w;

    glm::vec4 c_screen = objectToScreenSpace * c.position;
    c_screen /= c_screen.w;

    const Vertex a1 = {a_screen, a.color};
    const Vertex b1 = {b_screen, b.color};
    const Vertex c1 = {c_screen, c.color};
    DrawTriangle(a1, b1, c1);
}

void DrawTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
{
    const auto min_x = (int)glm::round(glm::min(a.position.x, glm::min(b.position.x, c.position.x)));
    const auto max_x = (int)glm::round(glm::max(a.position.x, glm::max(b.position.x, c.position.x)));
    const auto min_y = (int)glm::round(glm::min(a.position.y, glm::min(b.position.y, c.position.y)));
    const auto max_y = (int)glm::round(glm::max(a.position.y, glm::max(b.position.y, c.position.y)));
    const auto dim_x = max_x - min_x;
    const auto dim_y = max_y - min_y;
    const auto dim = dim_x * dim_y;

    // TODO fix overlapping triangles using top-left edge rule
    const int bias_0 = IsTopLeftOfTriangle(b.position, a.position) ? 0 : 0;
    const int bias_1 = IsTopLeftOfTriangle(c.position, b.position) ? 0 : 0;
    const int bias_2 = IsTopLeftOfTriangle(a.position, c.position) ? 0 : 0;
    
    const glm::vec3 a_to_b = b.position - a.position;
    const glm::vec3 a_to_c = c.position - a.position;
    for(int i = 0; i < dim; ++i)
    {
        const int x = min_x + i % dim_x;
        const int y = min_y + i / dim_x;

        // this is actually parallelogram area, but the ratio is the same between triangles and parallelograms
        // when calculating the barycentric coordinates
        const ftype triangle_area = a_to_c.x * a_to_b.y - a_to_c.y * a_to_b.x;
        
        if(triangle_area == 0)
        {
            continue;
        }
        
        // calculate the barycentric coordinates
        const glm::vec2 a_to_p{x - a.position.x, y - a.position.y};
        const ftype alpha = (a_to_p.x * a_to_b.y - a_to_p.y * a_to_b.x) / triangle_area;
        const ftype beta = -(a_to_p.x * a_to_c.y - a_to_p.y * a_to_c.x) / triangle_area;
        const ftype gamma = 1 - alpha - beta;
        const bool is_outside_triangle = alpha + bias_0 < 0 || beta + bias_1 < 0 || gamma + bias_2 < 0;

        if(is_outside_triangle)
        {
            continue;
        }

        const ftype z = gamma * a.position.z + alpha * c.position.z + beta * b.position.z;
        const glm::vec4 color = gamma * a.color + alpha * c.color + beta * b.color;
        DrawPixel(x, y, z, color);
    }
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
    const bool is_flat_edge = a_to_b.y == 0 && a_to_b.x > 0;
    const bool is_left_edge = a_to_b.y < 0;
    return is_flat_edge || is_left_edge;
}