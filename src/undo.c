#include <stdio.h>
#include <stdlib.h>
#include "undo.h"
#include "beds.h"

void us_init(UndoStack *s) { s->top = NULL; s->size = 0; }

void us_free(UndoStack *s) {
    UndoNode *n = s->top;
    while (n) {
        UndoNode *nx = n->next;
        free(n);
        n = nx;
    }
    s->top = NULL;
    s->size = 0;
}

void us_push(UndoStack *s, const UndoNode *src) {
    UndoNode *n = malloc(sizeof *n);
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    *n = *src;
    n->next = s->top;
    s->top = n;
    s->size++;
}

int us_pop(UndoStack *s, UndoNode *out) {
    if (!s->top) return 0;
    UndoNode *n = s->top;
    *out = *n;
    out->next = NULL;
    s->top = n->next;
    free(n);
    s->size--;
    return 1;
}

void us_print(const UndoStack *s, const HashTable *ht) {
    if (!s->top) { printf("  Audit log is empty.\n"); return; }
    printf("  Audit log (most recent first, %d action(s)):\n", s->size);
    int i = 1;
    for (UndoNode *n = s->top; n; n = n->next, i++) {
        Patient *p = ht_find(ht, n->patient_id);
        const char *nm = p ? p->name : "?";
        if (n->type == ACT_ADMIT) {
            if (n->queued)
                printf("   %2d. QUEUED     %s (ID %d) for %s\n", i, nm, n->patient_id, beds_ward_name(n->ward));
            else
                printf("   %2d. ADMIT      %s (ID %d) -> %s bed %d\n", i, nm, n->patient_id,
                       beds_ward_name(n->ward), n->bed + 1);
        } else {
            printf("   %2d. DISCHARGE  %s (ID %d) from %s bed %d\n", i, nm, n->patient_id,
                   beds_ward_name(n->ward), n->bed + 1);
        }
    }
}
