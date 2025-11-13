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
    pg->ptr = (struct page**)calloc(ordem, sizeof(struct page*));

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
page* insertKey(page* pg, int ordem, int key) {
    //vê se ta vazio -> vê se é igual -> vê se é menor
    if(pg->usedKeys == 0) {
        pg->values[0].matricula = key;
        pg->usedKeys++;
        return pg;
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

    //SE TA CHEIA
    //Ai executa a lógica de jogar a mediana pra cima
    if(pg->usedKeys == (ordem - 1)) {           //Página cheia
        int mid = (int)ceil(ordem / 2) - 1;     //como ceil retorna um double, faz casting (-1 pq começa em 0)

        page* sister = (page*)malloc(sizeof(page));
        initializePage(sister, ordem);
        
        //Aqui que mora o primeiro problema
        // O número a ser inserido (na posição) ta abaixo, acima ou é o valor do meio?
        
        //LÓGICA TÁ DESCRITA LÁ NO ONENOTE

        if(posicao == mid) {
            //Como aqui, é a key que eu to inserindo que vai subir, todos a partir da posição até o final
            //vão pra dentro da sister.

            page* father = insertKey(pg->father, ordem, key);
            int father_pos;
            //Agora a GRANDE QUESTÃO É como que eu ligo o ponteiro esquerdo e o direito?
            //E o pior, tem que ser genérico, por que pode ter propagado recursivo infinitamente pra cima
            
            //PSEUDO SEARCH
            for(int i = 0; i < father->usedKeys; i++) {
                if(father->values[i].matricula == key) {
                    father_pos = i;
                }
            }

            //Insere todas pra dentro da key
            for(int i = posicao; i < (ordem - 1); i++) {
                insertKey(sister, ordem, pg->values[i].matricula); //usedKeys já é atualizado automaticamente com o insert
                
                //Limpa a posição e atualiza o used keys da página original
                pg->values[i].matricula = 0;
                pg->usedKeys--;
            }

            //Atualiza os ponteiros
            father->ptr[father_pos] = pg;
            father->ptr[(father_pos + 1)] = sister;

        } else if (posicao < mid) {
            //Insere no pai
            page* father = insertKey(pg->father, ordem, pg->values[mid].matricula);

            pg->values[mid].matricula = 0;
            pg->usedKeys--;

            //Procura onde o pai foi inserido
            int father_pos;

            //PSEUDO SEARCH
            for(int i = 0; i < father->usedKeys; i++) {
                if(father->values[i].matricula == pg->values[mid].matricula) {
                    father_pos = i;
                }
            }

            //Insere todas pra dentro da key
            for(int i = posicao; i < (ordem - 1); i++) {
                insertKey(sister, ordem, pg->values[i].matricula); //usedKeys já é atualizado automaticamente com o insert
                
                //Limpa a posição e atualiza o used keys da página original
                pg->values[i].matricula = 0;
                pg->usedKeys--;
            }

            //Insere a key dentro da página original (da esquerda)
            insertKey(pg, ordem, key);

            //Atualiza os ponteiros
            father->ptr[father_pos] = pg;
            father->ptr[(father_pos + 1)] = sister;
            
        } else { //posição > mid
            
            //Insere todas pra dentro da key
            for(int i = posicao; i < (ordem - 1); i++) {
                insertKey(sister, ordem, pg->values[i].matricula); //usedKeys já é atualizado automaticamente com o insert
                
                //Limpa a posição e atualiza o used keys da página original
                pg->values[i].matricula = 0;
                pg->usedKeys--;
            }

            insertKey(sister, ordem, key);

            //Insere no pai
            page* father = insertKey(pg->father, ordem, pg->values[mid].matricula);
            
            //Procura onde o pai foi inserido
            int father_pos;

            //PSEUDO SEARCH
            for(int i = 0; i < father->usedKeys; i++) {
                if(father->values[i].matricula == pg->values[mid].matricula) {
                    father_pos = i;
                }
            }


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
        pg->usedKeys++;
        return pg;

    //Se a posição não tá livre (e tem espaço) empurra tudo pro lado
    } else {
        int current = -1;
        int copy = pg->values[posicao].matricula;

        page* current_ptr = NULL;
        page* copy_ptr = pg->ptr[posicao];

        for(int i = posicao + 1; current != 0; i++) {
            //Copy inicia com o primeiro valor que vai pro lado
            //O loop inicia na primeira posição de cópia
    
            //não precisa testar se terminou a page por que aqui a gente tem certeza que tem um espaço vazio

            current = pg->values[i].matricula;  //Salva o número que vai ser removido
            pg->values[i].matricula = copy;     //Copia o número pro próximo espaço (move 1 pro lado)

            copy = current;                     //Atualiza o copy pra copiar o que acabou de ser removido 1 pro lado

            //Lógica igual das páginas pra atualizar os ponteiros
            current_ptr = pg->ptr[i];
            pg->ptr[i] = copy_ptr;

            copy_ptr = current_ptr;

        }
        pg->usedKeys++;
        return pg;
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
