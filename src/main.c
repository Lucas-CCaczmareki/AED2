#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

//B TREE ORDEM 5

typedef struct {
    int matricula;
    // char nome[50];
} value;

typedef struct {
    int usedKeys;
    value* values;          //vetor de ORDEM-1 valores
    struct page** ptr;      //vetor dinâmico de X ponteiros (X = Ordem da BTREE)
    struct page* father;    //ponteiro pra armazenar o pai (pra fazer o insert é necessário)
} page;

/* OK */
void initializePage(page* pg, int ordem) {
    pg->usedKeys = 0;
    pg->values = (value*)calloc(ordem - 1, sizeof(value));
    pg->ptr = (struct page**)calloc(ordem, sizeof(value));

    pg->father = NULL; //n tenho certeza se inicializa nulo
    return;
}

/* OK */
void freePage(page* pg) {
    free(pg->values);
    free(pg->ptr);

    pg->values = NULL;
    pg->ptr = NULL;
    pg->father = NULL;
}

/* DOING */
void insertKey(page* pg, int ordem, int key) {
    //vê se ta vazio -> vê se é igual -> vê se é menor
    if(pg->usedKeys == 0) {
        pg->values[0].matricula = key;
        return;
    }
    
    //A partir daqui se executar é por que tem espaço na página atual
    int posicao = 0;

    //Se não tá vazio, move o posição pro ponteiro que vamos colocar o dado
    for(int i = 0; i < pg->usedKeys; i++) {
        if(key == pg->values[i].matricula) {            //é igual?
            //se for igual não é pra inserir
            printf("Matricula já inserida!\n");
            return;
        } else if (key < pg->values[i].matricula) {     //é menor?
            break;
        } else {                                        //é maior
            posicao++;
        }
    }

    //Ai executa a lógica de jogar a mediana pra cima
    if(pg->usedKeys == (ordem - 1)) {           //Página cheia
        int mid = (int)ceil(ordem / 2) - 1;     //como ceil retorna um double, faz casting (-1 pq começa em 0)

        page* sister = (page*)malloc(sizeof(page));
        
        //Aqui que mora o primeiro problema
        // O número a ser inserido (na posição) ta abaixo, acima ou é o valor do meio?
        
        //LÓGICA TÁ DESCRITA LÁ NO ONENOTE

        if(posicao == mid) {

        } else if (posicao < mid) {

        } else { //posição > mid

        }

        mid++; //reaproveita o mid como ponteiro pra copiar os itens pra nova página
        for(int i = 0; mid <= (ordem - 1); i++) {
            sister->values[i].matricula = pg->values[mid].matricula;
            mid++;
        }

    }

    //Se a posição que ele precisa ser inserido tá livre, insere
    if(pg->values[posicao].matricula == 0) {
        pg->values[posicao].matricula = key;

    //Se a posição não tá livre (e tem espaço) empurra tudo pro lado
    } else {
        int current = -1;
        int copy = pg->values[posicao].matricula;

        for(int i = posicao + 1; current != 0; i++) {
            //Copy inicia com o primeiro valor que vai pro lado
            //O loop inicia na primeira posição de cópia
            
            current = pg->values[i].matricula;  //Salva o número que vai ser removido
            pg->values[i].matricula = copy;     //Copia o número pro próximo espaço (move 1 pro lado)

            copy = current;                     //Atualiza o copy pra copiar o que acabou de ser removido 1 pro lado
        }
    }
}

/* OK */
void createBTREE(page* root, int ordem) {
    initializePage(root, ordem);
    // return &root;
}

int main () {
    page root;
    createBTREE(&root, 5);
    
    root.values[0].matricula = 1;
    printf("%d\n", root.values[0].matricula);
    // printf("Ponteiro 0: %p\n", (void*)root.ptr[0]); // Imprimirá (nil) ou 0x0

    freePage(&root);
}
