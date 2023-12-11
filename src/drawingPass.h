#pragma once

#include <random>

#include "myview.h"


/// Base class to rendering processes
struct DrawingPass {
    virtual void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) = 0;
    virtual ~DrawingPass() = default;
};

struct FillPass : public DrawingPass {
    inline explicit FillPass(const nanogui::Vector4i &fillColor = {255,255,255,255})
            : m_fillColor(fillColor) {}
    void render(const MyView::PointCollection& /*points*/, uint8_t*buffer, int w, int h) override{
#pragma omp parallel for default(none) shared(buffer, w, h)
        for(auto j = 0; j<w*h; ++j){
            buffer[j*4] = m_fillColor.x();
            buffer[j*4+1] = m_fillColor.y();
            buffer[j*4+2] = m_fillColor.z();
            buffer[j*4+3] = m_fillColor.w();
        }
    }
    nanogui::Vector4i m_fillColor;
};

struct RandomPass : public DrawingPass {
    inline explicit RandomPass() : DrawingPass(), gen(rd()) {}
    void render(const MyView::PointCollection& /*points*/, uint8_t*buffer, int w, int h) override{
#pragma omp parallel for default(none) shared(buffer, w, h)
        for(auto j = 0; j<w*h; ++j){
            float grad = 255*float(j)/float(w*h);
            buffer[j*4] = grad;
            buffer[j*4+1] = grad;
            buffer[j*4+2] = grad;
            buffer[j*4+3] = distrib(gen);
        }
    }

private:
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen; // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib{1, 255};
};

struct DisplayPoint : public DrawingPass {
    inline explicit DisplayPoint(const nanogui::Vector4i &pointColor = {0,0,0,255})
            : DrawingPass(), m_pointColor(pointColor) {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        using VectorType = typename MyView::PointCollection::VectorType;
        const auto pLargeSize = 2.f*m_halfSize;
#pragma omp parallel for default(none) shared(points, buffer, w, h,pLargeSize)
        for (int pid = 0; pid< points.point_count(); ++pid){
            const auto& p = points.point_data()[pid];
            // Build vector that is orthogonal to the normal vector
            const VectorType& tangent {p.normal().y(), p.normal().x()};
            int i (std::floor(p.pos().x()));
            int j (std::floor(p.pos().y()));
            for (int u = std::floor(-pLargeSize); u <= int(std::ceil(pLargeSize)); ++u ){
                int ii = i+u;
                if(ii>=0 && ii<w){
                    for (int v = std::floor(-pLargeSize); v <= int(std::ceil(pLargeSize)); ++v ) {
                        VectorType localPos {u,v};
                        int jj = j + v;
                        if (j >= 0 && j < h) {
                            bool draw = (localPos.squaredNorm() < m_halfSize * m_halfSize)  // draw point
                                    ||  (std::abs(localPos.dot(tangent)) < 2 && localPos.dot(p.normal())>0.f) // draw normal
                                    ;
                            if (draw) {
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
        }
    }
    nanogui::Vector4i m_pointColor;
    float m_halfSize{1.f};
};
