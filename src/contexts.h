//
// Created by Nicolas Mellado on 16/04/2025.
//

#pragma once

struct RenderingContext {
    size_t w {0};
    size_t h {0};
    /// Scale factor applied to the point coordinates
    float scale {1};

    /// Convert distance from pixel to point space
    [[nodiscard]] inline float pixToPoint(int i) const
    { return this->scale * i;}
    /// Convert texture pixel coordinate to point space coordinage
    [[nodiscard]] inline std::pair<float, float>pixToPoint(int i, int j) const
    { return {pixToPoint(i), pixToPoint(j)};}
    /// Convert distance from point to pixel space
    [[nodiscard]] inline int pointToPix(float x) const
    { return x / this->scale;}
    /// Convert point coordinates to the texture pixel space
    [[nodiscard]] inline std::pair<int, int>pointToPix(float x, float y) const
    { return {pointToPix(x), pointToPix(y)};}
    /// Convert point coordinates to the texture pixel space
    template<typename vec2>
    [[nodiscard]] inline std::pair<int, int>pointToPix(vec2 p) const
    { return {pointToPix(p.x()), pointToPix(p.y())};}

};

