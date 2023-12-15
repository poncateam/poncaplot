#pragma once

#include <Ponca/Fitting>


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

using PlaneFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::CovariancePlaneFit>;
using SphereFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::SphereFit>;
using OrientedSphereFit= Ponca::Basket<DataPoint ,WeightFunc, Ponca::OrientedSphereFit>;
using UnorientedSphereFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::UnorientedSphereFit>;