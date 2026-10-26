#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hospital.h"
#include "storage.h"

Hospital H;

void hosp_init(void) {
    memset(&H, 0, sizeof H);
    ht_init(&H.patients);
    th_init(&H.triage);
    beds_init(&H.beds);
    us_init(&H.undo);
    oh_init(&H.liver, ORGAN_LIVER);
    oh_init(&H.heart, ORGAN_HEART);
    ns_init(&H.nurses);
    cg_init(&H.city);
}

void hosp_shutdown(void) {
    if (H.closed) return;
    H.closed = 1;
    storage_save();
    ht_free(&H.patients);
    th_free(&H.triage);
    beds_free(&H.beds);
    us_free(&H.undo);
    oh_free(&H.liver);
    oh_free(&H.heart);
}

static int ward_for_severity(int sev) {
    if (sev >= 8) return WARD_ICU;
    if (sev >= 5) return WARD_EMERGENCY;
    return WARD_GENERAL;
}

int hosp_register(const char *name, int age, const char *blood, int severity, const char *complaint) {
    Patient p;
    memset(&p, 0, sizeof p);
    p.id = H.patients.next_id;
    snprintf(p.name, sizeof p.name, "%s", name);
    p.age = age;
    snprintf(p.blood, sizeof p.blood, "%s", blood);
    p.severity = severity;
    snprintf(p.complaint, sizeof p.complaint, "%s", complaint);
    p.status = ST_TRIAGE;
    p.ward = -1;
    p.bed = -1;
    p.seq = ++H.seq;
    Patient *q = ht_insert(&H.patients, &p);
    if (!q) return 0;
    TriageEntry e = { q->id, q->severity, q->seq };
    th_push(&H.triage, e);
    return q->id;
}

/* Put a patient into a bed (or the waiting queue if the ward is full) and log it. */
static void do_admit(Patient *p, int ward, int prev_status) {
    UndoNode u;
    memset(&u, 0, sizeof u);
    u.type = ACT_ADMIT;
    u.patient_id = p->id;
    u.ward = ward;
    u.prev_status = prev_status;

    int b = beds_first_free(&H.beds, ward);
    if (b >= 0) {
        beds_set(&H.beds, ward, b, p->id);
        p->status = ST_ADMITTED;
        p->ward = ward;
        p->bed = b;
        u.bed = b;
        u.queued = 0;
        printf("  %s (ID %d) admitted to %s ward, bed %d.\n", p->name, p->id,
               beds_ward_name(ward), b + 1);
    } else {
        beds_enqueue_back(&H.beds, p->id, ward);
        p->status = ST_WAITING_BED;
        p->ward = ward;
        p->bed = -1;
        u.bed = -1;
        u.queued = 1;
        printf("  %s ward is full. %s (ID %d) added to the bed waiting queue (position %d).\n",
               beds_ward_name(ward), p->name, p->id, H.beds.waiting);
    }
    us_push(&H.undo, &u);
}

void hosp_call_next(void) {
    TriageEntry e;
    if (!th_pop(&H.triage, &e)) { printf("  Triage queue is empty.\n"); return; }
    Patient *p = ht_find(&H.patients, e.patient_id);
    if (!p) { printf("  Record for ID %d is missing.\n", e.patient_id); return; }
    int ward = ward_for_severity(p->severity);
    printf("  Calling next patient: %s (ID %d), severity %d, \"%s\"\n", p->name, p->id,
           p->severity, p->complaint);
    do_admit(p, ward, ST_TRIAGE);
}

void hosp_admit(int patient_id, int ward) {
    Patient *p = ht_find(&H.patients, patient_id);
    if (!p) { printf("  No patient with ID %d.\n", patient_id); return; }
    if (p->status == ST_ADMITTED || p->status == ST_WAITING_BED) {
        printf("  %s is already %s.\n", p->name, p->status == ST_ADMITTED ? "admitted" : "waiting for a bed");
        return;
    }
    int prev = p->status;
    if (prev == ST_TRIAGE) th_remove_patient(&H.triage, p->id);
    do_admit(p, ward, prev);
}

