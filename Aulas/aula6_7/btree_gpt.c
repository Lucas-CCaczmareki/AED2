#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/*
 Fixed B-Tree (order m = max children).
 - keeps your struct page layout
 - split/insert (split-then-insert)
 - search
 - remove (top-down, borrow/merge)
 - print visual
 - free tree
*/

typedef struct {
    int matricula;
    // char nome[50];
} value;

struct page;
typedef struct page page;

struct page {
    int numKeys;
    bool isLeaf;
    value* values;       // size m-1
    struct page** ptr;   // size m
    struct page* father;
};

/* initialize a page */
void initializePage(page* pg, int ordem, bool is_leaf) {
    pg->numKeys = 0;
    pg->isLeaf = is_leaf;
    pg->father = NULL;
    pg->values = (value*)calloc(ordem - 1, sizeof(value));
    pg->ptr = (page**)calloc(ordem, sizeof(page*));
    if (!pg->values || !pg->ptr) {
        perror("calloc");
        exit(1);
    }
}

/* create root */
void createBTREE(page* root, int ordem) {
    initializePage(root, ordem, true);
}

/* print tree visual */
void print_tree_visual(page* root, int level, bool isLast) {
    if (!root) return;
    for (int i = 0; i < level; i++) {
        printf("%s", (i == level - 1) ? (isLast ? "    " : "│   ") : "    ");
    }
    printf("%s[", (level == 0) ? "ROOT " : "└── ");
    for (int i = 0; i < root->numKeys; i++) {
        if (i) printf(" | ");
        printf("%d", root->values[i].matricula);
    }
    printf("]\n");
    if (!root->isLeaf) {
        for (int i = 0; i <= root->numKeys; i++) {
            if (root->ptr[i]) {
                bool last = (i == root->numKeys);
                print_tree_visual(root->ptr[i], level + 1, last);
            }
        }
    }
}

/* traverse in-order (debug) */
void traverse(page* root) {
    if (!root) return;
    int i;
    for (i = 0; i < root->numKeys; i++) {
        if (!root->isLeaf) traverse(root->ptr[i]);
        printf("%d ", root->values[i].matricula);
    }
    if (!root->isLeaf) traverse(root->ptr[i]);
}

/* split child parent->ptr[i] which is full_child */
void splitChild(page* parent, int i, page* full_child, int ordem) {
    page* sister = (page*)malloc(sizeof(page));
    initializePage(sister, ordem, full_child->isLeaf);
    sister->father = parent;

    int oldNum = full_child->numKeys;
    int mid_index = oldNum / 2;              // use current number of keys (works for even/odd)
    value midKey = full_child->values[mid_index];

    // copy keys after mid to sister
    int j = 0;
    for (int k = mid_index + 1; k < oldNum; k++) {
        sister->values[j++] = full_child->values[k];
        sister->numKeys++;
    }

    // copy children if internal
    if (!full_child->isLeaf) {
        for (int k = mid_index + 1; k <= oldNum; k++) {
            sister->ptr[k - (mid_index + 1)] = full_child->ptr[k];
            if (sister->ptr[k - (mid_index + 1)]) sister->ptr[k - (mid_index + 1)]->father = sister;
        }
    }

    // reduce full_child key count
    full_child->numKeys = mid_index;

    // shift parent keys and ptrs to make room
    for (int k = parent->numKeys - 1; k >= i; k--) {
        parent->values[k + 1] = parent->values[k];
        parent->ptr[k + 2] = parent->ptr[k + 1];
    }

    parent->values[i] = midKey;
    parent->ptr[i + 1] = sister;
    parent->numKeys++;

    full_child->father = parent;
    sister->father = parent;
}

/* insert into non-full node */
void insertIntoNonFull(page* pg, int key, int ordem) {
    if (pg->isLeaf) {
        int pos = pg->numKeys - 1;
        while (pos >= 0 && key < pg->values[pos].matricula) {
            pg->values[pos + 1] = pg->values[pos];
            pos--;
        }
        pg->values[pos + 1].matricula = key;
        pg->numKeys++;
    } else {
        int i = pg->numKeys - 1;
        while (i >= 0 && key < pg->values[i].matricula) i--;
        i++;
        if (pg->ptr[i] == NULL) {
            // Shouldn't normally happen if tree is correct, but be safe
            pg->ptr[i] = (page*)malloc(sizeof(page));
            initializePage(pg->ptr[i], ordem, true);
            pg->ptr[i]->father = pg;
        }
        if (pg->ptr[i]->numKeys == (ordem - 1)) {
            splitChild(pg, i, pg->ptr[i], ordem);
            if (key > pg->values[i].matricula) i++;
        }
        insertIntoNonFull(pg->ptr[i], key, ordem);
    }
}

