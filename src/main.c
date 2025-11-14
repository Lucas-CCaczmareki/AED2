#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/*
 B-Tree implementation (order m = max children).
 This file includes:
 - create/init
 - insert (split-then-insert style)
 - search
 - remove (top-down, Cormen-style, using order m)
 - printing (visual)
 - free tree
*/


typedef struct {
    int matricula;
    // char nome[50];
} value;

struct page;               // forward declaration
typedef struct page page;

struct page {
    int numKeys;
    bool isLeaf;
    value* values;          // size m-1
    struct page** ptr;      // size m
    struct page* father;
};

//------------------------------------------------------
//                FUNÇÕES DE CONTROLE
//------------------------------------------------------

/* OK */
void initializePage(page* pg, int ordem, bool is_leaf) {
    pg->numKeys = 0;
    pg->isLeaf = is_leaf;
    pg->father = NULL;

    //Aloca memória e zera as chaves e ponteiros (null)
    pg->values = (value*)calloc(ordem - 1, sizeof(value));
    pg->ptr = (struct page**)calloc(ordem, sizeof(struct page*));

    if (pg->values == NULL || pg->ptr == NULL) {
        printf("ERRO: Falha ao alocar memória pra página!\n");
        exit(1);
    }
}

/* OK */
void createBTREE(page* root, int ordem) {
    initializePage(root, ordem, true);
    // return &root;
}

// Funções de print feitas pelo chat gpt
/* OK */
void print_tree(page* root, int ordem, int level) {
    if (!root) return;
    printf("Level %d | keys=%d : ", level, root->numKeys);
    for (int i = 0; i < root->numKeys; i++) printf("%d ", root->values[i].matricula);
    printf("\n");
    if (!root->isLeaf) {
        for (int i = 0; i <= root->numKeys; i++) {
            if (root->ptr[i]) print_tree(root->ptr[i], ordem, level + 1);
        }
    }
}
void print_tree_visual(page* root, int level, bool isLast) {
    if (!root) return;

    // imprime indentação
    for (int i = 0; i < level; i++) {
        printf("%s", (i == level - 1) ? (isLast ? "    " : "│   ") : "    ");
    }

    // imprime o nó atual
    printf("%s[", (level == 0) ? "ROOT " : "└── ");
    for (int i = 0; i < root->numKeys; i++) {
        printf("%d", root->values[i].matricula);
        if (i < root->numKeys - 1) printf(" | ");
    }
    printf("]\n");

    // imprime filhos (recursivamente)
    if (!root->isLeaf) {
        for (int i = 0; i <= root->numKeys; i++) {
            if (root->ptr[i]) {
                bool isLastChild = (i == root->numKeys);
                print_tree_visual(root->ptr[i], level + 1, isLastChild);
            }
        }
    }
}

void traverse(page* root) {
    if (!root) return;
    int i;

    for(i = 0; i < root->numKeys; i++) {
        if(!root->isLeaf) traverse(root->ptr[i]);
        printf("%d ", root->values[i].matricula);
    }
    if (!root->isLeaf) traverse(root->ptr[i]);
}

//------------------------------------------------------
//                  SPLIT THEN INSERT
//------------------------------------------------------
// Esse tipo de inserção é diferente do que eu estudei.
// O que eu estudei é o insert then split
// Porém esse possui uma programação mais intuitiva, ent vamo dale
//------------------------------------------------------

