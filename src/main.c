/* Trie 9 keys - to text
2 → A B C
3 → D E F
4 → G H I
5 → J K L
6 → M N O
7 → P Q R S
8 → T U V
9 → W X Y Z

Recebe um número 2272 tem que ir na trie e retornar todas palavras.

Pra armazenar na trie: lê a palavra, converte em número
c -> 2
a -> 2
s -> 7
a -> 2

Se for a primeira vai criando os nodos, se já existir percorre até o final
Testa se é fim de palavra, se não é, marca como é e adiciona a string
Se já é fim de palavra, adiciona a string no próximo espaço válido.

Ou seja, a trie vai armazenar os números, e todo nodo que é fim de palavra vai conter um
buffer com todas palavras que aquela sequência de números representa.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct node {
    char c;
    bool end;

    struct node* next[8]; //os números, talvez precise de menos já que o 0 e 1 não são usados

    //faz um buffer (tipo o pBuffer) com todas as palavras e guarda o \n. Ai vai salvando igual o pBuffer
    //o \n já vai servir de divisória e vai ficar uma baita stringzona só com todas palavras e um \0 no final.
    char* words_buffer; //as palavras que podem ou não finalizar aqui
    //int buffer_size;

} node;

/* OK */
void string_to_number(char* input, char* output) {
    int i = 0; //índice para a string de entrada (input)
    int j = 0; //índice para a string de saída (output)

    while (input[i] != '\0') {
        //Os caracteres especiais são divididos em 2 bytes
        //0xC3 marca o início deles

        //Se é caracter especial
            //Precisa adicionar mais casos
        if((unsigned char) input[i] == 0xC3) {
            i++; //pula pro seugndo byte pra identificar o caractere
            switch ((unsigned char)input[i]) {
                
                // Todos 'a's especiais (á à â ã ä Á À Â Ã Ä)
                case 0xA1: case 0xA0: case 0xA2: case 0xA3: case 0xA4:
                case 0x81: case 0x80: case 0x82: case 0x83: case 0x84:
                    output[j++] = '2';
                    break;
                
                // Ç e ç
                case 0xA7: case 0x87: 
                    output[j++] = '2';
                    break;

                // Todos 'e's especiais (é è ê ë É È Ê Ë)
                case 0xA9: case 0xA8: case 0xAA: case 0xAB:
                case 0x89: case 0x88: case 0x8A: case 0x8B:
                    output[j++] = '3';
                    break;

                // Todos 'i's especiais (í ì î ï Í Ì Î Ï)
                case 0xAD: case 0xAC: case 0xAE: case 0xAF:
                case 0x8D: case 0x8C: case 0x8E: case 0x8F:
                    output[j++] = '4';
                    break;
                
                // Todos 'o's especiais (ó ò ô õ ö Ó Ò Ô Õ Ö)
                case 0xB3: case 0xB2: case 0xB4: case 0xB5: case 0xB6:
                case 0x93: case 0x92: case 0x94: case 0x95: case 0x96:
                    output[j++] = '6';
                    break;

                //Todos u's especiais (ú ù û ü Ú Ù Û Ü)
                case 0xBA: case 0xB9: case 0xBB: case 0xBC:
                case 0x9A: case 0x99: case 0x9B: case 0x9C:
                    output[j++] = '8';
                    break;

                //Caso seja algo que não queremos tratar
                default:
                    //Faz nada, só pula pro próximo
                    break;
            }
        //Se não é caracter especial (C3XX)
        } else {
            /* NESSE ELSE AQUI METE-LHE AS LETRA NORMAL E TRNASFORMA EM NÚMERO TAMBÉM.*/
            
            //transforma pra minúsculo
            unsigned char ch = tolower((unsigned char)input[i]);

            switch (ch)
            {
            case 'a': case 'b': case 'c':
                output[j++] = '2';
                break;
            
            case 'd': case 'e': case 'f':
                output[j++] = '3';
                break;

            case 'g': case 'h': case 'i':
                output[j++] = '4';
                break;

            case 'j': case 'k': case 'l':
                output[j++] = '5';
                break;
                
            case 'm': case 'n': case 'o':
                output[j++] = '6';
                break;

            case 'p': case 'q': case 'r': case 's':
                output[j++] = '7';
                break;

            case 't': case 'u': case 'v':
                output[j++] = '8';
                break;

            case 'w': case 'x': case 'y': case 'z':
                output[j++] = '9';
                break;

            default:
                //Só copia as letra (ignora caracter tipo hifen ou barra)
                break;
            }
        }
        i++;
    }
    output[j] = '\0';
}

/* OK */
node* createNode() {
    //Cria a cabeça da Trie
    node* head = (node *)malloc(sizeof(node));

    head->end = false;
    head->words_buffer = (char*)malloc(sizeof(char));
    head->words_buffer[0] = '\0';

    //Inicializa todos filhos como null (0-9)
    for(int i = 0; i < 8; i++) {
        head->next[i] = NULL;
        
    }

    return head;
}

/* OK (SEARCHWORDS) */
bool search(node** head, char* word, char* nineKey) {
    node* p_node = *head;
    int size = strlen(nineKey);
    int word_size = strlen(word);

    //Vai precisar modificar.
    for(int i = 0; i < size; i++) {
        p_node = p_node->next[(int)nineKey[i] - '0' - 2];
        if(p_node == NULL) {
            //significa que não ta inserida, nem o ninekey
            return false;
        } else {
            //se tem o número continua a empreitada
            if(nineKey[i+1] == '\0') { //se chegou no fim da palavra
                char* str_finder = p_node->words_buffer;
                
                //strstr retorna a primeira ocorrência da subpalavra word no buffer.
                //Como procuramos por exatamente "caça\n", mesmo que haja uma palavra do tipo "caçada\n" o strstr ainda acerta
                //por causa do \n.

                //SEG FAULT AQUI
                if(str_finder == NULL) { //se o buffer começa vazio
                    return false; //a palavra n ta lá
                } else {
                    if ((str_finder = strstr(str_finder, word)) != NULL) {
                        return true;
                    } else {
                        return false; 
                    }
                }
            }
        }
    }
}

