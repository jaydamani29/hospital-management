#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "patient.h"
#include "beds.h"

static const char *ST_NAMES[] = {"Registered", "In triage", "Admitted",
                                 "Waiting for bed", "Discharged"};

const char *status_name(int st) {
    return (st >= 0 && st <= ST_DISCHARGED) ? ST_NAMES[st] : "Unknown";
}

static unsigned hash_id(int id) { return (unsigned)id % HASH_SIZE; }

void ht_init(HashTable *t) {
    for (int i = 0; i < HASH_SIZE; i++) t->bucket[i] = NULL;
    t->count = 0;
    t->next_id = 1001;
}

void ht_free(HashTable *t) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HNode *n = t->bucket[i];
        while (n) {
            HNode *nx = n->next;
            free(n);
            n = nx;
        }
        t->bucket[i] = NULL;
    }
    t->count = 0;
}

Patient *ht_find(const HashTable *t, int id) {
    for (HNode *n = t->bucket[hash_id(id)]; n; n = n->next)
        if (n->p.id == id) return &n->p;
    return NULL;
}

Patient *ht_insert(HashTable *t, const Patient *p) {
    if (ht_find(t, p->id)) return NULL;
    HNode *n = malloc(sizeof *n);
    if (!n) return NULL;
    n->p = *p;
    unsigned h = hash_id(p->id);
    n->next = t->bucket[h];          /* insert at head of chain: O(1) */
    t->bucket[h] = n;
    t->count++;
    if (p->id >= t->next_id) t->next_id = p->id + 1;
    return &n->p;
}

int ht_remove(HashTable *t, int id) {
    unsigned h = hash_id(id);
    HNode *prev = NULL, *cur = t->bucket[h];
    while (cur) {
        if (cur->p.id == id) {
            if (prev) prev->next = cur->next;
            else t->bucket[h] = cur->next;
            free(cur);
            t->count--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

void ht_foreach(const HashTable *t, PatientVisitor fn, void *ctx) {
    for (int i = 0; i < HASH_SIZE; i++)
        for (HNode *n = t->bucket[i]; n; n = n->next) fn(&n->p, ctx);
}

void patient_print_header(void) {
    printf("  %-6s %-20s %3s %-5s %3s  %-15s %-12s %s\n",
           "ID", "Name", "Age", "Blood", "Sev", "Status", "Location", "Complaint");
}

void patient_print(const Patient *p) {
    char loc[32];
    if (p->status == ST_ADMITTED)
        snprintf(loc, sizeof loc, "%s #%d", beds_ward_name(p->ward), p->bed + 1);
    else if (p->status == ST_WAITING_BED)
        snprintf(loc, sizeof loc, "wait:%s", beds_ward_name(p->ward));
    else
        snprintf(loc, sizeof loc, "-");
    printf("  %-6d %-20.20s %3d %-5s %3d  %-15s %-12s %s\n", p->id, p->name, p->age,
           p->blood, p->severity, status_name(p->status), loc, p->complaint);
}

typedef struct { Patient **a; int n; } Collect;

static void collect_cb(Patient *p, void *v) {
    Collect *c = v;
    c->a[c->n++] = p;
}

static int cmp_id(const void *x, const void *y) {
    const Patient *a = *(Patient *const *)x;
    const Patient *b = *(Patient *const *)y;
    return (a->id > b->id) - (a->id < b->id);
}

void ht_print_all(const HashTable *t) {
    if (t->count == 0) { printf("  No patients registered yet.\n"); return; }
    Collect c;
    c.a = malloc(sizeof(Patient *) * (size_t)t->count);
    c.n = 0;
    if (!c.a) { printf("  Out of memory.\n"); return; }
    ht_foreach(t, collect_cb, &c);
    qsort(c.a, (size_t)c.n, sizeof(Patient *), cmp_id);
    patient_print_header();
    for (int i = 0; i < c.n; i++) patient_print(c.a[i]);
    printf("  Total: %d patient(s)\n", c.n);
    free(c.a);
}

static int ci_contains(const char *hay, const char *needle) {
    size_t hn = strlen(hay), nn = strlen(needle);
    if (nn == 0) return 1;
    if (nn > hn) return 0;
    for (size_t i = 0; i + nn <= hn; i++) {
        size_t j = 0;
        while (j < nn && tolower((unsigned char)hay[i + j]) == tolower((unsigned char)needle[j])) j++;
        if (j == nn) return 1;
    }
    return 0;
}

typedef struct { const char *part; int found; } NameCtx;

static void name_cb(Patient *p, void *v) {
    NameCtx *c = v;
    if (ci_contains(p->name, c->part)) {
        if (c->found == 0) patient_print_header();
        patient_print(p);
        c->found++;
    }
}

void ht_search_name(const HashTable *t, const char *part) {
    NameCtx c = { part, 0 };
    ht_foreach(t, name_cb, &c);
    if (c.found == 0) printf("  No patient matches \"%s\".\n", part);
}
