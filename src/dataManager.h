#pragma once

#include <map>
#include <optional>
#include <utility> //pair
#include <vector>
#include <string>
#include <iostream>

#include <nanogui/vector.h>

#include <Ponca/SpatialPartitioning>

#include "poncaTypes.h"
#include "drawingPasses/distanceField.h"
#include "drawingPasses/poncaFitField.h"


#ifndef M_PI
// Source: http://www.geom.uiuc.edu/~huberty/math5337/groupe/digits.html
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944592307816406
#endif
#define DEFAULT_POINT_ANGLE M_PI / 2.


struct DrawingPass;

/// Structure holding shared data
struct DataManager {
public:
//    using KdTree = Ponca::KdTree<DataPoint>;
    using KdTree = Ponca::KdTreeDenseBase<Ponca::KdTreeDefaultTraits<DataPoint,MyKdTreeNode>>;
    using PointContainer  = std::vector<nanogui::Vector3f>; // stores x,y,normal angle in radians
    using VectorType = typename KdTree::VectorType;

    DataManager();
    ~DataManager();

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

    /// Names of the supported drawing passes
    static constexpr size_t nbSupportedDrawingPasses = 5;
    const std::map<const std::string, size_t> supportedDrawingPasses {
                    {"Distance Field", 0},
                    {"Plane", 1},
                    {"Sphere", 2},
                    {"Oriented Sphere", 3},
                    {"Unoriented Sphere", 4}
            };

    DrawingPass* getDrawingPass(const std::string& name);

    /// Build a a drawing pass
    /// \param index of the pass name in supportedDrawingPasses
    DrawingPass* getDrawingPass(size_t index);

    template <typename Functor>
    bool processPass(int index, Functor f) {
        switch (index) {
            case 0: //Distance Field
                f(dynamic_cast<DistanceFieldWithKdTree*>(getDrawingPass(0)));
                break;
            case 1: // Plane
                f(dynamic_cast<PlaneFitField*>(getDrawingPass(1)));
                break;
            case 2: // Sphere
                f(dynamic_cast<SphereFitField*>(getDrawingPass(2)));
                break;
            case 3: // Oriented Sphere
                f(dynamic_cast<OrientedSphereFitField*>(getDrawingPass(3)));
                break;
            case 4: // Unoriented Sphere
                f(dynamic_cast<UnorientedSphereFitField*>(getDrawingPass(4)));
                break;
            default:
                return false;
        }
        return true;
    }


private:
    PointContainer m_points;
    KdTree m_tree;
    std::function<void()> m_updateFunction {[](){}};

    std::array<DrawingPass*,nbSupportedDrawingPasses> m_drawingPasses;
};

