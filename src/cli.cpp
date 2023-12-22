#include "cli.h"

#include "myview.h"
#include "dataManager.h"
#include "drawingPass.h"
#include "drawingPasses/distanceField.h"



PoncaPlotCLI::PoncaPlotCLI(DataManager* mgr) : m_dataMgr(mgr){

}

bool
PoncaPlotCLI::run(int argc, char **argv) {
    return false;
}
