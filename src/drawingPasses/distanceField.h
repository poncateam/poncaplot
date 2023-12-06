#pragma once
#include "../drawingPass.h"


struct DistanceField : public DrawingPass {
    inline explicit DistanceField() : DrawingPass() {}
    void render(const MyView::PointCollection& points, uint8_t*buffer, int w, int h) override{
        double normFactor (std::max(w,h)/2);
        for (int j = 0; j < h; ++j ) {
            for (int i = 0; i < w; ++i) {
                auto *b = buffer + (i + j * w) * 4;
                int minDist = normFactor; //distance should necessarily be smaller
                for (const auto &p: points) {
                    int u(std::floor(p.x()));
                    int v(std::floor(p.y()));
                    auto dist = int(std::sqrt((i-u)*(i-u) + (j-v)*(j-v)));
                    minDist = std::min(dist, minDist);
                }
                auto col = uint (255. * minDist / normFactor);
                b[0] = b[1] = b[2] = col;
                b[3] = 255;
            }
        }
    }
};
