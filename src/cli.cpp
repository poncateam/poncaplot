#include "cli.h"

#include "myview.h"
#include "dataManager.h"
#include "drawingPass.h"
#include "drawingPasses/distanceField.h"

#include "argparse/argparse.hpp"



PoncaPlotCLI::PoncaPlotCLI(DataManager* mgr) : m_dataMgr(mgr){

}

bool
PoncaPlotCLI::run(int argc, char **argv) {

    std::vector<std::string> names;
    names.resize(m_dataMgr->nbSupportedDrawingPasses);
    for (const auto& p : m_dataMgr->supportedDrawingPasses)
        names[p.second] = p.first;
    std::string namesStr;
    for (const auto& n : names)
        namesStr.append("\"" + n + "\" ");



    argparse::ArgumentParser program("poncaplot-cli");

    program.add_argument("scale")
            .help("scale size (in pixels)")
            .scan<'g', float>()
            .default_value(10);
    program.add_argument("-i", "--input")
            .required()
            .help("input file (.pts or .txt)");
    program.add_argument("-o", "--output")
            .help("output file (image)");
    program.add_argument("-f", "--fitType")
            .help("fit type: [" + namesStr + "]");

    try {
        program.parse_args(argc, argv);
        auto inputPath = program.get("-i");
        if (! inputPath.empty())
        {
            m_dataMgr->loadPointCloud(inputPath);
            // load other properties
            if( ! program.is_used("-o")) return false; // be sure that GUI is started.
        }
    }
    catch (const std::exception& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        return false;
    }

    // do processing
    return true;
}
