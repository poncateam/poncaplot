#pragma once

#include <vector>

#include <nanogui/vector.h>

#include <Ponca/SpatialPartitioning>

class DataPoint
{
public:
    enum {Dim = 2};
    using Scalar = float;
    using VectorType = Eigen::Vector<Scalar,Dim>;
    using MatrixType = Eigen::Matrix<Scalar,Dim,Dim>;
    [[nodiscard]] inline const auto& pos() const {return m_pos;}
    [[nodiscard]] inline const auto& normal() const {return m_normal;}
    /// \fixme Use maps to avoid duplication
    explicit inline DataPoint(const nanogui::Vector3f &pn)
            : m_pos({pn.x(), pn.y()}), m_normal({std::cos(pn.z()), std::sin(pn.z())}) {}
private:
    VectorType m_pos, m_normal;
};

/// Structure holding shared data
struct DataManager {
public:
    using PointCollection = Ponca::KdTree<DataPoint>;
    using PointContainer  = std::vector<nanogui::Vector3f>; // stores x,y,normal angle in radians

    /// Read access to point collection
    inline const PointCollection& getPointCollection() const { return m_tree; }

    /// Update point collection from point container
    inline void updatePointCollection() {
        if(m_points.empty()) m_tree.clear();
        else m_tree.build(m_points );
        m_updateFunction();
    }

    /// Read access to point container
    inline const PointContainer& getPointContainer() const { return m_points; }

    /// Read access to point container
    /// \warning
    inline PointContainer& getPointContainer() { return m_points; }

    /// Set Update function, called after each point update
    inline void setUpdateFunction(std::function<void()> &&f) { m_updateFunction = f; }

private:
    PointContainer m_points;
    PointCollection m_tree;
    std::function<void()> m_updateFunction {[](){}};
};

