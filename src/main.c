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
    celula* table;
    long long HASH_SIZE;
} HashTable;

//Declaração das funções
void initHash(HashTable* hash);
long long h1(char* key, long long HASH_SIZE);
long long h2(char* key, long long HASH_SIZE);
long long doubleHash(HashTable* hash, char* key, long long HASH_SIZE);
long long doubleHashSearch(HashTable* hash, char* key, long long HASH_SIZE);
HashTable* insertHash(HashTable* hash, item* item);
HashTable* reHash(HashTable* hash);
item* searchHash(HashTable* hash, char* key);

int main () {
    // celula* hash = (celula *)malloc(sizeof(celula) * HASH_SIZE);
    // initHash(hash);
    
    HashTable* hash = (HashTable*)malloc(sizeof(HashTable));
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
            hash = insertHash(hash, iP); //passa o endereço do item
        }
    }
    printf("Hash criada com sucesso!\n");
    
    
    item* I = searchHash(hash, "cleber");
    printf("Hash[Cleber] = %s,%s", I->key, I->data);

    // item* iP = (item*)malloc(sizeof(item)); //item pointer

    // strcpy(iP->data, "abolicionista\0");
    // strcpy(iP->key, "adj,0\n\0");

    // long long k = h1(iP->data);
    // printf("%lld", k);
}

/* OK */
void initHash(HashTable* hash) {
    hash->HASH_SIZE = 20007;
    hash->table = (celula*)malloc(sizeof(celula)*hash->HASH_SIZE);

    for(int i = 0; i < hash->HASH_SIZE; i++) {
        hash->table[i].i = NULL;
        hash->table[i].neverOccup = true;
        hash->table[i].Occup = false;
        hash->table[i].removed = false;
    }
    // printf("Hash criada com sucesso!\n");
}

/* OK 
Polynomial Rolling Hash */
long long h1(char* key, long long HASH_SIZE) {
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
long long h2(char* key, long long HASH_SIZE) {
    int size = strlen(key);
    long long k = 0;
    for(int i = 0; i < size; i++) {
        k += (unsigned char)key[i];
    }
    return k % HASH_SIZE;
}

/* OK */
long long doubleHash(HashTable* hash, char* key, long long HASH_SIZE) {
    int i = 0;
    long long k = 0;
    while (i < HASH_SIZE) {
        k = (h1(key, HASH_SIZE) + i * h2(key, HASH_SIZE)) % HASH_SIZE;

        if(!hash->table[k].Occup) {
            //Se não tá ocupada
            return k;
        }
        i++;
    }
    return -1; //significa que a hashtable tá cheia, ai com base num if disso, redimensiona
}

long long doubleHashSearch(HashTable* hash, char* key, long long HASH_SIZE) {
    int i = 0;
    long long k = 0;
    while (i < HASH_SIZE) {
        k = (h1(key, HASH_SIZE) + i * h2(key, HASH_SIZE)) % HASH_SIZE;

        // Ta completamente errado isso aqui
        if(hash->table[i].i == NULL) {
            //faz nada
        } else if (hash->table[i].removed == false && hash->table[i].i->key[0] != '\0') {
            if(strcmp(hash->table[i].i->key, key) == 0) {
                return k;
            }
        }
        i++;
    }
    return -1;
}

/* OK */
HashTable* insertHash(HashTable* hash, item* item) {
    static long long colisao = 0;
    long long indice = h1(item->key, hash->HASH_SIZE);

    // printf("%lld\n", indice);
    // printf("%s,%s", item->key, item->data);

    // Significa que não tem nenhum item aqui e ele ta virgem
    if(hash->table[indice].i == NULL) {
        hash->table[indice].i = item;
        hash->table[indice].neverOccup = false;
        hash->table[indice].Occup = true;

    // Um item pode existir mas estar removido, e sua posição livre
    } else {
        if(hash->table[indice].Occup == false) {
            hash->table[indice].i = item;
            hash->table[indice].Occup = true;
            hash->table[indice].removed = false;
        
        // A celula tá ocupada
        } else {
            indice = h2(item->key, hash->HASH_SIZE);

            //printf("%lld\n", indice);

            //Tenta inserir de novo
            if(hash->table[indice].i == NULL) {
                hash->table[indice].i = item;
                hash->table[indice].neverOccup = false;
                hash->table[indice].Occup = true;
            } else {
                if(hash->table[indice].Occup == false) {
                    hash->table[indice].i = item;
                    hash->table[indice].Occup = true;
                    hash->table[indice].removed = false;
                
                // A celula tá ocupada
                } else {
                    indice = doubleHash(hash, item->key, hash->HASH_SIZE);
                    while(indice == -1) {
                        printf("Colisao: %lld\n", ++colisao);
                        hash = reHash(hash);
                        indice = doubleHash(hash, item->key, hash->HASH_SIZE);
                    }
                    hash->table[indice].i = item;
                    hash->table[indice].neverOccup = false;
                    hash->table[indice].Occup = true;
                    hash->table[indice].removed = false;
                }       
            }
        }
    }
    return hash;
}

/* OK */
HashTable* reHash(HashTable* hash) {
    HashTable* tempHash = (HashTable*)malloc(sizeof(HashTable));
    initHash(tempHash);

    tempHash->HASH_SIZE = hash->HASH_SIZE * 2;
    tempHash->table = (celula*)malloc(sizeof(celula)*tempHash->HASH_SIZE);
    

    if(tempHash->table == NULL) {
        printf("Redimensionamento da hash falhou!\n");
        return hash; //retorna a hash table original
    }

    //Aqui eu vou precisar percorrer a hashtable original e mudar TODOS os itens de lugar
    for(int i = 0; i < hash->HASH_SIZE; i++) {
        //Se existe um item
        if(hash->table[i].i != NULL) {
            insertHash(tempHash, hash->table[i].i);
        }
    } 
    //Aqui a tempHash tem a nossa hash original certinha
    printf("Hash redimensionada com sucesso!\n");
    free(hash->table);
    free(hash);

    return tempHash;
}

/* DOING */
item* searchHash(HashTable* hash, char* key) {
    long long indice = h1(key, hash->HASH_SIZE);
    if(strcmp(hash->table[indice].i->key, key) == 0) {
        return hash->table[indice].i;
    } else {
        indice = h2(key, hash->HASH_SIZE);
        if(strcmp(hash->table[indice].i->key, key) == 0) {
            return hash->table[indice].i;
        } else {
            indice = doubleHashSearch(hash, key, hash->HASH_SIZE);
            if(indice == -1) {
                return NULL;
            } else {
                return hash->table[indice].i;
            }
        }
    }
}