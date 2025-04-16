#pragma once
#include "../drawingPass.h"


struct DistanceField : public DrawingPass {
    inline explicit DistanceField() : DrawingPass() {}
    void render(const KdTree& points, float*buffer, RenderingContext ctx) override {
        if(points.points().empty())
        {
            buffer[1] = ColorMap::NO_FIELD;
            return;
        }

        float maxVal = 0;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, ctx) reduction(max : maxVal)
        for (int j = 0; j < ctx.h; ++j ) {
            for (int i = 0; i < ctx.w; ++i) {
                auto *b = buffer + (i + j * ctx.w) * 4;
                float minDist {float(ctx.w*ctx.h)};  //distance should necessarily be smaller
                for (const auto &p : points.points()) {
                    int u(std::floor(p.pos().x()));
                    int v(std::floor(p.pos().y()));
                    auto dist = float(std::sqrt((i-u)*(i-u) + (j-v)*(j-v)));
                    minDist = std::min(dist, minDist);
                }
                b[0] = minDist;
                b[2] = ColorMap::VALUE_IS_VALID;
                b[3] = ColorMap::SCALAR_FIELD;
                if (std::abs(minDist)> maxVal) maxVal = std::abs(minDist);
            }
        }
        buffer[1] = maxVal;
    }
};
struct DistanceFieldWithKdTree : public DrawingPass {
    inline explicit DistanceFieldWithKdTree() : DrawingPass() {}
    void render(const KdTree& points, float*buffer, RenderingContext ctx) override{
        if(points.points().empty())
        {
            buffer[1] = ColorMap::NO_FIELD;
            return;
        }

        float maxVal = 0;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, ctx) reduction(max : maxVal)
        for (int j = 0; j < ctx.h; ++j ) {
            for (int i = 0; i < ctx.w; ++i) {
                auto *b = buffer + (i + j * ctx.w) * 4;
                DataPoint::VectorType query (i, j);
                auto res = points.nearest_neighbor( query );
                if(res.begin()!=res.end()) {
                    auto nei = points.points()[res.get()].pos();
                    float dist = (nei-query).norm();
                    b[0] = dist;
                    b[2] = ColorMap::VALUE_IS_VALID;
                    b[3] = ColorMap::SCALAR_FIELD;
                    if (std::abs(dist)> maxVal) maxVal = std::abs(dist);
                }
            }
        }
        buffer[1] = maxVal;
    }
};

// display distance field clamped by the current scale value
struct DistanceFieldFromOnePoint : public BaseFitField, public OnePointFitFieldBase {
    inline explicit DistanceFieldFromOnePoint() : BaseFitField(), OnePointFitFieldBase() {}
    void render(const KdTree& points, float*buffer, RenderingContext ctx) override {
        if(points.points().empty())
        {
            buffer[1] = ColorMap::NO_FIELD;
            return;
        }

#pragma omp parallel for collapse(2) default(none) shared(points, buffer, ctx)
        for (int j = 0; j < ctx.h; ++j ) {
            for (int i = 0; i < ctx.w; ++i) {
                auto *b = buffer + (i + j * ctx.w) * 4;

                auto p = points.points()[pointId].pos();

                int u(std::floor(p.x()));
                int v(std::floor(p.y()));
                auto dist = float(std::sqrt((i-u)*(i-u) + (j-v)*(j-v)));

                if(dist < params.m_scale) {
                    b[0] = dist;
                    b[2] = ColorMap::VALUE_IS_VALID;
                    b[3] = ColorMap::SCALAR_FIELD;
                }
                else {
                    b[2] = ColorMap::VALUE_IS_INVALID;
                }
            }
        }
        buffer[1] = params.m_scale;
        buffer[3] = ColorMap::SCALAR_FIELD;
    }
};