/* OK? */
bool insertWord(node** head, char* word, char* nineKey) {
    int size = strlen(nineKey); 
    int i;
    node* p_node = NULL;
    node* ant_node = NULL;

    p_node = *head;
    ant_node = *head;

    //Trata o caso da palavra já ta lá (ex: 2272(e casa\n (confere o \n pois pode ter palavras com essa sub-palavra) no buffer já))
    if(search(head, word, nineKey)) {
        printf("Essa palavra já está contida!\n");
        return false;
    } else {
        //Se não tiver nada lá.
        for(int i = 0; i < size; i++) {
            ant_node = p_node;
            p_node = p_node->next[(int)nineKey[i] - '0' - 2];

            //Se já tem o número que a gente leu.
            if (p_node != NULL) {
                if(nineKey[i+1] == '\0') { // se é fim de palavra
                    p_node->end = true;
                    
                    //Aloca espaço e copia a word pro buffer
                    char* aux = (char*)realloc(p_node->words_buffer, (strlen(p_node->words_buffer) + strlen(word) + 1));
                    if(aux == NULL) {
                        printf("A realocação do word buffer falhou!");
                        exit(1);
                    } else {
                        p_node->words_buffer = aux; // troca o ponteiro pro novo endereço de memória
                        
                        //Concatena pra dentro do buffer
                        strcat(p_node->words_buffer, word);

                        //Atualiza o tamanho do buffer
                        //p_node->buffer_size += strlen(word) + 1;
                    }
                }

            //Se não tem o número que foi lido
            } else {
                //Cria um novo nodo e modifica valor
                p_node = createNode();
                p_node->c = nineKey[i];

                //Linka o nodo anterior com o novo nodo criado
                ant_node->next[(int)nineKey[i] - '0' - 2] = p_node;

                if(nineKey[i+1] == '\0') { // se é fim de palavra
                    p_node->end = true;
                    
                    //Aloca espaço e copia a word pro buffer
                    char* aux = (char*)realloc(p_node->words_buffer, (strlen(p_node->words_buffer) + strlen(word) + 1));
                    if(aux == NULL) {
                        printf("A realocação do word buffer falhou!");
                        exit(1);
                    } else {
                        p_node->words_buffer = aux; // troca o ponteiro pro novo endereço de memória
                        
                        //Concatena pra dentro do buffer
                        strcat(p_node->words_buffer, word);

                        //Atualiza o tamanho do buffer
                        //p_node->buffer_size += strlen(word) + 1;
                    }
                }
            }
        }
    }
    return true;
}

void searchNum(node** head, char* nineKey) {
    node* p_node = *head;
    int i = 0;    

    while(nineKey[i] != '\0') {
        p_node = p_node->next[(int)nineKey[i] - '0' - 2];
        if(p_node == NULL) {
            printf("Esse número não tem nenhuma palavra correspondente!\n");
            return;
        }
        i++;
    }
    printf("Palavras correspondentes: \n%s", p_node->words_buffer);
    
}

void searchPrefixNum(node** head, char* nineKey) {
    node* p_node = *head;
    int i = 0;    

    while(nineKey[i] != '\0') {
        p_node = p_node->next[(int)nineKey[i] - '0' - 2];
        if(p_node == NULL) {
            printf("Esse número não tem nenhuma palavra correspondente!\n");
            return;
        }
        i++;
    }
    printf("Palavras correspondentes: \n%s", p_node->words_buffer);
    
}

void menu(node** head) {
    int opt;
    printf("\n-------------------- MENU --------------------\n");
    printf("1. Buscar por prefixo.\n");
    printf("2. Busca estrita.\n");
    printf("2. Sair\n");
    printf("----------------------------------------------\n");

    printf("Digite a sua opção: ");
    scanf("%d", &opt);

    switch (opt)
    {
    case 1:
        char nineKey[50];
        printf("Digite o código de dígitos: ");
        getchar(); //limpa o buffer
        scanf("%[^\n]s", nineKey);

        break;
    
    case 2:
        char nineKey[50];
        printf("Digite o código de dígitos: ");
        getchar(); //limpa o buffer
        scanf("%[^\n]s", nineKey);
        searchNum(head, nineKey);
        break;

    case 3:
        exit(1);
        break;

    default:
        break;
    }
}

int main () {
    node* head = createNode();
    char word[100];
    char nineKey[50];
    
    
    FILE* f = fopen("palavras.txt", "r");

    if(f == NULL) {
        printf("Erro ao abrir o arquivo\n");
        exit(1);
    }

    //Vou precisar pegar 1 linha do programa e dividir 2 informações: Palavra e Sentimento
    while(fgets(word, 100, f) != NULL) {
        //Normaliza a palavra
        string_to_number(word, nineKey);

        //Insere a string na trie de símbolos
        insertWord(&head, word, nineKey);
    }

    fclose(f);
    

    //menu(&head);
    // printf("Trie carregada, em tese (nao olhei dentro ainda)\n");

    // if(search(&head, "casa\n", "2272")) {
    //     printf("A trie ta carregada mesmo\n");
    // } else {
    //     printf("Fodeu total\n");
    // }

    while(true) {
        menu(&head);
    }

}