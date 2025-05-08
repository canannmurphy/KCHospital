#pragma once
#include "ClinicManager.h"
#include "../utils/utils.h"

inline ClinicManager initializeClinics()
{
    ClinicManager manager;
    loadClinicFromCSV("data/cleanData_Heart.csv", "Heart", manager);
    loadClinicFromCSV("data/cleanData_Pulmonary.csv", "Pulmonary", manager);
    loadClinicFromCSV("data/cleanData_Plastic.csv", "Plastic", manager);
    return manager;
}