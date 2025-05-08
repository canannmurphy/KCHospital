#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include "../utils/magic_enum.h"

using namespace std;

enum ActionType
{
    added,
    processed,
    cancelled
};

struct Transaction
{
    ActionType action;
    string timestamp;
    string coordinator;
    string clinicName;
    string patientID;
    string ssn;
    bool isCritical;

    void print(ofstream &out) const
    {
        out << "[" << timestamp << "] "
            << coordinator << " " << magic_enum::enum_name(action) << " "
            << (isCritical ? "CRITICAL " : "")
            << "patient " << patientID
            << " (SSN: " << ssn << ") in " << clinicName << ".\n";
    }
};

inline void logTransaction(const Transaction &t, const std::string &logFile = "reports/transaction.txt")
{
    std::ofstream out(logFile, std::ios::app);
    if (out.is_open())
    {
        t.print(out);
        out.close();
    }
    else
    {
        std::cerr << "Unable to open transaction log file.\n";
    }
}