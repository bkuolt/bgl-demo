
#include <assimp/cimport.h>      // aiPropertyStore
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/material.h>
#include <assimp/scene.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

#include <assimp/Importer.hpp>

#include "model.hpp"
#include "box.hpp"
#include "importer.hpp"  //  TODO
#include "gfx.hpp"       //  TODO

#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>


namespace bgl {

namespace {

inline bool is_textured(const aiMesh &mesh) noexcept {
    return mesh.mTextureCoords[0] != nullptr;
}

inline bool has_material(const aiMesh &mesh) noexcept {
    return mesh.mMaterialIndex != 0;
}

/*********************************************************
 *                     OpenGL Code                       *
 *********************************************************/
QOpenGLBuffer create_vbo(QOpenGLBuffer &vbo, const aiMesh &mesh) {
    vbo.bind();
    vbo.allocate(sizeof(Vertex) * mesh.mNumVertices);
    auto buffer { reinterpret_cast<Vertex*>(vbo.map(QOpenGLBuffer::ReadWrite)) };
    if (buffer == nullptr) {
        vbo.release();
        throw std::runtime_error{"could not map VBO"};
    }

    for (auto i = 0u; i < mesh.mNumVertices; ++i) {
        buffer[i].normal = vec3{mesh.mNormals[i].x, mesh.mNormals[i].y, mesh.mNormals[i].z};
        buffer[i].position = vec3{mesh.mVertices[i].x, mesh.mVertices[i].y, mesh.mVertices[i].z};
    }

    if (is_textured(mesh)) {
        if (mesh.mNumUVComponents[0] != 2) {
            vbo.unmap();
            vbo.release();
            throw std::runtime_error{"only one texture channel supported"};
        }
        for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
            buffer[i].texcoords = vec2{mesh.mTextureCoords[0][i].x, 1.0 - mesh.mTextureCoords[0][i].y};
        }
    }

    if (!vbo.unmap()) {
        vbo.release();
        throw std::runtime_error{"could not unmap VBO"};
    }
    vbo.release();
    return vbo;
}

void create_ibo(QOpenGLBuffer &ibo, const aiMesh &mesh) {
    ibo.bind();
    ibo.allocate(sizeof(GLuint) * mesh.mNumFaces * 3);
    GLuint *buffer {reinterpret_cast<GLuint*>(ibo.map(QOpenGLBuffer::WriteOnly))};
    if (buffer == nullptr) {
        throw std::runtime_error{"could not map IBO"};
    }

    for (auto i = 0u; i < mesh.mNumFaces; ++i) {
        assert(mesh.mFaces[i].mNumIndices == 3);
        std::copy_n(mesh.mFaces[i].mIndices, 3, buffer);
        buffer += 3;
    }

    if (!ibo.unmap()) {
        throw std::runtime_error{"could not unmap IBO"};
    }
    ibo.release();
}

// program must be bound!!!!
void create_vao(QOpenGLVertexArrayObject &vao, QOpenGLBuffer &vbo, QOpenGLShaderProgram &program) {
    program.bind();
    vao.bind();
    vbo.bind();
    const auto stride{sizeof(Vertex)};
    set_va_attribute(program.attributeLocation("position"), 3, GL_FLOAT, stride, offsetof(Vertex, position));
    set_va_attribute(program.attributeLocation("normal"), 3, GL_FLOAT, stride, offsetof(Vertex, normal));
    set_va_attribute(program.attributeLocation("texcoords"), 2, GL_FLOAT, stride, offsetof(Vertex, texcoords));
    vao.release();
    vbo.release();
    program.release();
}

/*********************************************************
 *                     Assimp Mesh Code                  *
 *********************************************************/
const aiScene *importScene(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        std::ostringstream oss;
        oss << "the file " << std::quoted(path.string()) << " does not exist";
        throw std::runtime_error{oss.str()};
    }

    aiPropertyStore *props = aiCreatePropertyStore();
    if (props == nullptr) {
        throw std::runtime_error{aiGetErrorString()};
    }

    aiSetImportPropertyInteger(props, AI_CONFIG_PP_PTV_NORMALIZE, 1);
    const aiScene *scene{aiImportFileExWithProperties(path.string().c_str(),
                                                      aiProcess_Triangulate |
                                                      aiProcess_GenSmoothNormals |
                                                      aiProcess_Triangulate |
                                                      aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices,
                                                      nullptr, props)};

    aiReleasePropertyStore(props);
    return scene ? scene
                    : throw std::runtime_error{aiGetErrorString()};
}

