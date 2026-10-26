#include <stdio.h>
#include <string.h>
#include "ambulance.h"

#define INF 1000000000

static void add_edge(CityGraph *g, int a, int b, int minutes) {
    g->w[a][b] = g->w[b][a] = minutes;
}

void cg_init(CityGraph *g) {
    static const char *names[G_N] = {
        "Central Hospital", "Railway Station", "Airport", "Market Square",
        "University Campus", "Old Town", "Riverside", "Tech Park",
        "Sunrise Hospital", "Lakeview Colony"
    };
    memset(g, 0, sizeof *g);
    for (int i = 0; i < G_N; i++) snprintf(g->place[i].name, sizeof g->place[i].name, "%s", names[i]);
    g->place[0].is_hospital = 1;
    g->place[8].is_hospital = 1;

    add_edge(g, 0, 1, 4);
    add_edge(g, 0, 3, 5);
    add_edge(g, 0, 5, 9);
    add_edge(g, 1, 3, 3);
    add_edge(g, 1, 2, 12);
    add_edge(g, 3, 4, 6);
    add_edge(g, 3, 5, 7);
    add_edge(g, 4, 7, 8);
    add_edge(g, 4, 8, 9);
    add_edge(g, 5, 6, 5);
    add_edge(g, 6, 8, 6);
    add_edge(g, 6, 9, 8);
    add_edge(g, 2, 7, 10);
    add_edge(g, 7, 9, 4);
    add_edge(g, 8, 9, 7);
}

void cg_print_places(const CityGraph *g) {
    for (int i = 0; i < G_N; i++)
        printf("   %d. %s%s\n", i, g->place[i].name, g->place[i].is_hospital ? "  [HOSPITAL]" : "");
}

void cg_print_roads(const CityGraph *g) {
    printf("  Roads (travel time in minutes):\n");
    for (int i = 0; i < G_N; i++)
        for (int j = i + 1; j < G_N; j++)
            if (g->w[i][j] > 0)
                printf("   %-18s <-> %-18s %2d min%s\n", g->place[i].name, g->place[j].name,
                       g->w[i][j], g->blocked[i][j] ? "   ** BLOCKED **" : "");
}

/* Dijkstra, O(V^2) with an array instead of a priority queue (V is tiny). */
static void dijkstra(const CityGraph *g, int src, int dist[], int prev[]) {
    int done[G_N] = {0};
    for (int i = 0; i < G_N; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;
    for (int it = 0; it < G_N; it++) {
        int u = -1;
        for (int i = 0; i < G_N; i++)
            if (!done[i] && dist[i] < INF && (u == -1 || dist[i] < dist[u])) u = i;
        if (u == -1) break;
        done[u] = 1;
        for (int v = 0; v < G_N; v++) {
            if (g->w[u][v] > 0 && !g->blocked[u][v] && !done[v] &&
                dist[u] + g->w[u][v] < dist[v]) {
                dist[v] = dist[u] + g->w[u][v];
                prev[v] = u;
            }
        }
    }
}

static int build_path(const int prev[], int src, int dst, int path[]) {
    int tmp[G_N], n = 0, cur = dst;
    while (cur != -1 && n < G_N) {
        tmp[n++] = cur;
        if (cur == src) break;
        cur = prev[cur];
    }
    if (n == 0 || tmp[n - 1] != src) return 0;
    for (int i = 0; i < n; i++) path[i] = tmp[n - 1 - i];
    return n;
}

int cg_route(const CityGraph *g, int src, int dst, int path[], int *len, int *total) {
    int dist[G_N], prev[G_N];
    if (src < 0 || src >= G_N || dst < 0 || dst >= G_N) return 0;
    dijkstra(g, src, dist, prev);
    if (dist[dst] >= INF) return 0;
    *len = build_path(prev, src, dst, path);
    *total = dist[dst];
    return *len > 0;
}

int cg_nearest_hospital(const CityGraph *g, int src, int *hosp, int path[], int *len, int *total) {
    int dist[G_N], prev[G_N], best = -1;
    if (src < 0 || src >= G_N) return 0;
    dijkstra(g, src, dist, prev);
    for (int i = 0; i < G_N; i++)
        if (g->place[i].is_hospital && dist[i] < INF && (best == -1 || dist[i] < dist[best])) best = i;
    if (best == -1) return 0;
    *hosp = best;
    *len = build_path(prev, src, best, path);
    *total = dist[best];
    return *len > 0;
}

int cg_set_blocked(CityGraph *g, int a, int b, int blocked) {
    if (a < 0 || a >= G_N || b < 0 || b >= G_N || g->w[a][b] <= 0) return 0;
    g->blocked[a][b] = g->blocked[b][a] = blocked;
    return 1;
}

void cg_print_path(const CityGraph *g, const int path[], int len, int total) {
    printf("  Route: ");
    for (int i = 0; i < len; i++) printf("%s%s", g->place[path[i]].name, i + 1 < len ? " -> " : "");
    printf("\n  Estimated travel time: %d minute(s)\n", total);
}
