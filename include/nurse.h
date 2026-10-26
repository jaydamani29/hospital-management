#ifndef NURSE_H
#define NURSE_H
#include "patient.h"

#define MAX_NURSES           50
#define DAYS                 7
#define SHIFTS               3       /* 0 = Morning, 1 = Evening, 2 = Night */
#define MAX_PER_SHIFT        5
#define SHIFT_LEN            8       /* hours */
#define MAX_SHIFTS_PER_WEEK  5       /* 40 hours */
#define REST_HOURS           12      /* minimum rest between two shifts */

typedef struct {
    int  id;
    char name[NAME_LEN];
    int  on_leave;
    int  shifts_week;
    int  hours_week;
    int  last_end;       /* absolute hour (day*24+hour) when last shift ended */
} Nurse;

typedef struct {
    Nurse nurses[MAX_NURSES];
    int   count;
    int   next_id;
    int   required;                              /* nurses needed per shift */
    int   roster[DAYS][SHIFTS][MAX_PER_SHIFT];   /* index into nurses[] */
    int   filled[DAYS][SHIFTS];
    int   generated;
} NurseSystem;

void ns_init(NurseSystem *ns);
int  ns_add(NurseSystem *ns, const char *name);                       /* new id or 0 */
int  ns_add_with_id(NurseSystem *ns, int id, const char *name, int on_leave);
int  ns_find_index(const NurseSystem *ns, int id);                    /* -1 if none */
int  ns_set_leave(NurseSystem *ns, int id, int on_leave);
void ns_generate(NurseSystem *ns);
void ns_print_nurses(const NurseSystem *ns);
void ns_print_roster(const NurseSystem *ns);
void ns_print_nurse_schedule(const NurseSystem *ns, int id);
void ns_print_on_duty(const NurseSystem *ns, int day, int hour);

#endif
