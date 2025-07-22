#pragma once

#include "../drawingPass.h"
#include "../poncaTypes.h"


template <typename _FitType>
struct FitField : public BaseFitField {
    inline explicit FitField() : BaseFitField() {}
    ~FitField() override = default;

    using FitType = _FitType;
    using NeighborFilter = typename FitType::NeighborFilter;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    void render(const KdTree& points, float*buffer, RenderingContext ctx) override{
        if(points.points().empty()) return;
        renderScalarField(points, buffer, ctx);
        if(drawingParams.renderTrajectories)
            renderPointsTrajectories(points, buffer, ctx);
    }

private:
    void renderScalarField(const KdTree& points, float*buffer, RenderingContext ctx){

        /// Compute scalar field
        const int h = ctx.h;
        const int w = ctx.w;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, ctx, w, h)
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * ctx.w) * 4;
                auto coord = ctx.pixToPoint(i,j);
                DataPoint::VectorType query (coord.first, coord.second);

                FitType fit;
                // Set the evaluation position
                for (int iter = 0; iter != params.m_iter; ++iter) {
                    // Set a weighting function instance
                    fit.setNeighborFilter(NeighborFilter(query, params.m_scale));
                    // Fit plane (method compute handles multipass fitting
                    if (fit.computeWithIds(points.range_neighbors(query, params.m_scale), points.points()) ==
                        Ponca::STABLE) {
                        query = fit.project(query);
                    }
                }

                if ( fit.isStable() ){
                    postProcess(fit);
                    float dist = fit.potential({coord.first,coord.second});

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
        buffer[1] = params.m_scale;
        buffer[3] = ColorMap::SCALAR_FIELD;
    }
    void renderPointsTrajectories(const KdTree& points, float*buffer, RenderingContext ctx){

#pragma omp parallel for default(none) shared (points, buffer, ctx)
        for (int i = 0; i < points.point_count(); ++i)
        {
            const auto& p = points.points()[i];
            /// x is going to follow the flow
            DataPoint::VectorType x, nextx = p.pos();

            /// We stop if the x does not move anymore, or after 10 iterations
            int projIter = 0;
            const int nbProjIter = 50;
            bool stop = false;
            float potential {0.f};
            do {
                FitType fit;

                // Set the evaluation position
                for (int iter = 0; iter != params.m_iter; ++iter) {
                    x = nextx;

                    // Set a weighting function instance
                    fit.setNeighborFilter(NeighborFilter(x, params.m_scale));
                    if (fit.computeWithIds(points.range_neighbors(x, params.m_scale), points.points()) ==
                        Ponca::STABLE) {
                        postProcess(fit);
                        nextx = fit.project(x);
                        potential = fit.potential(nextx);

                        // ask to stop the projection procedure if motion is below one pixel
                        int nbPix = bresenham(ctx.pointToPix( x ),  ctx.pointToPix( nextx ),
                                    {ctx.w, ctx.h},
                                         [buffer, ctx](int x, int y){
                             auto *b = buffer + (x + y * ctx.w) * 4;
                             b[2] = ColorMap::VALUE_IS_BORDER;
                             b[3] = ColorMap::SCALAR_FIELD;
                        });
                        stop |= nbPix<=1;

                    } else stop = true;
                }

            } while (!stop
                     && ++projIter < nbProjIter
//            && ! x.isApprox(nextx)
//            && potential >= 10e-5
                    );
        }
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
