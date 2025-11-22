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

    long* children_offsets;         // vetor de offsets (não alocados). Vai ser alocado quando carregarmos a página da memória

} page;

// Nó serializado pra disco
typedef struct {
    int numKeys;
    bool isLeaf;

    Item items[MAX_KEYS];

    long child_offsets[M];          // Offsets dos filhos no .idx
    long father_offset;             // Offset do pai no .idx
} SerializedPage;


typedef struct {
    long root_offset;           // Offset da raiz no .idx
    int ordem;                  // Ordem da árvore
    int num_nodes;              // Contador de nós
} BtreeInfo;

// ==================== DECLARAÇÃO DE FUNÇÕES =======================
FILE* openDataFile(const char* filename, const char* mode);
FILE* openIndexFile(const char* filename, const char* mode);
long insertRecord(const char* filename, void* rec, size_t recSize);
bool searchRecord(const char* filename, long offset, void* rec, size_t recSize);
bool updateRecord(const char* filename, long offset, void* rec, size_t recSize);
bool deleteRecord(const char* filename, long offset, size_t recSize);
void listarAlunos();
void initializePage(page* pg, bool is_leaf);
void splitChild(page *parent, int i, page* fullChild);
void insertIntoNonFull(page* pg, Item item, const char* idx_file);
void insertIntoNonFull_disk(page* pg, Item item, const char* idx_file);
void insertBtree(page** root_ptr, Item item, const char* idx_file);
void insertBtree_disk(page** root_ptr, Item item, const char* idx_file);
long search(page* root, int key);
long search_disk(page* root, int key, const char* idx_file);
void removeBtree_disk(page** root_ptr, int key, const char* idx_file);

SerializedPage serializePage(page* pg);
long savePage_disk(FILE* fp, page* pg);
page* loadPage_disk(FILE* fp, long offset);
void saveBTree_recursive(FILE* fp, page* node);
void saveBTree(const char* idx_file, page* root);
page* loadBTree(const char* idx_file);
void freeTree(page* n);


// ==================== FUNÇÕES DE TESTE ====================

void teste_completo_btree() {
    printf("=== TESTE COMPLETO: B-TREE COM DISCO ===\n\n");
    
    const char* idx_file = "test.idx";
    remove(idx_file);
    
    // ====== PARTE 1: INSERÇÃO ======
    printf("--- PARTE 1: INSERÇÃO ---\n");
    page* root = (page*)malloc(sizeof(page));
    initializePage(root, true);
    
    int keys[] = {50, 30, 70, 20, 40, 60, 80, 10, 25, 35};
    for (int i = 0; i < 10; i++) {
        Item item = {keys[i], keys[i] * 10};
        insertBtree_disk(&root, item, idx_file);
        printf("✓ Inserido: %d\n", keys[i]);
    }
    
    freeTree(root);
    root = NULL;
    
    // ====== PARTE 2: CARREGAMENTO E BUSCA ======
    printf("\n--- PARTE 2: CARREGAMENTO E BUSCA ---\n");
    root = loadBTree(idx_file);
    
    if (root) {
        printf("✓ Árvore carregada\n");
        for (int i = 0; i < 10; i++) {
            long offset = search_disk(root, keys[i], idx_file);
            printf("Busca %d: offset=%ld %s\n", 
                   keys[i], offset, offset == keys[i]*10 ? "✓" : "✗");
        }
    }
    
    // ====== PARTE 3: REMOÇÃO ======
    printf("\n--- PARTE 3: REMOÇÃO ---\n");
    int remove_keys[] = {20, 50, 70};
    for (int i = 0; i < 3; i++) {
        removeBtree_disk(&root, remove_keys[i], idx_file);
        printf("✓ Removido: %d\n", remove_keys[i]);
    }
    
    // Verifica remoções
    printf("\nVerificando remoções:\n");
    for (int i = 0; i < 3; i++) {
        long offset = search_disk(root, remove_keys[i], idx_file);
        printf("Busca %d: %s\n", remove_keys[i], 
               offset == -1 ? "✓ Não encontrado (correto)" : "✗ ERRO: ainda existe");
    }
    
    freeTree(root);
    printf("\n✅ TESTE COMPLETO!\n");
}