/* Essa função divide o filho 'i' do nó 'parent', que está cheio */
/* OK */
void splitChild(page* parent, int i, page* full_child, int ordem) {
    // printf("DEBUG: Chamando split do filho %d\n", i);

    // 1. Cria uma nova página (irmã)
    page* sister = (page*)malloc(sizeof(page));
    initializePage(sister, ordem, full_child->isLeaf);
    sister->father = parent;

    // 2. Encontrar a mediana da página cheia
    // Pra ordem 5, uma página cheia tem 4 chaves [0, 1, 2, 3]. Mediana é o índice 1. 
    int mid_index = full_child->numKeys / 2;
    value mid_key = full_child->values[mid_index];


    // 3. Copiar a segunda metade das chaves do nó cheio pra 'sister'
    int j = 0;
    for(int k = (mid_index + 1); k < full_child->numKeys; k++) {
        //Joga pra dentro da sister em ordem e atualiza o contador da sister
        sister->values[j++] = full_child->values[k];      
        
        // Atualiza a contagem de keys na sister
        sister->numKeys++;
    }

    // 4. Se não for folha, copia os ponteiros
    if(!full_child->isLeaf) {
        // Começa no índice à direita do valor do meio (o ponteiro à direita) e vai até o fim copiando os ponteiros
        for(int k = (mid_index + 1); k <= full_child->numKeys; k++) {
            sister->ptr[(k - (mid_index + 1))] = full_child->ptr[k];
        }
    }

    // 5. Atualiza a contagem do filho original
    full_child->numKeys = mid_index;

    // 6. Move as chaves do pai pra direita, abrindo espaço pra que foi promovida
    //Começa no primeiro índice livre (= numKeys, pois o indice começa em 0)
    for(int k = parent->numKeys - 1; k >= i; k--) {
        parent->values[k+1] = parent->values[(k)];
        parent->ptr[(k+2)] = parent->ptr[k+1]; //como o ponteiro tem um índice a mais
    }

    // 7. Insere a chave promovida e o novo ponteiro
    //i indica o ponteiro do filho a ser splitado que é = ao índice da posição que vai o número promovido
    parent->values[i] = mid_key; 
    parent->ptr[i+1] = sister;  //+1 pra preencher o ponteiro da direita com a nova página
    parent->numKeys++;

    //atualiza os pais das páginas após split
    full_child->father = parent;
    sister->father = parent;

    //debug
     printf("Split completo: subiu %d (mid_index=%d oldNum=%d)\n",
           mid_key.matricula, mid_index, 5);
}

// Insere a chave em um nó que garantidamente NÃO TÁ CHEIO
/* OK */
void insertIntoNonFull(page* pg, int key, int ordem) {

    if(pg->isLeaf) {
        // 1. Se é folha, acha a posição e insere
        printf("DEBUG: Inserindo %d em folha nao cheia.\n", key);

        int pos = pg->numKeys - 1; //pq o índice começa em 0.

        // loopa de trás pra frente
        // PUXA as chaves maiores para a direita
        while (pos >= 0 && key < pg->values[pos].matricula) {
            pg->values[pos+1] = pg->values[pos];
            pos--;
        }

        // Insere a chave
        pg->values[pos+1].matricula = key;
        pg->numKeys++;

    } else {
        // 2. Se NÃO é folha, acha o filho certo pra descer
        int i = pg->numKeys - 1;
        
        while(i >= 0 && key < pg->values[i].matricula) {
            i--;
        }
        i++; //desce pro ponteiro à direita da chave.

        // 3. Checa se o filho está cheio ANTES de descer
        if(pg->ptr[i] != NULL && pg->ptr[i]->numKeys == (ordem - 1)) {
            // Se o filho tá cheio, divide ele primeiro
            splitChild(pg, i, pg->ptr[i], ordem);

            //A chave a ser inserida pode ir pro filho original
            //ou pra nova irmã. Precisa checar
            if (key > pg->values[i].matricula) {
                i++;
            }
        }

        // 4. Chama recursivamente pro filho (que não tá cheio)
        insertIntoNonFull(pg->ptr[i], key, ordem);
    }
}

// Função insert principal que junta as outras
/* OK */
void insert(page** root_ptr, int key, int ordem) {
    page* root = *root_ptr;

    // Caso 1: A raiz tá cheia
    // Única maneirda de crescer a árvore em altura
    if (root->numKeys == (ordem - 1)) {
        // Cria uma nova raiz
        page* new_root = (page*)malloc(sizeof(page));
        initializePage(new_root, ordem, false); //nova raiz nunca é uma folha

        new_root->ptr[0] = root; //raiz antiga vira filha da nova
        root->father = new_root;

        // Dividir a raiz antiga (agora filha 0 da new_root)
        splitChild(new_root, 0, root, ordem);
        
        // New root começa com 1 chave. Escolhemos pra qual lado descer
        int i = 0;

        //Decide se vamo descer pela direita ou pela esquerda
        if (new_root->values[0].matricula < key) {
            i++;
        }

        insertIntoNonFull(new_root->ptr[i], key, ordem);

        // Atualiza o ponteiro da raiz no main
        *root_ptr = new_root;
    } else {
        // Caso 2: A raiz não tá cheia, só insere.
        insertIntoNonFull(root, key, ordem);
    }
}


