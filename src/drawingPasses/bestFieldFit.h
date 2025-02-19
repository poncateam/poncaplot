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
    using WeightFunc = typename FitType::WeightFunction;

    virtual void configureAndFit(const KdTree& points, FitType& fit) = 0;

    void render(const KdTree& points, float*buffer, int w, int h) override{
        if(points.points().empty()) return;

        //Fit on all points
        FitType fit;
        configureAndFit(points, fit);

        float maxVal = 0;
        if (fit.isStable()) {
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, w, h,fit) reduction(max : maxVal)
            for (int j = 0; j < h; ++j) {
                for (int i = 0; i < w; ++i) {
                    auto *b = buffer + (i + j * w) * 4;

                    b[2] = ColorMap::VALUE_IS_VALID;
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


// Fit a unique primitive to the entire point cloud
template <typename _FitType>
struct BestFitField : public SingleFitField<_FitType, DrawingPass> {
    inline explicit BestFitField() : SingleFitField<_FitType, DrawingPass> () {}
    ~BestFitField() override = default;

    using FitType = _FitType;
    using WeightFunc = typename FitType::WeightFunction;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    inline void configureAndFit(const KdTree& points, FitType& fit) override {
        // Configure computation to be centered on the point cloud coordinates
        fit.setWeightFunc(WeightFunc(points.nodes()[0].getAabb()->diagonal().norm()));
        fit.init(points.nodes()[0].getAabb()->center());
        // Compute fit
        fit.compute(points.points());
        postProcess(fit);
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
    using WeightFunc = typename FitType::WeightFunction;
    using Scalar     = typename FitType::Scalar;

    /// Method called at the end of the fitting process, only for stable fits
    virtual void postProcess(FitType& /*fit*/){};

    inline void configureAndFit(const KdTree& points, FitType& fit) override {
        // Configure computation to be centered on the point cloud coordinates
        fit.setWeightFunc(WeightFunc(BaseFitField::params.m_scale));
        auto query = points.points()[pointId].pos();
        // Compute fit
        for (int iter = 0; iter != BaseFitField::params.m_iter; ++iter) {
            fit.init(query);
            if (fit.computeWithIds(points.range_neighbors(query, BaseFitField::params.m_scale), points.points()) ==
                Ponca::STABLE) {
                postProcess(fit);
                query = fit.project(query);
            }
            else{
                std::cerr << "MLS iteration failed" << std::endl;
            }
        }
    }
};

using OnePlaneFitField = OnePointFitField<ConstPlaneFit>;

struct OneSphereFitField : public OnePointFitField<ConstSphereFit>{
    void postProcess(typename OnePointFitField<ConstSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};

struct OneOrientedSphereFitField : public OnePointFitField<ConstOrientedSphereFit>{
    void postProcess(typename OnePointFitField<ConstOrientedSphereFit>::FitType& fit) override { fit.applyPrattNorm(); };
};