/* main insert */
void insert(page** root_ptr, int key, int ordem) {
    page* root = *root_ptr;
    if (root->numKeys == (ordem - 1)) {
        page* new_root = (page*)malloc(sizeof(page));
        initializePage(new_root, ordem, false);
        new_root->ptr[0] = root;
        root->father = new_root;
        splitChild(new_root, 0, root, ordem);
        int i = 0;
        if (new_root->values[0].matricula < key) i++;
        insertIntoNonFull(new_root->ptr[i], key, ordem);
        *root_ptr = new_root;
    } else {
        insertIntoNonFull(root, key, ordem);
    }
}

/* search */
page* search(page* root, int key) {
    page* cur = root;
    while (cur != NULL) {
        int i = 0;
        while (i < cur->numKeys && key > cur->values[i].matricula) i++;
        if (i < cur->numKeys && key == cur->values[i].matricula) return cur;
        if (cur->isLeaf) return NULL;
        cur = cur->ptr[i];
    }
    return NULL;
}

/* helpers for removal */
int min_keys_for_order(int ordem) {
    int t = (ordem + 1) / 2; // ceil(m/2)
    return t - 1;
}

/* get predecessor (max in subtree rooted at node->ptr[idx]) */
int getPredecessor(page* node, int idx) {
    page* cur = node->ptr[idx];
    while (!cur->isLeaf) {
        cur = cur->ptr[cur->numKeys];
        if (!cur) break;
    }
    return cur->values[cur->numKeys - 1].matricula;
}

/* get successor (min in subtree rooted at node->ptr[idx+1]) */
int getSuccessor(page* node, int idx) {
    page* cur = node->ptr[idx + 1];
    while (!cur->isLeaf) {
        cur = cur->ptr[0];
        if (!cur) break;
    }
    return cur->values[0].matricula;
}

/* borrow from left sibling */
void borrowFromLeft(page* parent, int childIndex, int ordem) {
    page* child = parent->ptr[childIndex];
    page* left = parent->ptr[childIndex - 1];

    // shift child's keys and pointers right by 1
    for (int k = child->numKeys - 1; k >= 0; k--) child->values[k + 1] = child->values[k];
    if (!child->isLeaf) {
        for (int k = child->numKeys; k >= 0; k--) child->ptr[k + 1] = child->ptr[k];
    }

    // move parent's separator down to child[0]
    child->values[0] = parent->values[childIndex - 1];

    // if internal, attach subtree from left (the pointer right of left's last key)
    if (!child->isLeaf) {
        child->ptr[0] = left->ptr[left->numKeys];
        if (child->ptr[0]) child->ptr[0]->father = child;
        left->ptr[left->numKeys] = NULL;
    }

    // move left's last key up to parent
    parent->values[childIndex - 1] = left->values[left->numKeys - 1];

    // adjust counts
    child->numKeys++;
    left->numKeys--;
}

/* borrow from right sibling */
void borrowFromRight(page* parent, int childIndex, int ordem) {
    page* child = parent->ptr[childIndex];
    page* right = parent->ptr[childIndex + 1];

    // move parent's separator down to child's last position
    child->values[child->numKeys] = parent->values[childIndex];

    if (!child->isLeaf) {
        child->ptr[child->numKeys + 1] = right->ptr[0];
        if (child->ptr[child->numKeys + 1]) child->ptr[child->numKeys + 1]->father = child;
    }

    // move right's first key up to parent
    parent->values[childIndex] = right->values[0];

    // shift right's keys left
    for (int k = 0; k < right->numKeys - 1; k++) right->values[k] = right->values[k + 1];
    if (!right->isLeaf) {
        for (int k = 0; k < right->numKeys; k++) right->ptr[k] = right->ptr[k + 1];
    }
    // clear moved pointer
    right->ptr[right->numKeys - 1] = NULL;

    child->numKeys++;
    right->numKeys--;
}

/* merge child i and i+1 into child i */
void mergeChildren(page* parent, int i, int ordem) {
    page* left = parent->ptr[i];
    page* right = parent->ptr[i + 1];

    // append parent's key
    left->values[left->numKeys] = parent->values[i];

    // copy right keys
    for (int k = 0; k < right->numKeys; k++) left->values[left->numKeys + 1 + k] = right->values[k];

    // if internal, copy right pointers
    if (!left->isLeaf) {
        for (int k = 0; k <= right->numKeys; k++) {
            left->ptr[left->numKeys + 1 + k] = right->ptr[k];
            if (left->ptr[left->numKeys + 1 + k]) left->ptr[left->numKeys + 1 + k]->father = left;
        }
    }

    // update left numKeys (use old left->numKeys saved)
    left->numKeys = left->numKeys + 1 + right->numKeys;

    // shift parent keys and ptrs left to remove parent's key i and pointer i+1
    for (int k = i; k < parent->numKeys - 1; k++) {
        parent->values[k] = parent->values[k + 1];
        parent->ptr[k + 1] = parent->ptr[k + 2];
    }
    parent->ptr[parent->numKeys] = NULL;
    parent->numKeys--;

    // free right node
    free(right->values);
    free(right->ptr);
    free(right);
}

/* remove key from leaf at idx */
void removeFromLeaf(page* node, int idx) {
    for (int k = idx; k < node->numKeys - 1; k++) node->values[k] = node->values[k + 1];
    node->numKeys--;
}

