#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beds.h"

static const char *WARD_NAMES[NUM_WARDS] = {"General", "Emergency", "ICU"};
static const int   WARD_BEDS[NUM_WARDS]  = {10, 6, 4};

const char *beds_ward_name(int ward) {
    return (ward >= 0 && ward < NUM_WARDS) ? WARD_NAMES[ward] : "?";
}

void beds_init(BedSystem *b) {
    memset(b->occupant, 0, sizeof b->occupant);
    for (int w = 0; w < NUM_WARDS; w++) b->total[w] = WARD_BEDS[w];
    b->head = b->tail = NULL;
    b->waiting = 0;
}

void beds_free(BedSystem *b) {
    WaitNode *n = b->head;
    while (n) {
        WaitNode *nx = n->next;
        free(n);
        n = nx;
    }
    b->head = b->tail = NULL;
    b->waiting = 0;
}

int beds_first_free(const BedSystem *b, int ward) {
    if (ward < 0 || ward >= NUM_WARDS) return -1;
    for (int i = 0; i < b->total[ward]; i++)
        if (b->occupant[ward][i] == 0) return i;
    return -1;
}

int beds_free_count(const BedSystem *b, int ward) {
    int c = 0;
    if (ward < 0 || ward >= NUM_WARDS) return 0;
    for (int i = 0; i < b->total[ward]; i++)
        if (b->occupant[ward][i] == 0) c++;
    return c;
}

int beds_occupant(const BedSystem *b, int ward, int bed) {
    if (ward < 0 || ward >= NUM_WARDS || bed < 0 || bed >= b->total[ward]) return 0;
    return b->occupant[ward][bed];
}

int beds_set(BedSystem *b, int ward, int bed, int patient_id) {
    if (ward < 0 || ward >= NUM_WARDS || bed < 0 || bed >= b->total[ward]) return 0;
    if (b->occupant[ward][bed] != 0) return 0;
    b->occupant[ward][bed] = patient_id;
    return 1;
}

void beds_clear(BedSystem *b, int ward, int bed) {
    if (ward < 0 || ward >= NUM_WARDS || bed < 0 || bed >= b->total[ward]) return;
    b->occupant[ward][bed] = 0;
}

static WaitNode *new_node(int patient_id, int ward) {
    WaitNode *n = malloc(sizeof *n);
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->patient_id = patient_id;
    n->ward = ward;
    n->next = NULL;
    return n;
}

void beds_enqueue_back(BedSystem *b, int patient_id, int ward) {
    WaitNode *n = new_node(patient_id, ward);
    if (b->tail) b->tail->next = n;
    else b->head = n;
    b->tail = n;
    b->waiting++;
}

void beds_enqueue_front(BedSystem *b, int patient_id, int ward) {
    WaitNode *n = new_node(patient_id, ward);
    n->next = b->head;
    b->head = n;
    if (!b->tail) b->tail = n;
    b->waiting++;
}

int beds_queue_remove(BedSystem *b, int patient_id) {
    WaitNode *prev = NULL, *cur = b->head;
    while (cur) {
        if (cur->patient_id == patient_id) {
            if (prev) prev->next = cur->next;
            else b->head = cur->next;
            if (b->tail == cur) b->tail = prev;
            free(cur);
            b->waiting--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

int beds_queue_first_for_ward(const BedSystem *b, int ward) {
    for (WaitNode *n = b->head; n; n = n->next)
        if (n->ward == ward) return n->patient_id;
    return 0;
}

void beds_print(const BedSystem *b, const HashTable *ht) {
    for (int w = 0; w < NUM_WARDS; w++) {
        int freec = beds_free_count(b, w);
        printf("  %-10s %d/%d occupied\n", WARD_NAMES[w], b->total[w] - freec, b->total[w]);
        for (int i = 0; i < b->total[w]; i++) {
            int pid = b->occupant[w][i];
            if (pid) {
                Patient *p = ht_find(ht, pid);
                printf("     Bed %-2d : %-20.20s (ID %d)\n", i + 1, p ? p->name : "?", pid);
            }
        }
    }
    if (b->waiting == 0) {
        printf("  Bed waiting queue: empty\n");
    } else {
        printf("  Bed waiting queue (%d):\n", b->waiting);
        int pos = 1;
        for (WaitNode *n = b->head; n; n = n->next, pos++) {
            Patient *p = ht_find(ht, n->patient_id);
            printf("     %d. %-20.20s (ID %d) -> %s\n", pos, p ? p->name : "?",
                   n->patient_id, beds_ward_name(n->ward));
        }
    }
}