//------------------------------------------------------
//                        BUSCA
//------------------------------------------------------
/* OK */
page* search(page* root, int key, int ordem) {
    //Desce até achar a key e retorna.
    //Se desceu o máximo (ponteiro == null) retorna null que significa que não tá na btree

    page* cur = root;

    while (cur != NULL) {
        // 1. Procura a primeira chave >= key dentro do nó
        int i;

        //Vai só até as chaves válidas (se a chave é 0 ela tá vazia)
        for(i = 0; cur->values[i].matricula != 0 && key > cur->values[i].matricula; i++) { //move o i até o nó correto
            //Se executando o loop pra última chave
            if(i == (ordem - 2)) { //-1 (keys) -1(index) = -2
                i++; //vai descer pelo ponteiro da direita por que se tá aqui já sabe que key é > o último.
                break;
            }
        } 
    
        // 2. Se encontrou exata: retorna cur
        if(key == cur->values[i].matricula) {
            return cur;
        }

        // 3. Se é folha retorna null (não existe)
        if(cur->isLeaf) {
            return NULL;
        }

        // 4. Caso contrário, desce pro filho apropriado
        cur = cur->ptr[i];
    }

    return NULL;
}

//------------------------------------------------------
//                       REMOÇÃO
//------------------------------------------------------
/* A remoção vai funcionar assim: vai descer a árvore normal buscando a chave pra remover
só que na hora de descer, vai conferir se o filho tá nos padrões mínimos da árvore, numa árvore de ordem 5 por exemplo
o mínimo de chaves é 2, então conferimos o numKey pra ver se ele é >= 2. Se for, tudo ok, desce e continua.

Se não, vai tratar os casos:
Caso 1(eu provavelmente programo numa função separada aqui): testa se o irmão da direita tem numKey >2 e 
tenta pegar o número mais a direita do irmão à esquerda, promove aquele número e desce o do pai pra ele. 
Se não der pela direita, tenta com o da esquerda, testa o numKey, se der, pega o mais a esquerda do da direita, promove,
e desce o do pai.

Caso 3: (outra função)
Se não der pela esquerda vai precisar fazer merge, ai escolhe o da direita (por exemplo) e faz merge
com a chave do pai e o filho.

Caso 2: a remoção está acontecendo num nó que não é folha. Testa se a direita numKeys > 2, se sim, pega o seu mais a
esquerda e bota no lugar do que vai ser removido. Depois desce pra direita e remove o seu mais a esquerda.
Se não der pela direita, tenta com a irmã esquerda, se numKeys > 2 pega o mais a direita e bota no lugar do que vai ser removido,
depois desce na esquerda e remove o mais a direita dela.
Se nenhum dos dois tem numKeys > 2 vai ter que fazer merge com os 2 e dps remover do pai.
Ai se o pai ficar fora dos padrão vai ter que fazer os testes pra ele... (oh inferno do krl)


*/

//retorna o mínimo de chaves que a página precisa ter
int minKeys_forOrder(int ordem) {
    // min_keys = ceil(m/2) - 1
    return ((ordem + 1) /2) -1;
}

//Acha o predecessor (número + a direita na subtree à esquerda)
int getPredecessor(page* node, int idx) {
    page* cur = node->ptr[idx];
    while (!cur->isLeaf) cur = cur->ptr[cur->numKeys];
    return cur->values[cur->numKeys - 1].matricula;
}

//Acha o sucessor (número + a esquerda na subtree à direita)
int getSuccessor(page* node, int idx) {
    page* cur = node->ptr[idx + 1];
    while (!cur->isLeaf) cur = cur->ptr[0];
    return cur->values[0].matricula;
}

