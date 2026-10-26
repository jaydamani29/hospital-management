#ifndef UNDO_H
#define UNDO_H
#include "patient.h"

enum { ACT_ADMIT = 1, ACT_DISCHARGE = 2 };

/* Linked-list stack; each node remembers what is needed to reverse the action. */
typedef struct UndoNode {
    int type;
    int patient_id;
    int ward;
    int bed;
    int queued;         /* ADMIT: 1 if patient was only placed in the waiting queue */
    int prev_status;    /* ADMIT: status before admission */
    int promoted_id;    /* DISCHARGE: waiting patient who took the freed bed (0 = none) */
    struct UndoNode *next;
} UndoNode;

typedef struct {
    UndoNode *top;
    int size;
} UndoStack;

void us_init(UndoStack *s);
void us_free(UndoStack *s);
void us_push(UndoStack *s, const UndoNode *n);
int  us_pop(UndoStack *s, UndoNode *out);
void us_print(const UndoStack *s, const HashTable *ht);

#endif
