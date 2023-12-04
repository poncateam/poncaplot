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
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/icons.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/colorpicker.h>
#include <nanogui/graph.h>
#include <nanogui/tabwidget.h>
#include <nanogui/texture.h>
#include <nanogui/shader.h>
#include <nanogui/renderpass.h>
#include <iostream>
#include <memory>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
#  pragma warning (disable: 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <stb_image.h>

const int tex_width = 500;
const int tex_height = 500;

using namespace nanogui;

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
                Texture::InterpolationMode::Nearest);

        for(auto j = 0; j!=tex_height*tex_width; ++j){
            float grad = 255*float(j)/float(tex_height*tex_width);
            m_textureBuffer[j*4] = grad;
            m_textureBuffer[j*4+1] = grad;
            m_textureBuffer[j*4+2] = grad;
            m_textureBuffer[j*4+3] = 255;

        }

        m_texture->upload(m_textureBuffer);

        ImageView *image_view = new ImageView(window);
        image_view->set_size(Vector2i(768,768));
        image_view->set_image( m_texture );
        image_view->center();


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
        /* Draw the user interface */
        Screen::draw(ctx);
    }

    virtual void draw_contents() {
        Screen::draw_contents();
//        m_render_pass->resize(framebuffer_size());
//        m_render_pass->begin();
//
//        m_shader->begin();
//        m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, true);
//        m_shader->end();
//
//        m_render_pass->end();
    }
private:
    ref<Shader> m_shader;
    ref<RenderPass> m_render_pass;

    uint8_t*  m_textureBuffer {nullptr};
    Texture*  m_texture {nullptr};
};

int main(int /* argc */, char ** /* argv */) {
    try {
        nanogui::init();

        /* scoped variables */ {
            ref<ExampleApplication> app = new ExampleApplication();
            app->dec_ref();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1 / 60.f * 1000);
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
