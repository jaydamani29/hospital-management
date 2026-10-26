#ifndef HOSPITAL_H
#define HOSPITAL_H

#include "patient.h"
#include "triage.h"
#include "beds.h"
#include "undo.h"
#include "transplant.h"
#include "nurse.h"
#include "ambulance.h"

/* One global object that owns every data structure. */
typedef struct {
    HashTable   patients;
    TriageHeap  triage;
    BedSystem   beds;
    UndoStack   undo;
    OrganHeap   liver;
    OrganHeap   heart;
    NurseSystem nurses;
    CityGraph   city;
    long        seq;
    int         closed;
} Hospital;

extern Hospital H;

void hosp_init(void);
void hosp_shutdown(void);            /* saves to disk and frees memory (safe to call twice) */

int  hosp_register(const char *name, int age, const char *blood, int severity, const char *complaint);
void hosp_call_next(void);
void hosp_admit(int patient_id, int ward);
void hosp_discharge(int patient_id);
void hosp_undo(void);
void hosp_change_severity(int patient_id, int severity);
void hosp_remove_from_triage(int patient_id);
void hosp_delete_record(int patient_id);
void hosp_load_demo(void);

#endif
