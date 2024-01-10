#include "cli.h"

#include "dataManager.h"
#include "drawingPass.h"
//#include "drawingPasses/distanceField.h"

#include "argparse/argparse.hpp"


#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_MSC_VER)
#  pragma warning (disable: 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#include "stb_image_write.h"
#include <cmath> // floor

PoncaPlotCLI::PoncaPlotCLI(DataManager* mgr) : m_dataMgr(mgr){

}

bool
PoncaPlotCLI::run(int argc, char **argv) {

    struct {
        std::string inputPath {};
        struct {
            std::string name {"Oriented Sphere"};
            float scale {40};
        } fitting;
        struct {
            std::string path {};
            size_t width {500};
            size_t height {500};
        } output;
    } params;

    std::vector<std::string> names;
    names.resize(m_dataMgr->nbSupportedDrawingPasses);
    for (const auto& p : m_dataMgr->supportedDrawingPasses)
        names[p.second] = p.first;
    std::string namesStr;
    for (const auto& n : names)
        namesStr.append("\"" + n + "\" ");



    argparse::ArgumentParser program("poncaplot-cli");
    program.add_argument("-i", "--input")
            .required()
            .help("input file (.pts or .txt)");

    // output controls
    {
        program.add_argument("-o", "--output")
                .help("output file (image)");
        program.add_argument("-w", "--width")
                .help("output image width (in pixels)")
                .default_value(params.output.width);
        program.add_argument("-h", "--height")
                .help("output image height (in pixels)")
                .default_value(params.output.height);
    }

    // fitting controls
    {
        auto ft = program.add_argument("-f", "--fitType")
                .default_value(params.fitting.name)
                .help("fit type: [" + namesStr + "]");
        for (const auto& type : m_dataMgr->supportedDrawingPasses)
            ft.add_choice(type.first);

        program.add_argument("-s")
                .help("scale size (in pixels)")
                .scan<'g', float>()
                .default_value(params.fitting.scale);
    }

    // return value of the method: do we skip the GUI ?
    bool skipGUI = true;

    bool loaded = false;
    try {
        program.parse_args(argc, argv);
        params.inputPath = program.get("-i");
        if (! params.inputPath.empty())
        {
            loaded = m_dataMgr->loadPointCloud(params.inputPath);;

            // load fit properties
            if (program.is_used("-f")) params.fitting.name = program.get("-f");
            if (program.is_used("-s")) params.fitting.scale = program.get<float>("-s");

            // load output properties
            auto output = program.present("-o");
            if( output ) {
                params.output.path   = output.value();
                if (program.is_used("-w")) params.output.width   = program.get<int>("-w");
                if (program.is_used("-h")) params.output.height  = program.get<int>("-h");
            } else
                skipGUI = false; // no output is set: display GUI with parameters set
        }
    }
    catch (const std::exception& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        skipGUI = false;
    }

    // configure and do rendering
    if (loaded && skipGUI){
        auto texture = new float [params.output.width * params.output.height * 4];

        // configure fitting
        std::cout << "Configure fitting" << std::endl;
        auto pass = m_dataMgr->getDrawingPass(params.fitting.name);

        if( auto fit = dynamic_cast<BaseFitField*>(pass) ) {
            fit->m_scale = params.fitting.scale;
        }

        // configure renderer
        std::cout << "Configure renderer" << std::endl;
        std::array<DrawingPass*,3> renderPasses{
                new FillPass( {1,1,1,1})
                , pass
                , new ColorMap({1,1,1,1})
//                , new DisplayPoint({0,0,0,1})
        };

        // render
        std::cout << "Render" << std::endl;
        const auto& points = m_dataMgr->getKdTree();
        for (auto* p : renderPasses) {
            p->render(points, texture, int(params.output.width), int(params.output.height));
        }

        std::cout << "Save image" << std::endl;
        {
            auto buffer = new char [params.output.width * params.output.height * 4];
            std::transform(texture, texture + params.output.width * params.output.height * 4,
                           buffer,
                           [](float in) -> char{
                return char(std::floor(in*255.f));
            });
            stbi_write_png(params.output.path.c_str(), params.output.width, params.output.height,
                           4, buffer, params.output.width * 4);
            stbi_image_free(buffer);
        }

        delete[] texture;
    }

    return skipGUI;
}
