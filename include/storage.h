#ifndef STORAGE_H
#define STORAGE_H

/* Plain-text persistence: patients.dat, nurses.dat, waitlist.dat (in the current folder). */
int  storage_save(void);
void storage_load(void);

#endif
