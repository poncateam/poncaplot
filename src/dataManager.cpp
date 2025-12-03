#include "dataManager.h"

#include <iostream>
#include <fstream>

DataManager::DataManager() {
    m_drawingPasses.fill(nullptr);
}

DataManager::~DataManager() {
    for (auto* p : m_drawingPasses)
        delete p;
}

bool
DataManager::savePointCloud(const std::string& path) const{
    if( path.empty() ) return false;

    std::ofstream file;
    file.open (path);

    if( ! file.is_open() ) return false;

    file << "# x y nx ny " << "\n";
    for( const auto & p : m_tree.points() ){
        file << p.pos().transpose() << " " << p.normal().transpose() << "\n";
    }
    file.close();
    return true;
}
bool
DataManager::loadPointCloud(const std::string& path){
    if( path.empty() ) return false;

    std::ifstream file;
    file.open (path);

    if( ! file.is_open() ) return false;

    m_points.clear();

    std::string line;
    std::vector<float> numbers;

    bool needToComputeNormals = false;

    while ( getline (file,line) )
    {
        // trim comments
        std::size_t found = line.find('#');
        if (found!=std::string::npos)
            line = line.substr(0,found);

        if ( ! line.empty() ) {
            numbers.clear();
            std::istringstream is(line);
            numbers.assign(std::istream_iterator<float>(is), std::istream_iterator<float>());

            if (numbers.size() == 2){ // loaded x-y only, set normal to default value
                m_points.emplace_back(numbers[0], numbers[1], DEFAULT_POINT_ANGLE);
                needToComputeNormals = true;
            } else if (numbers.size() == 4){ // loaded x-y only, set normal to default value
                m_points.emplace_back(numbers[0], numbers[1], std::atan2(numbers[3], numbers[2]));
            } else { // malformed line
                std::cerr << "Skipping malformed line: ["  << line << "]" << std::endl;
            }
        }
    }
    file.close();
    updateKdTree();

    // Use plane fit to compute unoriented normals. Use knn and constant weights
    if(needToComputeNormals) computeNormals();

    return true;
}

void
DataManager::computeNormals(int k){
    using WeightFunc = Ponca::DistWeightFunc<DataPoint,Ponca::ConstantWeightKernel<typename DataPoint::Scalar> >;
    using PlaneFit = Ponca::Basket<DataPoint ,WeightFunc, Ponca::CovariancePlaneFit>;

    std::cout << "Recompute normals" << std::endl;

    for (auto& pp : m_points){
        VectorType p {pp.x(),pp.y()};
        PlaneFit fit;
        fit.setWeightFunc({p});
        // Set the evaluation position
        fit.init();
        // Fit plane (method compute handles multipass fitting
        if (fit.computeWithIds(m_tree.k_nearest_neighbors(p, k), m_tree.points()) == Ponca::STABLE) {
            pp.z() = std::acos(fit.primitiveGradient().normalized().x());
        } else
            std::cerr << "Something weird happened here..." << std::endl;
    }
    updateKdTree();
}

void
DataManager::fitPointCloudToRange(const std::pair<float,float>& rangesEnd, const std::pair<float,float>& rangesStart){
    if (m_points.empty()) return;
    if (m_tree.node_count() == 0) updateKdTree();
    auto aabb = m_tree.nodes()[0].getAabb();
    if (aabb){
        VectorType requestedSize {rangesEnd.first - rangesStart.first, rangesEnd.second - rangesStart.second};
        VectorType scaleFactors = requestedSize.array() / aabb->diagonal().array();
        float scale = scaleFactors.minCoeff();
        for (auto& p : m_points)
        {
            p.x() *= scale;
            p.y() *= scale;
        }
        updateKdTree();
    }
}

DrawingPass*
DataManager::getDrawingPass(const std::string& name){
    return getDrawingPass(supportedDrawingPasses.at(name));
}


#define WRITE_NEW_FIT_CASE(ID,FTYPE) \
    case ID:                     \
        *p = new FTYPE();\
        break;

DrawingPass*
DataManager::getDrawingPass(size_t index){
    if (index >= nbSupportedDrawingPasses) return nullptr;

    DrawingPass** p = &(m_drawingPasses[index]);
    if((*p) == nullptr) {
        switch (index) {
            WRITE_NEW_FIT_CASE(0,DistanceFieldWithKdTree)
            WRITE_NEW_FIT_CASE(1,PlaneFitField)
            WRITE_NEW_FIT_CASE(2,SphereFitField)
            WRITE_NEW_FIT_CASE(3,OrientedSphereFitField)
            WRITE_NEW_FIT_CASE(4,UnorientedSphereFitField)
            WRITE_NEW_FIT_CASE(5,BestPlaneFitField)
            WRITE_NEW_FIT_CASE(6,BestSphereFitField)
            WRITE_NEW_FIT_CASE(7,BestOrientedSphereFitField)
            WRITE_NEW_FIT_CASE(8,OnePlaneFitField)
            WRITE_NEW_FIT_CASE(9,OneSphereFitField)
            WRITE_NEW_FIT_CASE(10,OneOrientedSphereFitField)
            WRITE_NEW_FIT_CASE(11,DistanceFieldFromOnePoint)

            default: throw std::runtime_error("Unknown Field type!");
        }
    }
    return *p;
}

#undef WRITE_NEW_FIT_CASE
