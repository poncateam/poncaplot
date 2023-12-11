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
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>

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

const int tex_width = 500;
const int tex_height = 500;

using namespace nanogui;

MyView *image_view {nullptr};



class ExampleApplication : public Screen {
public:



    ExampleApplication()
    : Screen(Vector2i(1024, 768), "PoncaPlot"){

        m_passes[0] = new FillPass( {255,255,255,255});
        m_passes[1] = new DistanceFieldWithKdTree();
        m_passes[2] = new DisplayPoint({255,0,0,255});

        inc_ref();
        auto *window = new Window(this, "Controls");
        window->set_position(Vector2i(0, 0));
        window->set_layout(new GroupLayout());


        new nanogui::Label(window, "Select Fit Type", "sans-bold");
        auto combo =new nanogui::ComboBox(window, { "Distance Field", "PlaneFit"});
        combo->set_callback([this](int id){
            delete (m_passes[1]);
            m_passes[1] = nullptr;
            switch (id) {
                case 0: m_passes[1] = new DistanceFieldWithKdTree(); break;
                case 1: m_passes[1] = new PlaneFitField(); break;
                default: throw std::runtime_error("Unknown Field type!");
            }
            buildPassInterface(id);
            renderPasses();
        });


        // create pass 1 interface
        {
            pass1Widget = new nanogui::Widget(window);
            pass1Widget->set_layout(new GroupLayout());
            new nanogui::Label(pass1Widget, "Background filling", "sans-bold");
            new nanogui::Label(pass1Widget, "Color");
            // dunno why, but sets colorpicker in range [0-255], but reads in [0-1]
            auto cp = new ColorPicker(pass1Widget, (dynamic_cast<FillPass *>(m_passes[0]))->m_fillColor);
            cp->set_final_callback([this](const Color &c) {
                dynamic_cast<FillPass *>(m_passes[0])->m_fillColor = 255.f * c;
                renderPasses();
            });
        }

        // create configurable passes interface
        {
            pass2WidgetDistanceField = new nanogui::Widget(window);
            pass2WidgetDistanceField->set_layout(new GroupLayout());
            new nanogui::Label(pass2WidgetDistanceField, "Distance Field", "sans-bold");
            new nanogui::Label(pass2WidgetDistanceField, "Test 2");
        }

        // create configurable passes interface
        {
            pass2WidgetPlane = new nanogui::Widget(window);
            pass2WidgetPlane->set_layout(new GroupLayout());
            new nanogui::Label(pass2WidgetPlane, "Plane fitting", "sans-bold");
            new nanogui::Label(pass2WidgetPlane, "Test 2");
        }

        // create pass 3 interface
        {
            pass3Widget = new nanogui::Widget(window);
            pass3Widget->set_layout(new GroupLayout());
            new nanogui::Label(pass3Widget, "Points Display", "sans-bold");
            new nanogui::Label(pass1Widget, "Color");
            // dunno why, but sets colorpicker in range [0-255], but reads in [0-1]
            auto cp = new ColorPicker(pass1Widget, (dynamic_cast<DisplayPoint *>(m_passes[2]))->m_pointColor);
            cp->set_final_callback([this](const Color &c) {
                dynamic_cast<DisplayPoint *>(m_passes[2])->m_pointColor = 255.f * c;
                renderPasses();
            });
            auto slider = new Slider(pass3Widget);
            slider->set_value(dynamic_cast<DisplayPoint *>(m_passes[2])->m_halfSize);
            slider->set_range({1,20});
            slider->set_callback([&](float value) {
                dynamic_cast<DisplayPoint *>(m_passes[2])->m_halfSize = int(value);
                renderPasses();
            });
        }

        window = new Window(this, "Image");
        window->set_position(Vector2i(200, 0));
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
                false); // manual mipmap update

        image_view = new MyView(window);
        image_view->set_size(Vector2i(768,768));
        image_view->set_image( m_texture );
        image_view->fitImage();
        image_view->center();
        image_view->setUpdateFunction([this](){ this->renderPasses();});

        renderPasses();

        // call perform_layout
        buildPassInterface(0);
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

    void buildPassInterface(int id){
        switch (id) {
            case 0:
                pass2WidgetDistanceField->set_visible(true);
                pass2WidgetPlane->set_visible(false);
                break;
            case 1:
                pass2WidgetDistanceField->set_visible(false);
                pass2WidgetPlane->set_visible(true);
                break;
            default: throw std::runtime_error("Unknown Field type!");
        }
        perform_layout();
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
    std::array<DrawingPass*,3> m_passes;
    bool m_needUpdate{false};
    Widget* pass1Widget, *pass2WidgetDistanceField, *pass2WidgetPlane, *pass3Widget;
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
