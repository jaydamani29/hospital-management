#include <stdio.h>
#include <string.h>
#include "nurse.h"

static const char *SHIFT_NAMES[SHIFTS] = {"Morning 06:00-14:00", "Evening 14:00-22:00",
                                          "Night   22:00-06:00"};
static const int   SHIFT_START[SHIFTS] = {6, 14, 22};
static const char *DAY_NAMES[DAYS]     = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

/* ---- circular queue of nurse indices (used for fair round-robin rotation) ---- */
typedef struct {
    int data[MAX_NURSES];
    int front;
    int size;
} CircQueue;

static void cq_init(CircQueue *q) { q->front = 0; q->size = 0; }

static int cq_push(CircQueue *q, int v) {
    if (q->size == MAX_NURSES) return 0;
    q->data[(q->front + q->size) % MAX_NURSES] = v;
    q->size++;
    return 1;
}

static int cq_pop(CircQueue *q, int *v) {
    if (q->size == 0) return 0;
    *v = q->data[q->front];
    q->front = (q->front + 1) % MAX_NURSES;
    q->size--;
    return 1;
}

void ns_init(NurseSystem *ns) {
    memset(ns, 0, sizeof *ns);
    ns->next_id = 1;
    ns->required = 2;
}

int ns_find_index(const NurseSystem *ns, int id) {
    for (int i = 0; i < ns->count; i++)
        if (ns->nurses[i].id == id) return i;
    return -1;
}

int ns_add_with_id(NurseSystem *ns, int id, const char *name, int on_leave) {
    if (ns->count >= MAX_NURSES || id <= 0 || ns_find_index(ns, id) >= 0) return 0;
    Nurse *n = &ns->nurses[ns->count++];
    memset(n, 0, sizeof *n);
    n->id = id;
    snprintf(n->name, sizeof n->name, "%s", name);
    n->on_leave = on_leave ? 1 : 0;
    n->last_end = -1000;
    if (id >= ns->next_id) ns->next_id = id + 1;
    ns->generated = 0;
    return id;
}

int ns_add(NurseSystem *ns, const char *name) {
    return ns_add_with_id(ns, ns->next_id, name, 0);
}

int ns_set_leave(NurseSystem *ns, int id, int on_leave) {
    int i = ns_find_index(ns, id);
    if (i < 0) return 0;
    ns->nurses[i].on_leave = on_leave ? 1 : 0;
    ns->generated = 0;
    return 1;
}

void ns_generate(NurseSystem *ns) {
    for (int i = 0; i < ns->count; i++) {
        ns->nurses[i].shifts_week = 0;
        ns->nurses[i].hours_week = 0;
        ns->nurses[i].last_end = -1000;
    }
    for (int d = 0; d < DAYS; d++)
        for (int s = 0; s < SHIFTS; s++) {
            ns->filled[d][s] = 0;
            for (int k = 0; k < MAX_PER_SHIFT; k++) ns->roster[d][s][k] = -1;
        }

    CircQueue q;
    cq_init(&q);
    for (int i = 0; i < ns->count; i++)
        if (!ns->nurses[i].on_leave) cq_push(&q, i);

    for (int d = 0; d < DAYS; d++) {
        for (int s = 0; s < SHIFTS; s++) {
            int start = d * 24 + SHIFT_START[s];
            int limit = q.size, tried = 0;
            while (ns->filled[d][s] < ns->required && tried < limit) {
                int idx;
                if (!cq_pop(&q, &idx)) break;
                tried++;
                Nurse *n = &ns->nurses[idx];
                int ok = n->shifts_week < MAX_SHIFTS_PER_WEEK &&
                         (start - n->last_end) >= REST_HOURS;
                if (ok) {
                    ns->roster[d][s][ns->filled[d][s]++] = idx;
                    n->shifts_week++;
                    n->hours_week += SHIFT_LEN;
                    n->last_end = start + SHIFT_LEN;
                }
                cq_push(&q, idx);        /* go to the back of the line */
            }
        }
    }
    ns->generated = 1;
}