void hosp_discharge(int patient_id) {
    Patient *p = ht_find(&H.patients, patient_id);
    if (!p) { printf("  No patient with ID %d.\n", patient_id); return; }
    if (p->status != ST_ADMITTED) { printf("  %s is not currently admitted.\n", p->name); return; }

    int w = p->ward, b = p->bed;
    beds_clear(&H.beds, w, b);
    p->status = ST_DISCHARGED;
    p->ward = -1;
    p->bed = -1;
    printf("  %s (ID %d) discharged from %s bed %d.\n", p->name, p->id, beds_ward_name(w), b + 1);

    int promoted = beds_queue_first_for_ward(&H.beds, w);
    if (promoted) {
        Patient *q = ht_find(&H.patients, promoted);
        beds_queue_remove(&H.beds, promoted);
        if (q) {
            beds_set(&H.beds, w, b, q->id);
            q->status = ST_ADMITTED;
            q->ward = w;
            q->bed = b;
            printf("  Bed given to waiting patient %s (ID %d).\n", q->name, q->id);
        } else {
            promoted = 0;
        }
    }

    UndoNode u;
    memset(&u, 0, sizeof u);
    u.type = ACT_DISCHARGE;
    u.patient_id = patient_id;
    u.ward = w;
    u.bed = b;
    u.promoted_id = promoted;
    us_push(&H.undo, &u);
}

void hosp_undo(void) {
    UndoNode u;
    if (!us_pop(&H.undo, &u)) { printf("  Nothing to undo.\n"); return; }
    Patient *p = ht_find(&H.patients, u.patient_id);
    if (!p) { printf("  Cannot undo: patient record not found.\n"); return; }

    if (u.type == ACT_ADMIT) {
        int expected = u.queued ? ST_WAITING_BED : ST_ADMITTED;
        if (p->status != expected) { printf("  Cannot undo: patient state has changed.\n"); return; }
        if (u.queued) beds_queue_remove(&H.beds, p->id);
        else beds_clear(&H.beds, u.ward, u.bed);
        p->ward = -1;
        p->bed = -1;
        p->status = u.prev_status;
        if (u.prev_status == ST_TRIAGE) {
            TriageEntry e = { p->id, p->severity, p->seq };
            th_push(&H.triage, e);
        }
        printf("  Undone: admission of %s. Status is now \"%s\".\n", p->name, status_name(p->status));
    } else {
        Patient *q = u.promoted_id ? ht_find(&H.patients, u.promoted_id) : NULL;
        int occ = beds_occupant(&H.beds, u.ward, u.bed);
        int ok = p->status == ST_DISCHARGED;
        if (u.promoted_id) ok = ok && q && q->status == ST_ADMITTED && occ == q->id;
        else ok = ok && occ == 0;
        if (!ok) { printf("  Cannot undo: bed or patient state has changed.\n"); return; }
        if (u.promoted_id) {
            beds_clear(&H.beds, u.ward, u.bed);
            beds_enqueue_front(&H.beds, q->id, u.ward);
            q->status = ST_WAITING_BED;
            q->ward = u.ward;
            q->bed = -1;
        }
        beds_set(&H.beds, u.ward, u.bed, p->id);
        p->status = ST_ADMITTED;
        p->ward = u.ward;
        p->bed = u.bed;
        printf("  Undone: discharge of %s. Back in %s bed %d.\n", p->name,
               beds_ward_name(u.ward), u.bed + 1);
        if (u.promoted_id) printf("  %s went back to the front of the bed waiting queue.\n", q->name);
    }
}

void hosp_change_severity(int patient_id, int severity) {
    Patient *p = ht_find(&H.patients, patient_id);
    if (!p) { printf("  No patient with ID %d.\n", patient_id); return; }
    if (p->status != ST_TRIAGE) { printf("  %s is not in the triage queue.\n", p->name); return; }
    th_remove_patient(&H.triage, p->id);
    p->severity = severity;
    TriageEntry e = { p->id, p->severity, p->seq };
    th_push(&H.triage, e);
    printf("  Severity of %s updated to %d; queue re-ordered.\n", p->name, severity);
}

