// Copyright 2020 Bastian Kuolt
#ifndef GFX_GFX_HPP_
#define GFX_GFX_HPP_

#include "gl.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "mesh.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>  // SDL_GLContext, SDL_Window

#include <ostream>      // std::ostream
#include <memory>       // std::shared_ptr
#include <filesystem>   // std::filesystem::path

#include "window.hpp"

namespace bgl {

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

class grid final {
 public:
    using Vertex = vec3;
    using VBO = bgl::VertexBuffer<Vertex>;
    using VAO = VertexArray<Vertex>;

    using SharedVBO = bgl::SharedVBO<Vertex>;
    using SharedVAO = bgl::SharedVAO<Vertex>;

    explicit grid(GLfloat size, std::size_t num_cells);
    void render(const mat4 &MVP);

    void translate(const vec3 &v) {
        _translation += v;
    }

 private:
    const GLfloat _cell_size;
    const std::size_t _num_cells;

    vec3 _translation;

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

}  // namespace bgl

#endif  // GFX_GFX_HPP_
