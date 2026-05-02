#define _CRT_SECURE_NO_WARNINGS
/* hub_search.c — City BST with linked list of parcels per city */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hub_search.h"
#include "storage.h"
#include "search.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

typedef struct PNode { char f[15][256]; struct PNode *next; } PNode;
typedef struct CNode { char city[50]; PNode *list; struct CNode *l, *r; } CNode;

static void title(char *s) {
    if (!s || !*s) return;
    s[0] = toupper((unsigned char)s[0]);
    for (int i = 1; s[i]; i++) s[i] = tolower((unsigned char)s[i]);
}

static CNode* insert(CNode *root, const char *city, PNode *p) {
    if (!root) {
        CNode *n = calloc(1, sizeof(CNode));
        strncpy(n->city, city, 49); n->list = p; return n;
    }
    int c = strcasecmp(city, root->city);
    if (c < 0) root->l = insert(root->l, city, p);
    else if (c > 0) root->r = insert(root->r, city, p);
    else { p->next = root->list; root->list = p; }
    return root;
}

static CNode* lookup(CNode *root, const char *city) {
    if (!root) return NULL;
    int c = strcasecmp(city, root->city);
    return (c == 0) ? root : lookup(c < 0 ? root->l : root->r, city);
}

static void free_tree(CNode *n) {
    if (!n) return;
    free_tree(n->l); free_tree(n->r);
    for (PNode *c = n->list, *t; c; c = t) { t = c->next; free(c); }
    free(n);
}

static void print_result(PNode *p, const char *dir, StatusNode *sr) {
    StatusNode *s = search_status_bst(sr, p->f[0]);
    printf("RESULT_START\nDirection: %s\nTracking No: %s\nSender: %s | %s | %s | %s\nReceiver: %s | %s | %s | %s\nWeight: %s\nType: %s\nInstructions: %s\nPriority: %s\nDate: %s\nTime: %s\nStatus: %s\nRESULT_END\n",
           dir, p->f[0], p->f[1], p->f[2], p->f[3], p->f[4], p->f[5], p->f[6], p->f[7], p->f[8],
           p->f[9], p->f[10], p->f[11], atoi(p->f[12]) ? "Express" : "Standard",
           s ? s->date : p->f[13], s ? s->time_str : p->f[14], s ? s->status : "Booked");
}

void search_by_hub(const char *city) {
    char hub[50]; strncpy(hub, city, 49); hub[49] = '\0'; title(hub);
    FILE *f = fopen("parcels.csv", "r");
    if (!f) { printf("NO_RESULTS\n"); return; }

    CNode *root = NULL; char line[2048];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        PNode *p = calloc(1, sizeof(PNode)); int pos = 0;
        for (int i = 0; i < 15; i++) pos = parse_csv_field(line, pos, p->f[i], 256);
        title(p->f[4]); title(p->f[8]); p->next = NULL;
        root = insert(root, p->f[4], p);
        if (strcasecmp(p->f[4], p->f[8]) != 0) {
            PNode *p2 = malloc(sizeof(PNode)); *p2 = *p; p2->next = NULL;
            root = insert(root, p->f[8], p2);
        }
    }
    fclose(f);

    CNode *res = lookup(root, hub);
    if (res) {
        StatusNode *sr = load_status_to_bst("status_log.csv");
        for (PNode *p = res->list; p; p = p->next) {
            if (strcasecmp(p->f[4], hub) == 0) print_result(p, "FROM", sr);
            if (strcasecmp(p->f[8], hub) == 0) print_result(p, "TO", sr);
        }
        free_status_bst(sr);
    } else printf("NO_RESULTS\n");

    free_tree(root);
}