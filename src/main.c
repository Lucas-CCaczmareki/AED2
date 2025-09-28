/*Trie case INSENSITIVE

Usando ela para interpretar

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct node {
    char c;
    bool end;
    int feels;
    struct node* next[26];
} node;

/* OK */
void normalize_string(char* input, char* output) {
    int i = 0; //índice para a string de entrada (input)
    int j = 0; //índice para a string de saída (output)

    while (input[i] != '\0') {
        //Os caracteres acentuados são divididos em 2 bytes
        //0xC3 marca o início deles
        if((unsigned char) input[i] == 0xC3) {
            i++; //pula pro seugndo byte pra identificar o caractere
            switch ((unsigned char)input[i]) {
                
                case 0xA0: // á
                case 0xA1: // à
                case 0xA2: // â
                case 0xA3: // ã
                    output[j] = 'a';
                    break;
                
                case 0xA7: // ç
                    output[j] = 'c';
                    break;

                case 0xA9: // é
                case 0xAA: // ê
                    output[j] = 'e';
                    break;

                case 0xAD: // í
                    output[j] = 'i';
                    break;
                
                case 0xB3: // ó
                case 0xB4: // ô
                case 0xB5: // õ
                    output[j] = 'o';
                    break;

                case 0xBA: // ú
                    output[j] = 'u';
                    break;

                //Caso seja algo que não queremos tratar
                default:
                    //Faz nada, só pula pro próximo
                    j--; //descrementa pra n deixar espaço em branco
                    break;
            }
        } else {
            //Se não for um caracter especial (C3XX)
            //converte pra minúsculo e copia
            output[j] = tolower((unsigned char)input[i]);
        }
        i++;
        j++;
    }
    output[j] = '\0';
}

/* OK */
node* createNode() {
    //Cria a cabeça da Trie
    node* head = (node *)malloc(sizeof(node));

    head->end = false;
    head->feels = 2; //número que não significa nada

    //Faz ele apontar pra 26 ponteiros nulos (começa vazia)
    for(int i = 0; i < 26; i++) {
        head->next[i] = NULL;
    }

    return head;
}

/* OK */
bool search(node** head, char* word) {
    node* p_node = *head;

    //Confere se word[0] - 'a' == NULL (ai temos certeza que a palavra não tá)
    if(p_node->next[(word[0] - 'a')] == NULL) {
        return false;
    }
    
    //Se chegou aqui, temos a possibilidade da palavra estar dentro da trie
    int size = strlen(word);
    

    //A ideia vai ser ir movendo com o cálculo de índice até a última letra, depois conferir a flag
    for(int i = 0; i < size; i++) {
        //Avança o p_node
        p_node = p_node->next[(word[i] - 'a')];
        
        //Confere se a próxima letra da palavra tá na trie;
        if(p_node == NULL) {
            //isso significa que a palavra não está contida
            return false;
        }
    }

    //Se não retornou false no loop, significa que o p_node tá apontando pra última letra da palavra
    if(p_node->end != true) {
        return false;
    } else {
        return true;
    }
    //return p_node->end;
}

/* OK */
int searchFeel(node** head, char* word) {
    node* p_node = *head;

    if(search(head, word)) {
        //Se entrou, a palavra existe na trie
        int size = strlen(word);
        
        //A ideia vai ser ir movendo com o cálculo de índice até a última letra, depois conferir a flag
        for(int i = 0; i < size; i++) {
            //Avança o p_node
            p_node = p_node->next[(word[i] - 'a')];    
        }
        
        //Aqui estamos com p_node no fim da palavra e então retornamos o índice de sentimento
        return p_node->feels;

    } else {
        printf("Essa palavra não existe no arquivo!\n");
        return 2;
    }
}

/* DOING */
void modifyFeel(node** head, char* word, int feel) {
    node* p_node = *head;

    if(search(head, word)) {
        //Se entrou, a palavra existe na trie
        int size = strlen(word);
        
        //A ideia vai ser ir movendo com o cálculo de índice até a última letra, depois conferir a flag
        for(int i = 0; i < size; i++) {
            //Avança o p_node
            p_node = p_node->next[(word[i] - 'a')];    
        }
        
        //Aqui estamos com p_node no fim da palavra e então retornamos o índice de sentimento
        p_node->feels = feel;
        return;

    } else {
        printf("Essa palavra não existe no arquivo!\n");
        return;
    }
}

