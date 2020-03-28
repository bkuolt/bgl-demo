// Copyright 2020 Bastian Kuolt
#ifndef GFX_GFX_HPP_
#define GFX_GFX_HPP_

#include "gl.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "mesh.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>  // SDL_GLContext, SDL_Window

#include <ostream>      // std::ostream
#include <memory>       // std::shared_ptr
#include <filesystem>   // std::filesystem::path

#include <SDL2/SDL_ttf.h>



namespace bgl {

using SharedWindow = std::shared_ptr<SDL_Window>;
using SharedContext = std::shared_ptr<SDL_GLContext>;

SharedWindow createFullScreenWindow();
SharedContext createGLContext(const SharedWindow &window);

std::ostream& operator<<(std::ostream &os, const vec2 &vector);
std::ostream& operator<<(std::ostream &os, const vec3 &vector);

#ifdef __linux
namespace console_color {
    constexpr char blue[] = "\x1B[34m";
    constexpr char red[] = "\x1B[31m";
    constexpr char white[] = "\x1B[37m";
    constexpr char magenta[] = "\x1B[35m";
    constexpr char yellow[] = "\x1B[33m";
    constexpr char green[] =  "\x1B[32m";
}  // namespace console_color

#endif  // __linux

class Camera {
 public:
    Camera() noexcept = default;
    Camera(const vec3 &position, const vec3 &viewCenter);
    Camera(const Camera&) = default;
    Camera(Camera&&) = default;
    virtual ~Camera() noexcept = default;

    Camera& operator=(const Camera&) = default;
    Camera& operator=(Camera&&) = default;

    void setPosition(const vec3 &position) noexcept;
    void setZoom(double factor = 1.0);
    void setViewCenter(const vec3 &center) noexcept;

    const vec3& getPosition() const noexcept;
    const vec3& getViewCenter() const noexcept;
    double getZoom() const noexcept;

    mat4 getMatrix() const noexcept;

    void rotate(const vec2 degrees) noexcept;

 private:
    vec3 _position { 0.0, 0.0, 1.0 };
    vec3 _center { 0.0, 0.0, 0.0 };
    double _zoom { 1.0 };
};

class grid final {
 public:
    using Vertex = vec3;
    using VBO = bgl::VertexBuffer<Vertex>;
    using VAO = VertexArray<Vertex>;

    using SharedVBO = bgl::SharedVBO<Vertex>;
    using SharedVAO = bgl::SharedVAO<Vertex>;

    explicit grid(GLfloat size, std::size_t num_cells);
    void render(const mat4 &MVP);

 private:
    const GLfloat _cell_size;
    const std::size_t _num_cells;

    void create_vbo();
    void create_ibo();
    void create_vao();

    SharedVBO _vbo;
    SharedIBO _ibo;
    SharedVAO _vao;
    SharedProgram _program;
};

using SharedGrid = std::shared_ptr<grid>;

inline SharedGrid CreateGrid(GLfloat size, size_t num_cells) {
   return std::make_shared<grid>(size, num_cells);
}

/* ---------------------------- Font ---------------------------- */

class Font;
class Text;

using SharedFont = std::shared_ptr<Font>;
using SharedText = std::shared_ptr<Text>;

class Font : protected std::enable_shared_from_this<Font> {
 public:
    friend class Text;

    Font(const std::filesystem::path &path, std::size_t size);
    virtual ~Font() noexcept;

    SharedText createText(const std::string &text = {});

 private:
    const std::size_t _size;
    TTF_Font *_handle { nullptr };
};

class Text {
 public:
    friend class Font;
    explicit Text(const SharedFont &font, const std::string &text = {});
    virtual ~Text() noexcept;

    void setText(const std::string &text);
    void render();

 private:
   const SharedFont _font;

   std::string _text;
   glm::ivec2 _dimensions;
   SDL_Texture *_texture { nullptr };
};

}  // namespace bgl

#endif  // GFX_GFX_HPP_
