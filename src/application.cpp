#include "application.h"

#include "myview.h"
#include "dataManager.h"
#include "drawingPass.h"
#include "drawingPasses/distanceField.h"

#include <nanogui/window.h>
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>

#include <nanogui/opengl.h> // GLFW_KEY_ESCAPE and others

using namespace nanogui;

#define CONFIG_PONCA_FIT_INTERFACE(FitWidget,fit,name)                                 \
        {                                                                              \
        FitWidget = new nanogui::Widget(window);                                       \
        FitWidget->set_layout(new GroupLayout());                                      \
        }

const int tex_width = 500;
const int tex_height = 500;

PoncaPlotApplication::PoncaPlotApplication() :
Screen(Vector2i(1024, 768), "PoncaPlot"), m_dataMgr(new DataManager()){

    m_dataMgr->setKdTreePostUpdateFunction([this]() { this->renderPasses(); });

    passDFWithKdTree      = new DistanceFieldWithKdTree();
    passPlaneFit          = new PlaneFitField();
    passSphereFit         = new SphereFitField();
    passOrientedSphereFit = new OrientedSphereFitField();
    passUnorientedSphereFit = new UnorientedSphereFitField();

    m_passes[0] = new FillPass( {1,1,1,1});
    m_passes[1] = passDFWithKdTree;
    m_passes[3] = new DisplayPoint({0,0,0,1});

    inc_ref();
    auto *window = new Window(this, "Utils");
    window->set_position(Vector2i(0, 0));
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
            m_dataMgr->fitPointCloudToRange({tex_width-bordersize, tex_height-bordersize},{bordersize,bordersize});
        });
    }

    window = new Window(this, "Fitting Controls");
    window->set_position(Vector2i(0, 200));
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

    {
        genericFitWidget = new nanogui::Widget(window);
        genericFitWidget->set_layout(new GroupLayout());
        new nanogui::Label(genericFitWidget, "Local Fitting", "sans-bold");
        new nanogui::Label(genericFitWidget, "Scale");
        auto slider = new Slider(genericFitWidget);
        slider->set_value(passPlaneFit->m_scale); // init with plane, but sync with current.
        slider->set_range({5,200});
        slider->set_callback([&](float value) {
            passPlaneFit->m_scale = value;
            passSphereFit->m_scale = value;
            passOrientedSphereFit->m_scale = value;
            passUnorientedSphereFit->m_scale = value;
            renderPasses();
        });

        new Label(genericFitWidget, "MLS Iterations :", "sans-bold");
        auto int_box = new IntBox<int>(genericFitWidget, passPlaneFit->m_iter);
        int_box->set_editable(true);
        int_box->set_spinnable(true);
        int_box->set_min_value(1);
        int_box->set_max_value(10);
        int_box->set_value_increment(1);
        int_box->set_callback([&](int value) {
            passPlaneFit->m_iter = value;
            passSphereFit->m_iter = value;
            passOrientedSphereFit->m_iter = value;
            passUnorientedSphereFit->m_iter = value;
            renderPasses();
        });
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
            dynamic_cast<ColorMap *>(m_passes[2])->m_isoColor = c;
            renderPasses();
        });
        auto slider = new Slider(pass3Widget);
        slider->set_value(dynamic_cast<DisplayPoint *>(m_passes[2])->m_halfSize);
        slider->set_range({1,20});
        slider->set_callback([&](float value) {
            dynamic_cast<DisplayPoint *>(m_passes[2])->m_halfSize = int(value);
            m_image_view->setSelectionThreshold(value);
            renderPasses();
        });
    }

    window = new Window(this, "Image");
    window->set_position(Vector2i(200, 0));
    window->set_size(Vector2i(768,768));
    window->set_layout(new GroupLayout(3));

    m_textureBufferPing = new float [tex_width * tex_height * 4]; // use Float32 RGBA textures
    m_textureBufferPong = new float [tex_width * tex_height * 4]; // use Float32 RGBA textures
    m_texture = new Texture(
            Texture::PixelFormat::RGBA,
            Texture::ComponentFormat::Float32,
            {tex_width,tex_height},
            Texture::InterpolationMode::Trilinear,
            Texture::InterpolationMode::Nearest,
            Texture::WrapMode::ClampToEdge);

    m_image_view = new MyView(window, m_dataMgr);
    m_image_view->set_size(Vector2i(768, 768));
    m_image_view->set_image(m_texture );
    m_image_view->fitImage();
    m_image_view->center();

    renderPasses(); // render twice to fill m_textureBufferPing and m_textureBufferPong
    renderPasses();

    // call perform_layout
    buildPassInterface(0);
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
    if (m_needUpdate){
        m_texture->upload((const uint8_t*) (m_computeInPing ? m_textureBufferPong : m_textureBufferPing));
        m_needUpdate = false;
    }
    Screen::draw_contents();
}

void
PoncaPlotApplication::buildPassInterface(int id){
    distanceFieldWidget->set_visible(false);
    genericFitWidget->set_visible(false);
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
        default: throw std::runtime_error("Unknown Field type!");
    }
    perform_layout();
}

void
PoncaPlotApplication::renderPasses() {
    std::cout << "[Main] Update texture" << std::endl;
    const auto& points = m_dataMgr->getKdTree();
    for (auto* p : m_passes) {
        p->render(points, m_computeInPing ? m_textureBufferPing : m_textureBufferPong, tex_width, tex_height);
    }

    m_computeInPing = ! m_computeInPing;
    m_needUpdate = true;
}

