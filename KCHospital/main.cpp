#include "clinic/initClinics.h"

using namespace std;
//DATA has been cleaned already through my engine

int main() {

    ClinicManager manager = initializeClinics();

   
    manager.addPatientTo("Heart", new Patient("Fisayo", "MyHomie", "999", true), "Canann");

    cout << "\n=== Heart Clinic ===\n";
    manager.printClinicPatients("Heart", cout);

    cout << "\n=== Pulmonary Clinic ===\n";
    manager.printClinicPatients("Pulmonary", cout);

    cout << "\n=== Plastic Clinic ===\n";
    manager.printClinicPatients("Plastic", cout);

    return 0;
}