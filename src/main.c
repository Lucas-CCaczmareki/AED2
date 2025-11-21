#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define M 5     // vai precisar ser fixo pq se nao coitado dos arquivo

#define MAX_KEYS M-1
#define MIN_KEYS ((M + 1) / 2 - 1)

#define MAX_CHILDREN M
#define MIN_CHILDREN ((M + 1) / 2)

/*
Implementando um sistema SGA com btree (com permanência em disco)

1. Para conseguir a permanência em disco:
    - Definir a estrutura do registro
    - Criar funções para escrever/ler registros no arquivo
    - Integrar com a b-tree para indexação (utilizando offsets como ponteiros)

2. Implementação com B-Tree:
    - Entra com uma chave e a btree (armazenada em disco) retorna um offset

*/

// ==================== STRUCTS REGISTROS ========================
//As structs começam com a flag pra facilitar a minha vida na hora do registro
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

// ====================== STRUCTS BTREE ==========================

typedef struct {
    int    key;                // matricula do aluno por exemplo
    long   data_offset;        // offset no arquivo alunos.dat por exemplo
} Item;

// Nó da btree em memória (com ponteiros)
typedef struct page {
    //Variáveis de controle
    int     numKeys;        
    bool    isLeaf;

    Item *items;                    // contém a chave e o offset do item

    struct page*    father;         // ponteiro pro pai (não sei se ta sendo usado)
    struct page**   ptr;            // vetor de ponteiros pros filhos
    long            disk_offset;    // offset da página no arquivo .idx
} page;

// Nó serializado pra disco
typedef struct {
    int numKeys;
    bool isLeaf;

    Item items[MAX_KEYS];

    long child_offsets[M];          // Offsets dos filhos no .idx
    long father_offset;             // Offset do pai no .idx
} SerializedPage;


// typedef struct {
//     long root_offset;           // Offset da raiz no .idx
//     int ordem;                  // Ordem da árvore
//     int num_nodes;              // Contador de nós
// } IndexMetadata;


// ========================= Funções de Disco ================================

/* 
 * Abre um dos arquivos de registro, se não existir, cria um
 */
FILE* openFile(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);
    if(fp == NULL && strcmp(mode, "rb+") == 0) { //strcmp retorna 0 se as strings são iguais
        //Se não existe, cria um arquivo
        fp = fopen(filename, "wb+");

        if (fp != NULL) {
            // Inicializa os metadados (root_offset = -1 indica árovre)
        }

    }
    return fp;
}

