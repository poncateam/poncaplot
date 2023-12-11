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
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>

#include <iostream>
#include <memory>
#include <thread>

#include "myview.h"
#include "drawingPass.h"
#include "drawingPasses/distanceField.h"
#include "drawingPasses/poncaFitField.h"


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
private:
    // We need to use a macro, to avoid the lambda to be defined in a local context
#define CONFIG_PONCA_FIT_INTERFACE(FitWidget,fit,name)                                 \
        {                                                                              \
        FitWidget = new nanogui::Widget(window);                                       \
        FitWidget->set_layout(new GroupLayout());                                      \
        new nanogui::Label(FitWidget, name, "sans-bold");                              \
        new nanogui::Label(FitWidget, "Scale");                                        \
        auto slider = new Slider(FitWidget);                                           \
        slider->set_value(fit->m_scale);                                               \
        slider->set_range({5,100});                                                    \
        slider->set_callback([&](float value) {                                        \
            fit->m_scale = value;                                                      \
            std::cout<< "Set scale to " << value << std::endl;                         \
            renderPasses();                                                            \
        });                                                                            \
        /* Positive integer widget */                                                  \
        new Label(FitWidget, "MLS Iterations :", "sans-bold");                         \
        auto int_box = new IntBox<int>(FitWidget, fit->m_iter);                        \
        int_box->set_editable(true);                                                   \
        int_box->set_spinnable(true);                                                  \
        int_box->set_min_value(1);                                                     \
        int_box->set_max_value(10);                                                    \
        int_box->set_value_increment(1);                                               \
        int_box->set_callback([&](int value) {                                         \
            fit->m_iter = value;                                                       \
            renderPasses();                                                            \
        });                                                                            \
        }



public:



    ExampleApplication()
    : Screen(Vector2i(1024, 768), "PoncaPlot"){

        passDFWithKdTree      = new DistanceFieldWithKdTree();
        passPlaneFit          = new PlaneFitField();
        passSphereFit         = new SphereFitField();
        passOrientedSphereFit = new OrientedSphereFitField();
        passUnorientedSphereFit = new UnorientedSphereFitField();

        m_passes[0] = new FillPass( {255,255,255,255});
        m_passes[1] = passDFWithKdTree;
        m_passes[2] = new DisplayPoint({255,0,0,255});

        inc_ref();
        auto *window = new Window(this, "Controls");
        window->set_position(Vector2i(0, 0));
        window->set_layout(new GroupLayout());


        new nanogui::Label(window, "Select Fit Type", "sans-bold");
        auto combo =new nanogui::ComboBox(window,
                                          { "Distance Field",
                                            "Plane",
                                            "Sphere",
                                            "Oriented Sphere",
                                            "Unoriented Sphere"});
        combo->set_callback([this](int id){
            switch (id) {
                case 0: m_passes[1] = passDFWithKdTree; break;
                case 1: m_passes[1] = passPlaneFit; break;
                case 2: m_passes[1] = passSphereFit; break;
                case 3: m_passes[1] = passOrientedSphereFit; break;
                case 4: m_passes[1] = passUnorientedSphereFit; break;
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
            distanceFieldWidget = new nanogui::Widget(window);
            distanceFieldWidget->set_layout(new GroupLayout());
            new nanogui::Label(distanceFieldWidget, "Distance Field", "sans-bold");
            new nanogui::Label(distanceFieldWidget, "no parameter available");
        }

        CONFIG_PONCA_FIT_INTERFACE(planeFitWidget,passPlaneFit,combo->items()[1])
        CONFIG_PONCA_FIT_INTERFACE(sphereFitWidget,passSphereFit,combo->items()[2])
        CONFIG_PONCA_FIT_INTERFACE(orientedSphereFitWidget,passOrientedSphereFit,combo->items()[3])
        CONFIG_PONCA_FIT_INTERFACE(unorientedSphereFitWidget,passUnorientedSphereFit,combo->items()[4])

        // create pass 3 interface
        {
            pass3Widget = new nanogui::Widget(window);
            pass3Widget->set_layout(new GroupLayout());
            new nanogui::Label(pass3Widget, "Points Display", "sans-bold");
            new nanogui::Label(pass3Widget, "Color");
            // dunno why, but sets colorpicker in range [0-255], but reads in [0-1]
            auto cp = new ColorPicker(pass3Widget, (dynamic_cast<DisplayPoint *>(m_passes[2]))->m_pointColor);
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
        distanceFieldWidget->set_visible(false);
        planeFitWidget->set_visible(false);
        sphereFitWidget->set_visible(false);
        orientedSphereFitWidget->set_visible(false);
        unorientedSphereFitWidget->set_visible(false);
        switch (id) {
            case 0:
                distanceFieldWidget->set_visible(true);
                break;
            case 1:
                planeFitWidget->set_visible(true);
                break;
            case 2:
                sphereFitWidget->set_visible(true);
                break;
            case 3:
                orientedSphereFitWidget->set_visible(true);
                break;
            case 4:
                unorientedSphereFitWidget->set_visible(true);
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
    Widget* pass1Widget, *distanceFieldWidget, *planeFitWidget, *sphereFitWidget, *orientedSphereFitWidget, *unorientedSphereFitWidget, *pass3Widget;
    DistanceFieldWithKdTree *passDFWithKdTree;
    PlaneFitField *passPlaneFit;
    SphereFitField *passSphereFit;
    OrientedSphereFitField *passOrientedSphereFit;
    UnorientedSphereFitField *passUnorientedSphereFit;
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
