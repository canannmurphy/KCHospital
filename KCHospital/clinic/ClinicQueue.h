#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <ostream>
#include "utils/magic_enum.h"

using namespace std;

enum Status
{
    Unassigned,
    Assigned,
    Processed,
    Cancelled
};

struct Patient
{
    Status status = Unassigned;
    string assignedCoordinator;
    string patientID;
    string firstName;
    string lastName;
    string ssn;
    string clinic;
    bool isCritical;
    Patient *next;
    Patient(const string &first, const string &last, const string &ssn_, bool critical)
        : firstName(first), lastName(last), ssn(ssn_), isCritical(critical), next(nullptr)
    {
        string lowerFirst = first;
        string lowerLast = last;
        transform(lowerFirst.begin(), lowerFirst.end(), lowerFirst.begin(), ::tolower);
        transform(lowerLast.begin(), lowerLast.end(), lowerLast.begin(), ::tolower);
        patientID = lowerFirst + "_" + lowerLast + "_" + ssn_;
    }
};

class ClinicQueue {
private:
    Patient *head = nullptr;
    Patient *tail = nullptr;
    int size = 0;
    const int MAX_SIZE = 18;

public:
    ClinicQueue() = default;
    int getSize() const { return size; }

    vector<Patient *> getBatch(int batchNumber, int batchSize = 9) const
    {
        vector<Patient *> batch;
        int startIndex = (batchNumber - 1) * batchSize;
        int endIndex = startIndex + batchSize;
        int currentIndex = 0;

        Patient *current = head;
        while (current && currentIndex < endIndex)
        {
            if (currentIndex >= startIndex)
            {
                batch.push_back(current);
            }
            current = current->next;
            currentIndex++;
        }

        return batch;
    }

    vector<Patient *> getAllPatients() const
    {
        vector<Patient *> patients;
        Patient *current = head;
        while (current)
        {
            patients.push_back(current);
            current = current->next;
        }
        return patients;
    }

    bool addPatient(Patient *p)
    {
        if (size >= MAX_SIZE || !p)
            return false;

        if (!head)
        {
            head = tail = p;
        }
        else if (p->isCritical)
        {
            Patient *current = head;
            Patient *prev = nullptr;

            while (current && current->isCritical)
            {
                prev = current;
                current = current->next;
            }

            if (!prev)
            {
                // Insert at head
                p->next = head;
                head = p;
            }
            else
            {
                // Insert after last critical
                p->next = current;
                prev->next = p;
            }

            if (!p->next)
                tail = p;
        }
        else
        {
            // Append regular at the end
            tail->next = p;
            tail = p;
        }

        size++;
        return true;
    }

    Patient *assignNextAvailable(const string &coordinator)
    {
        Patient *current = head;
        while (current)
        {
            if (current->status == Unassigned)
            {
                current->status = Assigned;
                current->assignedCoordinator = coordinator;
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    bool processPatient(const string &patientID, const string &coordinator)
    {
        Patient *current = head;
        while (current)
        {
            if (current->patientID == patientID)
            {
                if (current->status == Assigned && current->assignedCoordinator == coordinator)
                {
                    current->status = Status::Processed;
                    return true;
                }
                else
                {
                    return false; // not assigned to this coordinator or already processed
                }
            }
            current = current->next;
        }
        return false; // not found
    }

    bool cancelPatient(const string &patientID, const string &coordinator)
    {
        Patient *current = head;
        while (current)
        {
            if (current->patientID == patientID)
            {
                if (current->status == Assigned && current->assignedCoordinator == coordinator)
                {
                    current->status = Status::Cancelled;
                    return true;
                }
                else
                {
                    return false; // not assigned to this coordinator or already handled
                }
            }
            current = current->next;
        }
        return false; // not found
    }

    Patient* removeAndProcessPatient(const string &patientID, const string &coordinator) {
        Patient *current = head;
        Patient *prev = nullptr;

        while (current) {
            if (current->patientID == patientID &&
                current->status == Assigned &&
                current->assignedCoordinator == coordinator) {

                if (prev) {
                    prev->next = current->next;
                } else {
                    head = current->next;
                }
                if (current == tail) {
                    tail = prev;
                }
                size--;
                return current;
            }
            prev = current;
            current = current->next;
        }
        return nullptr;
    }

    Patient* removeAndCancelPatient(const string &patientID, const string &coordinator) {
        Patient *current = head;
        Patient *prev = nullptr;

        while (current) {
            if (current->patientID == patientID &&
                current->status == Assigned &&
                current->assignedCoordinator == coordinator) {

                if (prev) {
                    prev->next = current->next;
                } else {
                    head = current->next;
                }
                if (current == tail) {
                    tail = prev;
                }
                size--;
                return current;
            }
            prev = current;
            current = current->next;
        }
        return nullptr;
    }

    Patient *findPatient(const string &patientID)
    {
        Patient *current = head;
        while (current)
        {
            if (current->patientID == patientID)
            {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    void printAll(ostream &out, const Status *filter = nullptr) const
    {
        Patient *current = head;
        while (current)
        {
            if (!filter || current->status == *filter)
            {
                out << (current->isCritical ? "[CRITICAL] " : "[REGULAR] ")
                    << current->firstName << " " << current->lastName
                    << " (SSN: " << current->ssn << ", ID: " << current->patientID << ")"
                    << " - Status: " << magic_enum::enum_name(current->status) << "\n";
            }
            current = current->next;
        }
    }
};
