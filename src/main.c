#include <stdio.h>
#include <stdlib.h>
#include "hospital.h"
#include "storage.h"
#include "util.h"

static void on_exit_cb(void) { hosp_shutdown(); }

static int ask_patient_id(void) { return read_int("  Patient ID: ", 1, 999999); }

static int ask_ward(void) {
    printf("   1. General (10 beds)   2. Emergency (6 beds)   3. ICU (4 beds)\n");
    return read_int("  Ward (1-3): ", 1, 3) - 1;
}

static void ui_register(void) {
    char name[NAME_LEN], blood[4], complaint[COMPLAINT_LEN];
    read_line("  Name: ", name, sizeof name);
    int age = read_int("  Age (0-120): ", 0, 120);
    read_blood("  Blood group (A+ A- B+ B- AB+ AB- O+ O-): ", blood);
    int sev = read_int("  Severity 1 (minor) to 10 (critical): ", 1, 10);
    read_line("  Complaint: ", complaint, sizeof complaint);
    int id = hosp_register(name, age, blood, sev, complaint);
    if (id) printf("  Registered with ID %d and added to the triage queue.\n", id);
    else    printf("  Registration failed.\n");
}

static void ui_patients(void) {
    for (;;) {
        printf("\n--- PATIENT RECORDS (hash table) ---\n");
        printf("  1. Register new patient\n  2. Search by ID\n  3. Search by name\n");
        printf("  4. List all patients\n  5. Delete a record\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 5);
        if (c == 0) return;
        if (c == 1) ui_register();
        else if (c == 2) {
            Patient *p = ht_find(&H.patients, ask_patient_id());
            if (p) { patient_print_header(); patient_print(p); }
            else printf("  Not found.\n");
        } else if (c == 3) {
            char s[NAME_LEN];
            read_line("  Name (or part of it): ", s, sizeof s);
            ht_search_name(&H.patients, s);
        } else if (c == 4) ht_print_all(&H.patients);
        else if (c == 5) hosp_delete_record(ask_patient_id());
    }
}

static void ui_triage(void) {
    for (;;) {
        printf("\n--- EMERGENCY TRIAGE (max-heap priority queue) ---\n");
        printf("  1. Register patient into queue\n  2. View queue\n");
        printf("  3. Call next (most critical) and admit\n  4. Change a patient's severity\n");
        printf("  5. Remove patient from queue\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 5);
        if (c == 0) return;
        if (c == 1) ui_register();
        else if (c == 2) th_print(&H.triage, &H.patients);
        else if (c == 3) hosp_call_next();
        else if (c == 4) {
            int id = ask_patient_id();
            hosp_change_severity(id, read_int("  New severity (1-10): ", 1, 10));
        } else if (c == 5) hosp_remove_from_triage(ask_patient_id());
    }
}

static void ui_beds(void) {
    for (;;) {
        printf("\n--- BEDS AND WARDS (array + linked-list queue) ---\n");
        printf("  1. Show beds and waiting queue\n  2. Admit patient to a ward\n");
        printf("  3. Discharge patient\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 3);
        if (c == 0) return;
        if (c == 1) beds_print(&H.beds, &H.patients);
        else if (c == 2) {
            int id = ask_patient_id();
            hosp_admit(id, ask_ward());
        } else if (c == 3) hosp_discharge(ask_patient_id());
    }
}

static void ui_waitlist_add(void) {
    int organ = read_int("  Organ (1 = Liver, 2 = Heart): ", 1, 2) - 1;
    int id = ask_patient_id();
    Patient *p = ht_find(&H.patients, id);
    if (!p) { printf("  Register the patient first (no record with ID %d).\n", id); return; }
    OrganHeap *h = organ == ORGAN_LIVER ? &H.liver : &H.heart;
    if (oh_contains(h, id)) { printf("  %s is already on the %s list.\n", p->name, organ_name(organ)); return; }
    WaitEntry w;
    w.patient_id = p->id;
    snprintf(w.name, sizeof w.name, "%s", p->name);
    snprintf(w.blood, sizeof w.blood, "%s", p->blood);
    if (organ == ORGAN_LIVER) w.urgency = read_int("  MELD score (6-40): ", 6, 40);
    else                      w.urgency = read_int("  Urgency (1 = low .. 10 = critical): ", 1, 10);
    w.wait_days = read_int("  Days already waiting (0-5000): ", 0, 5000);
    w.organ = organ;
    w.priority = 0;
    if (oh_add(h, &w)) printf("  %s added to the %s waiting list.\n", p->name, organ_name(organ));
    else printf("  Could not add to the list.\n");
}

static void ui_organ_available(void) {
    int organ = read_int("  Organ available (1 = Liver, 2 = Heart): ", 1, 2) - 1;
    char donor[4];
    read_blood("  Donor blood group: ", donor);
    int hrs = read_int("  Hours since retrieval (0-24): ", 0, 24);
    int via = organ_viability_hours(organ);
    printf("  %s stays viable for %d hours.\n", organ_name(organ), via);
    if (hrs >= via) { printf("  Organ is no longer viable. Discarded.\n"); return; }

    printf("  Where is the donor hospital/location?\n");
    cg_print_places(&H.city);
    int loc = read_int("  Donor location (0-9): ", 0, G_N - 1);
    int path[G_N], len = 0, total = 0;
    if (!cg_route(&H.city, loc, 0, path, &len, &total)) {
        printf("  No open road from the donor location to Central Hospital.\n");
        return;
    }
    int remaining_min = (via - hrs) * 60;
    cg_print_path(&H.city, path, len, total);
    if (total >= remaining_min) {
        printf("  Transport takes %d min but only %d min of viability remain. Cannot use this organ.\n",
               total, remaining_min);
        return;
    }

    WaitEntry w;
    OrganHeap *h = organ == ORGAN_LIVER ? &H.liver : &H.heart;
    if (oh_match(h, donor, &w)) {
        printf("\n  MATCH FOUND\n");
        printf("  Recipient : %s (ID %d), blood %s, %s %d, waiting %d days\n", w.name,
               w.patient_id, w.blood, urgency_label(organ), w.urgency, w.wait_days);
        printf("  Time left : %d min after transport. Alert the surgical team now.\n",
               remaining_min - total);
        printf("  Recipient removed from the %s waiting list.\n", organ_name(organ));
    } else {
        printf("  No compatible recipient (blood group %s) on the %s list.\n", donor, organ_name(organ));
    }
}

static void ui_transplant(void) {
    for (;;) {
        printf("\n--- ORGAN TRANSPLANT (priority heaps) ---\n");
        printf("  1. Add patient to waiting list\n  2. View liver list\n  3. View heart list\n");
        printf("  4. Organ available - find best match\n  5. Remove patient from a list\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 5);
        if (c == 0) return;
        if (c == 1) ui_waitlist_add();
        else if (c == 2) oh_print(&H.liver);
        else if (c == 3) oh_print(&H.heart);
        else if (c == 4) ui_organ_available();
        else if (c == 5) {
            int organ = read_int("  List (1 = Liver, 2 = Heart): ", 1, 2) - 1;
            int id = ask_patient_id();
            OrganHeap *h = organ == ORGAN_LIVER ? &H.liver : &H.heart;
            printf(oh_remove(h, id) ? "  Removed from the list.\n" : "  Patient is not on that list.\n");
        }
    }
}

static void ui_ambulance(void) {
    for (;;) {
        printf("\n--- AMBULANCE ROUTING (graph + Dijkstra) ---\n");
        printf("  1. Show map\n  2. Dispatch: nearest hospital from pickup point\n");
        printf("  3. Shortest route between two places\n  4. Block / unblock a road\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 4);
        if (c == 0) return;
        int path[G_N], len = 0, total = 0;
        if (c == 1) { cg_print_places(&H.city); cg_print_roads(&H.city); }
        else if (c == 2) {
            cg_print_places(&H.city);
            int src = read_int("  Pickup location (0-9): ", 0, G_N - 1);
            int hosp = 0;
            if (cg_nearest_hospital(&H.city, src, &hosp, path, &len, &total)) {
                printf("  Nearest hospital: %s\n", H.city.place[hosp].name);
                cg_print_path(&H.city, path, len, total);
            } else printf("  No hospital reachable from there.\n");
        } else if (c == 3) {
            cg_print_places(&H.city);
            int a = read_int("  From (0-9): ", 0, G_N - 1);
            int b = read_int("  To (0-9): ", 0, G_N - 1);
            if (cg_route(&H.city, a, b, path, &len, &total)) cg_print_path(&H.city, path, len, total);
            else printf("  No open route.\n");
        } else if (c == 4) {
            cg_print_places(&H.city);
            int a = read_int("  Road end A (0-9): ", 0, G_N - 1);
            int b = read_int("  Road end B (0-9): ", 0, G_N - 1);
            int bl = read_int("  1 = block, 0 = reopen: ", 0, 1);
            if (cg_set_blocked(&H.city, a, b, bl)) printf("  Road %s.\n", bl ? "blocked" : "reopened");
            else printf("  There is no direct road between those places.\n");
        }
    }
}

static void ui_nurses(void) {
    for (;;) {
        printf("\n--- NURSE SCHEDULING (circular queue rotation) ---\n");
        printf("  1. Add nurse\n  2. List nurses\n  3. Mark leave / return\n");
        printf("  4. Set nurses needed per shift (now %d)\n  5. Generate weekly roster\n", H.nurses.required);
        printf("  6. View roster\n  7. One nurse's schedule\n  8. Who is on duty at a given time\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 8);
        if (c == 0) return;
        if (c == 1) {
            char name[NAME_LEN];
            read_line("  Nurse name: ", name, sizeof name);
            int id = ns_add(&H.nurses, name);
            if (id) printf("  Added with ID %d. Generate the roster again to include her/him.\n", id);
            else printf("  Cannot add (list full).\n");
        } else if (c == 2) ns_print_nurses(&H.nurses);
        else if (c == 3) {
            int id = read_int("  Nurse ID: ", 1, 999999);
            int lv = read_int("  1 = on leave, 0 = back to work: ", 0, 1);
            if (ns_set_leave(&H.nurses, id, lv)) printf("  Updated. Regenerate the roster.\n");
            else printf("  No nurse with ID %d.\n", id);
        } else if (c == 4) {
            H.nurses.required = read_int("  Nurses per shift (1-5): ", 1, MAX_PER_SHIFT);
            H.nurses.generated = 0;
            printf("  Updated. Regenerate the roster.\n");
        } else if (c == 5) {
            if (H.nurses.count == 0) { printf("  Add nurses first (or load demo data).\n"); continue; }
            ns_generate(&H.nurses);
            ns_print_roster(&H.nurses);
        } else if (c == 6) ns_print_roster(&H.nurses);
        else if (c == 7) ns_print_nurse_schedule(&H.nurses, read_int("  Nurse ID: ", 1, 999999));
        else if (c == 8) {
            int d = read_int("  Day (1 = Mon .. 7 = Sun): ", 1, 7) - 1;
            int h = read_int("  Hour (0-23): ", 0, 23);
            ns_print_on_duty(&H.nurses, d, h);
        }
    }
}

static void ui_undo(void) {
    for (;;) {
        printf("\n--- UNDO / AUDIT LOG (stack) ---\n");
        printf("  1. Undo last admit/discharge\n  2. View audit log\n  0. Back\n");
        int c = read_int("  Choice: ", 0, 2);
        if (c == 0) return;
        if (c == 1) hosp_undo();
        else us_print(&H.undo, &H.patients);
    }
}

int main(void) {
    hosp_init();
    atexit(on_exit_cb);
    printf("==============================================\n");
    printf("   HOSPITAL MANAGEMENT SYSTEM  (DSA project)\n");
    printf("==============================================\n");
    storage_load();
    if (H.patients.count == 0 && H.nurses.count == 0)
        printf("Tip: choose option 8 to load demo data.\n");

    for (;;) {
        printf("\n=========== MAIN MENU ===========\n");
        printf("  1. Patient records      (hash table)\n");
        printf("  2. Emergency triage     (max-heap)\n");
        printf("  3. Beds and wards       (queue)\n");
        printf("  4. Organ transplant     (heap)\n");
        printf("  5. Ambulance routing    (graph, Dijkstra)\n");
        printf("  6. Nurse scheduling     (circular queue)\n");
        printf("  7. Undo / audit log     (stack)\n");
        printf("  8. Load demo data\n");
        printf("  0. Save and exit\n");
        int c = read_int("  Choice: ", 0, 8);
        switch (c) {
        case 1: ui_patients(); break;
        case 2: ui_triage(); break;
        case 3: ui_beds(); break;
        case 4: ui_transplant(); break;
        case 5: ui_ambulance(); break;
        case 6: ui_nurses(); break;
        case 7: ui_undo(); break;
        case 8: hosp_load_demo(); break;
        case 0:
            printf("Data saved. Goodbye.\n");
            return 0;
        }
    }
}
