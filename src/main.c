#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define M 5

#define MAX_KEYS M-1
#define MIN_KEYS ceil(M/2)-1

#define MAX_CHILDREN M
#define MIN_CHILDREN ceil(M/2)

/*
Implementando um sistema SGA com btree (com permanência em disco)

1. Para conseguir a permanência em disco:
    - Definir a estrutura do registro
    - Criar funções para escrever/ler registros no arquivo
    - Integrar com a b-tree para indexação (utilizando offsets como ponteiros)

*/

typedef struct {
    int matricula;
    char nome[50];
    bool ativo;
} Aluno;


/* 
 * Abre o arquivo de alunos, se não existir, cria um
 */
FILE* abrirArqAlunos(const char* modo) {
    FILE* fp = fopen("alunos.dat", modo);
    if(fp == NULL && strcmp(modo, "rb+") == 0) { //strcmp retorna 0 se as strings são iguais
        //Se não existe, cria um arquivo
        fp = fopen("alunos.dat", "wb+");
    }
    return fp;
}

/*
 * Insere um aluno no arquivo e retorna o offset (posição) onde
 * foi escrito. Este offset será usado como "ponteiro" na B-tree.
 * 
*/
long inserirAlunoArq(Aluno* aluno) {
    FILE* fp = abrirArqAlunos("rb+");
    
    if(fp == NULL) {
        printf("Erro ao abrir arquivo de alunos!\n");
        return -1;
    }

    //Marca esse registro como ativo
    aluno->ativo = true;


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

    // Escreve o registro
    /* Explicando o fwrite
    Parâmetros:
        1. Ponteiro pro elemento(s) que será escrito (pode ser um array)
        2. O tamanho individual de cada elemento que vai ser escrito
        3. A quantidade de elementos que vai ser escrito
        4. Ponteiro que identifica o fluxo de dados (stream) onde o dado vai ser guardado.

    Retorno: retorna o número total de elementos gravados com sucesso
    */
    size_t escritos = fwrite(aluno, sizeof(Aluno), 1, fp);

    fclose(fp); //fecha o arquivo

    if(escritos != 1) {
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
bool buscarAluno(long offset, Aluno* aluno) {
    //Abrir o arquivo
    FILE* fp = abrirArqAlunos("rb");
    
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
    //Lê o registro, atualiza os dados de aluno com o que foi lido
    /* Explicando o fread
    Parâmetros:
        - ponteiro pra onde o fread vai despejar o que foi lido do disco
        - tamanho em bytes do elemento que tu vai ler
        - quantos elementos tu vai ler
        - ponteiro pro fluxo de entrada

    Retorno: número de elmentos lido.
    */
    size_t lidos = fread(aluno, sizeof(Aluno), 1, fp);
    fclose(fp); //fecha o arquivo

    if (lidos != 1) { //se leu a mais ou a menos, deu problema
        printf("Erro ao ler aluno do arquivo!\n");
        return false;
    }

    //Verifica se o registro lido é válido
    if(!aluno->ativo) {
        //se o aluno não estiver ativo
        return false;
    }
    //se o aluno estiver ativo
    return true; //lembra que já jogamos pra dentro do aluno (que vem como parâmetro) os dados lidos!
}

/*
 * Atualiza um aluno no registro (arquivo)
*/
bool atualizarAluno(long offset, Aluno* aluno) {
    FILE* fp = abrirArqAlunos("rb+"); //abre na leitura binária

    //testa se deu bom
    if(fp == NULL) {
        printf("Erro ao abrir o arquivo!\n");
        return false;
    }

    //Posiciona o ponteiro no offset
    if(fseek(fp, offset, SEEK_SET) != 0) {
        printf("Erro ao posicionar no offset!\n");
        fclose(fp);
        return false;
    }

    aluno->ativo = true; //atualiza pra true pra garantir

    size_t escritos = fwrite(aluno, sizeof(Aluno), 1, fp);
    if(escritos != 1) {
        printf("Erro ao atualizar registro do aluno!\n");
        return false;
    }

    return true;
}

/* 
 * Marca um aluno como deletado (sem remover ele fisicamente. Padrão banco de dados)
*/
bool deletarAluno(long offset){
    FILE* fp = abrirArqAlunos("rb+"); //abre no modo leitura e escrita
    
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

    Aluno aluno;
    if(fread(&aluno, sizeof(aluno), 1, fp) != 1) {
        printf("Erro ao ler do registro\n");
        fclose(fp);
        return false;
    }

    aluno.ativo = false;

    //Volta pra posição e sobreescreve
    fseek(fp, offset, SEEK_SET);
    size_t escritos = fwrite(&aluno, sizeof(Aluno), 1, fp);
    fclose(fp);

    if(escritos != 1) {
        printf("Erro ao deletar aluno!\n");
        return false;
    }

    return true;
}


/*
 * Lista todos os alunos ativos naquele registro
*/
void listarAlunos() {
    FILE* fp = abrirArqAlunos("rb");
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

    Aluno a1 = {12345, "João Silva", true};
    Aluno a2 = {67890, "Maria Santos", true};
    Aluno a3 = {11111, "Pedro Oliveira", true};

    long offset1 = inserirAlunoArq(&a1);
    long offset2 = inserirAlunoArq(&a2);
    long offset3 = inserirAlunoArq(&a3);

    // Teste 2: Listar todos
    printf("\n2. Listando todos os alunos:\n");
    listarAlunos();

    // Teste 3: Buscar por offset
    printf("\n3. Buscando aluno no offset %ld:\n", offset2);
    Aluno buscado;
    if (buscarAluno(offset2, &buscado)) {
        printf("Encontrado: Matrícula %d - %s\n", 
            buscado.matricula, buscado.nome);
    }

    // Teste 4: Atualizar
    printf("\n4. Atualizando nome do aluno...\n");
    strcpy(a2.nome, "Maria Santos Sousa");
    atualizarAluno(offset2, &a2);
    listarAlunos();

    // Teste 5: Deletar
    printf("\n5. Deletando aluno no offset %ld...\n", offset1);
    deletarAluno(offset1);
    listarAlunos();

    return 0;
}