/* forward */
void removeInternal(page** root_ptr, page* node, int key, int ordem);

/* recursive remove */
void removeInternal(page** root_ptr, page* node, int key, int ordem) {
    if (!node) return;
    int min = min_keys_for_order(ordem);
    int i = 0;
    while (i < node->numKeys && key > node->values[i].matricula) i++;

    /* CASE 1: key is in this node */
    if (i < node->numKeys && node->values[i].matricula == key) {
        if (node->isLeaf) {
            removeFromLeaf(node, i);
            return;
        } else {
            // internal node
            page* left = node->ptr[i];
            page* right = node->ptr[i + 1];
            if (left && left->numKeys > min) {
                int pred = getPredecessor(node, i);
                node->values[i].matricula = pred;
                removeInternal(root_ptr, left, pred, ordem);
                return;
            } else if (right && right->numKeys > min) {
                int succ = getSuccessor(node, i);
                node->values[i].matricula = succ;
                removeInternal(root_ptr, right, succ, ordem);
                return;
            } else {
                // both have min -> merge left, key, right
                mergeChildren(node, i, ordem); // merged into node->ptr[i]
                // after merge, recurse on merged child
                removeInternal(root_ptr, node->ptr[i], key, ordem);
                return;
            }
        }
    } else {
        /* CASE 2: key is not in this node -> descend to child i */
        if (node->isLeaf) {
            // key not present
            return;
        }
        page* child = node->ptr[i];
        if (!child) return;

        // ensure child has at least min keys before descending
        if (child->numKeys < min) {
            page* left = (i > 0 ? node->ptr[i - 1] : NULL);
            page* right = (i < node->numKeys ? node->ptr[i + 1] : NULL);

            if (left && left->numKeys > min) {
                borrowFromLeft(node, i, ordem);
            } else if (right && right->numKeys > min) {
                borrowFromRight(node, i, ordem);
            } else {
                // merge preference: if right exists, merge child with right (i), else merge left with child (i-1)
                if (right) {
                    mergeChildren(node, i, ordem); // merges child i and i+1 into child i
                    child = node->ptr[i]; // merged node at i
                } else if (left) {
                    mergeChildren(node, i - 1, ordem); // merges left(i-1) and child(i) into left(i-1)
                    child = node->ptr[i - 1];
                    i = i - 1;
                }
            }
        }
        // descend
        removeInternal(root_ptr, child, key, ordem);
    }
}

/* remove wrapper */
void removeKey(page** root_ptr, int key, int ordem) {
    if (!root_ptr || !(*root_ptr)) return;
    page* root = *root_ptr;
    removeInternal(root_ptr, root, key, ordem);

    // if root became empty, adjust
    if (root->numKeys == 0) {
        if (!root->isLeaf) {
            page* new_root = root->ptr[0];
            if (new_root) new_root->father = NULL;
            free(root->values);
            free(root->ptr);
            free(root);
            *root_ptr = new_root;
        } else {
            // tree empty
            free(root->values);
            free(root->ptr);
            free(root);
            *root_ptr = NULL;
        }
    }
}

/* free entire tree recursively */
void freeTree(page* n) {
    if (!n) return;
    if (!n->isLeaf) {
        for (int i = 0; i <= n->numKeys; i++) {
            if (n->ptr[i]) freeTree(n->ptr[i]);
        }
    }
    free(n->values);
    free(n->ptr);
    free(n);
}
int main(void) {
    int ordem;

    printf("Digite a ordem da arvore B (>=3): ");
    scanf("%d", &ordem);

    page* root = (page*)malloc(sizeof(page));
    createBTREE(root, ordem);

    int op, x;

    while (1) {
        printf("\n========== MENU ==========\n");
        printf("1 — Inserir\n");
        printf("2 — Remover\n");
        printf("3 — Buscar\n");
        printf("4 — Imprimir arvore\n");
        printf("5 — Travessia (in-order)\n");
        printf("0 — Sair\n");
        printf("==========================\n");
        printf("Escolha: ");
        scanf("%d", &op);

        switch (op) {
            case 1:
                printf("Digite a chave para inserir: ");
                scanf("%d", &x);
                insert(&root, x, ordem);
                printf("Inserido!\n");
                break;

            case 2:
                printf("Digite a chave para remover: ");
                scanf("%d", &x);
                removeKey(&root, x, ordem);
                printf("Removido!\n");
                break;

            case 3: {
                printf("Digite a chave para buscar: ");
                scanf("%d", &x);
                page* r = search(root, x);
                if (r) printf("Chave %d encontrada!\n", x);
                else   printf("Chave %d NAO encontrada.\n", x);
            } break;

            case 4:
                if (root) print_tree_visual(root, 0, true);
                else printf("Arvore vazia.\n");
                break;

            case 5:
                printf("In-order: ");
                traverse(root);
                printf("\n");
                break;

            case 0:
                printf("Fechando...\n");
                freeTree(root);
                return 0;

            default:
                printf("Opcao invalida!\n");
        }
    }
}