/* OK */
bool insertWord(node** head, char* word, int feel) {
    int size = strlen(word); 
    int i;
    node* p_node = NULL;
    node* ant_node = NULL;

    //Vai precisar conferir se já existe
    if(search(head, word)) {
        //se retornar true:
        printf("Essa palavra já está na trie!\n");
        return false;
    } else {
        //se retornou false, ou a palavra não tá lá 0 letras, ou tem um pedaço, ou tá toda e não ta marcada
        
        //A ideia é ir avançando até que a trie não tenha as letras da palavra
        //E então, criar os nodos e adicionar essas letras

        //Ou marcar fim de palavra se ela já tiver todas letras

        p_node = *head;
        ant_node = *head;
        for(i = 0; i < size; i++) {
            p_node = p_node->next[word[i] - 'a'];

            //Se a letra da palavra não existe na trie
            if(p_node == NULL) {
                p_node = createNode();
                ant_node->next[word[i] - 'a'] = p_node;
                p_node->c = word[i];

                if(word[i+1] == '\0') {
                    p_node->end = true;
                    p_node->feels = feel;
                }
            //Se a letra existe
            } else {
                //E é fim de palavra
                if(word[i+1] == '\0') {
                    p_node->end = true;
                    p_node->feels = feel;
                }
            }

            //se a letra existe e não é fim de palavra vai pra próxima iteração
            ant_node = p_node;
        }
        //Código antigo que tava dando erro, vou deixar aqui caso precise
        /*
            //Avança o p_node
            p_node = p_node->next[(word[i] - 'a')];
            
            //Confere se a próxima letra da palavra tá na trie;
            
            //cabo a palavra, essa iteração representa a última letra
            if(word[i+1] == '\0') {
                //ultima letra da palavra, essa letra pode ou não estar na trie

                //se n ta
                if(p_node->next[word[i] - 'a'] == NULL) {
                    //vai inseri
                }
                //se ta
                else {
                    //marca
                }



                break;
            } else { //ainda não tá no final
                if(p_node->next[word[i+1] - 'a'] == NULL) {
                    //isso significa que chegamos na última letra que a trie tem dessa palavra.
                    break;
                }
            }


        }
        //Após o break, p_node está na última letra existente da palavra e i no índice da última letra achada
        i++;

        //Tem que testar se a palavra chega na última letra sem o break
        //Se chegou no fim da palavra
        if(i == (size-1)) {
            p_node->end = true;
            return true;
        } 
        
        //Se não tá no fim da palavra
        while(i < size) {
            p_node = createNode();                      //Cria um nodo novo
            //p_node = p_node->next[(word[i] - 'a')];   //Move o ponteiro pra esse novo nodo
            p_node->c = word[i];                        //Coloca o caracter lá
            i++;
        }        
    }
    //Quando sair desse while, ta inserida
    return true;
        
    */
   
    }
    return true;
}

/* OK */
bool removeWord(node** head, char* word) {
    int size = strlen(word);
    node* p_node = *head;

    if(search(head, word)) {
        //Move o p_node até a última letra da palavra
        for(int i = 0; i < size; i++) {
            p_node = p_node->next[(word[i] - 'a')];
        }
        p_node->end = false; //retira a flag de palavra
        return true;
    } else {
        printf("Essa palavra não está na trie!\n");
        return false;
    }
}

void menu(node** head) {
    int opt = 0;
    char word[50];
    char normalized_word[50];

    while (true) {
        printf("-------- MENU --------\n");
        printf("1. Busca de polaridade\n");
        printf("2. Editar polaridade\n");
        printf("3. Salvar arquivo\n");
        printf("4. Sair\n");
        printf("----------------------\n");
        printf("Digite sua opção: ");
        scanf("%d", &opt);

        int feel = 2;

        switch (opt)
        {
        case 1: // Busca a polaridade
            printf("Digite a palavra que você deseja saber a polaridade: ");
            scanf("%s", word);
            normalize_string(word, normalized_word);
            
            feel = searchFeel(head, normalized_word);

            if(feel == 0) {
                printf("O sentimento de '%s' é neutro!\n\n", normalized_word);
            } else if (feel == -1) {
                printf("O sentimento de '%s' é negativo!\n\n", normalized_word);
            } else if (feel == 1) {
                printf("O sentimento de '%s' é positivo!\n\n", normalized_word);
            }

            break;

        case 2: //Editar polaridade
            printf("Digite a palavra para editar polaridade: ");
            scanf("%s", word);
            normalize_string(word, normalized_word);
            
            printf("Digite a polaridade: ");
            scanf("%d", &feel);

            modifyFeel(head, normalized_word, feel);
            printf("\n");

            break;
        
        case 3:
            printf("Not implemented");
            break;

        case 4:
            exit(1);
            break;

        default:
            printf("Opcao invalida!");
            break;
        }
    }
    return;
}

int main () {
    node* head = createNode();
    char line[100];
    char word[50];
    char normalized_word[50];
    int feel;

    //int opt;
    //menu(&head, word, opt);

    FILE* f = fopen("OpLexicon.txt", "r");

    if(f == NULL) {
        printf("Erro ao abrir o arquivo\n");
        exit(1);
    }

    //Vou precisar pegar 1 linha do programa e dividir 2 informações: Palavra e Sentimento
    while(fgets(line, 100, f) != NULL) {
        //aqui vai a lógica pra dividir
        //int size = strlen(line);
        //bool divide = false;

        //Regras sscanf
        //%49 (aceita no máx 49 chars, deixando espaço pro \0)
        //[^,] (lê no max 49 até achar uma ,)
        //%*[^,] (lê tudo e consome (*) at[e achar outra ,])
        //%d lê um inteiro
        if(sscanf(line, "%49[^,],%*[^,],%d", word, &feel) == 2) {
            //armazena a primeira leitura no dest1
            //armazena a segunda no dest2
            //compara pra ver se retornou a leitura de 2 itens (o desejado)

            //normaliza a string tirando os acentos
            normalize_string(word, normalized_word);

            //Insere a string normalizada na Trie
            insertWord(&head, normalized_word, feel);
        }

        /* Leitura de string antiga (ineficiente)
        //Pega as informações da linha
        for(int i = 0; i < size; i++) {
            //Lê até achar ',' e guarda em palavra
            if(line[i] != ',' && divide == false) {
                word[i] = line[i];
            } else {
                word[i] = '\0';
                divide = true;
            }

            //Se chegou no segundo ',' (a flag indica isso) salva o número do feeling
            if(line[i] == ',' && divide == true) {
                feel = line[i+1];
            }
        }
        */
    }
    fclose(f);

    printf("Trie carregada com sucesso!\n\n");

    menu(&head);

}