void hosp_remove_from_triage(int patient_id) {
    Patient *p = ht_find(&H.patients, patient_id);
    if (!p) { printf("  No patient with ID %d.\n", patient_id); return; }
    if (p->status != ST_TRIAGE) { printf("  %s is not in the triage queue.\n", p->name); return; }
    th_remove_patient(&H.triage, p->id);
    p->status = ST_REGISTERED;
    printf("  %s removed from the triage queue (status: Registered).\n", p->name);
}

void hosp_delete_record(int patient_id) {
    Patient *p = ht_find(&H.patients, patient_id);
    if (!p) { printf("  No patient with ID %d.\n", patient_id); return; }
    if (p->status == ST_TRIAGE || p->status == ST_ADMITTED || p->status == ST_WAITING_BED) {
        printf("  Cannot delete: patient is in triage, admitted or waiting for a bed.\n");
        return;
    }
    if (oh_contains(&H.liver, patient_id) || oh_contains(&H.heart, patient_id)) {
        printf("  Cannot delete: patient is on a transplant waiting list. Remove from the list first.\n");
        return;
    }
    /* undo log may still mention this id; us_print/undo handle a missing record safely */
    printf("  Record of %s (ID %d) deleted.\n", p->name, patient_id);
    ht_remove(&H.patients, patient_id);
}

void hosp_load_demo(void) {
    if (H.patients.count > 0 || H.nurses.count > 0) {
        printf("  Demo data can only be loaded into an empty system.\n");
        printf("  (Delete patients.dat, nurses.dat and waitlist.dat to reset.)\n");
        return;
    }
    int id[7];
    id[0] = hosp_register("Ramesh Kumar", 54, "B+", 9, "Chest pain");
    id[1] = hosp_register("Sunita Verma", 41, "O+", 4, "Fractured arm");
    id[2] = hosp_register("Arjun Mehta", 63, "A+", 8, "Liver failure");
    id[3] = hosp_register("Priya Nair", 29, "AB+", 6, "High fever");
    id[4] = hosp_register("Mohammed Ali", 58, "O-", 10, "Cardiac arrest");
    id[5] = hosp_register("Kavita Joshi", 35, "B-", 3, "Migraine");
    id[6] = hosp_register("Rahul Singh", 47, "A-", 7, "Heart failure");

    struct { int idx; int organ; int urg; int days; } wl[] = {
        {2, ORGAN_LIVER, 32, 120}, {1, ORGAN_LIVER, 24, 300}, {5, ORGAN_LIVER, 28, 45},
        {0, ORGAN_HEART, 9, 60},   {4, ORGAN_HEART, 10, 5},   {6, ORGAN_HEART, 7, 200}
    };
    for (size_t i = 0; i < sizeof wl / sizeof wl[0]; i++) {
        Patient *p = ht_find(&H.patients, id[wl[i].idx]);
        if (!p) continue;
        WaitEntry w;
        memset(&w, 0, sizeof w);
        w.patient_id = p->id;
        snprintf(w.name, sizeof w.name, "%s", p->name);
        snprintf(w.blood, sizeof w.blood, "%s", p->blood);
        w.urgency = wl[i].urg;
        w.wait_days = wl[i].days;
        oh_add(wl[i].organ == ORGAN_LIVER ? &H.liver : &H.heart, &w);
    }

    static const char *nn[] = {"Anita Sharma", "Bhavna Patel", "Chitra Nair", "Deepa Iyer",
                               "Esha Reddy", "Farah Khan", "Gita Menon", "Harsha Rao",
                               "Isha Gupta", "Jaya Singh"};
    for (size_t i = 0; i < sizeof nn / sizeof nn[0]; i++) ns_add(&H.nurses, nn[i]);
    ns_generate(&H.nurses);

    printf("  Demo data loaded: 7 patients (in triage), 3 liver + 3 heart candidates,\n");
    printf("  10 nurses with a generated weekly roster.\n");
}
