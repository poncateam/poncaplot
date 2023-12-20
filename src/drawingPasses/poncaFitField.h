#pragma once

#include "../drawingPass.h"
#include "../poncaTypes.h"

struct BaseFitField : public DrawingPass{
    inline explicit BaseFitField() : DrawingPass() {}
    ~BaseFitField() override = default;
    float m_scale {20.f};
    int   m_iter  {1};
};

template <typename _FitType>
struct FitField : public BaseFitField {
    inline explicit FitField() : BaseFitField() {}
    ~FitField() override = default;

    using FitType = _FitType;
    using WeightFunc = typename FitType::WeightFunction;


    void render(const DataManager::KdTree& points, float*buffer, int w, int h) override{
        if(points.point_data().empty()) return;

        const auto normFactor = float(std::max(w,h));
#pragma omp parallel for collapse(2) default(none) shared(normFactor, points, buffer, w, h)
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                DataPoint::VectorType query (i, j);

                FitType fit;
                // Set a weighting function instance
                fit.setWeightFunc(WeightFunc(m_scale));
                // Set the evaluation position
                for (int iter = 0; iter != m_iter; ++iter) {
                    fit.init(query);
                    // Fit plane (method compute handles multipass fitting
                    if (fit.computeWithIds(points.range_neighbors(query, m_scale), points.point_data()) ==
                        Ponca::STABLE) {
                        query = fit.project(query);
                    }
                }
                if (fit.isStable()){
                    float dist = fit.potential({i,j});
                    if (dist * dist < 1.4)
                        b[0] = b[1] = b[2] = 255;
                    else {
                        auto col = int(255. * dist / m_scale);
                        if (dist > 0)
                            b[0] = b[1] = b[2] = col;
                        else {
                            b[0] = -col;
                            b[1] = b[2] = 0;
                        }
                    }
                    b[3] = 255;
                }
            }
        }
    }
};

using PlaneFitField = FitField<PlaneFit>;
using SphereFitField = FitField<SphereFit>;
using OrientedSphereFitField = FitField<OrientedSphereFit>;
using UnorientedSphereFitField = FitField<UnorientedSphereFit>;