void borrowFromLeft(page* parent, int childIndex, int ordem) {
    page* child = parent->ptr[childIndex];
    page* left = parent->ptr[childIndex - 1];

    //shifta as keys da filha pra direita
    for(int k = child->numKeys - 1; k >= 0; k--) {
        child->values[k + 1] = child->values[k];
    }

    //se não é uma folha (é página interna), shifta os ponteiros
    if(!child->isLeaf) {
        for (int k = child->numKeys; k >= 0; k--) {
            child->ptr[k + 1] = child->ptr[k];
        }
    }

    //move do pai pra filha
    child->values[0] = parent->values[childIndex - 1];
    if(!child->isLeaf) {
        child->ptr[0] = left->ptr[left->numKeys];
        if (child->ptr[0]) child->ptr[0]->father = child;
    }

    //move a última da esquerda pro pai
    parent->values[childIndex - 1] = left->values[left->numKeys - 1];

    //ajusta os contadores
    child->numKeys++;
    left->numKeys--;
}

void borrowFromRight(page* parent, int childIndex, int ordem) {
    page* child = parent->ptr[childIndex];
    page* right = parent->ptr[childIndex + 1];

    //Move 
    child->values[child->numKeys] = parent->values[childIndex];
    if(!child->isLeaf) {
        child->ptr[child->numKeys + 1] = right->ptr[0];
        if (child->ptr[child->numKeys + 1]) child->ptr[child->numKeys + 1]->father = child;
    }

    //Move a primeira chave da filha direita pro pai
    parent->values[childIndex] = right->values[0];

    //Move as chaves 1 pra esquerda
    for(int k = 0; k < right->numKeys - 1; k++) {
        right->values[k] = right->values[k + 1];
    }
    if (!right->isLeaf) {
        for (int k = 0; k < right->numKeys; k++) {
            right->ptr[k] = right->ptr[k + 1];
        }
    }
    right->ptr[right->numKeys] = NULL;
    
    //Atualiza os contadores
    child->numKeys++;
    right->numKeys--;
}

void mergeChildren(page* parent, int i, int ordem) {
    page* left = parent->ptr[i];
    page* right = parent->ptr[i + 1];

    //Move a chave do pai pro filho da esquerda
    left->values[left->numKeys] = parent->values[i];

    //copia as chaves do filho da direita pro filho da esquerda
    for (int k = 0; k < right->numKeys; k++) {
        left->values[left->numKeys + 1 + k] = right->values[k];
    }

    //Copia os ponteiros dos filhos se for necessário
    if (!left->isLeaf) {
        for (int k = 0; k <= right->numKeys; k++) {
            left->ptr[left->numKeys + 1 + k] = right->ptr[k];
            if(left->ptr[left->numKeys + 1 + k]) {
                left->ptr[left->numKeys + 1 + k]->father = left;
            }
        }
    }

    //atualiza o numKeys da esquerda
    left->numKeys = left->numKeys + 1 + right->numKeys;

    // move as chaves do pai e os ponteiros pra remover o valor e o ponteiro da direita
    for (int k = i; k < parent->numKeys - 1; k++) {
        parent->values[k] = parent->values[k + 1];
        parent->ptr[k + 1] = parent->ptr[k + 2];
    }
    parent->ptr[parent->numKeys] = NULL;
    parent->numKeys--;

    //libera o filho à direita
    free(right->values);
    free(right->ptr);
    free(right);
}

// remove uma chave de uma folha dado um índice
void removeFromLeaf(page* pg, int idx) {
    for (int k = idx; k < pg->numKeys - 1; k++) {
        pg->values[k] = pg->values[k+1];
    }
    pg->numKeys--;
}

// remove uma chave de uma página interna dado um índice
void removeFromInternal(page** root_ptr, page* pg, int idx, int ordem) {
    int min = minKeys_forOrder(ordem);
    int k = pg->values[idx].matricula;

    // se o filho à esquerda tiver mais que o mínimo de chaves, substituímos pelo predecessor
    if (pg->ptr[idx]->numKeys > min) {
        int pred = getPredecessor(pg, idx);
        pg->values[idx].matricula = pred;

        page* child = pg->ptr[idx];
    } else if (pg->ptr[idx + 1]->numKeys > min) {
        int succ = getSuccessor(pg, idx);
        pg->values[idx].matricula = succ;
    } else {
        //merge
    }
}

//Forward declaration
void removeInternal(page** root_ptr, page* pg, int key, int ordem);

