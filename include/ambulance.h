#ifndef AMBULANCE_H
#define AMBULANCE_H

#define G_N 10

typedef struct {
    char name[28];
    int  is_hospital;
} Place;

/* Weighted undirected graph (adjacency matrix). Weight = travel minutes, 0 = no road. */
typedef struct {
    Place place[G_N];
    int   w[G_N][G_N];
    int   blocked[G_N][G_N];
} CityGraph;

void cg_init(CityGraph *g);
void cg_print_places(const CityGraph *g);
void cg_print_roads(const CityGraph *g);
int  cg_route(const CityGraph *g, int src, int dst, int path[], int *len, int *total);
int  cg_nearest_hospital(const CityGraph *g, int src, int *hosp, int path[], int *len, int *total);
int  cg_set_blocked(CityGraph *g, int a, int b, int blocked);   /* 0 if no such road */
void cg_print_path(const CityGraph *g, const int path[], int len, int total);

#endif