void ns_print_nurses(const NurseSystem *ns) {
    if (ns->count == 0) { printf("  No nurses registered yet.\n"); return; }
    printf("  %-4s %-22s %-10s %6s %6s\n", "ID", "Name", "Status", "Shifts", "Hours");
    for (int i = 0; i < ns->count; i++) {
        const Nurse *n = &ns->nurses[i];
        printf("  %-4d %-22.22s %-10s %6d %6d\n", n->id, n->name,
               n->on_leave ? "On leave" : "Active", n->shifts_week, n->hours_week);
    }
}

void ns_print_roster(const NurseSystem *ns) {
    if (!ns->generated) {
        printf("  Roster not generated yet (or out of date). Generate it first.\n");
        return;
    }
    printf("  Weekly roster (%d nurse(s) per shift, %d h shifts, %d h rest rule, max %d shifts/week)\n",
           ns->required, SHIFT_LEN, REST_HOURS, MAX_SHIFTS_PER_WEEK);
    for (int d = 0; d < DAYS; d++) {
        printf("  %s\n", DAY_NAMES[d]);
        for (int s = 0; s < SHIFTS; s++) {
            printf("    %s : ", SHIFT_NAMES[s]);
            for (int k = 0; k < ns->filled[d][s]; k++)
                printf("%s%s", ns->nurses[ns->roster[d][s][k]].name, k + 1 < ns->filled[d][s] ? ", " : "");
            if (ns->filled[d][s] < ns->required)
                printf("%s** UNDERSTAFFED (%d of %d) **", ns->filled[d][s] ? "  " : "", ns->filled[d][s], ns->required);
            printf("\n");
        }
    }
}

void ns_print_nurse_schedule(const NurseSystem *ns, int id) {
    int idx = ns_find_index(ns, id);
    if (idx < 0) { printf("  No nurse with ID %d.\n", id); return; }
    if (!ns->generated) { printf("  Roster not generated yet (or out of date).\n"); return; }
    printf("  Schedule for %s (ID %d)%s:\n", ns->nurses[idx].name, id,
           ns->nurses[idx].on_leave ? " - ON LEAVE" : "");
    int any = 0;
    for (int d = 0; d < DAYS; d++)
        for (int s = 0; s < SHIFTS; s++)
            for (int k = 0; k < ns->filled[d][s]; k++)
                if (ns->roster[d][s][k] == idx) {
                    printf("    %s  %s\n", DAY_NAMES[d], SHIFT_NAMES[s]);
                    any = 1;
                }
    if (!any) printf("    (no shifts this week)\n");
    printf("  Total: %d shift(s), %d hour(s)\n", ns->nurses[idx].shifts_week, ns->nurses[idx].hours_week);
}

void ns_print_on_duty(const NurseSystem *ns, int day, int hour) {
    if (!ns->generated) { printf("  Roster not generated yet (or out of date).\n"); return; }
    int s, d = day;
    if (hour >= 6 && hour < 14) s = 0;
    else if (hour >= 14 && hour < 22) s = 1;
    else if (hour >= 22) s = 2;
    else { s = 2; d = day - 1; }        /* 00:00-06:00 belongs to the previous night shift */
    if (d < 0) {
        printf("  Early Monday morning belongs to Sunday's night shift, which is outside this roster.\n");
        return;
    }
    printf("  On duty %s %02d:00 (%s):\n", DAY_NAMES[day], hour, SHIFT_NAMES[s]);
    if (ns->filled[d][s] == 0) { printf("    nobody\n"); return; }
    for (int k = 0; k < ns->filled[d][s]; k++) {
        const Nurse *n = &ns->nurses[ns->roster[d][s][k]];
        printf("    %s (ID %d)\n", n->name, n->id);
    }
}
