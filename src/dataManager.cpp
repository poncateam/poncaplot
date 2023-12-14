#include "dataManager.h"

#include <iostream>
#include <fstream>

bool
DataManager::savePointCloud(const std::string& path) const{
    if( path.empty() ) return false;

    std::ofstream file;
    file.open (path);

    if( ! file.is_open() ) return false;

    file << "# x y nx ny " << "\n";
    for( const auto & p : m_tree.point_data() ){
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
            } else if (numbers.size() == 4){ // loaded x-y only, set normal to default value
                m_points.emplace_back(numbers[0], numbers[1], std::acos(numbers[2]));
            } else { // malformed line
                std::cerr << "Skipping malformed line: ["  << line << "]" << std::endl;
            }
        }
    }

    file.close();
    updateKdTree();
    return true;
}

void
DataManager::fitPointCloudToRange(const std::pair<float,float>& rangesEnd, const std::pair<float,float>& rangesStart){

}

