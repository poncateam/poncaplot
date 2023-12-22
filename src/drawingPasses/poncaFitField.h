#pragma once

#include "../drawingPass.h"
#include "../poncaTypes.h"

struct BaseFitField : public DrawingPass{
    inline explicit BaseFitField() : DrawingPass() {}
    ~BaseFitField() override = default;
    float m_scale {40.f};
    int   m_iter  {1};
};

template <typename _FitType>
struct FitField : public BaseFitField {
    inline explicit FitField() : BaseFitField() {}
    ~FitField() override = default;

    using FitType = _FitType;
    using WeightFunc = typename FitType::WeightFunction;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    void render(const DataManager::KdTree& points, float*buffer, int w, int h) override{
        if(points.point_data().empty()) return;

        float maxVal = 0;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, w, h) reduction(max : maxVal)
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

                if ( (b[2] = fit.isStable()) ){
                    postProcess(fit);
                    float dist = fit.potential({i,j});
                    if (std::abs(dist)> maxVal) maxVal = std::abs(dist);

                    b[0] = dist;                     // set pixel value
                    b[3] = ColorMap::SCALAR_FIELD;   // set field type
                }
            }
        }
        // store data for colormap processing (see #ColorMap)
        buffer[1] = maxVal;
        buffer[3] = ColorMap::SCALAR_FIELD;
    }
};

using PlaneFitField = FitField<PlaneFit>;

struct SphereFitField : public FitField<SphereFit>{
    void postProcess(typename FitField<SphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};
struct OrientedSphereFitField : public FitField<OrientedSphereFit>{
    void postProcess(typename FitField<OrientedSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};
struct UnorientedSphereFitField : public FitField<UnorientedSphereFit>{
    void postProcess(typename FitField<UnorientedSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};
