/*
    src/example1.cpp -- C++ version of an example application that shows
    how to use the various widget classes. For a Python implementation, see
    '../python/example1.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <iostream>

#include "dataManager.h"
#include "application.h"
#include "cli.h"

DataManager *mgr {nullptr};

void clean(){
    if (mgr){
        delete mgr;
        mgr = nullptr;
    }
}

int main(int argc , char ** argv) {
    mgr = new DataManager();

    PoncaPlotCLI cli(mgr);

    if (! cli.run(argc, argv)) {
        std::cout << "CLI does not want to run: launching graphic app" << std::endl;
        try {
            nanogui::init();

            /* scoped variables */ {
                nanogui::ref<PoncaPlotApplication> app = new PoncaPlotApplication(mgr);
                app->dec_ref();
                app->draw_all();
                app->set_visible(true);
                nanogui::mainloop(1 / 10.f * 1000);
            }

            nanogui::shutdown();
        } catch (const std::exception &e) {
            std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
            std::cerr << error_msg << std::endl;
            clean();
            return -1;
        } catch (...) {
            std::cerr << "Caught an unknown error!" << std::endl;
        }
    }

    clean();
    return 0;
}
