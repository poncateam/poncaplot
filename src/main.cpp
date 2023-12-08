/*
    src/example1.cpp -- C++ version of an example application that shows
    how to use the various widget classes. For a Python implementation, see
    '../python/example1.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/popupbutton.h>
#include <nanogui/texture.h>

#include <iostream>
#include <memory>
#include <thread>

#include "myview.h"
#include "drawingPass.h"
#include "drawingPasses/distanceField.h"
#include "drawingPasses/plane.h"


#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
#  pragma warning (disable: 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <stb_image.h>

const int tex_width = 200;
const int tex_height = 200;

using namespace nanogui;

MyView *image_view {nullptr};



class ExampleApplication : public Screen {
public:



    ExampleApplication()
    : Screen(Vector2i(1024, 768), "PoncaPlot"){
        inc_ref();
        auto *window = new Window(this, "Controls");
        window->set_position(Vector2i(0, 0));
        window->set_layout(new GroupLayout());

        window = new Window(this, "Image");
        window->set_position(Vector2i(100, 0));
        window->set_size(Vector2i(768,768));
        window->set_layout(new GroupLayout(3));

        m_textureBuffer = new uint8_t [tex_width*tex_height];
        m_texture = new Texture(
                Texture::PixelFormat::RGBA,
                Texture::ComponentFormat::UInt8,
                {tex_width,tex_height},
                Texture::InterpolationMode::Trilinear,
                Texture::InterpolationMode::Nearest,
                Texture::WrapMode::ClampToEdge,
                1,
                Texture::TextureFlags::ShaderRead,
                true); // manual mipmap update

        m_passes.clear();
        m_passes.push_back( new FillPass( {255,255,255,255}));
//        m_passes.push_back( new DistanceField());
//        m_passes.push_back( new DistanceFieldWithKdTree()); // too slow
        m_passes.push_back( new PlaneFitField() );
        m_passes.push_back( new DisplayPoint({255,0,0,255}));

        image_view = new MyView(window);
        image_view->set_size(Vector2i(768,768));
        image_view->set_image( m_texture );
        image_view->fitImage();
        image_view->center();
        image_view->setUpdateFunction([this](){ this->renderPasses();});

        renderPasses();

        perform_layout();
    }

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboard_event(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            set_visible(false);
            return true;
        }
        return false;
    }

    virtual void draw(NVGcontext *ctx) {
//        if (m_needUpdate)
            Screen::draw(ctx);
    }

    virtual void draw_contents() {
        if (m_needUpdate){
            m_texture->upload(m_textureBuffer);
            m_needUpdate = false;
        }
        Screen::draw_contents();
    }

    void renderPasses() {
        std::cout << "[Main] Update texture" << std::endl;
        const auto& points = image_view->getPointCollection();
        for (auto* p : m_passes) {
            p->render(points, m_textureBuffer, tex_width, tex_height);
        }

        m_needUpdate = true;
    }
private:
    uint8_t*  m_textureBuffer {nullptr};
    Texture*  m_texture {nullptr};
    std::vector<DrawingPass*> m_passes;
    bool m_needUpdate{false};
};

int main(int /* argc */, char ** /* argv */) {
    try {
        nanogui::init();

        /* scoped variables */ {
            ref<ExampleApplication> app = new ExampleApplication();
            app->dec_ref();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1 / 10.f * 1000);
        }

        nanogui::shutdown();
    } catch (const std::exception &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            std::cerr << error_msg << std::endl;
        #endif
        return -1;
    } catch (...) {
        std::cerr << "Caught an unknown error!" << std::endl;
    }

    return 0;
}
