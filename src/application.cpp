#include "application.h"
#include "appBase.h"

#include "myview.h"
#include "dataManager.h"
#include "drawingPass.h"

#include <nanogui/window.h>
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>

#include <nanogui/opengl.h> // GLFW_KEY_ESCAPE and others

using namespace nanogui;

#define CONFIG_PONCA_FIT_INTERFACE(FitWidget,fit,name)                                 \
        {                                                                              \
        FitWidget = new nanogui::Widget(window);                                       \
        FitWidget->set_layout(new GroupLayout());                                      \
        }


namespace poncaplot {
    const int tex_width = 500;
    const int tex_height = 500;

    PoncaPlotApplication::PoncaPlotApplication(DataManager *mgr) :
            Screen(Vector2i(1200, 1024), "PoncaPlot"), m_dataMgr(mgr) {

        m_dataMgr->setKdTreePostUpdateFunction([this]() { this->renderPasses(); });

        // force creation of all supported DrawingPasses
        for (int i = 0; i != m_dataMgr->nbSupportedDrawingPasses; ++i)
            m_dataMgr->getDrawingPass(i);

        m_passes[0] = new FillPass({1, 1, 1, 1});
        m_passes[1] = m_dataMgr->getDrawingPass("MLS - Oriented Sphere");
        m_passes[2] = new ColorMap({1, 1, 1, 1});
        m_passes[3] = new DisplayPoint({0, 0, 0, 1});

        inc_ref();
        auto *window = new Window(this, "Utils");
        window->set_position(Vector2i(1000, 0));
        window->set_layout(new GroupLayout());

        // IO
        {
            auto *tools = new Widget(window);
            tools->set_layout(new BoxLayout(Orientation::Vertical,
                                            Alignment::Middle, 0, 6));
            auto *b = new Button(tools, "Open point cloud");
            b->set_callback([&] {
                auto path = file_dialog(
                        {{"dat", "Text file x y nx y"},
                         {"txt", "Text file x y nx y"}}, false);
                std::cout << "Load file from: " << path << std::endl;
                m_dataMgr->loadPointCloud(path);
                pointIdSelector->set_max_value(m_dataMgr->getPointContainer().size() - 1);
            });
            b = new Button(tools, "Save point cloud");
            b->set_callback([&] {
                auto path = file_dialog(
                        {{"dat", "Text file x y nx y"},
                         {"txt", "Text file x y nx y"}}, true);
                std::cout << "Save file to: " << path << std::endl;
                m_dataMgr->savePointCloud(path);
            });
            b = new Button(tools, "Fit point cloud view");
            b->set_callback([&] {
                const int bordersize = 20;
                m_dataMgr->fitPointCloudToRange({tex_width - bordersize, tex_height - bordersize},
                                                {bordersize, bordersize});
            });
            b = new Button(tools, "Flip x");
            b->set_callback([&] {
                const int bordersize = 20;
                for (auto &v: m_dataMgr->getPointContainer()) {
                    v.x() = tex_width - v.x();
                }
                m_dataMgr->fitPointCloudToRange({tex_width - bordersize, tex_height - bordersize},
                                                {bordersize, bordersize});
            });
            b = new Button(tools, "Flip y");
            b->set_callback([&] {
                const int bordersize = 20;
                for (auto &v: m_dataMgr->getPointContainer()) {
                    v.y() = tex_height - v.y();
                }
                m_dataMgr->fitPointCloudToRange({tex_width - bordersize, tex_height - bordersize},
                                                {bordersize, bordersize});
            });
            b = new Button(tools, "Save image");
            b->set_callback([&] {
                auto path = file_dialog(
                        {{"png", "PNG image"}}, true);
                std::cout << "Save file to: " << path << std::endl;
                // use buffer that has already been computed (not the next one)
                float *texture = m_computeInPing ? m_textureBufferPong : m_textureBufferPing;
                write_image(m_texture->size().x(), m_texture->size().y(), texture, path);
            });
        }

        window = new Window(this, "Fitting Controls");
        window->set_position(Vector2i(0, 0));
        window->set_layout(new GroupLayout());


        new nanogui::Label(window, "Select Fit Type", "sans-bold");

        std::vector<std::string> names;
        names.resize(m_dataMgr->nbSupportedDrawingPasses);
        for (const auto &p: m_dataMgr->supportedDrawingPasses)
            names[p.second] = p.first;

        auto combo = new nanogui::ComboBox(window, names);
        combo->set_selected_index(3);
        combo->set_callback([this](int id) {
            m_passes[1] = m_dataMgr->getDrawingPass(id);
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
                dynamic_cast<FillPass *>(m_passes[0])->m_fillColor = c;
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

        passPlaneFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("MLS - Plane"));
        passSphereFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("MLS - Sphere"));
        passOrientedSphereFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("MLS - Oriented Sphere"));
        passUnorientedSphereFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("MLS - Unoriented Sphere"));
        passOnePlaneFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("One Fit - Plane"));
        passOneSphereFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("One Fit - Sphere"));
        passOneOrientedSphereFit = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("One Fit - Oriented Sphere"));
        passScaleView = dynamic_cast<BaseFitField *>(m_dataMgr->getDrawingPass("One Point - Scale"));

        {
            genericFitWidget = new nanogui::Widget(window);
            genericFitWidget->set_layout(new GroupLayout());
            new nanogui::Label(genericFitWidget, "Local Fitting", "sans-bold");
            new nanogui::Label(genericFitWidget, "Scale");
            auto slider = new Slider(genericFitWidget);
            slider->set_value(passPlaneFit->params.m_scale); // init with plane, but sync with current.
            slider->set_range({5, 200});
            slider->set_callback([&](float value) {
                passPlaneFit->params.m_scale = value;
                passSphereFit->params.m_scale = value;
                passOrientedSphereFit->params.m_scale = value;
                passUnorientedSphereFit->params.m_scale = value;
                passOnePlaneFit->params.m_scale = value;
                passOneSphereFit->params.m_scale = value;
                passOneOrientedSphereFit->params.m_scale = value;
                passScaleView->params.m_scale = value;
                renderPasses();
            });

            new Label(genericFitWidget, "MLS Iterations :", "sans-bold");
            auto int_box = new IntBox<int>(genericFitWidget, passPlaneFit->params.m_iter);
            int_box->set_editable(true);
            int_box->set_spinnable(true);
            int_box->set_min_value(1);
            int_box->set_max_value(10);
            int_box->set_value_increment(1);
            int_box->set_callback([&](int value) {
                passPlaneFit->params.m_iter = value;
                passSphereFit->params.m_iter = value;
                passOrientedSphereFit->params.m_iter = value;
                passUnorientedSphereFit->params.m_iter = value;
                passOnePlaneFit->params.m_iter = value;
                passOneSphereFit->params.m_iter = value;
                passOneOrientedSphereFit->params.m_iter = value;
                renderPasses();
            });
        }

        {
            singlePointFitWidget = new nanogui::Widget(window);
            singlePointFitWidget->set_layout(new GroupLayout());
            new nanogui::Label(singlePointFitWidget, "Evaluation Point", "sans-bold");
            new nanogui::Label(singlePointFitWidget, "Id");
            pointIdSelector = new IntBox<int>(singlePointFitWidget, 0);
            pointIdSelector->set_editable(true);
            pointIdSelector->set_spinnable(true);
            pointIdSelector->set_min_value(0);
            pointIdSelector->set_max_value(m_dataMgr->getPointContainer().size());
            pointIdSelector->set_value_increment(1);
            pointIdSelector->set_callback([&](int value) {
                dynamic_cast<OnePointFitFieldBase *>(passOnePlaneFit)->pointId = value;
                dynamic_cast<OnePointFitFieldBase *>(passOneSphereFit)->pointId = value;
                dynamic_cast<OnePointFitFieldBase *>(passOneOrientedSphereFit)->pointId = value;
                dynamic_cast<OnePointFitFieldBase *>(passScaleView)->pointId = value;
                renderPasses();
            });
        }

        CONFIG_PONCA_FIT_INTERFACE(planeFitWidget, passPlaneFit, combo->items()[1])
        CONFIG_PONCA_FIT_INTERFACE(sphereFitWidget, passSphereFit, combo->items()[2])
        CONFIG_PONCA_FIT_INTERFACE(orientedSphereFitWidget, passOrientedSphereFit, combo->items()[3])
        CONFIG_PONCA_FIT_INTERFACE(unorientedSphereFitWidget, passUnorientedSphereFit, combo->items()[4])

        // create pass 3 interface
        {
            pass3Widget = new nanogui::Widget(window);
            pass3Widget->set_layout(new GroupLayout());
            new nanogui::Label(pass3Widget, "Colormap", "sans-bold");
            new nanogui::Label(pass3Widget, "0-iso color");
            // dunno why, but sets colorpicker in range [0-255], but reads in [0-1]
            auto cp = new ColorPicker(pass3Widget, (dynamic_cast<ColorMap *>(m_passes[2]))->m_isoColor);
            cp->set_final_callback([this](const Color &c) {
                dynamic_cast<ColorMap *>(m_passes[2])->m_isoColor = c;
                renderPasses();
            });
            new nanogui::Label(pass3Widget, "Default color");
            cp = new ColorPicker(pass3Widget, (dynamic_cast<ColorMap *>(m_passes[2]))->m_defaultColor);
            cp->set_final_callback([this](const Color &c) {
                dynamic_cast<ColorMap *>(m_passes[2])->m_defaultColor = c;
                renderPasses();
            });
            new nanogui::Label(pass3Widget, "Number of isolines");
            auto int_box = new IntBox<int>(pass3Widget, dynamic_cast<ColorMap *>(m_passes[2])->m_isoQuantifyNumber);
            int_box->set_editable(true);
            int_box->set_spinnable(true);
            int_box->set_min_value(1);
            int_box->set_max_value(20);
            int_box->set_value_increment(1);
            int_box->set_callback([&](int value) {
                dynamic_cast<ColorMap *>(m_passes[2])->m_isoQuantifyNumber = value;
                renderPasses();
            });

            new nanogui::Label(pass3Widget, "0-isoline width");
            auto slider = new Slider(pass3Widget);
            slider->set_value(dynamic_cast<ColorMap *>(m_passes[2])->m_isoWidth);
            slider->set_range({0.1, 3.});
            slider->set_callback([&](float value) {
                dynamic_cast<ColorMap *>(m_passes[2])->m_isoWidth = value;
                renderPasses();
            });
        }

        // create pass 4 interface
        {
            pass4Widget = new nanogui::Widget(window);
            pass4Widget->set_layout(new GroupLayout());
            new nanogui::Label(pass4Widget, "Points Display", "sans-bold");
            new nanogui::Label(pass4Widget, "Color");
            // dunno why, but sets colorpicker in range [0-255], but reads in [0-1]
            auto cp = new ColorPicker(pass4Widget, (dynamic_cast<DisplayPoint *>(m_passes[3]))->m_pointColor);
            cp->set_final_callback([this](const Color &c) {
                dynamic_cast<DisplayPoint *>(m_passes[3])->m_pointColor = c;
                renderPasses();
            });
            auto slider = new Slider(pass4Widget);
            slider->set_value(dynamic_cast<DisplayPoint *>(m_passes[3])->m_halfSize);
            slider->set_range({1, 20});
            slider->set_callback([&](float value) {
                dynamic_cast<DisplayPoint *>(m_passes[3])->m_halfSize = int(value);
                m_image_view->setSelectionThreshold(value);
                renderPasses();
            });
        }

        window = new Window(this, "Image");
        window->set_position(Vector2i(210, 0));
        window->set_size(Vector2i(768, 768));
        window->set_layout(new GroupLayout(3));

        m_textureBufferPing = new float[tex_width * tex_height * 4]; // use Float32 RGBA textures
        m_textureBufferPong = new float[tex_width * tex_height * 4]; // use Float32 RGBA textures
        m_texture = new Texture(
                Texture::PixelFormat::RGBA,
                Texture::ComponentFormat::Float32,
                {tex_width, tex_height},
                Texture::InterpolationMode::Trilinear,
                Texture::InterpolationMode::Nearest,
                Texture::WrapMode::ClampToEdge);

        m_image_view = new MyView(window, m_dataMgr);
        m_image_view->set_size(Vector2i(768, 768));
        m_image_view->set_image(m_texture);
        m_image_view->fitImage();
        m_image_view->center();

        buildPassInterface(3);

        renderPasses();
        renderPasses(); // render twice to fill m_textureBufferPing and m_textureBufferPong
    }


    bool
    PoncaPlotApplication::keyboard_event(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboard_event(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            set_visible(false);
            return true;
        }
        return false;
    }

    void
    PoncaPlotApplication::draw(NVGcontext *ctx) {
//        if (m_needUpdate)
        Screen::draw(ctx);
    }

    void
    PoncaPlotApplication::draw_contents() {
        if (m_needUpdate) {
            m_texture->upload((const uint8_t *) (m_computeInPing ? m_textureBufferPong : m_textureBufferPing));
            m_needUpdate = false;
        }
        Screen::draw_contents();
    }

    void
    PoncaPlotApplication::buildPassInterface(int id) {
        distanceFieldWidget->set_visible(false);
        genericFitWidget->set_visible(false);
        singlePointFitWidget->set_visible(false);
        planeFitWidget->set_visible(false);
        sphereFitWidget->set_visible(false);
        orientedSphereFitWidget->set_visible(false);
        unorientedSphereFitWidget->set_visible(false);
        switch (id) {
            case 0:
                distanceFieldWidget->set_visible(true);
                break;
            case 1:
                genericFitWidget->set_visible(true);
                planeFitWidget->set_visible(true);
                break;
            case 2:
                genericFitWidget->set_visible(true);
                sphereFitWidget->set_visible(true);
                break;
            case 3:
                genericFitWidget->set_visible(true);
                orientedSphereFitWidget->set_visible(true);
                break;
            case 4:
                genericFitWidget->set_visible(true);
                unorientedSphereFitWidget->set_visible(true);
                break;
            case 5:
            case 6:
            case 7:
                break;
            case 8:
            case 9:
            case 10:
            case 11:
                genericFitWidget->set_visible(true);
                singlePointFitWidget->set_visible(true);
                break;
            default:
                throw std::runtime_error("Unknown Field type!");
        }
        perform_layout();
    }

    void
    PoncaPlotApplication::renderPasses() {
        std::cout << "[Main] Update texture" << std::endl;
        const auto &points = m_dataMgr->getKdTree();
        for (auto *p: m_passes) {
            p->render(points, m_computeInPing ? m_textureBufferPing : m_textureBufferPong, tex_width, tex_height);
        }

        m_computeInPing = !m_computeInPing;
        m_needUpdate = true;
    }
}
