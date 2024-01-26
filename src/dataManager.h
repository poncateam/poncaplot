#pragma once

#include <optional>
#include <utility> //pair
#include <vector>
#include <string>
#include <iostream>

#include <nanogui/vector.h>

#include <Ponca/SpatialPartitioning>

#include "poncaTypes.h"


#ifndef M_PI
// Source: http://www.geom.uiuc.edu/~huberty/math5337/groupe/digits.html
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944592307816406
#endif
#define DEFAULT_POINT_ANGLE M_PI / 2.

template <typename NodeIndex, typename Scalar, int DIM, typename _AabbType = Eigen::AlignedBox<Scalar, DIM>>
struct MyKdTreeInnerNode : public Ponca::KdTreeDefaultInnerNode<NodeIndex, Scalar, DIM> {
    using AabbType = _AabbType;
    AabbType m_aabb{};
};
//
//template <typename Index, typename NodeIndex, typename DataPoint, typename LeafSize = Index>
//using MyKdTreeNode = Ponca::KdTreeCustomizableNode<Index, NodeIndex, DataPoint, LeafSize,
//        MyKdTreeInnerNode<NodeIndex, typename DataPoint::Scalar, DataPoint::Dim> >;

template <typename Index, typename NodeIndex, typename DataPoint, typename LeafSize = Index>
struct MyKdTreeNode : Ponca::KdTreeCustomizableNode<Index, NodeIndex, DataPoint, LeafSize,
        MyKdTreeInnerNode<NodeIndex, typename DataPoint::Scalar, DataPoint::Dim>> {

    using Base = Ponca::KdTreeCustomizableNode<Index, NodeIndex, DataPoint, LeafSize,
            MyKdTreeInnerNode<NodeIndex, typename DataPoint::Scalar, DataPoint::Dim>>;
    using AabbType  = typename Base::AabbType;

    void configure_range(Index start, Index size, const AabbType &aabb)
    {
        Base::configure_range(start, size, aabb);
        if (! Base::is_leaf() )
        {
            Base::getAsInner().m_aabb = aabb;
        }
    }
    [[nodiscard]] inline std::optional<AabbType> getAabb() const {
        if (! Base::is_leaf())
            return Base::getAsInner().m_aabb;
        else
            return std::optional<AabbType>();
    }
};

/// Structure holding shared data
struct DataManager {
public:
//    using KdTree = Ponca::KdTree<DataPoint>;
    using KdTree = Ponca::KdTreeDenseBase<Ponca::KdTreeDefaultTraits<DataPoint,MyKdTreeNode>>;
    using PointContainer  = std::vector<nanogui::Vector3f>; // stores x,y,normal angle in radians
    using VectorType = typename KdTree::VectorType;

    /// Read access to point collection
    inline const KdTree& getKdTree() const { return m_tree; }

    /// Update point collection from point container
    inline void updateKdTree() {
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
    inline void setKdTreePostUpdateFunction(std::function<void()> &&f) { m_updateFunction = f; }

    /// IO: save current point cloud to file
    bool savePointCloud(const std::string& path) const;

    /// IO: load point cloud from file
    bool loadPointCloud(const std::string& path);

    /// Utils: fit point cloud to coordinates ranges
    void fitPointCloudToRange(const std::pair<float,float>& rangesEnd,
                              const std::pair<float,float>& rangesStart = {0,0});

    /// Utils: compute normals using covariance plane fit
    /// \param Number of neighbors (3 means current point and 2 closest points: left and right)
    void computeNormals(int k = 3);

private:
    PointContainer m_points;
    KdTree m_tree;
    std::function<void()> m_updateFunction {[](){}};
};

