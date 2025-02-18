#pragma once

#include <Ponca/Fitting>

#include <optional>

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

using WeightFunc = Ponca::DistWeightFunc<DataPoint,Ponca::SmoothWeightKernel<typename DataPoint::Scalar> >;
using ConstWeightFunc = Ponca::DistWeightFunc<DataPoint,Ponca::ConstantWeightKernel<typename DataPoint::Scalar> >;

using PlaneFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::CovariancePlaneFit>;
using ConstPlaneFit = Ponca::Basket<DataPoint ,ConstWeightFunc, Ponca::CovariancePlaneFit>;
using SphereFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::SphereFit>;
using ConstSphereFit = Ponca::Basket<DataPoint ,ConstWeightFunc, Ponca::SphereFit>;
using OrientedSphereFit= Ponca::Basket<DataPoint ,WeightFunc, Ponca::OrientedSphereFit>;
using ConstOrientedSphereFit= Ponca::Basket<DataPoint ,ConstWeightFunc, Ponca::OrientedSphereFit>;
using UnorientedSphereFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::UnorientedSphereFit>;




#include <Ponca/SpatialPartitioning>

template <typename NodeIndex, typename Scalar, int DIM, typename _AabbType = Eigen::AlignedBox<Scalar, DIM>>
struct MyKdTreeInnerNode : public Ponca::KdTreeDefaultInnerNode<NodeIndex, Scalar, DIM> {
    using AabbType = _AabbType;
    AabbType m_aabb{};
};

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

using KdTree = Ponca::KdTreeBase<Ponca::KdTreeDefaultTraits<DataPoint,MyKdTreeNode>>;
