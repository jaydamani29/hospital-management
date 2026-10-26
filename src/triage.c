#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "triage.h"

static int higher(const TriageEntry *x, const TriageEntry *y) {
    if (x->severity != y->severity) return x->severity > y->severity;
    return x->seq < y->seq;
}

static void swap_e(TriageEntry *x, TriageEntry *y) {
    TriageEntry t = *x; *x = *y; *y = t;
}

static void sift_up(TriageHeap *h, int i) {
    while (i > 0) {
        int p = (i - 1) / 2;
        if (higher(&h->a[i], &h->a[p])) { swap_e(&h->a[i], &h->a[p]); i = p; }
        else break;
    }
}

static void sift_down(TriageHeap *h, int i) {
    for (;;) {
        int l = 2 * i + 1, r = l + 1, best = i;
        if (l < h->size && higher(&h->a[l], &h->a[best])) best = l;
        if (r < h->size && higher(&h->a[r], &h->a[best])) best = r;
        if (best == i) break;
        swap_e(&h->a[i], &h->a[best]);
        i = best;
    }
}

void th_init(TriageHeap *h) {
    h->size = 0;
    h->cap = 16;
    h->a = malloc(sizeof(TriageEntry) * (size_t)h->cap);
    if (!h->a) { fprintf(stderr, "Out of memory\n"); exit(1); }
}

void th_free(TriageHeap *h) {
    free(h->a);
    h->a = NULL;
    h->size = h->cap = 0;
}

int th_push(TriageHeap *h, TriageEntry e) {
    if (h->size == h->cap) {
        int nc = h->cap * 2;
        TriageEntry *t = realloc(h->a, sizeof(TriageEntry) * (size_t)nc);
        if (!t) return 0;
        h->a = t;
        h->cap = nc;
    }
    h->a[h->size] = e;
    sift_up(h, h->size);
    h->size++;
    return 1;
}

int th_peek(const TriageHeap *h, TriageEntry *out) {
    if (h->size == 0) return 0;
    *out = h->a[0];
    return 1;
}

int th_pop(TriageHeap *h, TriageEntry *out) {
    if (h->size == 0) return 0;
    *out = h->a[0];
    h->size--;
    if (h->size > 0) {
        h->a[0] = h->a[h->size];
        sift_down(h, 0);
    }
    return 1;
}

int th_contains(const TriageHeap *h, int patient_id) {
    for (int i = 0; i < h->size; i++)
        if (h->a[i].patient_id == patient_id) return 1;
    return 0;
}

int th_remove_patient(TriageHeap *h, int patient_id) {
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

void th_print(const TriageHeap *h, const HashTable *ht) {
    if (h->size == 0) { printf("  Triage queue is empty.\n"); return; }
    TriageHeap c;
    c.size = h->size;
    c.cap = h->size;
    c.a = malloc(sizeof(TriageEntry) * (size_t)c.cap);
    if (!c.a) { printf("  Out of memory.\n"); return; }
    memcpy(c.a, h->a, sizeof(TriageEntry) * (size_t)c.size);

    printf("  Triage queue (most critical first):\n");
    int rank = 1;
    TriageEntry e;
    while (th_pop(&c, &e)) {            /* pop from a COPY so the real heap is untouched */
        Patient *p = ht_find(ht, e.patient_id);
        printf("  %2d. [Severity %2d] ID %d  %-20.20s %s\n", rank++, e.severity,
               e.patient_id, p ? p->name : "?", p ? p->complaint : "");
    }
    free(c.a);
}
