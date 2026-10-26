#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "hospital.h"
#include "util.h"

#define F_PATIENTS "patients.dat"
#define F_NURSES   "nurses.dat"
#define F_WAITLIST "waitlist.dat"

/* ------------------------------ saving ------------------------------ */

static void save_patient_cb(Patient *p, void *ctx) {
    fprintf((FILE *)ctx, "%d|%s|%d|%s|%d|%s|%d|%d|%d|%ld\n", p->id, p->name, p->age, p->blood,
            p->severity, p->complaint, p->status, p->ward, p->bed, p->seq);
}

static void save_heap(FILE *f, const OrganHeap *h) {
    for (int i = 0; i < h->size; i++) {
        const WaitEntry *e = &h->a[i];
        fprintf(f, "%d|%s|%s|%d|%d|%d\n", e->patient_id, e->name, e->blood, e->organ,
                e->urgency, e->wait_days);
    }
}

int storage_save(void) {
    int ok = 1;
    FILE *f = fopen(F_PATIENTS, "w");
    if (f) {
        fprintf(f, "# id|name|age|blood|severity|complaint|status|ward|bed|seq\n");
        ht_foreach(&H.patients, save_patient_cb, f);
        fclose(f);
    } else { fprintf(stderr, "Warning: cannot write %s\n", F_PATIENTS); ok = 0; }

    f = fopen(F_NURSES, "w");
    if (f) {
        fprintf(f, "# id|name|on_leave\n");
        for (int i = 0; i < H.nurses.count; i++)
            fprintf(f, "%d|%s|%d\n", H.nurses.nurses[i].id, H.nurses.nurses[i].name,
                    H.nurses.nurses[i].on_leave);
        fclose(f);
    } else { fprintf(stderr, "Warning: cannot write %s\n", F_NURSES); ok = 0; }

    f = fopen(F_WAITLIST, "w");
    if (f) {
        fprintf(f, "# patient_id|name|blood|organ|urgency|wait_days\n");
        save_heap(f, &H.liver);
        save_heap(f, &H.heart);
        fclose(f);
    } else { fprintf(stderr, "Warning: cannot write %s\n", F_WAITLIST); ok = 0; }
    return ok;
}

/* ------------------------------ loading ------------------------------ */

typedef struct { Patient **arr; int n; } RebuildCtx;

static void rebuild_cb(Patient *p, void *v) {
    RebuildCtx *c = v;
    if (p->seq > H.seq) H.seq = p->seq;
    switch (p->status) {
    case ST_TRIAGE: {
        TriageEntry e = { p->id, p->severity, p->seq };
        th_push(&H.triage, e);
        break;
    }
    case ST_ADMITTED:
        if (!beds_set(&H.beds, p->ward, p->bed, p->id)) {
            p->status = ST_REGISTERED; p->ward = -1; p->bed = -1;
        }
        break;
    case ST_WAITING_BED:
        if (p->ward >= 0 && p->ward < NUM_WARDS) c->arr[c->n++] = p;
        else { p->status = ST_REGISTERED; p->ward = -1; p->bed = -1; }
        break;
    default:
        break;
    }
}

static int cmp_seq(const void *x, const void *y) {
    const Patient *a = *(Patient *const *)x;
    const Patient *b = *(Patient *const *)y;
    return (a->seq > b->seq) - (a->seq < b->seq);
}

static void load_patients(void) {
    FILE *f = fopen(F_PATIENTS, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        Patient p;
        memset(&p, 0, sizeof p);
        int id, age, sev, st, ward, bed;
        long seq;
        char name[NAME_LEN], blood[4], comp[COMPLAINT_LEN];
        int n = sscanf(line, "%d|%49[^|]|%d|%3[^|]|%d|%59[^|]|%d|%d|%d|%ld",
                       &id, name, &age, blood, &sev, comp, &st, &ward, &bed, &seq);
        if (n != 10 || id <= 0 || sev < 1 || sev > 10 || st < 0 || st > ST_DISCHARGED ||
            !valid_blood(blood))
            continue;
        p.id = id; p.age = age; p.severity = sev; p.status = st;
        p.ward = ward; p.bed = bed; p.seq = seq;
        snprintf(p.name, sizeof p.name, "%s", name);
        snprintf(p.blood, sizeof p.blood, "%s", blood);
        snprintf(p.complaint, sizeof p.complaint, "%s", comp);
        ht_insert(&H.patients, &p);
    }
    fclose(f);

    RebuildCtx c;
    c.n = 0;
    c.arr = malloc(sizeof(Patient *) * (size_t)(H.patients.count + 1));
    if (!c.arr) return;
    ht_foreach(&H.patients, rebuild_cb, &c);
    qsort(c.arr, (size_t)c.n, sizeof(Patient *), cmp_seq);
    for (int i = 0; i < c.n; i++) {
        c.arr[i]->bed = -1;
        beds_enqueue_back(&H.beds, c.arr[i]->id, c.arr[i]->ward);
    }
    free(c.arr);
}

static void load_waitlist(void) {
    FILE *f = fopen(F_WAITLIST, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        WaitEntry w;
        memset(&w, 0, sizeof w);
        int pid, organ, urg, days;
        char name[NAME_LEN], blood[4];
        int n = sscanf(line, "%d|%49[^|]|%3[^|]|%d|%d|%d", &pid, name, blood, &organ, &urg, &days);
        if (n != 6 || pid <= 0 || (organ != ORGAN_LIVER && organ != ORGAN_HEART) ||
            !valid_blood(blood) || days < 0)
            continue;
        w.patient_id = pid; w.organ = organ; w.urgency = urg; w.wait_days = days;
        snprintf(w.name, sizeof w.name, "%s", name);
        snprintf(w.blood, sizeof w.blood, "%s", blood);
        oh_add(organ == ORGAN_LIVER ? &H.liver : &H.heart, &w);
    }
    fclose(f);
}

static void load_nurses(void) {
    FILE *f = fopen(F_NURSES, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        int id, leave;
        char name[NAME_LEN];
        if (sscanf(line, "%d|%49[^|]|%d", &id, name, &leave) != 3) continue;
        ns_add_with_id(&H.nurses, id, name, leave);
    }
    fclose(f);
}

void storage_load(void) {
    load_patients();
    load_waitlist();
    load_nurses();
    if (H.patients.count || H.nurses.count)
        printf("Loaded saved data: %d patient(s), %d nurse(s), %d liver and %d heart candidate(s).\n",
               H.patients.count, H.nurses.count, H.liver.size, H.heart.size);
}
