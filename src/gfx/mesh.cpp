#include <iostream>
#include <stdexcept>

#include "mesh.hpp"


namespace bgl {

Mesh::Mesh()
    : _vbo { QOpenGLBuffer::VertexBuffer },
      _ibo { QOpenGLBuffer::IndexBuffer } {
    if (!_vbo.create()) {
        throw std::runtime_error { "could not create VBO" };
    }
    if (!_ibo.create()) {
        throw std::runtime_error { "could not create IBO" };
    }
    if (!_vao.create()) {
        throw std::runtime_error { "could not create VAO" };
    }
}

void Mesh::render(GLenum mode, GLuint count) {
    std::cout << "Wrendering "<<count << std::endl;
    bind();
    glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
    if (glGetError() != GL_NO_ERROR) {
        release();
        throw std::runtime_error { "glDrawElements() failed" };
    }
    release();
}

void Mesh::render(GLenum mode) {
    _ibo.bind();
    const auto size { _ibo.size() };
    _ibo.release();
    std::cout << "VBO size: " << size << std::endl;

    render(mode, static_cast<GLuint>(size) / sizeof(GLuint));
}

void Mesh::bind() {
    _vao.bind();
    _vbo.bind();
    _ibo.bind();

}

void Mesh::release() {
    _vao.release();
    _vbo.release();
    _ibo.release();
}

}  // namespace bgl
