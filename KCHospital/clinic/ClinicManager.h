#pragma once
#include <unordered_map>
#include "transactionLogger.h"
#include "ClinicQueue.h"
#include "../utils/utils.h"

using namespace std;

class ClinicManager
{
private:
    unordered_map<string, ClinicQueue> clinics;
    unordered_map<std::string, Patient*> coordinatorToPatient;

    void recordTransaction(const string &clinicCode, const string &coordinator, const Patient *patient, ActionType action)
    {

        Transaction t;
        t.timestamp = getCurrentTimestamp(); // assumes utility function
        t.coordinator = coordinator;
        t.action = action;
        t.clinicName = clinicCode;
        t.patientID = patient->patientID;
        t.ssn = patient->ssn;
        t.isCritical = patient->isCritical;

        logTransaction(t); // assumes defined in utils
    }

public:
    ClinicManager()
    {
        clinics.emplace("Heart", ClinicQueue());
        clinics.emplace("Pulmonary", ClinicQueue());
        clinics.emplace("Plastic", ClinicQueue());
    }

    std::vector<Patient*> getClinicBatch(const std::string& code, int batchNumber, int batchSize = 9) const {
        std::cout << "Clinic passed " << code;
        auto it = clinics.find(code);
        if (it != clinics.end()) {
            return it->second.getBatch(batchNumber, batchSize);
        }
        return {};
    }

    // Search for patients by first and last name across all clinics.
    std::vector<Patient*> searchPatients(const std::string& firstName, const std::string& lastName) const {
        std::vector<Patient*> matches;
        for (const auto& [clinicName, queue] : clinics) {
            const auto& all = queue.getAllPatients();
            for (Patient* p : all) {
                if (p->firstName == firstName && p->lastName == lastName) {
                    matches.push_back(p);
                }
            }
        }
        return matches;
    }

    const unordered_map<string, ClinicQueue>& getAllClinics() const {
        return clinics;
    }

    void addClinic(const string &code)
    {
        clinics.emplace(code, ClinicQueue());
    }

    bool addPatientTo(const string &code, Patient *p, const string &coordinator = "SYSTEM")
    {
        for (auto& [code, queue] : clinics) {
            // Check for duplicate patientID
            if (queue.findPatient(p->patientID)) {
                std::cerr << "⚠️ Duplicate patient: " << p->patientID << " already exists in " << code << ".\n";
                return false;
            }
        }

        auto it = clinics.find(code);
        if (it != clinics.end()) {
            // Check if clinic is full
            if (it->second.getSize() >= 10) {
                std::cerr << "⚠️ Cannot add patient: " << code << " clinic is at full capacity.\n";
                return false;
            }

            p->clinic = code;

            if (it->second.addPatient(p)) {
                if (coordinator != "SYSTEM") {
                    recordTransaction(code, coordinator, p, added);
                }
                return true;
            }
        }
        return false;
    }

    //function to return a clinics state object that holds size
    std::unordered_map<std::string, int> getClinicSizes() const {
        std::unordered_map<std::string, int> result;
        for (const auto& [code, queue] : clinics) {
            result[code] = queue.getSize();
        }
        return result;
    }

    Patient *assignPatientFrom(const string &code, const string &coordinator)
    {
        auto it = clinics.find(code);
        if (it != clinics.end())
        {
            return it->second.assignNextAvailable(coordinator);
        }
        return nullptr;
    }

    bool processAssignedPatient(const std::string& coordinator) {
        Patient* patient = getAssignedPatient(coordinator);
        if (!patient) return false;
    
        std::string clinic = patient->clinic;
        auto it = clinics.find(clinic);
        if (it == clinics.end()) return false;
    
        bool result = it->second.removeAndProcessPatient(coordinator);
        if (result) {
            recordTransaction(clinic, coordinator, patient, processed);
            coordinatorToPatient.erase(coordinator);
        }
        return result;
    }
    
    bool cancelAssignedPatient(const std::string& coordinator) {
        Patient* patient = getAssignedPatient(coordinator);
        if (!patient) return false;
    
        std::string clinic = patient->clinic;
        auto it = clinics.find(clinic);
        if (it == clinics.end()) return false;
    
        bool result = it->second.removeAndCancelPatient(coordinator);
        if (result) {
            recordTransaction(clinic, coordinator, patient, cancelled);
            coordinatorToPatient.erase(coordinator);
        }
        return result;
    }

    void printClinicPatients(const string &code, ostream &out, const Status *filter = nullptr) const
    {
        auto it = clinics.find(code);
        if (it != clinics.end())
        {
            it->second.printAll(out, filter);
        }
        else
        {
            out << "Clinic not found.\n";
        }
    }

    Patient* getAssignedPatient(const string& coordinator) const {
        auto it = coordinatorToPatient.find(coordinator);
        return (it != coordinatorToPatient.end()) ? it->second : nullptr;
    }

    void releaseCoordinator(const string& name) {
        auto it = coordinatorToPatient.find(name);
        if (it != coordinatorToPatient.end()) {
            Patient* p = it->second;
            p->status = Unassigned;
            p->assignedCoordinator.clear();
            coordinatorToPatient.erase(it);
        }
    }

    void trackAssignment(const std::string& coordinator, Patient* p) {
        if (p) {
            p->status = Assigned;
            p->assignedCoordinator = coordinator;
            coordinatorToPatient[coordinator] = p;
        }
    }
};

inline void loadClinicFromCSV(const std::string &filename, const std::string &clinicName, ClinicManager &manager)
{
    std::vector<std::unordered_map<std::string, std::string>> records;
    static bool seeded = false;
    if (!seeded)
    {
        srand(time(nullptr));
        seeded = true;
    }

    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::vector<std::string> headers;

    if (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string header;
        while (std::getline(ss, header, ','))
        {
            headers.push_back(header);
        }
    }

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string cell;
        std::unordered_map<std::string, std::string> row;
        size_t col = 0;

        while (std::getline(ss, cell, ',') && col < headers.size())
        {
            row[headers[col]] = cell;
            ++col;
        }
        records.push_back(row);
    }
    file.close();

    for (const auto &row : records)
    {
        try
        {
            std::string first = row.at("firstName");
            std::string last = row.at("lastName");
            std::string ssn = row.at("ssn");

            bool isCritical = (rand() % 4 == 0); // roughly 25% chance

            Patient *p = new Patient(first, last, ssn, isCritical);
            manager.addPatientTo(clinicName, p);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error loading patient from row: " << e.what() << std::endl;
        }
    }
}
