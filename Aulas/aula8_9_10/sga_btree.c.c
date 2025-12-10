#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define M 5                             // vai precisar ser fixo pq se nao coitado dos arquivo
#define MAX_KEYS (M-1)
#define MIN_KEYS (((M + 1) / 2) - 1)

// ==================== STRUCTS ====================
// As structs de registro começam com a flag pra facilitar a minha vida na hora do registro
typedef struct {
    bool ativo;
    int matricula;
    char nome[50];
} Aluno;

typedef struct {
    bool ativo;
    int codigo;
    char nome[50];
} Disciplina;

typedef struct {
    bool ativo;
    int id_matricula;
    int matricula_aluno;
    int cod_disciplina;
    float media_final;
} Matricula;

typedef struct {
    int key;                // matricula do aluno por exemplo
    long data_offset;       // offset no arquivo alunos.dat por exemplo
} Item;

// Nó da btree em memória (com ponteiros)
typedef struct page {
    // variáveis de controle
    int numKeys;
    bool isLeaf;

    Item *items;                // contém a chave e o offset do item

    struct page** ptr;          // vetor de ponteiros pra filhos
    struct page* father;        // ponteiro pro pai
    long disk_offset;           // offset da página no arquivo .idx
    long* children_offsets;     // vetor de offsets (não alocados)
} page;

// Nó serializado pra disco
typedef struct {
    int numKeys;
    bool isLeaf;
    Item items[MAX_KEYS];
    long child_offsets[M];
    long father_offset;
} SerializedPage;

typedef struct {
    long root_offset;   // Offset da raiz no .idx
    int ordem;          // Ordem da arv
    int num_nodes;      // Contador de nós
} BtreeInfo;

// ==================== FUNÇÕES AUXILIARES ====================

void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

bool validar_nome(const char* nome) {
    //Verifica se ta dentro do limite de caracteres
    if (strlen(nome) == 0 || strlen(nome) >= 50) {
        return false;
    }
    // Verifica se não é só espaços
    for (int i = 0; nome[i]; i++) {
        if (!isspace(nome[i])) return true;
    }
    return false;
}

bool validar_nota(float nota) {
    return nota >= 0.0 && nota <= 10.0;
}

bool confirmar_acao(const char* mensagem) {
    char resp;
    printf("%s (s/n): ", mensagem);
    scanf(" %c", &resp);
    limpar_buffer();
    return (resp == 's' || resp == 'S');
}

void log_operacao(const char* operacao, const char* detalhes) {
    FILE* log = fopen("sga.log", "a");
    if (log) {
        fprintf(log, "[%s] %s\n", operacao, detalhes);
        fclose(log);
    }
}

// ==================== FUNÇÕES DE ARQUIVO ====================

FILE* openDataFile(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);
    if(fp == NULL && strcmp(mode, "rb+") == 0) {
        fp = fopen(filename, "wb+");
    }
    return fp;
}

FILE* openIndexFile(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);
    if(fp == NULL && strcmp(mode, "rb+") == 0) {
        fp = fopen(filename, "wb+");
        if (fp != NULL) {
            BtreeInfo info = {-1, M, 0};
            fwrite(&info, sizeof(BtreeInfo), 1, fp);
            fflush(fp);
        }
    }
    return fp;
}

long insertRecord(const char* filename, void* rec, size_t recSize) {
    FILE* fp = openDataFile(filename, "rb+");

    if(fp == NULL) {
        printf("- Erro ao abrir arquivo %s!\n", filename);
        return -1;
    }

    /* Explicação 
     * cast do register (void) pra um ponteiro de booleano
     * acessa o booleano com o primeiro *
     * Isso funciona por que todas estruturas de registro iniciam
     * com a flag. Ou seja, o primeiro byte é sempre essa flag.
    */
    //Marca esse registro como ativo
    *((bool*)rec) = true;

    /* Explicação do fseek
        Parâmetros    
            - 1 ponteiro pra objeto, arquivo, etc. O nosso alunos.dat (um fluxo de dados em geral (stream))
            - offset em bytes.
            - posição a partir de onde o offset vai ser adicionando. Pode ser:
                * SEEK_SET: começo do arquivo; SEEK_CUR: posição atual do ponteiro; SEEK_END: final do arquivo
        
        Retorno: 0 em caso de sucesso. Algo != de 0 no caso contrário.
    */
    //Vai pro final do arquivo
    fseek(fp, 0, SEEK_END);

    /* Explicação do ftell
        Ele recebe como parâmetro um ponteiro pra uma stream de dados e retorna a 
        posição do ponteiro nesse arquivo. Ou seja, nosso offset.
    */
    // Guarda a posição atual (offset)
    long offset = ftell(fp);

    /* Explicando o fwrite
    Parâmetros:
        1. Ponteiro pro elemento(s) que será escrito (pode ser um array)
        2. O tamanho individual de cada elemento que vai ser escrito
        3. A quantidade de elementos que vai ser escrito
        4. Ponteiro que identifica o fluxo de dados (stream) onde o dado vai ser guardado.

    Retorno: retorna o número total de elementos gravados com sucesso
    */
    // Escreve o registro
    size_t written = fwrite(rec, recSize, 1, fp);

    fclose(fp);     // fecha o arquivo

    if(written != 1) {
        printf("- Erro ao escrever no arquivo!\n");
        return -1;  //indicador de erro
    }

    // printf("Aluno inserido no offset: %ld\n", offset); //debug
    return offset;
}

bool searchRecord(const char* filename, long offset, void* rec, size_t recSize) {
    //Abrir o arquivo
    FILE* fp = openDataFile(filename, "rb");

    if(fp == NULL) return false;
    if(fseek(fp, offset, SEEK_SET) != 0) {
        fclose(fp);
        return false;
    }

    //Caso tenha dado certo, vamos ler o registro naquele offset
    //Lê o registro, atualiza os dados do ponteiro com o que foi lido
    /* Explicando o fread
    Parâmetros:
        - ponteiro pra onde o fread vai despejar o que foi lido do disco
        - tamanho em bytes do elemento que tu vai ler
        - quantos elementos tu vai ler
        - ponteiro pro fluxo de entrada

    Retorno: número de elmentos lido.
    */
    size_t lidos = fread(rec, recSize, 1, fp);
    fclose(fp);

    /* Explicação
     * acessa o primeiro byte do registro, que é sempre a flag
    */
    //Verifica se o registro lido é válido
    if (lidos != 1 || !*((bool*)rec)) return false;
    return true;
}

bool updateRecord(const char* filename, long offset, void* rec, size_t recSize) {
    //abre na leitura e escrita
    FILE* fp = openDataFile(filename, "rb+");
    
    //testa se deu bom
    if (fp == NULL) {
        printf("- Erro ao abrir arquivo para atualização!\n");
        return false;
    }

    // Posiciona no offset
    if (fseek(fp, offset, SEEK_SET) != 0) {
        printf("- Erro ao posicionar no arquivo!\n");
        fclose(fp);
        return false;
    }

    // Mantém o registro como ativo
    *((bool*)rec) = true;

    // Sobrescreve o registro
    size_t escritos = fwrite(rec, recSize, 1, fp);
    fclose(fp);
    return (escritos == 1);
}

bool deleteRecord(const char* filename, long offset, size_t recSize){
    FILE* fp = openDataFile(filename, "rb+");
    if(fp == NULL) {
        printf("- Erro ao abrir arquivo!\n");
        return false;
    }
    if(fseek(fp, offset, SEEK_SET) != 0) {
        printf("- Erro ao posicionar no offset!\n");
        fclose(fp);
        return false;
    }
    void* rec = malloc(recSize);
    if(fread(rec, recSize, 1, fp) != 1) {
        printf("- Erro ao ler registro!\n");
        free(rec);
        fclose(fp);
        return false;
    }
    *((bool*)rec) = false;
    fseek(fp, offset, SEEK_SET);
    size_t escritos = fwrite(rec, recSize, 1, fp);
    fclose(fp);
    free(rec);
    return (escritos == 1);
}

