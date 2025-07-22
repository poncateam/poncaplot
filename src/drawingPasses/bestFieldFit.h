#pragma once
#include "../drawingPass.h"
#include "poncaFitField.h"

// Base class to fit a unique primitive
template <typename _FitType, typename _Base>
struct SingleFitField : public _Base {
    inline explicit SingleFitField() : _Base() {}
    ~SingleFitField() override = default;

    using Base = _Base;
    using FitType = _FitType;
    using NeighborFilter = typename FitType::NeighborFilter;

    virtual float configureAndFit(const KdTree& points, FitType& fit, RenderingContext ctx) = 0;

    void render(const KdTree& points, float*buffer, RenderingContext ctx) override{
        if(points.points().empty()) return;

        //Fit on all points
        FitType fit;
        float maxVal = configureAndFit(points, fit, ctx);
        m_lastFit = fit;
        if (fit.isStable()) {
            const int h = ctx.h;
            const int w = ctx.w;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, ctx,fit, w, h)
            for (int j = 0; j < h; ++j) {
                for (int i = 0; i < w; ++i) {
                    auto *b = buffer + (i + j * ctx.w) * 4;

                    b[2] = ColorMap::VALUE_IS_VALID;
//                    float dist = fit.potential( {i, j});
                    auto coord = ctx.pixToPoint(i,j);
                    float dist = fit.potential( { coord.first, coord.second } );

                    b[0] = fit.isSigned() ? dist : std::abs(dist);  // set pixel value
                    b[3] = ColorMap::SCALAR_FIELD;                         // set field type
                }
            }
            // store data for colormap processing (see #ColorMap)
            buffer[1] = maxVal;
            buffer[3] = ColorMap::SCALAR_FIELD;
        }

        if(Base::drawingParams.renderTrajectories)
            renderPointsTrajectories(points, buffer, ctx);
    }

private:
    _FitType m_lastFit;
    void renderPointsTrajectories(const KdTree& points, float*buffer, RenderingContext ctx) {
#pragma omp parallel for default(none) shared (points, buffer, ctx)
        for (int i = 0; i < points.point_count(); ++i) {
            const auto& p = points.points()[i];
            Base::bresenham(ctx.pointToPix(p.pos()), ctx.pointToPix(m_lastFit.project(p.pos())),{ctx.w, ctx.h},
                            [buffer, ctx](int x, int y) {
                                auto *b = buffer + (x + y * ctx.w) * 4;
                                b[2] = ColorMap::VALUE_IS_BORDER;
                                b[3] = ColorMap::SCALAR_FIELD;
                            });
        }
    }
};


// Fit a unique primitive to the entire point cloud
template <typename _FitType>
struct BestFitField : public SingleFitField<_FitType, DrawingPass> {
    inline explicit BestFitField() : SingleFitField<_FitType, DrawingPass> () {}
    ~BestFitField() override = default;

    using FitType = _FitType;
    using NeighborFilter = typename FitType::NeighborFilter;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    inline float configureAndFit(const KdTree& points, FitType& fit, RenderingContext ctx) override {
        // Configure computation to be centered on the point cloud coordinates
        float scale = points.nodes()[0].getAabb()->diagonal().norm();
        fit.setNeighborFilter(NeighborFilter(points.nodes()[0].getAabb()->center(), scale));
        // Compute fit
        fit.compute(points.points());
        postProcess(fit);
        return scale;
    }
};

using BestPlaneFitField = BestFitField<ConstPlaneFit>;

struct BestSphereFitField : public BestFitField<ConstSphereFit>{
    void postProcess(typename BestFitField<ConstSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};

struct BestOrientedSphereFitField : public BestFitField<ConstOrientedSphereFit>{
    void postProcess(typename BestFitField<ConstOrientedSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};


// Fit a primitive to single point of the point cloud
template <typename _FitType>
struct OnePointFitField : public SingleFitField<_FitType, BaseFitField>, public OnePointFitFieldBase {
    inline explicit OnePointFitField() : SingleFitField<_FitType, BaseFitField> (), OnePointFitFieldBase() {}
    ~OnePointFitField() override = default;

    using FitType     = _FitType;
    using NeighborFilter = typename FitType::NeighborFilter;
    using Scalar     = typename FitType::Scalar;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    inline float configureAndFit(const KdTree& points, FitType& fit, RenderingContext ctx) override {
        auto query = points.points()[pointId].pos();
        // Compute fit
        for (int iter = 0; iter != BaseFitField::params.m_iter; ++iter) {
            // Configure computation to be centered on the point cloud coordinates
            fit.setNeighborFilter(NeighborFilter(query, BaseFitField::params.m_scale));
            if (fit.computeWithIds(points.range_neighbors(query, BaseFitField::params.m_scale), points.points()) ==
                Ponca::STABLE) {
                postProcess(fit);
                query = fit.project(query);
            }
            else{
                std::cerr << "MLS iteration failed" << std::endl;
            }
        }
        return BaseFitField::params.m_scale;
    }
};

using OnePlaneFitField = OnePointFitField<ConstPlaneFit>;

struct OneSphereFitField : public OnePointFitField<ConstSphereFit>{
    void postProcess(typename OnePointFitField<ConstSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};

struct OneOrientedSphereFitField : public OnePointFitField<ConstOrientedSphereFit>{
    void postProcess(typename OnePointFitField<ConstOrientedSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};