/*
 * Insere um registro(genérico) no arquivo e retorna o offset (posição) onde
 * foi escrito. Este offset será usado como "ponteiro" na B-tree.
 * 
*/
long insertRecord(const char* filename, void* rec, size_t recSize) {
    FILE* fp = openFile(filename, "rb+");
    
    if(fp == NULL) {
        printf("Erro ao abrir arquivo de alunos!\n");
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

    fclose(fp); //fecha o arquivo

    if(written != 1) {
        printf("Erro ao escrever aluno no arquivo!\n");
        return -1; //indicador de erro
    }

    printf("Aluno inserido no offset: %ld\n", offset); //debug
    return offset;
}

/* 
 * Receb um offset, procura o dado no arquivo. Atualiza a memória com o dado lido e retorna
 * true se o dado é válido, false se o dado for inválido 
 * 
*/
bool searchRecord(const char* filename, long offset, void* rec, size_t recSize) {
    //Abrir o arquivo
    FILE* fp = openFile(filename, "rb");
    
    if(fp == NULL) {
        printf("Erro ao abrir o arquivo!");
        exit(1);
    }

    //Move o arquivo pro offset
    if(fseek(fp, offset, SEEK_SET) != 0) { //se encontramos a posição com sucesso
        printf("Erro ao posicionar nesse offset!\n");
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
    fclose(fp); //fecha o arquivo

    if (lidos != 1) { //se leu a mais ou a menos, deu problema
        printf("Erro ao ler aluno do arquivo!\n");
        return false;
    }

    /* Explicação
     * acessa o primeiro byte do registro, que é sempre a flag
    */
    //Verifica se o registro lido é válido
    if(!*((bool*)rec)) {
        //se o aluno não estiver ativo
        return false;
    }
    //se o aluno estiver ativo
    return true; //lembra que já jogamos pra dentro do aluno (que vem como parâmetro) os dados lidos!
}

/*
 * Atualiza um aluno no registro (arquivo)
 * enquanto tu não estourar o tamanho pré definido essa função funciona
 * então adicionar alguma verificação de segurança no futuro é uma boa
*/
bool updateRecord(const char* filename, long offset, void* rec, size_t recSize) {
    FILE* fp = openFile(filename, "rb+"); //abre na leitura e escrita
    
    //testa se deu bom
    if (fp == NULL) {
        printf("Erro ao abrir arquivo de alunos!\n");
        return false;
    }
    
    // Posiciona no offset
    if (fseek(fp, offset, SEEK_SET) != 0) {
        printf("Erro ao posicionar no offset!\n");
        fclose(fp);
        return false;
    }
    
    // Mantém o registro como ativo
    *((bool*)rec) = true;
    
    // Sobrescreve o registro
    size_t escritos = fwrite(rec, recSize, 1, fp);
    fclose(fp);
    
    if (escritos != 1) {
        printf("Erro ao atualizar aluno!\n");
        return false;
    }
    
    return true;
}

/* 
 * Marca um registro como deletado (sem remover ele fisicamente. Padrão banco de dados)
*/
bool deleteRecord(const char* filename, long offset, size_t recSize){
    FILE* fp = openFile(filename, "rb+"); //abre no modo leitura e escrita
    
    //testa se deu bom
    if(fp == NULL) {
        printf("Erro ao abrir registro!\n");
        return false;
    }

    //Move pro offset
    if(fseek(fp, offset, SEEK_SET) != 0) {
        printf("Erro ao posicionar no offset\n");
        fclose(fp);
        return false;
    }

    //cria o ponteiro pro registro (record)
    void* rec = malloc(recSize);
    if(fread(rec, recSize, 1, fp) != 1) {
        printf("Erro ao ler do registro\n");
        fclose(fp);
        return false;
    }

    // Identifica ele como deletado
    *((bool*)rec) = false;

    //Volta pra posição e sobreescreve
    fseek(fp, offset, SEEK_SET);
    size_t escritos = fwrite(rec, recSize, 1, fp);
    fclose(fp);

    if(escritos != 1) {
        printf("Erro ao deletar aluno!\n");
        return false;
    }

    free(rec);
    return true;
}

/*
 * Lista todos os alunos ativos naquele registro
*/
void listarAlunos() {
    FILE* fp = openFile("alunos.dat", "rb");
    if(fp == NULL) {
        printf("Nenhum aluno cadastrado ainda.\n");
        return;
    }

    Aluno aluno;
    long offset = 0;
    int contador = 0;

    printf("\n===================== LISTA DE ALUNOS =====================\n");

    while(fread(&aluno, sizeof(Aluno), 1, fp) == 1) { //enquanto continuarmos lendo um aluno
        if(aluno.ativo) {
            printf("Offset: %ld\t | Matricula: %d\t | Nome: %s\n",
                offset, aluno.matricula, aluno.nome);
            contador++; //+ um aluno válido
        }
        
        offset += sizeof(Aluno); // move o arquivo 1 aluno pra frente.
    }

    printf("Total de alunos ativos: %d\n", contador);
    fclose(fp);

}

// ========================= Funções da B-Tree =============================

/*
 * Inicializa uma página
*/
void initializePage(page* pg, bool is_leaf) {
    // Variáveis de controle
    pg->numKeys = 0;
    pg->isLeaf = is_leaf;
    
    // Cria um vetor de itens (key e offset pro dado no arquivo)
    pg->items = (Item*)calloc(MAX_KEYS, sizeof(Item));

    // Ponteiros de controle
    pg->father = NULL;      //revisar se no fim eu to usando ou não
    pg->ptr = (page**)calloc(M, sizeof(page*));

    // Teste de segurança
    if (pg->items == NULL || pg->ptr == NULL) {
        printf("ERRO DE ALOCAÇÃO DA PAGINA\n");
        exit(1);
    }
}

/* Divide o filho: parent->ptr[i] que é o full_child */
void splitChild(page *parent, int i, page* fullChild) {
    // Aloca a nova página (irmã) pra dividir a full child
    page* newSister = (page*)malloc(sizeof(page));
    initializePage(newSister, fullChild->isLeaf); //se o irmão é folha, a irmã também é
    
    newSister->father = parent; //conferir se ainda tá sendo usado

    int oldNumKeys = fullChild->numKeys;
    int mid_index = oldNumKeys / 2;         // usa o número atual de chaves, funciona pra impar ou par
    
    Item midItem = fullChild->items[mid_index];

    // Copia as chaves DEPOIS do mid pra irmã
    int j = 0;
    for (int k = mid_index + 1; k < oldNumKeys; k++) {
        newSister->items[j++] = fullChild->items[k]; //j++ incrementa DEPOIS da atribuição
        newSister->numKeys++;
    }

    // Copia os ponteiros SE for uma página interna (não-folha)
    j = 0;
    if(!fullChild->isLeaf) {
        for (int k = mid_index + 1; k <= oldNumKeys; k++) {
            newSister->ptr[j] = fullChild->ptr[k];
            
            //Atualiza o ponteiro do pai do filho (não sei se vai ser necessário)
            if (newSister->ptr[j] != NULL) {
                newSister->ptr[j]->father = newSister;
            }
            
            j++; //atualiza contador da irmã
        }
    }

    // Atualiza o contados de chaves do irmão (fullChild)
    fullChild->numKeys = mid_index;

    // Move os ponteiros do pai pra ter espaço
        // Começa do último espaço ocupado (numKeys - 1 pq índice inicia em 0)
    for (int k = parent->numKeys - 1; k >= i; k--) { //loop do fim até i (meio)
        parent->items[k + 1] = parent->items[k];
        parent->ptr[k + 2] = parent->ptr[k + 1];    //pq tem 1 ponteiro a + do que chave
    }

    //O pai é garantido ter um espaço, pq se não tivesse ele seria dividido antes de descermos pra ele
        //Isso acontece pela definição do split-then-insert

    parent->items[i] = midItem;     // Item do meio é promovido pro pai
    parent->ptr[i + 1] = newSister; // ponteiro à direita do item promovido é a irmã criada na divisão
    parent->numKeys++;

    //Atualiza os ponteiros de pai (mas provavelmente não são necessários)
    fullChild->father = parent;
    newSister->father = parent;
}

/* Insere numa página que ainda não tá cheia */
void insertIntoNonFull(page* pg, Item item) {
    //A inserção só ocorre de fato, em folhas, propriedade da B-Tree
    if (pg->isLeaf) {
        int pos = pg->numKeys - 1; //pq índice começa em 0

        while (pos >= 0 && item.key < pg->items[pos].key) {
            //Isso aqui sempre funciona pq é garantido pelo menos 1 espaço vazio nessa função
            pg->items[pos + 1] = pg->items[pos]; //move 1 pra direita
            pos--;
        }

        //O que fica vazio é o índice seguinte de onde o "pos" parou
        pg->items[pos + 1] = item;
        pg->numKeys++;
    
    // Caso a página NÃO seja folha, desce recursivamente até achar a folha correspondente
    } else {
        int pos = pg->numKeys - 1;

        while (pos >= 0 && item.key < pg->items[pos].key) pos--;
        pos++; //pos agora aponta pra onde o item deve ser inserido

        //Segurança extra pra não ter como dar segfault
        if (pg->ptr[pos] == NULL) { //se o ponteiro à a esquerda for nulo
            // Não é pra isso acontecer nunca, já que se não for folha, o ponteiro tem filho
            // mas se por algum erro isso aconteceu... Cria o filho e fica por isso
            pg->ptr[pos] = (page*)malloc(sizeof(page));
            initializePage(pg->ptr[pos], true);

            pg->ptr[pos]->father = pg; //não ta sendo usado até então, mas vamo ver
        }

        // Split preventivo do Split-Then-Insert. 
            // Confere se o filho que vai descer ta cheio, e se tiver, splita
        if (pg->ptr[pos]->numKeys == MAX_KEYS) {
            splitChild(pg, pos, pg->ptr[pos]);

            // SE a chave que vamos inserir é MAIOR QUE a chave promovida
            if (item.key > pg->items[pos].key) pos++; //descer pelo ponteiro da direita
            // caso contrário, o incremento não é feito, descemos pelo ponteiro da esquerda
        }
        insertIntoNonFull(pg->ptr[pos], item);
    }
}

/* Insert principal */
void insertBtree(page** root_ptr, Item item) {
    page* root = *root_ptr;

    // Se a raiz tá cheia, faz a divisão preventiva
    if (root->numKeys == MAX_KEYS) {
        page* new_root = (page*)malloc(sizeof(page));
        initializePage(new_root, false);

        new_root->ptr[0] = root;
        root->father = new_root;
        splitChild(new_root, 0, root); //splita a root original

        // Confere se desce pela esquerda ou direita
        int pos = 0;
        if (item.key > new_root->items[0].key) pos++;

        // Desce até achar a folha onde ele vai ser inserido
        insertIntoNonFull(new_root->ptr[pos], item);
        
        // Atualiza a referência da árvore
        *root_ptr = new_root;
    
    // Se a raiz NÃO está cheia
    } else {
        // Desce recursivamente pelos filhos até uma folha e insere
        insertIntoNonFull(root, item);
    }
}

/* Procura na btree uma key, e retorna o offset daquele item no arquivo */
long search(page* root, int key) {
    page* cur = root;

    while (cur != NULL) {
        int i = 0;

        // move o contador até a posição
        while (i < cur->numKeys && key > cur->items[i].key) i++;

        // Se a posição corresponde à chave procurada, retorna o offset
        if (i < cur->numKeys && key == cur->items[i].key) return cur->items[i].data_offset;

        // Se não é a chave procurada, e é uma folha, retorna -1 (não encontramos)
        if (cur->isLeaf) return -1;

        // Se não é uma folha, desce na árvore e continua o loop
        cur = cur->ptr[i];
    }

    //Se desceu toda árvore e ainda não achou, retorna o -1
    return -1;
}

// ================== Funções de serialização da B-Tree =====================
/* Transforma uma página em memória ram pro formato de uma página em disco */
SerializedPage serializePage(page* pg) {
    SerializedPage sp;

    // Copia as coisas que são igual
    sp.numKeys = pg->numKeys;
    sp.isLeaf = pg->isLeaf;

    // Copia os itens
    for (int i = 0; i < MAX_KEYS; i++) {
        if(i < pg->numKeys) {
            //Enquanto tiver filho, copia.
            sp.items[i] = pg->items[i];
        } else {
            // Não tem filho, offset e chave não existem. 
            // -1 e 0 pra indicar
            sp.items[i].data_offset = -1;
            sp.items[i].key = 0;
        }
    }

    // Converte os ponteiros em offsets
    for (int i = 0; i < M; i++) {
        if (i <= pg->numKeys && pg->ptr[i] != NULL) {
            sp.child_offsets[i] = pg->ptr[i]->disk_offset;
        } else {
            sp.child_offsets[i] = -1;
        }
    }

    // Converte o ponteiro do pai em offset
    sp.father_offset = (pg->father != NULL) ? pg->father->disk_offset : -1;

    return sp;
}

/* Escreve um nó no disco, e retorna o offset daquele nó */
long savePage_disk(FILE* fp, page* pg) {
    // posso eventualmente usar o open file aqui dentro ou no main/outra função, whatever
    
    //Se tá salvando uma nula, retorna -1. Offset não existe
    if (pg == NULL) return -1;

    // Se a página já tem um offset (já tá em disco), sobrescreve o que tá escrito no arquivo
    if (pg->disk_offset != -1) {
        fseek(fp, pg->disk_offset, SEEK_SET); // vai até o offset
    } else {
        // Senão, vai pro final do arquivo e escreve a página lá
        // (meio que fodase onde a página tá no arquivo, já que vai ser tudo por offset)
        
        fseek(fp, 0, SEEK_END);     // move pro final
        pg->disk_offset = ftell(fp);    // salva o offset
    }

    //Converte a página pra formato de disco e salva
    SerializedPage sp = serializePage(pg);
    fwrite(&sp, sizeof(SerializedPage), 1, fp);
    fflush(fp); //força a gravação imediata no disco (limpando o buffer)

    return pg->disk_offset;
}

/* Pega do arquivo uma página serializada e converte para uma página em memória RAM */
page* loadPage_disk(FILE* fp, long offset) {
    if (offset == -1) return NULL;

    //Posiciona o ponteiro do arquivo no offset
    fseek(fp, offset, SEEK_SET);

    // Carregando a página serializada pra memória
    SerializedPage sp;
    if (fread(&sp, sizeof(SerializedPage), 1, fp) != 1) {
        printf("Erro ao ler nó do disco (offset %ld)\n", offset);
        return NULL;
    }

    // Reconstruindo a página em memória a partir da página serializada
    page* pg = (page*)malloc(sizeof(page));
    pg->numKeys = sp.numKeys;
    pg->isLeaf = sp.isLeaf;
    pg->disk_offset = offset;

    // alloca os arrays
    pg->items = (Item*)calloc(MAX_KEYS, sizeof(Item));
    pg->ptr = (page**)calloc(M, sizeof(page*));

    // Copia os itens da serializada pra página em memória
    for (int i = 0; i < pg->numKeys; i++) {
        pg->items[i] = sp.items[i];
    }

    // APENAS A PÁGINA ATUAL É CARREGADA PRA MEMÓRIA
    // as outras são carregadas sobre demanda, se necessário
    for (int i = 0; i < M; i++) {
        pg->ptr[i] = NULL;
    }
    pg->father = NULL;

    return pg;
}

/* Percorre a árvore recursivamente e salva todos os nós */
void saveBTree_recursive(FILE* fp, page* node) {
    if (node == NULL) return;
    
    // Primeiro salva os filhos (pós-ordem)
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->ptr[i] != NULL) {
                saveBTree_recursive(fp, node->ptr[i]);
            }
        }
    }
    
    // Depois salva este nó
    savePage_disk(fp, node);
}