// ==================== SISTEMA SGA - CAMADA DE APLICAÇÃO ====================

void menu_principal() {
    printf("\n========== SISTEMA DE GERENCIAMENTO ACADÊMICO ==========\n");
    printf("1.  Gerenciar Alunos\n");
    printf("2.  Gerenciar Disciplinas\n");
    printf("3.  Gerenciar Matrículas\n");
    printf("4.  Listar Todos os Dados\n");
    printf("0.  Sair\n");
    printf("========================================================\n");
    printf("Escolha: ");
}

void menu_alunos() {
    printf("\n--- GERENCIAR ALUNOS ---\n");
    printf("1. Inserir Aluno\n");
    printf("2. Buscar Aluno\n");
    printf("3. Atualizar Aluno\n");
    printf("4. Deletar Aluno\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

void menu_disciplinas() {
    printf("\n--- GERENCIAR DISCIPLINAS ---\n");
    printf("1. Inserir Disciplina\n");
    printf("2. Buscar Disciplina\n");
    printf("3. Atualizar Disciplina\n");
    printf("4. Deletar Disciplina\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

void menu_matriculas() {
    printf("\n--- GERENCIAR MATRÍCULAS ---\n");
    printf("1. Matricular Aluno\n");
    printf("2. Buscar Matrícula\n");
    printf("3. Lançar/Atualizar Nota\n");
    printf("4. Remover Matrícula\n");
    printf("0. Voltar\n");
    printf("Escolha: ");
}

// ========== FUNÇÕES CRUD ALUNOS ==========

void inserir_aluno(page** root_alunos) {
    Aluno aluno;
    printf("\nMatrícula: ");
    scanf("%d", &aluno.matricula);
    getchar(); // limpa buffer
    
    // Verifica se já existe
    long existe = search_disk(*root_alunos, aluno.matricula, "alunos.idx");
    if (existe != -1) {
        printf("✗ Aluno com essa matrícula já existe!\n");
        return;
    }
    
    printf("Nome: ");
    fgets(aluno.nome, 50, stdin);
    aluno.nome[strcspn(aluno.nome, "\n")] = 0; // remove \n
    
    long offset = insertRecord("alunos.dat", &aluno, sizeof(Aluno));
    if (offset != -1) {
        Item item = {aluno.matricula, offset};
        insertBtree_disk(root_alunos, item, "alunos.idx");
        printf("✓ Aluno inserido com sucesso!\n");
    }
}

void buscar_aluno(page* root_alunos) {
    int matricula;
    printf("\nMatrícula: ");
    scanf("%d", &matricula);
    
    long offset = search_disk(root_alunos, matricula, "alunos.idx");
    if (offset == -1) {
        printf("✗ Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    if (searchRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
        printf("\n--- ALUNO ENCONTRADO ---\n");
        printf("Matrícula: %d\n", aluno.matricula);
        printf("Nome: %s\n", aluno.nome);
    } else {
        printf("✗ Erro ao ler dados do aluno!\n");
    }
}

void atualizar_aluno(page* root_alunos) {
    int matricula;
    printf("\nMatrícula: ");
    scanf("%d", &matricula);
    getchar();
    
    long offset = search_disk(root_alunos, matricula, "alunos.idx");
    if (offset == -1) {
        printf("✗ Aluno não encontrado!\n");
        return;
    }
    
    Aluno aluno;
    if (!searchRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
        printf("✗ Erro ao ler aluno!\n");
        return;
    }
    
    printf("Nome atual: %s\n", aluno.nome);
    printf("Novo nome: ");
    fgets(aluno.nome, 50, stdin);
    aluno.nome[strcspn(aluno.nome, "\n")] = 0;
    
    if (updateRecord("alunos.dat", offset, &aluno, sizeof(Aluno))) {
        printf("✓ Aluno atualizado com sucesso!\n");
    } else {
        printf("✗ Erro ao atualizar!\n");
    }
}

void deletar_aluno(page** root_alunos, page** root_matriculas) {
    int matricula;
    printf("\nMatrícula: ");
    scanf("%d", &matricula);
    
    long offset = search_disk(*root_alunos, matricula, "alunos.idx");
    if (offset == -1) {
        printf("✗ Aluno não encontrado!\n");
        return;
    }
    
    // CONSISTÊNCIA REFERENCIAL: Remove todas matrículas do aluno
    printf("Removendo matrículas associadas...\n");
    FILE* fp = openDataFile("matriculas.dat", "rb");
    if (fp) {
        Matricula mat;
        long mat_offset = 0;
        while (fread(&mat, sizeof(Matricula), 1, fp) == 1) {
            if (mat.ativo && mat.matricula_aluno == matricula) {
                deleteRecord("matriculas.dat", mat_offset, sizeof(Matricula));
                removeBtree_disk(root_matriculas, mat.id_matricula, "matriculas.idx");
                printf("  ✓ Matrícula %d removida\n", mat.id_matricula);
            }
            mat_offset += sizeof(Matricula);
        }
        fclose(fp);
    }
    
    // Remove o aluno
    if (deleteRecord("alunos.dat", offset, sizeof(Aluno))) {
        removeBtree_disk(root_alunos, matricula, "alunos.idx");
        printf("✓ Aluno deletado com sucesso!\n");
    } else {
        printf("✗ Erro ao deletar!\n");
    }
}

// ========== FUNÇÕES CRUD DISCIPLINAS ==========

void inserir_disciplina(page** root_disciplinas) {
    Disciplina disc;
    printf("\nCódigo da disciplina: ");
    scanf("%d", &disc.codigo);
    getchar();
    
    long existe = search_disk(*root_disciplinas, disc.codigo, "disciplinas.idx");
    if (existe != -1) {
        printf("✗ Disciplina com esse código já existe!\n");
        return;
    }
    
    printf("Nome: ");
    fgets(disc.nome, 50, stdin);
    disc.nome[strcspn(disc.nome, "\n")] = 0;
    
    long offset = insertRecord("disciplinas.dat", &disc, sizeof(Disciplina));
    if (offset != -1) {
        Item item = {disc.codigo, offset};
        insertBtree_disk(root_disciplinas, item, "disciplinas.idx");
        printf("✓ Disciplina inserida com sucesso!\n");
    }
}

void buscar_disciplina(page* root_disciplinas) {
    int codigo;
    printf("\nCódigo: ");
    scanf("%d", &codigo);
    
    long offset = search_disk(root_disciplinas, codigo, "disciplinas.idx");
    if (offset == -1) {
        printf("✗ Disciplina não encontrada!\n");
        return;
    }
    
    Disciplina disc;
    if (searchRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
        printf("\n--- DISCIPLINA ENCONTRADA ---\n");
        printf("Código: %d\n", disc.codigo);
        printf("Nome: %s\n", disc.nome);
    }
}

void atualizar_disciplina(page* root_disciplinas) {
    int codigo;
    printf("\nCódigo: ");
    scanf("%d", &codigo);
    getchar();
    
    long offset = search_disk(root_disciplinas, codigo, "disciplinas.idx");
    if (offset == -1) {
        printf("✗ Disciplina não encontrada!\n");
        return;
    }
    
    Disciplina disc;
    if (!searchRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
        printf("✗ Erro ao ler disciplina!\n");
        return;
    }
    
    printf("Nome atual: %s\n", disc.nome);
    printf("Novo nome: ");
    fgets(disc.nome, 50, stdin);
    disc.nome[strcspn(disc.nome, "\n")] = 0;
    
    if (updateRecord("disciplinas.dat", offset, &disc, sizeof(Disciplina))) {
        printf("✓ Disciplina atualizada!\n");
    }
}

void deletar_disciplina(page** root_disciplinas, page** root_matriculas) {
    int codigo;
    printf("\nCódigo: ");
    scanf("%d", &codigo);
    
    long offset = search_disk(*root_disciplinas, codigo, "disciplinas.idx");
    if (offset == -1) {
        printf("✗ Disciplina não encontrada!\n");
        return;
    }
    
    // CONSISTÊNCIA REFERENCIAL
    printf("Removendo matrículas associadas...\n");
    FILE* fp = openDataFile("matriculas.dat", "rb");
    if (fp) {
        Matricula mat;
        long mat_offset = 0;
        while (fread(&mat, sizeof(Matricula), 1, fp) == 1) {
            if (mat.ativo && mat.cod_disciplina == codigo) {
                deleteRecord("matriculas.dat", mat_offset, sizeof(Matricula));
                removeBtree_disk(root_matriculas, mat.id_matricula, "matriculas.idx");
                printf("  ✓ Matrícula %d removida\n", mat.id_matricula);
            }
            mat_offset += sizeof(Matricula);
        }
        fclose(fp);
    }
    
    if (deleteRecord("disciplinas.dat", offset, sizeof(Disciplina))) {
        removeBtree_disk(root_disciplinas, codigo, "disciplinas.idx");
        printf("✓ Disciplina deletada!\n");
    }
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
    
    printf("\nMatrícula do aluno: ");
    scanf("%d", &mat.matricula_aluno);
    
    // Verifica se aluno existe
    if (search_disk(root_alunos, mat.matricula_aluno, "alunos.idx") == -1) {
        printf("✗ Aluno não encontrado!\n");
        return;
    }
    
    printf("Código da disciplina: ");
    scanf("%d", &mat.cod_disciplina);
    
    // Verifica se disciplina existe
    if (search_disk(root_disciplinas, mat.cod_disciplina, "disciplinas.idx") == -1) {
        printf("✗ Disciplina não encontrada!\n");
        return;
    }
    
    mat.id_matricula = proximo_id_matricula();
    mat.media_final = 0.0;
    
    long offset = insertRecord("matriculas.dat", &mat, sizeof(Matricula));
    if (offset != -1) {
        Item item = {mat.id_matricula, offset};
        insertBtree_disk(root_matriculas, item, "matriculas.idx");
        printf("✓ Matrícula realizada! ID: %d\n", mat.id_matricula);
    }
}

void buscar_matricula(page* root_matriculas) {
    int id;
    printf("\nID da matrícula: ");
    scanf("%d", &id);
    
    long offset = search_disk(root_matriculas, id, "matriculas.idx");
    if (offset == -1) {
        printf("✗ Matrícula não encontrada!\n");
        return;
    }
    
    Matricula mat;
    if (searchRecord("matriculas.dat", offset, &mat, sizeof(Matricula))) {
        printf("\n--- MATRÍCULA ---\n");
        printf("ID: %d\n", mat.id_matricula);
        printf("Aluno: %d\n", mat.matricula_aluno);
        printf("Disciplina: %d\n", mat.cod_disciplina);
        printf("Média Final: %.2f\n", mat.media_final);
    }
}

void atualizar_nota(page* root_matriculas) {
    int id;
    printf("\nID da matrícula: ");
    scanf("%d", &id);
    
    long offset = search_disk(root_matriculas, id, "matriculas.idx");
    if (offset == -1) {
        printf("✗ Matrícula não encontrada!\n");
        return;
    }
    
    Matricula mat;
    if (!searchRecord("matriculas.dat", offset, &mat, sizeof(Matricula))) {
        printf("✗ Erro ao ler matrícula!\n");
        return;
    }
    
    printf("Média atual: %.2f\n", mat.media_final);
    printf("Nova média: ");
    scanf("%f", &mat.media_final);
    
    if (updateRecord("matriculas.dat", offset, &mat, sizeof(Matricula))) {
        printf("✓ Nota atualizada!\n");
    }
}

void remover_matricula(page** root_matriculas) {
    int id;
    printf("\nID da matrícula: ");
    scanf("%d", &id);
    
    long offset = search_disk(*root_matriculas, id, "matriculas.idx");
    if (offset == -1) {
        printf("✗ Matrícula não encontrada!\n");
        return;
    }
    
    if (deleteRecord("matriculas.dat", offset, sizeof(Matricula))) {
        removeBtree_disk(root_matriculas, id, "matriculas.idx");
        printf("✓ Matrícula removida!\n");
    }
}

// ========== LISTAGENS ==========

void listar_todos() {
    printf("\n========== ALUNOS ==========\n");
    FILE* fp = openDataFile("alunos.dat", "rb");
    if (fp) {
        Aluno a;
        while (fread(&a, sizeof(Aluno), 1, fp) == 1) {
            if (a.ativo) {
                printf("Mat: %d | Nome: %s\n", a.matricula, a.nome);
            }
        }
        fclose(fp);
    }
    
    printf("\n========== DISCIPLINAS ==========\n");
    fp = openDataFile("disciplinas.dat", "rb");
    if (fp) {
        Disciplina d;
        while (fread(&d, sizeof(Disciplina), 1, fp) == 1) {
            if (d.ativo) {
                printf("Cod: %d | Nome: %s\n", d.codigo, d.nome);
            }
        }
        fclose(fp);
    }
    
    printf("\n========== MATRÍCULAS ==========\n");
    fp = openDataFile("matriculas.dat", "rb");
    if (fp) {
        Matricula m;
        while (fread(&m, sizeof(Matricula), 1, fp) == 1) {
            if (m.ativo) {
                printf("ID: %d | Aluno: %d | Disc: %d | Nota: %.2f\n",
                       m.id_matricula, m.matricula_aluno, 
                       m.cod_disciplina, m.media_final);
            }
        }
        fclose(fp);
    }
}

// ==================== MAIN ====================

int main() {
    // Carrega ou cria árvores
    page* root_alunos = loadBTree("alunos.idx");
    if (!root_alunos) {
        root_alunos = (page*)malloc(sizeof(page));
        initializePage(root_alunos, true);
    }
    
    page* root_disciplinas = loadBTree("disciplinas.idx");
    if (!root_disciplinas) {
        root_disciplinas = (page*)malloc(sizeof(page));
        initializePage(root_disciplinas, true);
    }
    
    page* root_matriculas = loadBTree("matriculas.idx");
    if (!root_matriculas) {
        root_matriculas = (page*)malloc(sizeof(page));
        initializePage(root_matriculas, true);
    }
    
    // Para rodar o teste ao invés do sistema:
    // teste_completo_btree();
    // return 0;
    
    int opcao, sub_opcao;
    
    while (1) {
        menu_principal();
        scanf("%d", &opcao);
        
        switch (opcao) {
            case 1: // Alunos
                while (1) {
                    menu_alunos();
                    scanf("%d", &sub_opcao);
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: inserir_aluno(&root_alunos); break;
                        case 2: buscar_aluno(root_alunos); break;
                        case 3: atualizar_aluno(root_alunos); break;
                        case 4: deletar_aluno(&root_alunos, &root_matriculas); break;
                    }
                }
                break;
                
            case 2: // Disciplinas
                while (1) {
                    menu_disciplinas();
                    scanf("%d", &sub_opcao);
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: inserir_disciplina(&root_disciplinas); break;
                        case 2: buscar_disciplina(root_disciplinas); break;
                        case 3: atualizar_disciplina(root_disciplinas); break;
                        case 4: deletar_disciplina(&root_disciplinas, &root_matriculas); break;
                    }
                }
                break;
                
            case 3: // Matrículas
                while (1) {
                    menu_matriculas();
                    scanf("%d", &sub_opcao);
                    if (sub_opcao == 0) break;
                    
                    switch (sub_opcao) {
                        case 1: matricular_aluno(&root_matriculas, root_alunos, root_disciplinas); break;
                        case 2: buscar_matricula(root_matriculas); break;
                        case 3: atualizar_nota(root_matriculas); break;
                        case 4: remover_matricula(&root_matriculas); break;
                    }
                }
                break;
                
            case 4: // Listar tudo
                listar_todos();
                break;
                
            case 0: // Sair
                printf("\nSalvando dados...\n");
                saveBTree("alunos.idx", root_alunos);
                saveBTree("disciplinas.idx", root_disciplinas);
                saveBTree("matriculas.idx", root_matriculas);
                
                freeTree(root_alunos);
                freeTree(root_disciplinas);
                freeTree(root_matriculas);
                
                printf("✓ Sistema encerrado!\n");
                return 0;
                
            default:
                printf("Opção inválida!\n");
        }
    }
    
    return 0;
}

// ========================= Funções de Disco ================================

/* 
 * Abre um dos arquivos de registro, se não existir, cria um
 */
FILE* openDataFile(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);

    if(fp == NULL && strcmp(mode, "rb+") == 0) { //strcmp retorna 0 se as strings são iguais
        //Se não existe, cria um arquivo
        fp = fopen(filename, "wb+");
    }
    return fp;
}

FILE* openIndexFile(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);

    if(fp == NULL && strcmp(mode, "rb+") == 0) { //strcmp retorna 0 se as strings são iguais
        //Se não existe, cria um arquivo
        fp = fopen(filename, "wb+");
    
        if (fp != NULL) {
            // Inicializa os metadados (root_offset = -1 indica árovre)
            BtreeInfo info = {-1, M, 0};
            fwrite(&info, sizeof(BtreeInfo), 1, fp);
            fflush(fp);
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
    FILE* fp = openDataFile(filename, "rb+");
    
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
    FILE* fp = openDataFile(filename, "rb");
    
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
    FILE* fp = openDataFile(filename, "rb+"); //abre na leitura e escrita
    
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
    FILE* fp = openDataFile(filename, "rb+"); //abre no modo leitura e escrita
    
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
    FILE* fp = openDataFile("alunos.dat", "rb");
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

    // Inicializa como não salva em disco
    pg->disk_offset = -1;

    // Não aloca os offsets na criação se não for necessário
    if (!is_leaf) {
        pg->children_offsets = (long*)malloc(sizeof(long) * M);
        for(int i = 0; i < M; i++) pg->children_offsets[i] = -1;
    } else {
        pg->children_offsets = NULL;
    }

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
            
            //Se sister não é uma folha, ela já é inicializada com os offsets criados
            newSister->children_offsets[j] = fullChild->children_offsets[k];
            
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
        parent->children_offsets[k + 2] = parent->children_offsets[k + 1]; //empurramos os offsets também

    }

    //O pai é garantido ter um espaço, pq se não tivesse ele seria dividido antes de descermos pra ele
        //Isso acontece pela definição do split-then-insert

    parent->items[i] = midItem;     // Item do meio é promovido pro pai
    parent->ptr[i + 1] = newSister; // ponteiro à direita do item promovido é a irmã criada na divisão
    
    //A nova página ainda não tem um offset, por isso apenas atualizamos com -1
    parent->children_offsets[i + 1] = -1;   //quando passar pelo savePage_disk ela é atualizada
    
    parent->numKeys++;

    //Atualiza os ponteiros de pai (mas provavelmente não são necessários)
    fullChild->father = parent;
    newSister->father = parent;
}

/* Insere numa página que ainda não tá cheia */
void insertIntoNonFull(page* pg, Item item, const char* idx_file) {
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

        //se o filho não tá na RAM (carrega do disco se for necessário)
        if (pg->ptr[pos] == NULL) { 
            //Verifica se ele existe em disco
            if (pg->children_offsets != NULL && pg->children_offsets[pos] != -1) {
                //Carrega do disco
                FILE* fp = openIndexFile(idx_file, "rb+");
                if (fp) {
                    pg->ptr[pos] = loadPage_disk(fp, pg->children_offsets[pos]);
                    
                    if (pg->ptr[pos]) {
                        pg->ptr[pos]->father = pg;
                    }
                    fclose(fp);
                }
            } else {
                // Cria um novo filho
                pg->ptr[pos] = (page*)malloc(sizeof(page));
                initializePage(pg->ptr[pos], true);
            }
        }    

        // Split preventivo do Split-Then-Insert. 
            // Confere se o filho que vai descer ta cheio, e se tiver, splita
        if (pg->ptr[pos]->numKeys == MAX_KEYS) {
            splitChild(pg, pos, pg->ptr[pos]);

            // SE a chave que vamos inserir é MAIOR QUE a chave promovida
            if (item.key > pg->items[pos].key) pos++; //descer pelo ponteiro da direita
            // caso contrário, o incremento não é feito, descemos pelo ponteiro da esquerda
        }
        insertIntoNonFull(pg->ptr[pos], item, idx_file);
    }
}

// =================== INSERT NO DISCO ==========================

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
                initializePage(pg->ptr[pos], true);
                pg->ptr[pos]->father = pg;
            }
        }

        if (pg->ptr[pos]->numKeys == MAX_KEYS) {
            splitChild(pg, pos, pg->ptr[pos]);
            if (item.key > pg->items[pos].key) pos++;
        }
        
        insertIntoNonFull_disk(pg->ptr[pos], item, idx_file);
    }
}

