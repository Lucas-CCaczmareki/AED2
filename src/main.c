#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

// #define HASH_SIZE 20007

typedef struct {
    char key[50];
    char data[50];
} item;

typedef struct {
    item* i;
    bool neverOccup;
    bool Occup;
    bool removed;
} celula;

typedef struct {
    celula* hash;
    long long HASH_SIZE;
} HashTable;

/* OK */
void initHash(celula* hash) {
    for(int i = 0; i < HASH_SIZE; i++) {
        hash[i].i = NULL;
        hash[i].neverOccup = true;
        hash[i].Occup = false;
        hash[i].removed = false;
    }
    // printf("Hash criada com sucesso!\n");
}

/* OK 
Polynomial Rolling Hash */
long long h1(char* key) {
    int size = strlen(key);

    long long p = 31; //base da potência
    long long pot_p = 1; //guarda a potencia (31^0, 31^1, ...)

    long long k = 0; //acumulador (indice final)
    
    for(int i = 0; i < size; i++) {
        //Calcula o termo atual do somatório
        long long termo_atual = (key[i] * pot_p);

        //Soma ele no acumulador
        k = (k + termo_atual) % HASH_SIZE;

        //A potência se atualiza iterativamente
        //1. pot = 1 * 31 = 31^1
        //2. pot = 31 * 31 = 31^2
        //3. pot = 31^2 * 31 = 31^3 ...
        pot_p = (pot_p * p) % HASH_SIZE; 
    }

    //Se k for positivo (não estourou o long long) retorna o valor normal
    //Se k for negativo, converte o resultado pro módulo matemático correto
    return (k + HASH_SIZE) % HASH_SIZE;
}

/* OK
Hash secundária, somatório simples. (determina deslocamento se houver colisão)*/
long long h2(char* key) {
    int size = strlen(key);
    long long k = 0;
    for(int i = 0; i < size; i++) {
        k += (key[i]);
    }
    return k % HASH_SIZE;
}

/* OK */
long long doubleHash(celula* hash, char* key) {
    int i = 0;
    long long k = 0;
    while (i < HASH_SIZE) {
        k = (h1(key) + i * h2(key)) % HASH_SIZE;

        if(!hash[k].Occup) {
            //Se não tá ocupada
            return k;
        }
        i++;
    }
    return -1; //significa que a hashtable tá cheia, ai com base num if disso, redimensiona
}

/* DOING */
celula* reHash(celula* hash) {
    static int i = 2;
    celula* tempHash = realloc(hash, sizeof(celula)*(HASH_SIZE*i++));
    
    if(tempHash == NULL) {
        printf("Redimensionamento da hash falhou!\n");
        return hash;
    }

    return tempHash;
}


/* OK */
void insertHash(celula* hash, item* item) {
    static long long colisao = 0;
    long long indice = h1(item->key);

    // printf("%lld\n", indice);
    // printf("%s,%s", item->key, item->data);

    // Significa que não tem nenhum item aqui e ele ta virgem
    if(hash[indice].i == NULL) {
        hash[indice].i = item;
        hash[indice].neverOccup = false;
        hash[indice].Occup = true;

    // Um item pode existir mas estar removido, e sua posição livre
    } else {
        if(hash[indice].Occup == false) {
            hash[indice].i = item;
            hash[indice].Occup = true;
            hash[indice].removed = false;
        
        // A celula tá ocupada
        } else {
            indice = h2(item->key);
            //Tenta inserir de novo
            if(hash[indice].i == NULL) {
                hash[indice].i = item;
                hash[indice].neverOccup = false;
                hash[indice].Occup = true;
            } else {
                if(hash[indice].Occup == false) {
                    hash[indice].i = item;
                    hash[indice].Occup = true;
                    hash[indice].removed = false;
                
                // A celula tá ocupada
                } else {
                    indice = doubleHash(hash, item->key);
                    while(indice == -1) {
                        printf("Colisao: %lld\n", ++colisao);
                        hash = reHash(hash);
                        indice = doubleHash(hash, item->key);
                    }
                    hash[indice].i = item;
                    hash[indice].neverOccup = false;
                    hash[indice].Occup = true;
                    hash[indice].removed = false;
                }       
            }
        }
    }
}

int main () {
    celula* hash = (celula *)malloc(sizeof(celula) * HASH_SIZE);
    initHash(hash);
    
    char buffer[100];

    FILE* f = fopen("OpLexicon.txt", "r");

    if (f == NULL) {
        printf("Erro ao abrir o arquivo!");
        exit(1);
    }

    while(fgets(buffer, 100, f) != NULL) {
        item* iP = (item*)malloc(sizeof(item)); //item pointer
        if(sscanf(buffer, "%49[^,],%49s", iP->key, iP->data) == 2) {
            insertHash(hash, iP); //passa o endereço do item
        }
    }
    printf("Hash criada com sucesso!\n");
    
    
    
    printf("Hash[4007] = %s,%s", hash[4007].i->data, hash[4007].i->key);

    // item* iP = (item*)malloc(sizeof(item)); //item pointer

    // strcpy(iP->data, "abolicionista\0");
    // strcpy(iP->key, "adj,0\n\0");

    // long long k = h1(iP->data);
    // printf("%lld", k);
}