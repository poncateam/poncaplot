#pragma once
#include "../drawingPass.h"


struct DistanceField : public DrawingPass {
    inline explicit DistanceField() : DrawingPass() {}
    void render(const DataManager::KdTree& points, float*buffer, int w, int h) override {
        if(points.point_data().empty())
        {
            buffer[1] = ColorMap::NO_FIELD;
            return;
        }

        float maxVal = 0;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, w, h) reduction(max : maxVal)
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                float minDist {float(w*h)};  //distance should necessarily be smaller
                for (const auto &p : points.point_data()) {
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
    void render(const DataManager::KdTree& points, float*buffer, int w, int h) override{
        if(points.point_data().empty())
        {
            buffer[1] = ColorMap::NO_FIELD;
            return;
        }

        float maxVal = 0;
#pragma omp parallel for collapse(2) default(none) shared(points, buffer, w, h) reduction(max : maxVal)
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                DataPoint::VectorType query (i, j);
                auto res = points.nearest_neighbor( query );
                if(res.begin()!=res.end()) {
                    auto nei = points.point_data()[res.get()].pos();
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
