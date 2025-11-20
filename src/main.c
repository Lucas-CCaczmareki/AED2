#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define M 5

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

*/

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

/* 
 * Abre um dos arquivos de registro, se não existir, cria um
 */
FILE* openArc(const char* arcName, const char* mode) {
    FILE* fp = fopen(arcName, mode);
    if(fp == NULL && strcmp(mode, "rb+") == 0) { //strcmp retorna 0 se as strings são iguais
        //Se não existe, cria um arquivo
        fp = fopen(arcName, "wb+");
    }
    return fp;
}

/*
 * Insere um registro(genérico) no arquivo e retorna o offset (posição) onde
 * foi escrito. Este offset será usado como "ponteiro" na B-tree.
 * 
*/
long insertRecord(const char* arcName, void* rec, size_t recSize) {
    FILE* fp = openArc(arcName, "rb+");
    
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
bool searchRecord(const char* arcName, long offset, void* rec, size_t recSize) {
    //Abrir o arquivo
    FILE* fp = openArc(arcName, "rb");
    
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
bool updateRecord(const char* arcName, long offset, void* rec, size_t recSize) {
    FILE* fp = openArc(arcName, "rb+"); //abre na leitura e escrita
    
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
bool deleteRecord(const char* arcName, long offset, size_t recSize){
    FILE* fp = openArc(arcName, "rb+"); //abre no modo leitura e escrita
    
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
    FILE* fp = openArc("alunos.dat", "rb");
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

//FUnções de teste
int main () {
    printf("=== TESTE DE GERENCIAMENTO DE ARQUIVOS - ALUNOS ===\n\n");
    
    // Teste 1: Inserir alunos
    printf("1. Inserindo alunos...\n");
    
    Aluno a1 = {true, 12345, "João Silva"};
    Aluno a2 = {true, 67890, "Maria Santos"};
    Aluno a3 = {true, 11111, "Pedro Oliveira"};
    
    long offset1 = insertRecord("alunos.dat", &a1, sizeof(Aluno));
    long offset2 = insertRecord("alunos.dat", &a2, sizeof(Aluno));
    long offset3 = insertRecord("alunos.dat", &a3, sizeof(Aluno));
    
    // Teste 2: Listar todos
    printf("\n2. Listando todos os alunos:\n");
    listarAlunos();
    
    // Teste 3: Buscar por offset
    printf("\n3. Buscando aluno no offset %ld:\n", offset2);
    Aluno buscado;
    if (searchRecord("alunos.dat", offset2, &buscado, sizeof(Aluno))) {
        
        printf("Encontrado: Matrícula %d - %s\n", 
               buscado.matricula, buscado.nome);
    }
    
    // Teste 4: Atualizar
    printf("\n4. Atualizando nome do aluno...\n");
    strcpy(a2.nome, "Maria Santos Sousa");
    updateRecord("alunos.dat", offset2, &a2, sizeof(Aluno));
    listarAlunos();
    
    // Teste 5: Deletar
    printf("\n5. Deletando aluno no offset %ld...\n", offset1);
    deleteRecord("alunos.dat", offset1, sizeof(Aluno));
    listarAlunos();
    
    return 0;
}