/* Libera a árvore inteira recursivamente */
void freeTree(page* n) {
    if (!n) return;
    if (!n->isLeaf) {
        for (int i = 0; i <= n->numKeys; i++) {
            if (n->ptr[i]) freeTree(n->ptr[i]);
        }
    }
    free(n->items);
    free(n->ptr);
    free(n);
}

//FUnções de teste
int main () {
    /*
    // printf("=== TESTE DE GERENCIAMENTO DE ARQUIVOS - ALUNOS ===\n\n");
    
    // // Teste 1: Inserir alunos
    // printf("1. Inserindo alunos...\n");
    
    // Aluno a1 = {true, 12345, "João Silva"};
    // Aluno a2 = {true, 67890, "Maria Santos"};
    // Aluno a3 = {true, 11111, "Pedro Oliveira"};
    
    // long offset1 = insertRecord("alunos.dat", &a1, sizeof(Aluno));
    // long offset2 = insertRecord("alunos.dat", &a2, sizeof(Aluno));
    // long offset3 = insertRecord("alunos.dat", &a3, sizeof(Aluno));
    
    // // Teste 2: Listar todos
    // printf("\n2. Listando todos os alunos:\n");
    // listarAlunos();
    
    // // Teste 3: Buscar por offset
    // printf("\n3. Buscando aluno no offset %ld:\n", offset2);
    // Aluno buscado;
    // if (searchRecord("alunos.dat", offset2, &buscado, sizeof(Aluno))) {
        
    //     printf("Encontrado: Matrícula %d - %s\n", 
    //            buscado.matricula, buscado.nome);
    // }
    
    // // Teste 4: Atualizar
    // printf("\n4. Atualizando nome do aluno...\n");
    // strcpy(a2.nome, "Maria Santos Sousa");
    // updateRecord("alunos.dat", offset2, &a2, sizeof(Aluno));
    // listarAlunos();
    
    // // Teste 5: Deletar
    // printf("\n5. Deletando aluno no offset %ld...\n", offset1);
    // deleteRecord("alunos.dat", offset1, sizeof(Aluno));
    // listarAlunos();
    
    // return 0;
    */
    page* root = (page*)malloc(sizeof(page));
    initializePage(root, true);

    Item i1 = {12345, 0};
    Item i2 = {11111, 150};

    insertBtree(&root, i1);
    insertBtree(&root, i2);

    long offset_i2 = search(root, 11111);
    printf("Offset encontrado: %ld\n", offset_i2);
    
    FILE* fp = openFile("test.idx", "rb+");

    saveBTree_recursive(fp, root);
    printf("offset root: %ld\n", root->disk_offset);
    long root_offset = root->disk_offset;

    freeTree(root);

    fseek(fp, 0, SEEK_SET); //volta o fp pro início

    page* loaded_root = loadPage_disk(fp, root_offset);
    offset_i2 = -1; //reseta offset
    offset_i2 = search(loaded_root, 11111);

    printf("Offset encontrado (após recarregar!): %ld\n", offset_i2);
}