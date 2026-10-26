#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transplant.h"
#include "util.h"

const char *organ_name(int organ) { return organ == ORGAN_LIVER ? "Liver" : "Heart"; }
int organ_viability_hours(int organ) { return organ == ORGAN_LIVER ? 12 : 6; }
const char *urgency_label(int organ) { return organ == ORGAN_LIVER ? "MELD" : "Urgency"; }

static int better(const WaitEntry *x, const WaitEntry *y) {
    if (x->priority != y->priority) return x->priority > y->priority;
    return x->patient_id < y->patient_id;
}

static void swap_w(WaitEntry *x, WaitEntry *y) { WaitEntry t = *x; *x = *y; *y = t; }

static void sift_up(OrganHeap *h, int i) {
    while (i > 0) {
        int p = (i - 1) / 2;
        if (better(&h->a[i], &h->a[p])) { swap_w(&h->a[i], &h->a[p]); i = p; }
        else break;
    }
}

static void sift_down(OrganHeap *h, int i) {
    for (;;) {
        int l = 2 * i + 1, r = l + 1, best = i;
        if (l < h->size && better(&h->a[l], &h->a[best])) best = l;
        if (r < h->size && better(&h->a[r], &h->a[best])) best = r;
        if (best == i) break;
        swap_w(&h->a[i], &h->a[best]);
        i = best;
    }
}

static int push(OrganHeap *h, const WaitEntry *e) {
    if (h->size == h->cap) {
        int nc = h->cap * 2;
        WaitEntry *t = realloc(h->a, sizeof(WaitEntry) * (size_t)nc);
        if (!t) return 0;
        h->a = t;
        h->cap = nc;
    }
    h->a[h->size] = *e;
    sift_up(h, h->size);
    h->size++;
    return 1;
}

static int pop(OrganHeap *h, WaitEntry *out) {
    if (h->size == 0) return 0;
    *out = h->a[0];
    h->size--;
    if (h->size > 0) {
        h->a[0] = h->a[h->size];
        sift_down(h, 0);
    }
    return 1;
}

void oh_init(OrganHeap *h, int organ) {
    h->size = 0;
    h->cap = 16;
    h->organ = organ;
    h->a = malloc(sizeof(WaitEntry) * (size_t)h->cap);
    if (!h->a) { fprintf(stderr, "Out of memory\n"); exit(1); }
}

void oh_free(OrganHeap *h) {
    free(h->a);
    h->a = NULL;
    h->size = h->cap = 0;
}

int oh_contains(const OrganHeap *h, int patient_id) {
    for (int i = 0; i < h->size; i++)
        if (h->a[i].patient_id == patient_id) return 1;
    return 0;
}

int oh_add(OrganHeap *h, const WaitEntry *e) {
    if (oh_contains(h, e->patient_id)) return 0;
    WaitEntry w = *e;
    w.organ = h->organ;
    w.priority = (long)w.urgency * 10000L + w.wait_days;
    return push(h, &w);
}

int oh_remove(OrganHeap *h, int patient_id) {
    for (int i = 0; i < h->size; i++) {
        if (h->a[i].patient_id == patient_id) {
            h->size--;
            if (i != h->size) {
                h->a[i] = h->a[h->size];
                sift_up(h, i);
                sift_down(h, i);
            }
            return 1;
        }
    }
    return 0;
}

/* Take the highest-priority recipient whose blood group is compatible.
   Incompatible higher-priority patients are popped aside and pushed back. */
int oh_match(OrganHeap *h, const char *donor_blood, WaitEntry *out) {
    if (h->size == 0) return 0;
    WaitEntry *skipped = malloc(sizeof(WaitEntry) * (size_t)h->size);
    if (!skipped) return 0;
    int ns = 0, found = 0;
    WaitEntry cur;
    while (pop(h, &cur)) {
        if (blood_compatible(donor_blood, cur.blood)) {
            *out = cur;
            found = 1;
            break;
        }
        skipped[ns++] = cur;
    }
    for (int i = 0; i < ns; i++) push(h, &skipped[i]);
    free(skipped);
    return found;
}

void oh_print(const OrganHeap *h) {
    if (h->size == 0) { printf("  %s waiting list is empty.\n", organ_name(h->organ)); return; }
    OrganHeap c;
    c.size = h->size;
    c.cap = h->size;
    c.organ = h->organ;
    c.a = malloc(sizeof(WaitEntry) * (size_t)c.cap);
    if (!c.a) { printf("  Out of memory.\n"); return; }
    memcpy(c.a, h->a, sizeof(WaitEntry) * (size_t)c.size);

    printf("  %s waiting list (highest priority first):\n", organ_name(h->organ));
    int rank = 1;
    WaitEntry e;
    while (pop(&c, &e)) {
        printf("  %2d. ID %-5d %-20.20s Blood %-3s %s %-2d  Waiting %4d days  (priority %ld)\n",
               rank++, e.patient_id, e.name, e.blood, urgency_label(h->organ),
               e.urgency, e.wait_days, e.priority);
    }
    free(c.a);
}
