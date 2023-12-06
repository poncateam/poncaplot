#pragma once

#include <random>

#include "myview.h"


/// Base class to rendering processes
struct DrawingPass {
    virtual void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) = 0;
};

struct FillPass : public DrawingPass {
    inline explicit FillPass(const nanogui::Vector4i &fillColor = {255,255,255,255})
            : m_fillColor(fillColor) {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        for(auto j = 0; j!=w*h; ++j){
            buffer[j*4] = m_fillColor.x();
            buffer[j*4+1] = m_fillColor.y();
            buffer[j*4+2] = m_fillColor.z();
            buffer[j*4+3] = m_fillColor.w();
        }
    }
    const nanogui::Vector4i m_fillColor;
};

struct RandomPass : public DrawingPass {
    inline explicit RandomPass() : DrawingPass(), gen(rd()) {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        for(auto j = 0; j!=w*h; ++j){
            float grad = 255*float(j)/float(w*h);
            buffer[j*4] = grad;
            buffer[j*4+1] = grad;
            buffer[j*4+2] = grad;
            buffer[j*4+3] = distrib(gen);
        }
    }

    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen; // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib{1, 255};
};

struct DisplayPoint : public DrawingPass {
    inline explicit DisplayPoint(const nanogui::Vector4i &pointColor = {0,0,0,255})
            : DrawingPass(), m_pointColor(pointColor) {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        for (const auto&p : points.point_data()){
            int i (std::floor(p.pos().x()));
            int j (std::floor(p.pos().y()));
            for (int u = -1; u <= 1; ++u ){
                for (int v = -1; v <= 1; ++v ){
                    //this is suboptimal: we check multiple time, could be done better.
                    int ii = i+u;
                    int jj = j+v;
                    if(ii>=0 &&ii<w&&j>=0&&j<h){
                        auto *b = buffer + (ii + jj * w) * 4;
                        b[0] = m_pointColor[0];
                        b[1] = m_pointColor[1];
                        b[2] = m_pointColor[2];
                        b[3] = m_pointColor[3];
                    }
                }
            }
        }
    }
    const nanogui::Vector4i m_pointColor;
};
