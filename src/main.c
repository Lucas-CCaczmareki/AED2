#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// B TREE ORDEM 5 (exemplo)
// Uma ordem 'm' significa:
// Max de chaves: m-1           (4)
// Regra de 50% de ocupação não está sendo usada
// Max de filhos: m             (5)
// Min de filhos: (m/2)         (3)


typedef struct {
    int matricula;
    // char nome[50];
} value;

struct page;               // forward declaration
typedef struct page page;

struct page {
    int numKeys;
    bool isLeaf;
    value* values;
    struct page** ptr;
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

freeTree(page* pg) {
    //libera todos os filhos
    //libera a página
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


int main() {
    int ordem = 5;
    page* root = (page*)malloc(sizeof(page));
    createBTREE(root, ordem);

    int keys[] = {10, 20, 30, 40, 50, 60, 70, 80};
    for (int i = 0; i < 8; i++) {
        printf("\nInserindo %d...\n", keys[i]);
        insert(&root, keys[i], ordem);
    }

    printf("\nRaiz final (%d chaves):\n", root->numKeys);
    for (int i = 0; i < root->numKeys; i++) {
        printf(" %d", root->values[i].matricula);
    }
    printf("\n");

    print_tree(root, ordem, 0);

    printf("\n--- ÁRVORE B ---\n");
    print_tree_visual(root, 0, true);
    printf("----------------\n");

    if(search(root, 20, ordem) != NULL) {
        printf("Achei!!!\n");
    } else { 
        printf("Não achei!!!\n");
    }

}