void removeInternal(page** root_ptr, page* pg, int key, int ordem) {
    if (!pg) return;
    int min = minKeys_forOrder(ordem);

    int i = 0;
    while(i < pg->numKeys && key > pg->values[i].matricula) i++;

    //CASO 1: Chave tá nessa página
    if (i < pg->numKeys && pg->values[i].matricula == key) {
        if (pg->isLeaf) {
            removeFromLeaf(pg, i);
            return;
        } else {
            //internal node
            //se filha esquerda tem > min keys, troca pelo predecessor
            if(pg->ptr[i]->numKeys > min) {
                int pred = getPredecessor(pg, i);
                pg->values[i].matricula = pred;
                removeInternal(root_ptr, pg->ptr[i], pred, ordem);
                return;
            }
            //se filha direita tem > min keys, troca pelo sucessor
            else if (pg->ptr[i+1]->numKeys > min) {
                int succ = getSuccessor(pg, i);
                pg->values[i].matricula = succ;
                removeInternal(root_ptr, pg->ptr[i + 1], succ, ordem);
                return;
            }
            //ambas filhas tem <= ao mínimo. Merge e faz recursão na que tomou
            else {
                mergeChildren(pg, i, ordem); //merge child i e i+1 na child i
                removeInternal(root_ptr, pg->ptr[i], key, ordem);
                return;
            }
        }
    } else {
        //CASO 2: a chave não tá nessa página, desce prauma filha
        if(pg->isLeaf) {
            return; //chave n ta aqui (por ja tamo numa folha e n tava)
        }
        page* child = pg->ptr[i];

        if(!child) return; //double check

        if(child->numKeys < min) {
            page* left = (i > 0 ? pg->ptr[i - 1] : NULL);
            page* right = (i < pg->numKeys ? pg->ptr[i + 1] : NULL);

            //tenta pegar emprestado da esquerda
            if (left && left->numKeys > min) {
                borrowFromLeft(pg, i, ordem);
            } 
            //Tenta pegar emprestado da direita
            else if (right && right->numKeys > min) {
                borrowFromRight(pg, i, ordem);
            } else {
                //merge com algum irmão. Prioriza direito, se não tiver, vai pro esquerdo
                if (right) {
                    mergeChildren(pg, i, ordem); //merge child i e i+1 na child i
                    child = pg->ptr[i];
                } else if (left) {
                    // merge dos filhos direito e esquerdo
                    mergeChildren(pg, i - 1, ordem);
                    child = pg->ptr[i - 1];
                    i = i - 1;
                }
            }
        }
        removeInternal(root_ptr, child, key, ordem);
    }
}

void removeKey(page** root_ptr, int key, int ordem) {
    if (!root_ptr || !(*root_ptr)) return;
    page* root = *root_ptr;
    removeInternal(root_ptr, root, key, ordem);

    //
    if (root->numKeys == 0) {
        if(!root->isLeaf) {
            page* new_root = root->ptr[0];
            new_root->father = NULL;

            free(root->values);
            free(root->ptr);
            free(root);
            *root_ptr = new_root;
        } else {
            free(root->values);
            free(root->ptr);
            free(root);
            *root_ptr = NULL;
        }
    }
}

void freeTree(page* n, int ordem) {
    if (!n) return;
    if (!n->isLeaf) {
        for (int i = 0; i <= n->numKeys; i++) {
            if (n->ptr[i]) freeTree(n->ptr[i], ordem);
        }
    }
    free(n->values);
    free(n->ptr);
    free(n);
}

/* -------------------- test main -------------------- */
int main(void) {
    int ordem = 5;
    page* root = (page*)malloc(sizeof(page));
    createBTREE(root, ordem);

    int keys[] = {10, 20, 30, 40, 50, 60, 70, 80};
    int nkeys = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < nkeys; i++) {
        printf("Insert %d\n", keys[i]);
        insert(&root, keys[i], ordem);
        print_tree_visual(root, 0, true);
        printf("-----\n");
    }

    printf("\nTraverse: ");
    traverse(root);
    printf("\n");

    printf("\nNow removing some keys\n");
    removeKey(&root, 30, ordem);
    print_tree_visual(root, 0, true);
    printf("-----\n");

    removeKey(&root, 10, ordem);
    print_tree_visual(root, 0, true);
    printf("-----\n");

    removeKey(&root, 50, ordem);
    print_tree_visual(root, 0, true);
    printf("-----\n");

    // cleanup
    if (root) freeTree(root, ordem);
    return 0;
}