// ==================== FUNÇÕES DE SERIALIZAÇÃO ====================

long savePage_disk(FILE* fp, page* pg) {
    if (pg == NULL) return -1;

    SerializedPage sp;
    sp.numKeys = pg->numKeys;
    sp.isLeaf = pg->isLeaf;

    for (int i = 0; i < MAX_KEYS; i++) {
        if (i < pg->numKeys) {
            sp.items[i] = pg->items[i];
        } else {
            sp.items[i].data_offset = -1;
            sp.items[i].key = 0;
        }
    }

    for (int i = 0; i < M; i++) {
        if (pg->children_offsets != NULL && i <= pg->numKeys) {
            sp.child_offsets[i] = pg->children_offsets[i];
        } else {
            sp.child_offsets[i] = -1;
        }
    }

    sp.father_offset = (pg->father != NULL) ? pg->father->disk_offset : -1;

    if (pg->disk_offset == -1) {
        fseek(fp, 0, SEEK_END);
        pg->disk_offset = ftell(fp);
    } else {
        fseek(fp, pg->disk_offset, SEEK_SET);
    }

    fwrite(&sp, sizeof(SerializedPage), 1, fp);
    fflush(fp);
    return pg->disk_offset;
}

void saveBTree_recursive(FILE* fp, page* node) {
    if (node == NULL) return;
    
    savePage_disk(fp, node);
    
    if (!node->isLeaf && node->children_offsets != NULL) {
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->ptr[i] != NULL) {
                saveBTree_recursive(fp, node->ptr[i]);
                node->children_offsets[i] = node->ptr[i]->disk_offset;
            }
        }
        savePage_disk(fp, node);
    }
}

void saveBTree(const char* idx_file, page* root) {
    FILE* fp = openIndexFile(idx_file, "rb+");
    if (fp == NULL) {
        printf("- Erro ao abrir arquivo de índice %s!\n", idx_file);
        return;
    }

    saveBTree_recursive(fp, root);

    BtreeInfo metadata;
    metadata.root_offset = (root != NULL) ? root->disk_offset : -1;
    metadata.ordem = M;
    metadata.num_nodes = 0;
    
    fseek(fp, 0, SEEK_SET);
    fwrite(&metadata, sizeof(BtreeInfo), 1, fp);
    fflush(fp);

    fclose(fp);
}

page* loadPage_disk(FILE* fp, long offset) {
    if (offset == -1) return NULL;

    fseek(fp, offset, SEEK_SET);

    SerializedPage sp;
    if (fread(&sp, sizeof(SerializedPage), 1, fp) != 1) {
        return NULL;
    }

    page* pg = (page*)malloc(sizeof(page));
    if (!pg) return NULL;
    
    pg->numKeys = sp.numKeys;
    pg->isLeaf = sp.isLeaf;
    pg->disk_offset = offset;
    pg->father = NULL;

    pg->items = (Item*)calloc(MAX_KEYS, sizeof(Item));
    pg->ptr = (page**)calloc(M, sizeof(page*));
    
    if (!pg->items || !pg->ptr) {
        free(pg->items);
        free(pg->ptr);
        free(pg);
        return NULL;
    }

    for (int i = 0; i < pg->numKeys; i++) {
        pg->items[i] = sp.items[i];
    }

    for (int i = 0; i < M; i++) {
        pg->ptr[i] = NULL;
    }

    if (!pg->isLeaf) {
        pg->children_offsets = (long*)malloc(sizeof(long) * M);
        if (!pg->children_offsets) {
            free(pg->items);
            free(pg->ptr);
            free(pg);
            return NULL;
        }
        for (int i = 0; i < M; i++) {
            pg->children_offsets[i] = sp.child_offsets[i];
        }
    } else {
        pg->children_offsets = NULL;
    }

    return pg;
}

