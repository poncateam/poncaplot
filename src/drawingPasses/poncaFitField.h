#pragma once

#include "../drawingPass.h"
#include "../poncaTypes.h"


template <typename _FitType>
struct FitField : public BaseFitField {
    inline explicit FitField() : BaseFitField() {}
    ~FitField() override = default;

    using FitType = _FitType;
    using WeightFunc = typename FitType::WeightFunction;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    void render(const KdTree& points, float*buffer, RenderingContext ctx) override{
        if(points.points().empty()) return;

        float maxVal = 0;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, ctx) reduction(max : maxVal)
        for (int j = 0; j < ctx.h; ++j ) {
            for (int i = 0; i < ctx.w; ++i) {
                auto *b = buffer + (i + j * ctx.w) * 4;
                auto coord = ctx.pixToPoint(i,j);
                DataPoint::VectorType query (coord.first, coord.second);

                FitType fit;
                // Set a weighting function instance
                fit.setWeightFunc(WeightFunc(params.m_scale));
                // Set the evaluation position
                for (int iter = 0; iter != params.m_iter; ++iter) {
                    fit.init(query);
                    // Fit plane (method compute handles multipass fitting
                    if (fit.computeWithIds(points.range_neighbors(query, params.m_scale), points.points()) ==
                        Ponca::STABLE) {
                        query = fit.project(query);
                    }
                }

                if ( fit.isStable() ){
                    postProcess(fit);
                    float dist = fit.potential({coord.first,coord.second});
                    if (std::abs(dist)> maxVal) maxVal = std::abs(dist);

                    b[0] = fit.isSigned() ? dist : std::abs(dist);  // set pixel value
                    b[2] = ColorMap::VALUE_IS_VALID;
                    b[3] = ColorMap::SCALAR_FIELD;                         // set field type
                }
                else{
                    b[2] = ColorMap::VALUE_IS_INVALID;
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
