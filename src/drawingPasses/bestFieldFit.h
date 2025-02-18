#pragma once
#include "../drawingPass.h"

// Fit a unique primitive to the entire point cloud
template <typename _FitType>
struct BestFitField : public DrawingPass {
    inline explicit BestFitField() : DrawingPass() {}
    ~BestFitField() override = default;

    using FitType = _FitType;
    using WeightFunc = typename FitType::WeightFunction;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    void render(const KdTree& points, float*buffer, int w, int h) override{
        if(points.points().empty()) return;

        //Fit on all points
        FitType fit;
        // Configure computation to be centered on the point cloud coordinates
        fit.setWeightFunc(WeightFunc(points.nodes()[0].getAabb()->diagonal().norm()));
        fit.init(points.nodes()[0].getAabb()->center());
        // Compute fit
        fit.compute(points.points());
        postProcess(fit);

        float maxVal = 0;
        if (fit.isStable()) {
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, w, h,fit) reduction(max : maxVal)
            for (int j = 0; j < h; ++j) {
                for (int i = 0; i < w; ++i) {
                    auto *b = buffer + (i + j * w) * 4;

                    (b[2] = fit.isStable());
                    float dist = fit.potential({i, j});
                    if (std::abs(dist) > maxVal) maxVal = std::abs(dist);

                    b[0] = fit.isSigned() ? dist : std::abs(dist);  // set pixel value
                    b[3] = ColorMap::SCALAR_FIELD;                         // set field type
                }
            }
            // store data for colormap processing (see #ColorMap)
            buffer[1] = maxVal;
            buffer[3] = ColorMap::SCALAR_FIELD;
        }
    }
};


using BestPlaneFitField = BestFitField<ConstPlaneFit>;

struct BestSphereFitField : public BestFitField<ConstSphereFit>{
    void postProcess(typename BestFitField<ConstSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};

struct BestOrientedSphereFitField : public BestFitField<ConstOrientedSphereFit>{
    void postProcess(typename BestFitField<ConstOrientedSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};