page* loadBTree(const char* idx_file) {
    FILE* fp = openIndexFile(idx_file, "rb+");
    if (fp == NULL) return NULL;

    BtreeInfo metadata;
    fseek(fp, 0, SEEK_SET);
    if (fread(&metadata, sizeof(BtreeInfo), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    if (metadata.root_offset == -1) {
        fclose(fp);
        return NULL;
    }

    page* root = loadPage_disk(fp, metadata.root_offset);
    fclose(fp);
    return root;
}

void initializePage(page* pg, bool is_leaf) {
    pg->numKeys = 0;
    pg->isLeaf = is_leaf;
    pg->items = (Item*)calloc(MAX_KEYS, sizeof(Item));
    pg->father = NULL;
    pg->ptr = (page**)calloc(M, sizeof(page*));
    pg->disk_offset = -1;
    
    if (!pg->items || !pg->ptr) {
        printf("- ERRO CRÍTICO: Falha na alocação de memória!\n");
        exit(1);
    }
    
    if (!is_leaf) {
        pg->children_offsets = (long*)malloc(sizeof(long) * M);
        if (!pg->children_offsets) {
            printf("- ERRO CRÍTICO: Falha na alocação de memória!\n");
            exit(1);
        }
        for(int i = 0; i < M; i++) pg->children_offsets[i] = -1;
    } else {
        pg->children_offsets = NULL;
    }
}

// ==================== FUNÇÕES DA B-TREE (INSERT) ====================

void splitChild(page *parent, int i, page* fullChild) {
    page* newSister = (page*)malloc(sizeof(page));
    if (!newSister) {
        printf("- ERRO: Falha ao alocar memória para split!\n");
        return;
    }
    
    initializePage(newSister, fullChild->isLeaf);
    newSister->father = parent;

    int oldNumKeys = fullChild->numKeys;
    int mid_index = oldNumKeys / 2;
    Item midItem = fullChild->items[mid_index];

    int j = 0;
    for (int k = mid_index + 1; k < oldNumKeys; k++) {
        newSister->items[j++] = fullChild->items[k];
        newSister->numKeys++;
    }

    j = 0;
    if(!fullChild->isLeaf) {
        for (int k = mid_index + 1; k <= oldNumKeys; k++) {
            newSister->ptr[j] = fullChild->ptr[k];
            newSister->children_offsets[j] = fullChild->children_offsets[k];
            if (newSister->ptr[j] != NULL) {
                newSister->ptr[j]->father = newSister;
            }
            j++;
        }
    }

    fullChild->numKeys = mid_index;

    for (int k = parent->numKeys - 1; k >= i; k--) {
        parent->items[k + 1] = parent->items[k];
        parent->ptr[k + 2] = parent->ptr[k + 1];
        parent->children_offsets[k + 2] = parent->children_offsets[k + 1];
    }

    parent->items[i] = midItem;
    parent->ptr[i + 1] = newSister;
    parent->children_offsets[i + 1] = -1;
    parent->numKeys++;

    fullChild->father = parent;
    newSister->father = parent;
}

void insertIntoNonFull_disk(page* pg, Item item, const char* idx_file) {
    if (pg->isLeaf) {
        int pos = pg->numKeys - 1;
        while (pos >= 0 && item.key < pg->items[pos].key) {
            pg->items[pos + 1] = pg->items[pos];
            pos--;
        }
        pg->items[pos + 1] = item;
        pg->numKeys++;
    } else {
        int pos = pg->numKeys - 1;
        while (pos >= 0 && item.key < pg->items[pos].key) pos--;
        pos++;

        if (pg->ptr[pos] == NULL) {
            if (pg->children_offsets != NULL && pg->children_offsets[pos] != -1) {
                FILE* fp = openIndexFile(idx_file, "rb+");
                if (fp) {
                    pg->ptr[pos] = loadPage_disk(fp, pg->children_offsets[pos]);
                    if (pg->ptr[pos]) {
                        pg->ptr[pos]->father = pg;
                    }
                    fclose(fp);
                }
            } else {
                pg->ptr[pos] = (page*)malloc(sizeof(page));
                if (!pg->ptr[pos]) {
                    printf("- ERRO: Falha ao alocar filho!\n");
                    return;
                }
                initializePage(pg->ptr[pos], true);
                pg->ptr[pos]->father = pg;
            }
        }

        if (pg->ptr[pos] && pg->ptr[pos]->numKeys == MAX_KEYS) {
            splitChild(pg, pos, pg->ptr[pos]);
            if (item.key > pg->items[pos].key) pos++;
        }
        
        if (pg->ptr[pos]) {
            insertIntoNonFull_disk(pg->ptr[pos], item, idx_file);
        }
    }
}

void insertBtree_disk(page** root_ptr, Item item, const char* idx_file) {
    if (!root_ptr || !(*root_ptr)) return;
    
    page* root = *root_ptr;

    if (root->numKeys == MAX_KEYS) {
        page* new_root = (page*)malloc(sizeof(page));
        if (!new_root) {
            printf("- ERRO: Falha ao criar nova raiz!\n");
            return;
        }
        initializePage(new_root, false);
        new_root->ptr[0] = root;
        root->father = new_root;
        splitChild(new_root, 0, root);

        int pos = 0;
        if (item.key > new_root->items[0].key) pos++;
        insertIntoNonFull_disk(new_root->ptr[pos], item, idx_file);
        *root_ptr = new_root;
    } else {
        insertIntoNonFull_disk(root, item, idx_file);
    }
    
    saveBTree(idx_file, *root_ptr);
}

// ==================== BUSCA COM CARREGAMENTO DO DISCO ====================

long search_disk(page* root, int key, const char* idx_file) {
    page* cur = root;

    while (cur != NULL) {
        int i = 0;
        while (i < cur->numKeys && key > cur->items[i].key) i++;

        if (i < cur->numKeys && key == cur->items[i].key) {
            return cur->items[i].data_offset;
        }

        if (cur->isLeaf) return -1;

        if (cur->ptr[i] == NULL) {
            if (cur->children_offsets != NULL && cur->children_offsets[i] != -1) {
                FILE* fp = openIndexFile(idx_file, "rb+");
                if (fp) {
                    cur->ptr[i] = loadPage_disk(fp, cur->children_offsets[i]);
                    if (cur->ptr[i]) cur->ptr[i]->father = cur;
                    fclose(fp);
                }
            }
            if (cur->ptr[i] == NULL) return -1;
        }
        
        cur = cur->ptr[i];
    }
    return -1;
}

// ==================== FUNÇÕES DE REMOÇÃO ====================

int getPredecessor(page* node, int idx, const char* idx_file) {
    page* cur = node->ptr[idx];
    
    while (cur && !cur->isLeaf) {
        int child_idx = cur->numKeys;
        if (cur->ptr[child_idx] == NULL && cur->children_offsets != NULL) {
            FILE* fp = openIndexFile(idx_file, "rb+");
            if (fp) {
                cur->ptr[child_idx] = loadPage_disk(fp, cur->children_offsets[child_idx]);
                if (cur->ptr[child_idx]) cur->ptr[child_idx]->father = cur;
                fclose(fp);
            }
        }
        cur = cur->ptr[child_idx];
        if (!cur) break;
    }
    return cur ? cur->items[cur->numKeys - 1].key : -1;
}

int getSuccessor(page* node, int idx, const char* idx_file) {
    page* cur = node->ptr[idx + 1];
    
    while (cur && !cur->isLeaf) {
        if (cur->ptr[0] == NULL && cur->children_offsets != NULL) {
            FILE* fp = openIndexFile(idx_file, "rb+");
            if (fp) {
                cur->ptr[0] = loadPage_disk(fp, cur->children_offsets[0]);
                if (cur->ptr[0]) cur->ptr[0]->father = cur;
                fclose(fp);
            }
        }
        cur = cur->ptr[0];
        if (!cur) break;
    }
    return cur ? cur->items[0].key : -1;
}

void borrowFromLeft(page* parent, int childIndex) {
    page* child = parent->ptr[childIndex];
    page* left = parent->ptr[childIndex - 1];

    for (int k = child->numKeys - 1; k >= 0; k--) {
        child->items[k + 1] = child->items[k];
    }
    if (!child->isLeaf) {
        for (int k = child->numKeys; k >= 0; k--) {
            child->ptr[k + 1] = child->ptr[k];
            if (child->children_offsets) {
                child->children_offsets[k + 1] = child->children_offsets[k];
            }
        }
    }

    child->items[0] = parent->items[childIndex - 1];

    if (!child->isLeaf) {
        child->ptr[0] = left->ptr[left->numKeys];
        if (child->children_offsets && left->children_offsets) {
            child->children_offsets[0] = left->children_offsets[left->numKeys];
        }
        if (child->ptr[0]) child->ptr[0]->father = child;
        left->ptr[left->numKeys] = NULL;
    }

    parent->items[childIndex - 1] = left->items[left->numKeys - 1];

    child->numKeys++;
    left->numKeys--;
}

void borrowFromRight(page* parent, int childIndex) {
    page* child = parent->ptr[childIndex];
    page* right = parent->ptr[childIndex + 1];

    child->items[child->numKeys] = parent->items[childIndex];

    if (!child->isLeaf) {
        child->ptr[child->numKeys + 1] = right->ptr[0];
        if (child->children_offsets && right->children_offsets) {
            child->children_offsets[child->numKeys + 1] = right->children_offsets[0];
        }
        if (child->ptr[child->numKeys + 1]) {
            child->ptr[child->numKeys + 1]->father = child;
        }
    }

    parent->items[childIndex] = right->items[0];

    for (int k = 0; k < right->numKeys - 1; k++) {
        right->items[k] = right->items[k + 1];
    }
    if (!right->isLeaf) {
        for (int k = 0; k < right->numKeys; k++) {
            right->ptr[k] = right->ptr[k + 1];
            if (right->children_offsets) {
                right->children_offsets[k] = right->children_offsets[k + 1];
            }
        }
        right->ptr[right->numKeys - 1] = NULL;
    }

    child->numKeys++;
    right->numKeys--;
}

void mergeChildren(page* parent, int i) {
    page* left = parent->ptr[i];
    page* right = parent->ptr[i + 1];

    left->items[left->numKeys] = parent->items[i];

    for (int k = 0; k < right->numKeys; k++) {
        left->items[left->numKeys + 1 + k] = right->items[k];
    }

    if (!left->isLeaf) {
        for (int k = 0; k <= right->numKeys; k++) {
            left->ptr[left->numKeys + 1 + k] = right->ptr[k];
            if (left->children_offsets && right->children_offsets) {
                left->children_offsets[left->numKeys + 1 + k] = right->children_offsets[k];
            }
            if (left->ptr[left->numKeys + 1 + k]) {
                left->ptr[left->numKeys + 1 + k]->father = left;
            }
        }
    }

    left->numKeys = left->numKeys + 1 + right->numKeys;

    for (int k = i; k < parent->numKeys - 1; k++) {
        parent->items[k] = parent->items[k + 1];
        parent->ptr[k + 1] = parent->ptr[k + 2];
        if (parent->children_offsets) {
            parent->children_offsets[k + 1] = parent->children_offsets[k + 2];
        }
    }
    parent->ptr[parent->numKeys] = NULL;
    parent->numKeys--;

    free(right->items);
    free(right->ptr);
    if (right->children_offsets) free(right->children_offsets);
    free(right);
}

void removeFromLeaf(page* node, int idx) {
    for (int k = idx; k < node->numKeys - 1; k++) {
        node->items[k] = node->items[k + 1];
    }
    node->numKeys--;
}

void removeInternal(page** root_ptr, page* node, int key, const char* idx_file);

void removeInternal(page** root_ptr, page* node, int key, const char* idx_file) {
    if (!node) return;
    
    int i = 0;
    while (i < node->numKeys && key > node->items[i].key) i++;

    if (i < node->numKeys && node->items[i].key == key) {
        if (node->isLeaf) {
            removeFromLeaf(node, i);
            return;
        } else {
            if (node->ptr[i] == NULL && node->children_offsets && node->children_offsets[i] != -1) {
                FILE* fp = openIndexFile(idx_file, "rb+");
                if (fp) {
                    node->ptr[i] = loadPage_disk(fp, node->children_offsets[i]);
                    if (node->ptr[i]) node->ptr[i]->father = node;
                    fclose(fp);
                }
            }
            if (node->ptr[i+1] == NULL && node->children_offsets && node->children_offsets[i+1] != -1) {
                FILE* fp = openIndexFile(idx_file, "rb+");
                if (fp) {
                    node->ptr[i+1] = loadPage_disk(fp, node->children_offsets[i+1]);
                    if (node->ptr[i+1]) node->ptr[i+1]->father = node;
                    fclose(fp);
                }
            }
            
            page* left = node->ptr[i];
            page* right = node->ptr[i + 1];
            
            if (left && left->numKeys > MIN_KEYS) {
                int pred = getPredecessor(node, i, idx_file);
                node->items[i].key = pred;
                removeInternal(root_ptr, left, pred, idx_file);
                return;
            } else if (right && right->numKeys > MIN_KEYS) {
                int succ = getSuccessor(node, i, idx_file);
                node->items[i].key = succ;
                removeInternal(root_ptr, right, succ, idx_file);
                return;
            } else {
                mergeChildren(node, i);
                removeInternal(root_ptr, node->ptr[i], key, idx_file);
                return;
            }
        }
    } else {
        if (node->isLeaf) return;
        
        if (node->ptr[i] == NULL && node->children_offsets && node->children_offsets[i] != -1) {
            FILE* fp = openIndexFile(idx_file, "rb+");
            if (fp) {
                node->ptr[i] = loadPage_disk(fp, node->children_offsets[i]);
                if (node->ptr[i]) node->ptr[i]->father = node;
                fclose(fp);
            }
        }
        
        page* child = node->ptr[i];
        if (!child) return;

        if (child->numKeys <= MIN_KEYS) {
            page* left = NULL;
            page* right = NULL;
            
            if (i > 0) {
                if (node->ptr[i-1] == NULL && node->children_offsets && node->children_offsets[i-1] != -1) {
                    FILE* fp = openIndexFile(idx_file, "rb+");
                    if (fp) {
                        node->ptr[i-1] = loadPage_disk(fp, node->children_offsets[i-1]);
                        if (node->ptr[i-1]) node->ptr[i-1]->father = node;
                        fclose(fp);
                    }
                }
                left = node->ptr[i - 1];
            }
            
            if (i < node->numKeys) {
                if (node->ptr[i+1] == NULL && node->children_offsets && node->children_offsets[i+1] != -1) {
                    FILE* fp = openIndexFile(idx_file, "rb+");
                    if (fp) {
                        node->ptr[i+1] = loadPage_disk(fp, node->children_offsets[i+1]);
                        if (node->ptr[i+1]) node->ptr[i+1]->father = node;
                        fclose(fp);
                    }
                }
                right = node->ptr[i + 1];
            }

            if (left && left->numKeys > MIN_KEYS) {
                borrowFromLeft(node, i);
            } else if (right && right->numKeys > MIN_KEYS) {
                borrowFromRight(node, i);
            } else {
                if (right) {
                    mergeChildren(node, i);
                    child = node->ptr[i];
                } else if (left) {
                    mergeChildren(node, i - 1);
                    child = node->ptr[i - 1];
                    i = i - 1;
                }
            }
        }
        removeInternal(root_ptr, child, key, idx_file);
    }
}

void removeBtree_disk(page** root_ptr, int key, const char* idx_file) {
    if (!root_ptr || !(*root_ptr)) return;
    page* root = *root_ptr;
    
    removeInternal(root_ptr, root, key, idx_file);

    if (root->numKeys == 0) {
        if (!root->isLeaf && root->ptr[0] != NULL) {
            page* new_root = root->ptr[0];
            new_root->father = NULL;
            free(root->items);
            free(root->ptr);
            if (root->children_offsets) free(root->children_offsets);
            free(root);
            *root_ptr = new_root;
        } else if (root->isLeaf) {
            free(root->items);
            free(root->ptr);
            if (root->children_offsets) free(root->children_offsets);
            free(root);
            *root_ptr = NULL;
        }
    }
    
    if (*root_ptr != NULL) {
        saveBTree(idx_file, *root_ptr);
    }
}

// ==================== LIBERAR MEMÓRIA ====================

void freeTree(page* n) {
    if (!n) return;
    if (!n->isLeaf) {
        for (int i = 0; i <= n->numKeys; i++) {
            if (n->ptr[i]) freeTree(n->ptr[i]);
        }
    }
    free(n->items);
    free(n->ptr);
    if(n->children_offsets != NULL) free(n->children_offsets);
    free(n);
}


/*
 * As funções de print foram todas criadas pela IA, pra ficar bonitinho.
*/
// ==================== SISTEMA SGA - CAMADA DE APLICAÇÃO ====================

void menu_principal() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║   SISTEMA DE GERENCIAMENTO ACADÊMICO (SGA) v1.0       ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  1. Gerenciar Alunos                                   ║\n");
    printf("║  2. Gerenciar Disciplinas                              ║\n");
    printf("║  3. Gerenciar Matrículas                               ║\n");
    printf("║  4. Relatórios                                         ║\n");
    printf("║  5. Backup do Sistema                                  ║\n");
    printf("║  0. Sair                                               ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("Escolha: ");
}

void menu_alunos() {
    printf("\n--- GERENCIAR ALUNOS ---\n");
    printf("1. Inserir Aluno\n");
    printf("2. Buscar Aluno\n");
    printf("3. Atualizar Aluno\n");
    printf("4. Deletar Aluno (com cascata)\n");
    printf("5. Listar Todos os Alunos\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

void menu_disciplinas() {
    printf("\n--- GERENCIAR DISCIPLINAS ---\n");
    printf("1. Inserir Disciplina\n");
    printf("2. Buscar Disciplina\n");
    printf("3. Atualizar Disciplina\n");
    printf("4. Deletar Disciplina (com cascata)\n");
    printf("5. Listar Todas as Disciplinas\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

void menu_matriculas() {
    printf("\n--- GERENCIAR MATRÍCULAS ---\n");
    printf("1. Matricular Aluno em Disciplina\n");
    printf("2. Buscar Matrícula\n");
    printf("3. Lançar/Atualizar Nota\n");
    printf("4. Remover Matrícula\n");
    printf("5. Listar Todas as Matrículas\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

void menu_relatorios() {
    printf("\n--- RELATÓRIOS ---\n");
    printf("1. Listar Matrículas de um Aluno\n");
    printf("2. Listar Alunos de uma Disciplina\n");
    printf("3. Calcular Média Geral de um Aluno\n");
    printf("4. Listar Tudo\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

// ========== FUNÇÕES CRUD ALUNOS ==========

void inserir_aluno(page** root_alunos) {
    Aluno aluno;
    printf("\n=== INSERIR ALUNO ===\n");
    printf("Matrícula: ");
    
    if (scanf("%d", &aluno.matricula) != 1 || aluno.matricula <= 0) {
        printf("- Matrícula inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long existe = search_disk(*root_alunos, aluno.matricula, "alunos.idx");
    if (existe != -1) {
        printf("- Aluno com essa matrícula já existe!\n");
        return;
    }
    
    printf("Nome: ");
    fgets(aluno.nome, 50, stdin);
    aluno.nome[strcspn(aluno.nome, "\n")] = 0;
    
    if (!validar_nome(aluno.nome)) {
        printf("- Nome inválido! (vazio ou muito longo)\n");
        return;
    }
    
    long offset = insertRecord("alunos.dat", &aluno, sizeof(Aluno));
    if (offset != -1) {
        Item item = {aluno.matricula, offset};
        insertBtree_disk(root_alunos, item, "alunos.idx");
        printf("Aluno '%s' (mat. %d) inserido com sucesso!\n", aluno.nome, aluno.matricula);
        
        char log_msg[100];
        sprintf(log_msg, "Inserido aluno: %d - %s", aluno.matricula, aluno.nome);
        log_operacao("INSERT_ALUNO", log_msg);
    } else {
        printf("- Erro ao inserir aluno!\n");
    }
}

void buscar_aluno(page* root_alunos) {
    int matricula;
    printf("\n=== BUSCAR ALUNO ===\n");
    printf("Matrícula: ");
    
    if (scanf("%d", &matricula) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(root_alunos, matricula, "alunos.idx");
    if (offset == -1) {
        printf("- Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    if (searchRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
        printf("\n╔════════════════════════════════════════╗\n");
        printf("║       ALUNO ENCONTRADO                 ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║ Matrícula: %-28d║\n", aluno.matricula);
        printf("║ Nome:      %-28s║\n", aluno.nome);
        printf("╚════════════════════════════════════════╝\n");
    } else {
        printf("- Erro ao ler dados do aluno!\n");
    }
}

void atualizar_aluno(page* root_alunos) {
    int matricula;
    printf("\n=== ATUALIZAR ALUNO ===\n");
    printf("Matrícula: ");
    
    if (scanf("%d", &matricula) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(root_alunos, matricula, "alunos.idx");
    if (offset == -1) {
        printf("- Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    if (!searchRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
        printf("- Erro ao ler aluno!\n");
        return;
    }
    
    printf("Nome atual: %s\n", aluno.nome);
    printf("Novo nome (Enter para manter): ");
    char novo_nome[50];
    fgets(novo_nome, 50, stdin);
    novo_nome[strcspn(novo_nome, "\n")] = 0;
    
    if (strlen(novo_nome) > 0 && validar_nome(novo_nome)) {
        strcpy(aluno.nome, novo_nome);
        
        if (updateRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
            printf("Aluno atualizado com sucesso!\n");
            
            char log_msg[100];
            sprintf(log_msg, "Atualizado aluno: %d - %s", aluno.matricula, aluno.nome);
            log_operacao("UPDATE_ALUNO", log_msg);
        } else {
            printf("- Erro ao atualizar!\n");
        }
    } else if (strlen(novo_nome) > 0) {
        printf("- Nome inválido!\n");
    } else {
        printf("Nenhuma alteração realizada.\n");
    }
}

void deletar_aluno(page** root_alunos, page** root_matriculas) {
    int matricula;
    printf("\n=== DELETAR ALUNO ===\n");
    printf("Matrícula: ");
    
    if (scanf("%d", &matricula) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(*root_alunos, matricula, "alunos.idx");
    if (offset == -1) {
        printf("- Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    if (!searchRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
        printf("- Erro ao ler aluno!\n");
        return;
    }
    
    printf("| ! | ATENÇÃO: Deletar o aluno '%s' removerá TODAS as suas matrículas!\n", aluno.nome);
    if (!confirmar_acao("Deseja continuar?")) {
        printf("Operação cancelada.\n");
        return;
    }
    
    // CONSISTÊNCIA REFERENCIAL
    printf("Removendo matrículas associadas...\n");
    int count = 0;
    FILE* fp = openDataFile("matriculas.dat", "rb");
    if (fp) {
        Matricula mat;
        long mat_offset = 0;
        while (fread(&mat, sizeof(Matricula), 1, fp) == 1) {
            if (mat.ativo && mat.matricula_aluno == matricula) {
                deleteRecord("matriculas.dat", mat_offset, sizeof(Matricula));
                removeBtree_disk(root_matriculas, mat.id_matricula, "matriculas.idx");
                count++;
            }
            mat_offset += sizeof(Matricula);
        }
        fclose(fp);
    }
    printf("%d matrícula(s) removida(s)\n", count);
    
    if (deleteRecord("alunos.dat", offset, sizeof(Aluno))) {
        removeBtree_disk(root_alunos, matricula, "alunos.idx");
        printf(" - Aluno '%s' deletado com sucesso!\n", aluno.nome);
        
        char log_msg[100];
        sprintf(log_msg, "Deletado aluno: %d - %s (%d matriculas)", matricula, aluno.nome, count);
        log_operacao("DELETE_ALUNO", log_msg);
    } else {
        printf("- Erro ao deletar!\n");
    }
}

void listar_alunos() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║                  LISTA DE ALUNOS                       ║\n");
    printf("╠═══════════╦════════════════════════════════════════════╣\n");
    printf("║ Matrícula ║ Nome                                       ║\n");
    printf("╠═══════════╬════════════════════════════════════════════╣\n");
    
    FILE* fp = openDataFile("alunos.dat", "rb");
    int count = 0;
    if (fp) {
        Aluno a;
        while (fread(&a, sizeof(Aluno), 1, fp) == 1) {
            if (a.ativo) {
                printf("║ %-9d ║ %-42s ║\n", a.matricula, a.nome);
                count++;
            }
        }
        fclose(fp);
    }
    
    printf("╚═══════════╩════════════════════════════════════════════╝\n");
    printf("Total: %d aluno(s)\n", count);
}

// ========== FUNÇÕES CRUD DISCIPLINAS ==========

void inserir_disciplina(page** root_disciplinas) {
    Disciplina disc;
    printf("\n=== INSERIR DISCIPLINA ===\n");
    printf("Código da disciplina: ");
    
    if (scanf("%d", &disc.codigo) != 1 || disc.codigo <= 0) {
        printf("- Código inválido!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long existe = search_disk(*root_disciplinas, disc.codigo, "disciplinas.idx");
    if (existe != -1) {
        printf("- Disciplina com esse código já existe!\n");
        return;
    }
    
    printf("Nome: ");
    fgets(disc.nome, 50, stdin);
    disc.nome[strcspn(disc.nome, "\n")] = 0;
    
    if (!validar_nome(disc.nome)) {
        printf("- Nome inválido!\n");
        return;
    }
    
    long offset = insertRecord("disciplinas.dat", &disc, sizeof(Disciplina));
    if (offset != -1) {
        Item item = {disc.codigo, offset};
        insertBtree_disk(root_disciplinas, item, "disciplinas.idx");
        printf(" - Disciplina '%s' (cód. %d) inserida!\n", disc.nome, disc.codigo);
        
        char log_msg[100];
        sprintf(log_msg, "Inserida disciplina: %d - %s", disc.codigo, disc.nome);
        log_operacao("INSERT_DISCIPLINA", log_msg);
    }
}

void buscar_disciplina(page* root_disciplinas) {
    int codigo;
    printf("\n=== BUSCAR DISCIPLINA ===\n");
    printf("Código: ");
    
    if (scanf("%d", &codigo) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(root_disciplinas, codigo, "disciplinas.idx");
    if (offset == -1) {
        printf("- Disciplina não encontrada!\n");
        return;
    }
    
    Disciplina disc;
    if (searchRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
        printf("\n╔════════════════════════════════════════╗\n");
        printf("║     DISCIPLINA ENCONTRADA              ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║ Código: %-31d║\n", disc.codigo);
        printf("║ Nome:   %-31s║\n", disc.nome);
        printf("╚════════════════════════════════════════╝\n");
    }
}

void atualizar_disciplina(page* root_disciplinas) {
    int codigo;
    printf("\n=== ATUALIZAR DISCIPLINA ===\n");
    printf("Código: ");
    
    if (scanf("%d", &codigo) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(root_disciplinas, codigo, "disciplinas.idx");
    if (offset == -1) {
        printf("- Disciplina não encontrada!\n");
        return;
    }
    
    Disciplina disc;
    if (!searchRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
        printf("- Erro ao ler disciplina!\n");
        return;
    }
    
    printf("Nome atual: %s\n", disc.nome);
    printf("Novo nome (Enter para manter): ");
    char novo_nome[50];
    fgets(novo_nome, 50, stdin);
    novo_nome[strcspn(novo_nome, "\n")] = 0;
    
    if (strlen(novo_nome) > 0 && validar_nome(novo_nome)) {
        strcpy(disc.nome, novo_nome);
        
        if (updateRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
            printf(" - Disciplina atualizada!\n");
            
            char log_msg[100];
            sprintf(log_msg, "Atualizada disciplina: %d - %s", disc.codigo, disc.nome);
            log_operacao("UPDATE_DISCIPLINA", log_msg);
        }
    } else if (strlen(novo_nome) > 0) {
        printf("- Nome inválido!\n");
    }
}

void deletar_disciplina(page** root_disciplinas, page** root_matriculas) {
    int codigo;
    printf("\n=== DELETAR DISCIPLINA ===\n");
    printf("Código: ");
    
    if (scanf("%d", &codigo) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(*root_disciplinas, codigo, "disciplinas.idx");
    if (offset == -1) {
        printf("- Disciplina não encontrada!\n");
        return;
    }
    
    Disciplina disc;
    if (!searchRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
        printf("- Erro ao ler disciplina!\n");
        return;
    }
    
    printf("| ! | ATENÇÃO: Deletar '%s' removerá TODAS as matrículas nela!\n", disc.nome);
    if (!confirmar_acao("Deseja continuar?")) {
        printf("Operação cancelada.\n");
        return;
    }
    
    printf("Removendo matrículas associadas...\n");
    int count = 0;
    FILE* fp = openDataFile("matriculas.dat", "rb");
    if (fp) {
        Matricula mat;
        long mat_offset = 0;
        while (fread(&mat, sizeof(Matricula), 1, fp) == 1) {
            if (mat.ativo && mat.cod_disciplina == codigo) {
                deleteRecord("matriculas.dat", mat_offset, sizeof(Matricula));
                removeBtree_disk(root_matriculas, mat.id_matricula, "matriculas.idx");
                count++;
            }
            mat_offset += sizeof(Matricula);
        }
        fclose(fp);
    }
    printf("  - %d matrícula(s) removida(s)\n", count);
    
    if (deleteRecord("disciplinas.dat", offset, sizeof(Disciplina))) {
        removeBtree_disk(root_disciplinas, codigo, "disciplinas.idx");
        printf(" - Disciplina '%s' deletada!\n", disc.nome);
        
        char log_msg[100];
        sprintf(log_msg, "Deletada disciplina: %d - %s (%d matriculas)", codigo, disc.nome, count);
        log_operacao("DELETE_DISCIPLINA", log_msg);
    }
}

void listar_disciplinas() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║               LISTA DE DISCIPLINAS                     ║\n");
    printf("╠══════════╦═════════════════════════════════════════════╣\n");
    printf("║ Código   ║ Nome                                        ║\n");
    printf("╠══════════╬═════════════════════════════════════════════╣\n");
    
    FILE* fp = openDataFile("disciplinas.dat", "rb");
    int count = 0;
    if (fp) {
        Disciplina d;
        while (fread(&d, sizeof(Disciplina), 1, fp) == 1) {
            if (d.ativo) {
                printf("║ %-8d ║ %-43s ║\n", d.codigo, d.nome);
                count++;
            }
        }
        fclose(fp);
    }
    
    printf("╚══════════╩═════════════════════════════════════════════╝\n");
    printf("Total: %d disciplina(s)\n", count);
}

// ========== FUNÇÕES CRUD MATRÍCULAS ==========

int proximo_id_matricula() {
    FILE* fp = openDataFile("matriculas.dat", "rb");
    if (!fp) return 1;
    
    int max_id = 0;
    Matricula mat;
    while (fread(&mat, sizeof(Matricula), 1, fp) == 1) {
        if (mat.id_matricula > max_id) max_id = mat.id_matricula;
    }
    fclose(fp);
    return max_id + 1;
}

void matricular_aluno(page** root_matriculas, page* root_alunos, page* root_disciplinas) {
    Matricula mat;
    
    printf("\n=== MATRICULAR ALUNO ===\n");
    printf("Matrícula do aluno: ");
    
    if (scanf("%d", &mat.matricula_aluno) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    if (search_disk(root_alunos, mat.matricula_aluno, "alunos.idx") == -1) {
        printf("- Aluno não encontrado! Cadastre o aluno primeiro.\n");
        return;
    }
    
    printf("Código da disciplina: ");
    if (scanf("%d", &mat.cod_disciplina) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    if (search_disk(root_disciplinas, mat.cod_disciplina, "disciplinas.idx") == -1) {
        printf("- Disciplina não encontrada! Cadastre a disciplina primeiro.\n");
        return;
    }
    
    // Verifica se já existe essa matrícula
    FILE* fp = openDataFile("matriculas.dat", "rb");
    if (fp) {
        Matricula temp;
        while (fread(&temp, sizeof(Matricula), 1, fp) == 1) {
            if (temp.ativo && temp.matricula_aluno == mat.matricula_aluno && 
                temp.cod_disciplina == mat.cod_disciplina) {
                printf("- Aluno já matriculado nesta disciplina!\n");
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }
    
    mat.id_matricula = proximo_id_matricula();
    mat.media_final = 0.0;
    
    long offset = insertRecord("matriculas.dat", &mat, sizeof(Matricula));
    if (offset != -1) {
        Item item = {mat.id_matricula, offset};
        insertBtree_disk(root_matriculas, item, "matriculas.idx");
        printf(" - Matrícula realizada! ID: %d\n", mat.id_matricula);
        
        char log_msg[100];
        sprintf(log_msg, "Matricula criada: ID=%d, Aluno=%d, Disc=%d", 
                mat.id_matricula, mat.matricula_aluno, mat.cod_disciplina);
        log_operacao("INSERT_MATRICULA", log_msg);
    }
}

void buscar_matricula(page* root_matriculas) {
    int id;
    printf("\n=== BUSCAR MATRÍCULA ===\n");
    printf("ID da matrícula: ");
    
    if (scanf("%d", &id) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(root_matriculas, id, "matriculas.idx");
    if (offset == -1) {
        printf("- Matrícula não encontrada!\n");
        return;
    }
    
    Matricula mat;
    if (searchRecord("matriculas.dat", offset, &mat, sizeof(Matricula))) {
        printf("\n╔════════════════════════════════════════╗\n");
        printf("║       MATRÍCULA ENCONTRADA             ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║ ID:          %-26d║\n", mat.id_matricula);
        printf("║ Aluno:       %-26d║\n", mat.matricula_aluno);
        printf("║ Disciplina:  %-26d║\n", mat.cod_disciplina);
        printf("║ Média Final: %-26.2f║\n", mat.media_final);
        printf("╚════════════════════════════════════════╝\n");
    }
}

void atualizar_nota(page* root_matriculas) {
    int id;
    printf("\n=== LANÇAR/ATUALIZAR NOTA ===\n");
    printf("ID da matrícula: ");
    
    if (scanf("%d", &id) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    
    long offset = search_disk(root_matriculas, id, "matriculas.idx");
    if (offset == -1) {
        printf("- Matrícula não encontrada!\n");
        limpar_buffer();
        return;
    }
    
    Matricula mat;
    if (!searchRecord("matriculas.dat", offset, &mat, sizeof(Matricula))) {
        printf("- Erro ao ler matrícula!\n");
        limpar_buffer();
        return;
    }
    
    printf("Média atual: %.2f\n", mat.media_final);
    printf("Nova média (0.0 - 10.0): ");
    
    float nova_nota;
    if (scanf("%f", &nova_nota) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    if (!validar_nota(nova_nota)) {
        printf(" - Nota deve estar entre 0.0 e 10.0!\n");
        return;
    }
    
    mat.media_final = nova_nota;
    
    if (updateRecord("matriculas.dat", offset, &mat, sizeof(Matricula))) {
        printf(" - Nota atualizada para %.2f!\n", nova_nota);
        
        char log_msg[100];
        sprintf(log_msg, "Nota atualizada: ID=%d, Nova nota=%.2f", id, nova_nota);
        log_operacao("UPDATE_NOTA", log_msg);
    }
}

void remover_matricula(page** root_matriculas) {
    int id;
    printf("\n=== REMOVER MATRÍCULA ===\n");
    printf("ID da matrícula: ");
    
    if (scanf("%d", &id) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    long offset = search_disk(*root_matriculas, id, "matriculas.idx");
    if (offset == -1) {
        printf("- Matrícula não encontrada!\n");
        return;
    }
    
    if (!confirmar_acao("Deseja realmente remover esta matrícula?")) {
        printf("Operação cancelada.\n");
        return;
    }
    
    if (deleteRecord("matriculas.dat", offset, sizeof(Matricula))) {
        removeBtree_disk(root_matriculas, id, "matriculas.idx");
        printf(" - Matrícula removida!\n");
        
        char log_msg[50];
        sprintf(log_msg, "Matricula removida: ID=%d", id);
        log_operacao("DELETE_MATRICULA", log_msg);
    }
}

void listar_matriculas() {
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                   LISTA DE MATRÍCULAS                        ║\n");
    printf("╠════════╦═══════════╦════════════╦═════════════════════════════╣\n");
    printf("║   ID   ║   Aluno   ║  Discipl.  ║      Média Final            ║\n");
    printf("╠════════╬═══════════╬════════════╬═════════════════════════════╣\n");
    
    FILE* fp = openDataFile("matriculas.dat", "rb");
    int count = 0;
    if (fp) {
        Matricula m;
        while (fread(&m, sizeof(Matricula), 1, fp) == 1) {
            if (m.ativo) {
                printf("║ %-6d ║ %-9d ║ %-10d ║ %-27.2f ║\n",
                       m.id_matricula, m.matricula_aluno, 
                       m.cod_disciplina, m.media_final);
                count++;
            }
        }
        fclose(fp);
    }
    
    printf("╚════════╩═══════════╩════════════╩═════════════════════════════╝\n");
    printf("Total: %d matrícula(s)\n", count);
}

// ========== RELATÓRIOS ==========

void listar_matriculas_aluno(page* root_alunos) {
    int matricula;
    printf("\n=== MATRÍCULAS DE UM ALUNO ===\n");
    printf("Matrícula do aluno: ");
    
    if (scanf("%d", &matricula) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    if (search_disk(root_alunos, matricula, "alunos.idx") == -1) {
        printf("- Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    long offset_aluno = search_disk(root_alunos, matricula, "alunos.idx");
    searchRecord("alunos.dat", offset_aluno, &aluno, sizeof(Aluno));
    
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║ Aluno: %-54s║\n", aluno.nome);
    printf("╠════════╦════════════╦══════════════════════════╦══════════════╣\n");
    printf("║   ID   ║  Discipl.  ║      Disciplina          ║ Média Final  ║\n");
    printf("╠════════╬════════════╬══════════════════════════╬══════════════╣\n");
    
    FILE* fp = openDataFile("matriculas.dat", "rb");
    int count = 0;
    if (fp) {
        Matricula m;
        while (fread(&m, sizeof(Matricula), 1, fp) == 1) {
            if (m.ativo && m.matricula_aluno == matricula) {
                Disciplina disc;
                long offset_disc = search_disk(NULL, m.cod_disciplina, "disciplinas.idx");
                char nome_disc[50] = "N/A";
                if (offset_disc != -1 && searchRecord("disciplinas.dat", offset_disc, &disc, sizeof(Disciplina))) {
                    strcpy(nome_disc, disc.nome);
                }
                
                printf("║ %-6d ║ %-10d ║ %-24s ║ %-12.2f ║\n",
                       m.id_matricula, m.cod_disciplina, nome_disc, m.media_final);
                count++;
            }
        }
        fclose(fp);
    }
    
    printf("╚════════╩════════════╩══════════════════════════╩══════════════╝\n");
    printf("Total: %d matrícula(s)\n", count);
}

void listar_alunos_disciplina(page* root_disciplinas) {
    int codigo;
    printf("\n=== ALUNOS DE UMA DISCIPLINA ===\n");
    printf("Código da disciplina: ");
    
    if (scanf("%d", &codigo) != 1) {
        printf("- Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    if (search_disk(root_disciplinas, codigo, "disciplinas.idx") == -1) {
        printf("- Disciplina não encontrada!\n");
        return;
    }
    
    Disciplina disc;
    long offset_disc = search_disk(root_disciplinas, codigo, "disciplinas.idx");
    searchRecord("disciplinas.dat", offset_disc, &disc, sizeof(Disciplina));
    
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║ Disciplina: %-49s║\n", disc.nome);
    printf("╠════════╦═══════════╦══════════════════════════╦══════════════╣\n");
    printf("║   ID   ║  Matríc.  ║      Aluno               ║ Média Final  ║\n");
    printf("╠════════╬═══════════╬══════════════════════════╬══════════════╣\n");
    
    FILE* fp = openDataFile("matriculas.dat", "rb");
    int count = 0;
    if (fp) {
        Matricula m;
        while (fread(&m, sizeof(Matricula), 1, fp) == 1) {
            if (m.ativo && m.cod_disciplina == codigo) {
                Aluno aluno;
                long offset_aluno = search_disk(NULL, m.matricula_aluno, "alunos.idx");
                char nome_aluno[50] = "N/A";
                if (offset_aluno != -1 && searchRecord("alunos.dat", offset_aluno, &aluno, sizeof(Aluno))) {
                    strcpy(nome_aluno, aluno.nome);
                }
                
                printf("║ %-6d ║ %-9d ║ %-24s ║ %-12.2f ║\n",
                       m.id_matricula, m.matricula_aluno, nome_aluno, m.media_final);
                count++;
            }
        }
        fclose(fp);
    }
    
    printf("╚════════╩═══════════╩══════════════════════════╩══════════════╝\n");
    printf("Total: %d aluno(s)\n", count);
}

void calcular_media_geral(page* root_alunos) {
    int matricula;
    printf("\n=== MÉDIA GERAL DE UM ALUNO ===\n");
    printf("Matrícula do aluno: ");
    
    if (scanf("%d", &matricula) != 1) {
        printf(" - Entrada inválida!\n");
        limpar_buffer();
        return;
    }
    limpar_buffer();
    
    if (search_disk(root_alunos, matricula, "alunos.idx") == -1) {
        printf(" - Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    long offset_aluno = search_disk(root_alunos, matricula, "alunos.idx");
    searchRecord("alunos.dat", offset_aluno, &aluno, sizeof(Aluno));
    
    FILE* fp = openDataFile("matriculas.dat", "rb");
    int count = 0;
    float soma = 0.0;
    
    if (fp) {
        Matricula m;
        while (fread(&m, sizeof(Matricula), 1, fp) == 1) {
            if (m.ativo && m.matricula_aluno == matricula) {
                soma += m.media_final;
                count++;
            }
        }
        fclose(fp);
    }
    
    if (count == 0) {
        printf(" - Aluno não possui matrículas!\n");
    } else {
        float media = soma / count;
        printf("\n╔════════════════════════════════════════╗\n");
        printf("║       MÉDIA GERAL DO ALUNO             ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║ Nome:       %-27s║\n", aluno.nome);
        printf("║ Matrícula:  %-27d║\n", aluno.matricula);
        printf("║ Disciplinas: %-26d║\n", count);
        printf("║ Média Geral: %-26.2f║\n", media);
        printf("╚════════════════════════════════════════╝\n");
    }
}

void listar_todos() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║                  RELATÓRIO COMPLETO                    ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    listar_alunos();
    printf("\n");
    listar_disciplinas();
    printf("\n");
    listar_matriculas();
}

// ========== BACKUP ==========

void backup_sistema() {
    printf("\n=== BACKUP DO SISTEMA ===\n");
    
    const char* arquivos[] = {
        "alunos.dat", "alunos.idx",
        "disciplinas.dat", "disciplinas.idx",
        "matriculas.dat", "matriculas.idx"
    };
    
    printf("Criando backup...\n");
    
    for (int i = 0; i < 6; i++) {
        char backup_name[100];
        sprintf(backup_name, "backup_%s", arquivos[i]);
        
        FILE* origem = fopen(arquivos[i], "rb");
        if (!origem) {
            printf("  | ! | Arquivo %s não encontrado, pulando...\n", arquivos[i]);
            continue;
        }
        
        FILE* destino = fopen(backup_name, "wb");
        if (!destino) {
            printf("  - Erro ao criar %s\n", backup_name);
            fclose(origem);
            continue;
        }
        
        char buffer[1024];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), origem)) > 0) {
            fwrite(buffer, 1, bytes, destino);
        }
        
        fclose(origem);
        fclose(destino);
        printf("  - %s -> %s\n", arquivos[i], backup_name);
    }
    
    printf(" - Backup completo!\n");
    log_operacao("BACKUP", "Backup completo realizado");
}

// ==================== MAIN ====================

int main() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║     INICIALIZANDO SISTEMA ACADÊMICO (SGA) v1.0        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    printf("Carregando índices...\n");
    
    // Carrega ou cria árvores
    page* root_alunos = loadBTree("alunos.idx");
    if (!root_alunos) {
        printf("  -> Criando novo índice de alunos\n");
        root_alunos = (page*)malloc(sizeof(page));
        if (!root_alunos) {
            printf(" - ERRO CRÍTICO: Falha ao alocar memória!\n");
            return 1;
        }
        initializePage(root_alunos, true);
    } else {
        printf("  - Índice de alunos carregado\n");
    }
    
    page* root_disciplinas = loadBTree("disciplinas.idx");
    if (!root_disciplinas) {
        printf("  -> Criando novo índice de disciplinas\n");
        root_disciplinas = (page*)malloc(sizeof(page));
        if (!root_disciplinas) {
            printf(" - ERRO CRÍTICO: Falha ao alocar memória!\n");
            freeTree(root_alunos);
            return 1;
        }
        initializePage(root_disciplinas, true);
    } else {
        printf("  - Índice de disciplinas carregado\n");
    }
    
    page* root_matriculas = loadBTree("matriculas.idx");
    if (!root_matriculas) {
        printf("  -> Criando novo índice de matrículas\n");
        root_matriculas = (page*)malloc(sizeof(page));
        if (!root_matriculas) {
            printf(" - ERRO CRÍTICO: Falha ao alocar memória!\n");
            freeTree(root_alunos);
            freeTree(root_disciplinas);
            return 1;
        }
        initializePage(root_matriculas, true);
    } else {
        printf("  - Índice de matrículas carregado\n");
    }
    
    printf("\n - Sistema pronto!\n");
    log_operacao("SISTEMA", "Sistema iniciado");
    
    int opcao, sub_opcao;
    
    while (1) {
        menu_principal();
        
        if (scanf("%d", &opcao) != 1) {
            printf("- Entrada inválida!\n");
            limpar_buffer();
            continue;
        }
        limpar_buffer();
        
        switch (opcao) {
            case 1: // Alunos
                while (1) {
                    menu_alunos();
                    
                    if (scanf("%d", &sub_opcao) != 1) {
                        printf("- Entrada inválida!\n");
                        limpar_buffer();
                        continue;
                    }
                    limpar_buffer();
                    
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: inserir_aluno(&root_alunos); break;
                        case 2: buscar_aluno(root_alunos); break;
                        case 3: atualizar_aluno(root_alunos); break;
                        case 4: deletar_aluno(&root_alunos, &root_matriculas); break;
                        case 5: listar_alunos(); break;
                        default: printf("- Opção inválida!\n");
                    }
                }
                break;
                
            case 2: // Disciplinas
                while (1) {
                    menu_disciplinas();
                    
                    if (scanf("%d", &sub_opcao) != 1) {
                        printf("- Entrada inválida!\n");
                        limpar_buffer();
                        continue;
                    }
                    limpar_buffer();
                    
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: inserir_disciplina(&root_disciplinas); break;
                        case 2: buscar_disciplina(root_disciplinas); break;
                        case 3: atualizar_disciplina(root_disciplinas); break;
                        case 4: deletar_disciplina(&root_disciplinas, &root_matriculas); break;
                        case 5: listar_disciplinas(); break;
                        default: printf("- Opção inválida!\n");
                    }
                }
                break;
                
            case 3: // Matrículas
                while (1) {
                    menu_matriculas();
                    
                    if (scanf("%d", &sub_opcao) != 1) {
                        printf("- Entrada inválida!\n");
                        limpar_buffer();
                        continue;
                    }
                    limpar_buffer();
                    
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: matricular_aluno(&root_matriculas, root_alunos, root_disciplinas); break;
                        case 2: buscar_matricula(root_matriculas); break;
                        case 3: atualizar_nota(root_matriculas); break;
                        case 4: remover_matricula(&root_matriculas); break;
                        case 5: listar_matriculas(); break;
                        default: printf("- Opção inválida!\n");
                    }
                }
                break;
                
            case 4: // Relatórios
                while (1) {
                    menu_relatorios();
                    
                    if (scanf("%d", &sub_opcao) != 1) {
                        printf("- Entrada inválida!\n");
                        limpar_buffer();
                        continue;
                    }
                    limpar_buffer();
                    
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: listar_matriculas_aluno(root_alunos); break;
                        case 2: listar_alunos_disciplina(root_disciplinas); break;
                        case 3: calcular_media_geral(root_alunos); break;
                        case 4: listar_todos(); break;
                        default: printf("- Opção inválida!\n");
                    }
                }
                break;
                
            case 5: // Backup
                backup_sistema();
                break;
                
            case 0: // Sair
                printf("\n╔════════════════════════════════════════════════════════╗\n");
                printf("║              ENCERRANDO SISTEMA...                     ║\n");
                printf("╚════════════════════════════════════════════════════════╝\n");
                
                printf("\nSalvando dados...\n");
                saveBTree("alunos.idx", root_alunos);
                printf(" - Índice de alunos salvo\n");
                
                saveBTree("disciplinas.idx", root_disciplinas);
                printf(" - Índice de disciplinas salvo\n");
                
                saveBTree("matriculas.idx", root_matriculas);
                printf(" -  Índice de matrículas salvo\n");
                
                printf("\nLiberando memória...\n");
                freeTree(root_alunos);
                freeTree(root_disciplinas);
                freeTree(root_matriculas);
                
                printf("\n Sistema encerrado com sucesso!\n");
                printf("Até logo!\n\n");
                
                log_operacao("SISTEMA", "Sistema encerrado");
                return 0;
                
            default:
                printf(" - Opção inválida!\n");
        }
    }
    
    return 0;
}