void load_meshes(Model& model, const aiScene &scene, QOpenGLShaderProgram &program) {
    if (scene.mNumMeshes == 0) {
        throw std::runtime_error{"empty model"};
    }

    std::vector<Mesh> &meshes { model.getMeshes() };
    meshes = std::vector<Mesh>(scene.mNumMeshes);

    std::cout << "loading " << meshes.size() << " meshes" << std::endl;

    for (auto i = 0u; i < meshes.size(); ++i) {
        const aiMesh &ai_mesh{*scene.mMeshes[i]};
        create_vbo(meshes[i]._vbo, ai_mesh);
        create_ibo(meshes[i]._ibo, ai_mesh);
        create_vao(meshes[i]._vao, meshes[i]._vbo, program);

        if (has_material(ai_mesh)) {
            meshes[i]._materialIndex = ai_mesh.mMaterialIndex;
        }
    }
}

BoundingBox calculate_bounding_box(const aiScene &scene) noexcept {
    struct Bound {
        float min;
        float max;
    };
    tvec3<Bound> bounds;  // TODO: simplifiy code

    BoundingBox boundingBox;
    for (auto i = 0u; i < scene.mNumMeshes; ++i) {
        const aiMesh &mesh{*scene.mMeshes[i]};

        for (auto vertex_index = 0u; vertex_index < mesh.mNumVertices; ++vertex_index) {
            for (auto c = 0; c < 3; ++c) {
                bounds[c].min = std::min(bounds[c].min, mesh.mVertices[vertex_index][c]);
                bounds[c].max = std::max(bounds[c].max, mesh.mVertices[vertex_index][c]);
            }
        }
    }

    const vec3 size {
        bounds.x.max - bounds.x.min,
        bounds.y.max - bounds.y.min,
        bounds.z.max - bounds.z.min
    };
    const vec3 center {
        bounds.x.min + (size.x / 2),
        bounds.y.min + (size.y / 2),
        bounds.z.min + (size.z / 2)
    };
    return BoundingBox { center, size };
}

/*********************************************************
 *                   Assimp Material Code                *
 *********************************************************/
vec3 get_color(const aiMaterial &material,
                const char *pKey, unsigned int type, unsigned int idx) {
    aiColor3D color;
    material.Get(pKey, type, idx, color);
    return {color.r, color.g, color.b};
}

float get_shininess(const aiMaterial &material) {
    return 0;  // TODO
}

const std::filesystem::path get_path(const aiMaterial &material, aiTextureType type,
	                               const std::filesystem::path &base_path) {
	aiString str;
	material.GetTexture(type, 0, &str);
	return { (base_path / str.data).string() };
}

std::shared_ptr<QOpenGLTexture> get_texture(const aiMaterial &material, aiTextureType type,
	                                        const std::filesystem::path &base_path) {
    const unsigned int texture_count{material.GetTextureCount(type)};
    if (texture_count >= 1) {
        auto texture { LoadTexture(get_path(material, type, base_path)) };
        if (texture_count > 1) {
            std::cout << "warning: found more textures than expected" << std::endl;
        }
        return texture;
    }
    return {};
}

Material load_material(const aiMaterial &material, const std::filesystem::path &base_path) {
    return {
        .diffuse = get_color(material, AI_MATKEY_COLOR_DIFFUSE),
        .ambient = get_color(material, AI_MATKEY_COLOR_AMBIENT),
        .specular = get_color(material, AI_MATKEY_COLOR_SPECULAR),
        .emissive = get_color(material, AI_MATKEY_COLOR_EMISSIVE),
        .shininess = get_shininess(material),
        .textures{
            .diffuse = get_texture(material, aiTextureType_DIFFUSE, base_path),
            .ambient = get_texture(material, aiTextureType_AMBIENT, base_path),
            .specular = get_texture(material, aiTextureType_SPECULAR, base_path),
            .emissive = get_texture(material, aiTextureType_EMISSIVE, base_path)} };
}

std::vector<Material> load_materials(const aiScene &scene, const std::filesystem::path &base_path) {
    std::cout << "loading " << scene.mNumMaterials << " materials" << std::endl;
    std::vector<Material> materials;
    for (auto i = 0u; i < scene.mNumMaterials; ++i) {
        materials.push_back(load_material(*scene.mMaterials[i], base_path));
    }
    return materials;
}

} // anonymous namespace

std::shared_ptr<Model> LoadModel(const std::filesystem::path &path){
    const auto model { std::make_shared<Model>() };
    const aiScene &scene { *importScene(path) };

    model->setProgram(LoadProgram({ "./assets/shaders/main.vs", "./assets/shaders/main.fs" }));
    load_meshes(*model, scene, *model->getProgram());
    model->setMaterials(load_materials(scene, path.parent_path()));
    model->setBoundingBox(calculate_bounding_box(scene));
    return model;
}

std::shared_ptr<QOpenGLTexture> LoadTexture(const std::filesystem::path &path) {
	QImage image { path.string().c_str() };
	std::cout << "loading " << path << std::endl;
	return std::make_shared<QOpenGLTexture>(image);
}

} // namespace bgl
