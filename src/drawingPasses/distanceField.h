#pragma once
#include "../drawingPass.h"


struct DistanceField : public DrawingPass {
    inline explicit DistanceField() : DrawingPass() {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override {
        if(points.point_data().empty()) return;

        const auto normFactor = float(std::max(w,h));
#pragma omp parallel for collapse(2) default(none) shared(normFactor, points, buffer, w, h)
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                auto minDist = int(normFactor); //distance should necessarily be smaller
                for (const auto &p : points.point_data()) {
                    int u(std::floor(p.pos().x()));
                    int v(std::floor(p.pos().y()));
                    auto dist = int(std::sqrt((i-u)*(i-u) + (j-v)*(j-v)));
                    minDist = std::min(dist, minDist);
                }
                auto col = int(255. * minDist / normFactor);
                b[0] = b[1] = b[2] = col;
                b[3] = 255;
            }
        }
    }
};
struct DistanceFieldWithKdTree : public DrawingPass {
    inline explicit DistanceFieldWithKdTree() : DrawingPass() {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        if(points.point_data().empty()) return;

        const auto normFactor = float(std::max(w,h));
#pragma omp parallel for collapse(2) default(none) shared(normFactor, points, buffer, w, h)
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                DataPoint::VectorType query (i, j);
                auto res = points.nearest_neighbor( query );
                if(res.begin()!=res.end()) {
                    auto nei = points.point_data()[res.get()].pos();
                    float dist = (nei-query).norm();
                    auto col = int (255. * dist / normFactor);
                    b[0] = b[1] = b[2] = col;
                    b[3] = 255;
                }
            }
        }
    }
};
