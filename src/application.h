#pragma once

#include <nanogui/screen.h>

// cannot forward declare aliases
#include "drawingPasses/poncaFitField.h"

// forward declarations
class DrawingPass;
class DistanceFieldWithKdTree;
class MyView;
class DataManager;

namespace nanogui{
    class Texture;
}



class PoncaPlotApplication : public nanogui::Screen {

public:
    PoncaPlotApplication();

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    void draw(NVGcontext *ctx) override;

    void draw_contents() override;

private:
    void buildPassInterface(int id);

    void renderPasses();
private:
    uint8_t*  m_textureBufferPing {nullptr}, *  m_textureBufferPong {nullptr};
    bool m_computeInPing{true};
    nanogui::Texture*  m_texture {nullptr};
    std::array<DrawingPass*,3> m_passes{nullptr, nullptr, nullptr};
    bool m_needUpdate{false};

    DataManager *m_dataMgr{nullptr};

    MyView *m_image_view {nullptr};
    Widget* pass1Widget, *distanceFieldWidget,
            *genericFitWidget,    //< parameters applicable to all fitting techniques
    *planeFitWidget, *sphereFitWidget, *orientedSphereFitWidget, *unorientedSphereFitWidget,
            *pass3Widget;
    DistanceFieldWithKdTree *passDFWithKdTree;
    PlaneFitField *passPlaneFit;
    SphereFitField *passSphereFit;
    OrientedSphereFitField *passOrientedSphereFit;
    UnorientedSphereFitField *passUnorientedSphereFit;
};
