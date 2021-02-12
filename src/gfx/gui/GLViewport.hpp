/**
 * @file viewport.cpp
 * @brief A simple OpenGL Qt Viewport
 */
#ifndef BGL_VIEWPORT_HPP_
#define BGL_VIEWPORT_HPP_

#include <QOpenGLWidget>

namespace bgl {

class GLViewport : public QOpenGLWidget {
 public:
    explicit GLViewport(QWidget *parent , void (*_render_callback)(float delta));

 protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;


    void (*_render_callback)(float delta) = nullptr;
};

}  // namespace bgl

#endif  // BGL_VIEWPORT_HPP_
