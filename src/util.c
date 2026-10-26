#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "util.h"

static void trim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

static void flush_rest_of_line(const char *buf) {
    if (!strchr(buf, '\n')) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
}

void read_line(const char *prompt, char *buf, size_t size) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, (int)size, stdin)) {
            printf("\nInput closed. Saving and exiting.\n");
            exit(0);
        }
        flush_rest_of_line(buf);
        trim(buf);
        for (char *p = buf; *p; p++)            /* '|' is our file separator */
            if (*p == '|') *p = ' ';
        if (buf[0] != '\0') return;
        printf("  Input cannot be empty.\n");
    }
}

int read_int(const char *prompt, int lo, int hi) {
    char buf[64];
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) {
            printf("\nInput closed. Saving and exiting.\n");
            exit(0);
        }
        flush_rest_of_line(buf);
        char *end;
        errno = 0;
        long v = strtol(buf, &end, 10);
        while (*end && isspace((unsigned char)*end)) end++;
        if (end == buf || *end != '\0' || errno != 0 || v < lo || v > hi) {
            printf("  Please enter a number between %d and %d.\n", lo, hi);
            continue;
        }
        return (int)v;
    }
}

static const char *BLOOD[] = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};

int valid_blood(const char *s) {
    for (size_t i = 0; i < sizeof BLOOD / sizeof BLOOD[0]; i++)
        if (strcmp(s, BLOOD[i]) == 0) return 1;
    return 0;
}

void read_blood(const char *prompt, char *out) {
    char tmp[32];
    for (;;) {
        read_line(prompt, tmp, sizeof tmp);
        for (char *p = tmp; *p; p++) *p = (char)toupper((unsigned char)*p);
        if (valid_blood(tmp)) {
            strcpy(out, tmp);               /* max 3 chars + NUL = 4 */
            return;
        }
        printf("  Invalid blood group. Use A+, A-, B+, B-, AB+, AB-, O+ or O-.\n");
    }
}

static void abo_group(const char *b, char *out) {
    size_t k = 0;
    while (b[k] && b[k] != '+' && b[k] != '-' && k < 7) { out[k] = b[k]; k++; }
    out[k] = '\0';
}

int blood_compatible(const char *donor, const char *recipient) {
    char d[8], r[8];
    abo_group(donor, d);
    abo_group(recipient, r);
    if (strcmp(d, "O") == 0)  return 1;
    if (strcmp(d, "A") == 0)  return strcmp(r, "A") == 0 || strcmp(r, "AB") == 0;
    if (strcmp(d, "B") == 0)  return strcmp(r, "B") == 0 || strcmp(r, "AB") == 0;
    if (strcmp(d, "AB") == 0) return strcmp(r, "AB") == 0;
    return 0;
}
