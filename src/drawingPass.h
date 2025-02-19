#pragma once

#include <random>
#include <iostream>

#include "poncaTypes.h"


struct OnePointFitFieldBase {
    unsigned int pointId {0};
};

struct FitParameters {
    float m_scale {40.f};
    int   m_iter  {1};
};

/// Base class to rendering processes
struct DrawingPass {
    virtual void render(const KdTree& points, float*buffer, int w, int h) = 0;
    virtual ~DrawingPass() = default;
};

struct BaseFitField : public DrawingPass{
    inline explicit BaseFitField() : DrawingPass() {}
    ~BaseFitField() override = default;
    FitParameters params;
};

struct FillPass : public DrawingPass {
    inline explicit FillPass(const nanogui::Vector4f &fillColor = {1,1,1,1})
            : m_fillColor(fillColor) {}
    void render(const KdTree& /*points*/, float*buffer, int w, int h) override{
#pragma omp parallel for default(none) shared(buffer, w, h)
        for(auto j = 0; j<w*h; ++j){
            buffer[j*4] = m_fillColor.x();
            buffer[j*4+1] = m_fillColor.y();
            buffer[j*4+2] = m_fillColor.z();
            buffer[j*4+3] = m_fillColor.w();
        }
    }
    nanogui::Vector4f m_fillColor;
};

struct RandomPass : public DrawingPass {
    inline explicit RandomPass() : DrawingPass(), gen(rd()) {}
    void render(const KdTree& /*points*/, float*buffer, int w, int h) override{
#pragma omp parallel for default(none) shared(buffer, w, h)
        for(auto j = 0; j<w*h; ++j){
            float grad = float(j)/float(w*h);
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
    inline explicit DisplayPoint(const nanogui::Vector4i &pointColor = {0,0,0,1})
            : DrawingPass(), m_pointColor(pointColor) {}
    void render(const KdTree& points, float*buffer, int w, int h) override{
        using VectorType = typename KdTree::VectorType;
        const auto pLargeSize = 2.f*m_halfSize;
#pragma omp parallel for default(none) shared(points, buffer, w, h,pLargeSize)
        for (int pid = 0; pid< points.point_count(); ++pid){
            const auto& p = points.points()[pid];
            // Build vector that is orthogonal to the normal vector
            const VectorType& tangent {p.normal().y(), -p.normal().x()};
            int i (std::floor(p.pos().x()));
            int j (std::floor(p.pos().y()));
            for (int u = std::floor(-pLargeSize); u <= int(std::ceil(pLargeSize)); ++u ){
                int ii = i+u;
                if(ii>=0 && ii<w){
                    for (int v = std::floor(-pLargeSize); v <= int(std::ceil(pLargeSize)); ++v ) {
                        VectorType localPos {u,v};
                        int jj = j + v;
                        if (jj >= 0 && jj < h) {
                            bool draw = (localPos.squaredNorm() < m_halfSize * m_halfSize)  // draw point
                                    ||  ((localPos.squaredNorm() < pLargeSize * pLargeSize)
                                         && (localPos.dot(p.normal()) > 0.f)
                                         && (std::abs(localPos.dot(tangent)) < 2)
                                         ) // draw normal
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
    nanogui::Vector4f m_pointColor;
    float m_halfSize{1.f};
};


/// Read the input texture and convert it to a color
/// Expected input for scalar fields
///   - R: value
///   - G: max value for the entire image
///   - B: 1 is value is valid, 0 otherwise (if 0, all fields the other are ignored)
///   - A: 10 (recognition bit)
///
/// Pixels with invalid values are set to default color
///
/// \note The recognition bit and the max values are read from the first pixel (buffer[1] and buffer[3] respectively),
///       and thus must be set even if the pixel is invalid
struct ColorMap : public DrawingPass {
    inline explicit ColorMap(const nanogui::Vector4i &isoColor = {1,1,1,1},
                             const nanogui::Vector4i &defaultColor = {1,1,1,0})
    : DrawingPass(), m_isoColor(isoColor), m_defaultColor(defaultColor) {}

    [[nodiscard]] inline float quantify(float in) const
    { return float(std::floor(in * float(m_isoQuantifyNumber)) / float(m_isoQuantifyNumber)); }

    void render(const KdTree& points, float*buffer, int w, int h) override{
        const FieldType ftype {int(buffer[3])};
        const auto maxVal = buffer[1];

        if (ftype == NO_FIELD) return;

#pragma omp parallel for default(none) shared(buffer, w, h, ftype, maxVal)
        for(auto j = 0; j<w*h; ++j){
            auto *b = buffer + j * 4;
            auto val = b[0];
            nanogui::Vector4f c =  m_defaultColor;

            switch (ftype) {
                case SCALAR_FIELD: {
                    if( FieldValueType(b[2]) )
                    {
                        if(std::abs(val) < m_isoWidth)
                        {
                            c = m_isoColor;
                        }
                        else if(val > 0.f)
                        {
                            c[0] = 1.f;
                            c[1] = c[2] = quantify(val / maxVal);
                        }
                        else
                        {
                            c[0] = c[1] = quantify(- val / maxVal);
                            c[2] = 1.f;
                        }
                        c[3] = 1;
                    }

                }
                    break;
                default:
                    break;
            }
            b[0] = c.x();
            b[1] = c.y();
            b[2] = c.z();
            b[3] = c.w();
        }
    }

    int m_isoQuantifyNumber {10};
    float m_isoWidth {0.8};
    nanogui::Vector4f m_isoColor;
    nanogui::Vector4f m_defaultColor;

    enum FieldType: int {
        SCALAR_FIELD = 10,
        NO_FIELD
    };

    enum FieldValueType: bool {
        VALUE_IS_VALID   = true,
        VALUE_IS_INVALID = false
    };
};
