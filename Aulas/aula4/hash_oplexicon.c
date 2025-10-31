/* Falta implementar as sugestões

Escolha valores adequados de ( p ) e ( m ) para evitar agrupamentos (clustering);

Monitore a taxa de ocupação da tabela (load factor);

Garanta que o programa não entre em laços infinitos em caso de colisões sucessivas;

Mantenha um controle de memória apropriado.

*/

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
// void initItem(item* item);
long long h1(char* key, long long HASH_SIZE);
long long h2(char* key, long long HASH_SIZE);
long long doubleHash(HashTable* hash, char* key, long long HASH_SIZE);
long long doubleHashSearch(HashTable* hash, char* key, long long HASH_SIZE);
HashTable* insertHash(HashTable* hash, item* item);
HashTable* reHash(HashTable* hash);
item* searchHash(HashTable* hash, char* key);
bool removeItem(HashTable* hash, char* key);

int main () {
    // celula* hash = (celula *)malloc(sizeof(celula) * HASH_SIZE);
    // initHash(hash);
    int opt = 0;
    HashTable* hash = (HashTable*)malloc(sizeof(HashTable));
    initHash(hash);
    char key[50];

    char buffer[100];

    FILE* f = fopen("OpLexicon.txt", "r");

    if (f == NULL) {
        printf("Erro ao abrir o arquivo!");
        exit(1);
    }

    while(fgets(buffer, 100, f) != NULL) {
        item* iP = (item*)malloc(sizeof(item)); //item pointer
        // initItem(iP);
        if(sscanf(buffer, "%49[^,],%49s", iP->key, iP->data) == 2) {
            hash = insertHash(hash, iP); //passa o endereço do item
        }
    }
    printf("Hash criada com sucesso!\n\n");

    while(true) {
        printf("\n------------------- MENU -------------------\n");
        printf("1. Inserir nova palavra\n");
        printf("2. Buscar palavra\n");
        printf("3. Remover palavra\n");
        printf("4. Sair\n");
        printf("--------------------------------------------\n");
        printf("Digite sua opção: ");
        scanf("%d", &opt);

        switch (opt)
        {
        case 1:
            /* code */
            item *i = (item*)malloc(sizeof(item));
            int polaridade;

            printf("Digite a palavra: ");
            getchar(); //limpa o buffer
            scanf("%[^\n]s", i->key);
            printf("[-1]Negativo, [0]Neutro, [1]Positivo\nDigite a polaridade: ");
            scanf("%d", &polaridade);

            //Montar a string pra data
            if(polaridade == -1) {
                strcpy(i->data, "adj,-1");
            } else if(polaridade == 0) {
                strcpy(i->data, "adj,0");
            } else if(polaridade == 1) {
                strcpy(i->data, "adj,1");
            }

            hash = insertHash(hash, i);

            break;
        
        case 2:
            item* iP;
            printf("Digite a palavra: ");
            getchar(); //limpa o buffer
            scanf("%[^\n]s", key);
            
            iP = searchHash(hash, key);
            if(iP != NULL) {
                printf("Hash[%s]: %s,%s\n",key , iP->key, iP->data);
            } else {
                printf("Essa palavra não está na Hash\n");
            }
            break;

        case 3:

            printf("Digite a palavra: ");
            getchar(); //limpa o buffer
            scanf("%[^\n]s", key);

            if(removeItem(hash, key)) {
                printf("'Hash[%s]' removido com sucesso!\n", key);
            } else {
                printf("Esse item já está removido ou não existe!\n");
            }
            break;

        case 4:
            free(hash->table);
            free(i);
            free(hash);
            exit(1);

        default:
            break;
        }

        // printf("Digite o código de dígitos: ");
        // getchar(); //limpa o buffer
        // scanf("%[^\n]s", nineKey);
    }
    
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

/* OK */
// void initItem(item* item) {
//     item->data[0] = '\0';
//     item->key[0] = '\0';
// }

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

/* OK */
long long doubleHashSearch(HashTable* hash, char* key, long long HASH_SIZE) {
    int i = 0;
    long long k = 0;
    
    while (i < HASH_SIZE) {
        k = (h1(key, HASH_SIZE) + i * h2(key, HASH_SIZE)) % HASH_SIZE;

        // Ta completamente errado isso aqui
        if(hash->table[k].Occup) {
            if(strcmp(hash->table[k].i->key, key) == 0) {
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
    return hash;
}

/* OK */
HashTable* reHash(HashTable* hash) {
    HashTable* tempHash = (HashTable*)malloc(sizeof(HashTable));
    tempHash->HASH_SIZE = hash->HASH_SIZE * 2;

    //Calloc já vai inicializar as células com false e null
    tempHash->table = (celula*)calloc(tempHash->HASH_SIZE, sizeof(celula));
    

    if(tempHash->table == NULL) {
        printf("Redimensionamento da hash falhou!\n");
        free(tempHash);
        return hash; //retorna a hash table original
    }

    //Aqui eu vou precisar percorrer a hashtable original e mudar TODOS os itens de lugar
    for(long long i = 0; i < hash->HASH_SIZE; i++) {
        //Se existe um item
        if(hash->table[i].Occup) {
            insertHash(tempHash, hash->table[i].i);
        }
    } 
    //Aqui a tempHash tem a nossa hash original certinha
    printf("Hash redimensionada com sucesso!\n");
    free(hash->table);
    free(hash);

    return tempHash;
}

/* OK */
item* searchHash(HashTable* hash, char* key) {
    long long indice = h1(key, hash->HASH_SIZE);

    if(hash->table[indice].Occup) {
        // printf("ind: %lld\nhashsize: %lld\nstring: %s", indice, hash->HASH_SIZE, hash->table[indice].i->data);

        if(hash->table[indice].i->key[0] != '\0') {
            if(strcmp(hash->table[indice].i->key, key) == 0) {
                // printf("%s, %s\n", hash->table[indice].i->key, key);
                return hash->table[indice].i;
            } else {
                indice = doubleHashSearch(hash, key, hash->HASH_SIZE);
                
                if(indice == -1) {
                    return NULL;
                } else {
                    // printf("%s, %s\n", hash->table[indice].i->key, key);
                    return hash->table[indice].i;
                }
            }
        }
    } else {
        indice = doubleHashSearch(hash, key, hash->HASH_SIZE);
        
        if (indice == -1) {
            return NULL;
        } else {
            // printf("%s\n", hash->table[indice].i->key);
            return hash->table[indice].i;
        }
    }
    return NULL;
}

/* DOING */
bool removeItem(HashTable* hash, char* key) {
    long long indice = h1(key, hash->HASH_SIZE);

    if(hash->table[indice].Occup) {
        if(strcmp(hash->table[indice].i->key, key) == 0) {
            hash->table[indice].Occup = false;
            hash->table[indice].removed = true;

            return true;
        } else {
            indice = doubleHashSearch(hash, key, hash->HASH_SIZE);
            
            if(indice == -1) {
                return false;
            } else {
                hash->table[indice].Occup = false;
                hash->table[indice].removed = true;
                
                return true;
            }
        }
    } else {
        indice = doubleHashSearch(hash, key, hash->HASH_SIZE);
        
        if(indice == -1) {
            return false;
        } else {
            hash->table[indice].Occup = false;
            hash->table[indice].removed = true;
            
            return true;
        }
    }
    return false;
}