void insertBtree_disk(page** root_ptr, Item item, const char* idx_file) {
    page* root = *root_ptr;

    if (root->numKeys == MAX_KEYS) {
        page* new_root = (page*)malloc(sizeof(page));
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
    
    // Salva automaticamente após inserção
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

        // Carrega filho do disco se necessário
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
    
    while (!cur->isLeaf) {
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
    return cur->items[cur->numKeys - 1].key;
}

int getSuccessor(page* node, int idx, const char* idx_file) {
    page* cur = node->ptr[idx + 1];
    
    while (!cur->isLeaf) {
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
    return cur->items[0].key;
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
            // Carrega filhos se necessário
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
        
        // Carrega filho se necessário
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
            // Carrega irmãos se necessário
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
    
    // Salva automaticamente após remoção
    if (*root_ptr != NULL) {
        saveBTree(idx_file, *root_ptr);
    }
}


/* Insert principal */
void insertBtree(page** root_ptr, Item item, const char* idx_file) {
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
        insertIntoNonFull(new_root->ptr[pos], item, idx_file);
        
        // Atualiza a referência da árvore
        *root_ptr = new_root;
    
    // Se a raiz NÃO está cheia
    } else {
        // Desce recursivamente pelos filhos até uma folha e insere
        insertIntoNonFull(root, item, idx_file);
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
            // Se o filho ESTA na RAM, pega o offset atualizado dele
            sp.child_offsets[i] = pg->ptr[i]->disk_offset;

            // Atualiza os offsets se existirem
            if (pg->children_offsets != NULL) {
                pg->children_offsets[i] = pg->ptr[i]->disk_offset;
            }
        
        // O filho tá no DISCO (ptr é NULL), mas temos os offsets
        } else if (pg->children_offsets != NULL) {
            sp.child_offsets[i] = pg->children_offsets[i];
            
        
        // É Folha
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
    //Se tá salvando uma nula, retorna -1. Offset não existe
    if (pg == NULL) return -1;

    //Converte a página pra formato de disco
    SerializedPage sp = serializePage(pg);

    // Se ainda não tem offset, aloca um novo
    if (pg->disk_offset == -1) {
        fseek(fp, 0, SEEK_END);         // move pro final
        pg->disk_offset = ftell(fp);    // salva o offset
        
    } else {
        //Se já existe, sobreescreve
        fseek(fp, pg->disk_offset, SEEK_SET); // vai até o offset
    }

    // Pega a página serializada e escreve no disco
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
    pg->father = NULL;

    // alloca os arrays
    pg->items = (Item*)calloc(MAX_KEYS, sizeof(Item));
    pg->ptr = (page**)calloc(M, sizeof(page*));

    // Copia os itens da serializada pra página em memória
    for (int i = 0; i < pg->numKeys; i++) {
        pg->items[i] = sp.items[i];
    }



    // APENAS A PÁGINA ATUAL É CARREGADA PRA MEMÓRIA
    // as outras são carregadas sobre demanda, se necessário
    // Porém, carrega todos os offsets, caso seja necessário carregar as outras

    // Se não é folha, precisamos alocar o vetor de offsets
    if(!pg->isLeaf) { 
        pg->children_offsets = (long*)malloc(sizeof(long) * M); //aloca espaço pra M longs
        
        for (int i = 0; i < M; i++) {
            // pg->ptr[i] = NULL;
            pg->children_offsets[i] = sp.child_offsets[i];
        }
    } else {
        //Se não tiver nenhum dado de offset, não aloca nada. Pois é folha
        pg->children_offsets = NULL;
        
        // Isso aqui já vem como NULL por causa do CALLOC
        // for (int i = 0; i < M; i++) {
        //     pg->ptr[i] = NULL;
        // }
    }

    return pg;
}

/* Percorre a árvore recursivamente e salva todos os nós */
void saveBTree_recursive(FILE* fp, page* node) {
    if (node == NULL) return;
    
    // Salva o nó ANTES dos filhos
    savePage_disk(fp, node);

    // Atualiza os offsets dos filhos no vetor
    if (!node->isLeaf && node->children_offsets != NULL) {
        for (int i = 0; i <= node->numKeys; i++) {
            if (node->ptr[i] != NULL) {
                // Salva o filho recursivamente (ele ganha o offset aqui)
                saveBTree_recursive(fp, node->ptr[i]);

                //Atualiza o offset no array do pai
                node->children_offsets[i] = node->ptr[i]->disk_offset;
            }
        }

        // Re salva o pai pra atualizar os offsets dos filhos
        savePage_disk(fp, node);
    }
}

/* Salva a BTRee num arquivo e atualiza o header de dados */
void saveBTree(const char* idx_file, page* root) {
    //Abre o arquivo
    FILE* fp = openIndexFile(idx_file, "rb+");
    
    if (fp == NULL) {
        printf("Erro ao abrir arquivo de índice!\n");
    }

    //Salva recursivo
    saveBTree_recursive(fp, root);

    //Atualiza os metadados
    BtreeInfo metadata;
    metadata.root_offset = (root != NULL) ? root->disk_offset : -1;
    metadata.ordem = M;
    metadata.num_nodes = 0; //contador ainda não foi implementado
    
    //Salva os metadados no início
    fseek(fp, 0, SEEK_SET);
    fwrite(&metadata, sizeof(BtreeInfo), 1, fp);
    fflush(fp);

    fclose(fp);
    printf("Árvore salva no disco com sucesso!\n");
}

/* Carrega a B Tree do disco para memória (apenas raiz) */
page* loadBTree(const char* idx_file) {
    //Abre o arquivo
    FILE* fp = openIndexFile(idx_file, "rb+");
    
    if (fp == NULL) {
        printf("Erro ao abrir o arquivo!\n");
        return NULL;
    }

    //Lê e carrega os metadados
    BtreeInfo metadata;
    fseek(fp, 0, SEEK_SET); //coloca o ponteiro no início pra garantir
    if (fread(&metadata, sizeof(BtreeInfo), 1, fp) != 1) {
        fclose(fp);
        printf("Erro ao ler metadados.\n");
        return NULL;
    }

    //Confere se a árvore tá vazia
    if (metadata.root_offset == -1) {
        fclose(fp);
        printf("Árvore vazia no disco.\n");
        return NULL;
    }

    //Carrega a raiz (filhos são carregados sob demanda)
    page* root = loadPage_disk(fp, metadata.root_offset);

    //Fecha arquivo, avisa terminal e retorna ponteiro pra root
    fclose(fp);

    if (root) {
        printf("Raiz carregada (offset=%ld, keys=%d)\n", 
               root->disk_offset, root->numKeys);
    }

    return root;
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
    
    if(n->children_offsets != NULL) {
        free(n->children_offsets);
    }

    free(n);
}
