#ifndef BEDS_H
#define BEDS_H
#include "patient.h"

#define NUM_WARDS 3
#define MAX_BEDS  12
enum { WARD_GENERAL = 0, WARD_EMERGENCY = 1, WARD_ICU = 2 };

/* Linked-list queue of patients waiting for a bed in a particular ward. */
typedef struct WaitNode {
    int patient_id;
    int ward;
    struct WaitNode *next;
} WaitNode;

typedef struct {
    int total[NUM_WARDS];
    int occupant[NUM_WARDS][MAX_BEDS];   /* patient id, 0 = free bed */
    WaitNode *head, *tail;
    int waiting;
} BedSystem;

const char *beds_ward_name(int ward);
void beds_init(BedSystem *b);
void beds_free(BedSystem *b);
int  beds_free_count(const BedSystem *b, int ward);
int  beds_first_free(const BedSystem *b, int ward);        /* -1 if full */
int  beds_occupant(const BedSystem *b, int ward, int bed);
int  beds_set(BedSystem *b, int ward, int bed, int patient_id);  /* 1 if placed */
void beds_clear(BedSystem *b, int ward, int bed);
void beds_enqueue_back(BedSystem *b, int patient_id, int ward);
void beds_enqueue_front(BedSystem *b, int patient_id, int ward);
int  beds_queue_remove(BedSystem *b, int patient_id);
int  beds_queue_first_for_ward(const BedSystem *b, int ward);   /* 0 if none */
void beds_print(const BedSystem *b, const HashTable *ht);

#endif
