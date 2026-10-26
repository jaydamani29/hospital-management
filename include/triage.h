#ifndef TRIAGE_H
#define TRIAGE_H
#include "patient.h"

/* Max-heap: highest severity first; ties -> earlier arrival first. */
typedef struct {
    int  patient_id;
    int  severity;
    long seq;
} TriageEntry;

typedef struct {
    TriageEntry *a;
    int size;
    int cap;
} TriageHeap;

void th_init(TriageHeap *h);
void th_free(TriageHeap *h);
int  th_push(TriageHeap *h, TriageEntry e);
int  th_pop(TriageHeap *h, TriageEntry *out);
int  th_peek(const TriageHeap *h, TriageEntry *out);
int  th_remove_patient(TriageHeap *h, int patient_id);
int  th_contains(const TriageHeap *h, int patient_id);
void th_print(const TriageHeap *h, const HashTable *ht);

#endif
