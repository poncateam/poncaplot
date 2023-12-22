#pragma once

// forward declarations
class DataManager;

class PoncaPlotCLI {

public:
    PoncaPlotCLI(DataManager* mgr);
    bool run(int argc, char**argv);

private:
    float*  m_texture {nullptr};
    DataManager *m_dataMgr{nullptr};
};
