#ifndef PATIENT_H
#define PATIENT_H

#define NAME_LEN      50
#define COMPLAINT_LEN 60
#define HASH_SIZE     101

enum { ST_REGISTERED = 0, ST_TRIAGE, ST_ADMITTED, ST_WAITING_BED, ST_DISCHARGED };

typedef struct {
    int  id;
    char name[NAME_LEN];
    int  age;
    char blood[4];
    int  severity;              /* 1 (minor) .. 10 (critical) */
    char complaint[COMPLAINT_LEN];
    int  status;
    int  ward;                  /* -1 if none */
    int  bed;                   /* -1 if none */
    long seq;                   /* arrival order, used as tie-breaker */
} Patient;

/* ---- Hash table with separate chaining (key = patient id) ---- */
typedef struct HNode {
    Patient p;
    struct HNode *next;
} HNode;

typedef struct {
    HNode *bucket[HASH_SIZE];
    int count;
    int next_id;
} HashTable;

typedef void (*PatientVisitor)(Patient *p, void *ctx);

void     ht_init(HashTable *t);
void     ht_free(HashTable *t);
Patient *ht_insert(HashTable *t, const Patient *p);   /* NULL if id exists */
Patient *ht_find(const HashTable *t, int id);
int      ht_remove(HashTable *t, int id);
void     ht_foreach(const HashTable *t, PatientVisitor fn, void *ctx);
void     ht_print_all(const HashTable *t);
void     ht_search_name(const HashTable *t, const char *part);
void     patient_print_header(void);
void     patient_print(const Patient *p);
const char *status_name(int st);

#endif
