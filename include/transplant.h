#ifndef TRANSPLANT_H
#define TRANSPLANT_H
#include "patient.h"

enum { ORGAN_LIVER = 0, ORGAN_HEART = 1 };

/* priority = urgency * 10000 + days waiting   (urgency dominates, waiting breaks ties) */
typedef struct {
    int  patient_id;
    char name[NAME_LEN];
    char blood[4];
    int  organ;
    int  urgency;       /* liver: MELD 6..40, heart: urgency 1..10 */
    int  wait_days;
    long priority;
} WaitEntry;

typedef struct {
    WaitEntry *a;
    int size;
    int cap;
    int organ;
} OrganHeap;

const char *organ_name(int organ);
int         organ_viability_hours(int organ);     /* how long the organ stays usable */
const char *urgency_label(int organ);

void oh_init(OrganHeap *h, int organ);
void oh_free(OrganHeap *h);
int  oh_add(OrganHeap *h, const WaitEntry *e);
int  oh_contains(const OrganHeap *h, int patient_id);
int  oh_remove(OrganHeap *h, int patient_id);
int  oh_match(OrganHeap *h, const char *donor_blood, WaitEntry *out);
void oh_print(const OrganHeap *h);

#endif
