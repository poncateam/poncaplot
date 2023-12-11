#pragma once
#include "../drawingPass.h"

#include <Ponca/Fitting>

struct PlaneFitField : public DrawingPass {
    inline explicit PlaneFitField() : DrawingPass() {}

    using PointType  = typename MyView::PointCollection::DataPoint;
    using Scalar     = typename PointType::Scalar;
    using WeightFunc = Ponca::DistWeightFunc<PointType,Ponca::SmoothWeightKernel<Scalar> >;
    using FitType = Ponca::Basket<PointType ,WeightFunc, Ponca::CovariancePlaneFit>;


    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        if(points.point_data().empty()) return;

        float scale = 20;

        double normFactor (scale);
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                DataPoint::VectorType query (i, j);

                FitType fit;
                // Set a weighting function instance
                fit.setWeightFunc(WeightFunc(scale));
                // Set the evaluation position
                fit.init(query);
                // Fit plane (method compute handles multipass fitting
                if( fit.computeWithIds(points.range_neighbors( query, scale ), points.point_data() ) == Ponca::STABLE ){
                    float dist = fit.potential(query);
                    if (dist*dist < 1.4)
                        b[0] = b[1] = b[2] = 255;
                    else
                    {
                        auto col = uint (255. * dist / normFactor);
                        if (dist > 0)
                            b[0] = b[1] = b[2] = col;
                        else {
                            b[0] = - col;
                            b[1] = b[2] = 0;
                        }
                    }
                    b[3] = 255;
                }
            }
        }
    }
};
