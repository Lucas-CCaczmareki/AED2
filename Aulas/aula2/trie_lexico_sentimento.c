/* Trie case INSENSITIVE 

Complexidade das funções

Sendo n o número de palavras e m o tamanho da string. As operações vão depender de m: pois percorrem caracter
a caracter.

-------------------------------------------------------------------
Inserção ->
Tempo:
    Normalizar: O(m) + Busca: O(m) + Inserir: O(m) = 3O(m) ~= O(m)
Espaço:
    O(m), vai criar até m nós novos por palavras.
-------------------------------------------------------------------

-------------------------------------------------------------------
Busca ->
Tempo:
    Normalizar + Busca = 2 O(m) = O(m)
Espaço:
    O(1), não aloca nada novo.
-------------------------------------------------------------------

-------------------------------------------------------------------
Edição -> 
Tempo:
    Normaliza + Busca + Edição = 3 O(m) = O(m)
Espaço:
    O(1), não cria nada novo na memória.
-------------------------------------------------------------------

-------------------------------------------------------------------
Remoção ->
Tempo:
    Normaliza + Busca + Remove = 3 O(m) = O(m)
Espaço:
    O(1), não desaloca nada, só muda a flag.
-------------------------------------------------------------------

-------------------------------------------------------------------
Salvamento ->
Tempo:
    O(T), sendo T o número de nós na árvore
Espaço:
    O(h), sendo H o número de níveis da árvore
-------------------------------------------------------------------
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

    char original_word[50]; //guarda a palavra original sem ser normalizada

    struct node* next[26];
} node;

/* OK 
Essa função foi criada com auxílio de IA. 
A IA foi utilizada para aprender como manipular os caracteres com acentos.
*/
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
                    output[j++] = 'a';
                    break;
                
                case 0xA7: // ç
                    output[j++] = 'c';
                    break;

                case 0xA8: // è
                case 0xA9: // é
                case 0xAA: // ê
                case 0xAB: // ë
                    output[j++] = 'e';
                    break;

                case 0xAD: // í
                    output[j++] = 'i';
                    break;
                
                case 0xB3: // ó
                case 0xB4: // ô
                case 0xB5: // õ
                    output[j++] = 'o';
                    break;

                case 0xBA: // ú
                    output[j++] = 'u';
                    break;

                //Caso seja algo que não queremos tratar
                default:
                    //Faz nada, só pula pro próximo
                    //j--; //descrementa pra n deixar espaço em branco
                    break;
            }
        } else {
            //Se não for um caracter especial (C3XX)

            //Só copia as letra (ignora caracter tipo hifen ou barra)
            unsigned char ch = tolower((unsigned char)input[i]);
            if (ch >= 'a' && ch <= 'z') {
                output[j++] = ch;   // só copia se for letra
            }
            // se for espaço, hífen, barra, número, ignora
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

/* OK */
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
bool insertWord(node** head, char* word, int feel, char* original_word) {
    int size = strlen(word); 
    int i;
    node* p_node = NULL;
    node* ant_node = NULL;

    //Vai precisar conferir se já existe
    if(search(head, word)) {
        //se retornar true:
        //Como tem várias palavras repetidas no txt, to comentando essa parte pra evitar estresse
        //printf("Essa palavra já está na trie!\n");
        return false;
    } else {
        //se retornou false, ou a palavra não tá lá 0 letras, ou tem um pedaço, ou tá toda e não ta marcada
        
        //A ideia é ir avançando até que a trie não tenha as letras da palavra
        //E então, criar os nodos e adicionar essas letras

        //Ou marcar fim de palavra se ela já tiver todas letras

        p_node = *head;
        ant_node = *head;
        
        


        for(i = 0; i < size; i++) {
            //printf("Inserindo: %s (char=%c idx=%d)\n", word, word[i], word[i] - 'a');
            p_node = p_node->next[word[i] - 'a'];

            //Se a letra da palavra não existe na trie
            if(p_node == NULL) {
                p_node = createNode();
                ant_node->next[word[i] - 'a'] = p_node;
                p_node->c = word[i];

                if(word[i+1] == '\0') {
                    p_node->end = true;
                    p_node->feels = feel;

                    strcpy(p_node->original_word, original_word);
                }
            //Se a letra existe
            } else {
                //E é fim de palavra
                if(word[i+1] == '\0') {
                    p_node->end = true;
                    p_node->feels = feel;

                    strcpy(p_node->original_word, original_word);

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
void saveTrieRecursive(node* current, char* buffer, int depth, FILE* outFile) {
    //Aqui o depth tá sendo passado por padrão, mas não possui uso prático por enquanto
    //Ele seria útil para reconstruir a string no buffer caso eu não salvasse a palavra original
    //mas pra salvar a palavra original eu teria que fazer com a trie aceitando acentos, e eu optei
    //por fazer normalizando a palavra. De qualquer forma fica a função padronizada.
    
    //Se o nodo tá vazio, n faz nada
    if(current == NULL) {
        return;
    }

    //Se é o fim de uma palavra, salva ela no arquivo
    if (current->end) {
        fprintf(outFile, "%s,adj,%d\n", current->original_word, current->feels);
    }

    /* Faz a chamada recursiva percorrendo todas as letras da Trie em ordem alfabética
    Caso os próximos sejam todos nulos volta 1 nível na recursão e continua pra próxima letra.
    Vai assim até acabar. */
    for (int i = 0; i < 26; i++) {
        if (current->next[i] != NULL) {
            saveTrieRecursive(current->next[i], buffer, depth + 1, outFile);
        }
    }

}

/* OK */
void saveTrieToFile(node** head, char* filename) {
    FILE* outFile = fopen(filename, "w");

    if (outFile == NULL) {
        printf("Não foi possível criar o arquivo de saída");
        return;
    }

    char buffer[50]; //buffer para construir as palavras enquanto percorremos a Trie
    
    //Manda o pai de todos pra salvar toda a Trie
    saveTrieRecursive(*head, buffer, 0, outFile);

    fclose(outFile);
    printf("Arquivo '%s' salvo com sucesso!\n\n", filename);
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
    int c;
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
            
            // Limpa todo o restante da linha (inclusive \n)
            while ((c = getchar()) != '\n' && c != EOF) { }

            fgets(word, sizeof(word), stdin);
            word[strcspn(word, "\n")] = '\0'; // remove o \n

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
            //scanf("%s", word);

            // Limpa todo o restante da linha (inclusive \n)
            while ((c = getchar()) != '\n' && c != EOF) { }

            fgets(word, sizeof(word), stdin);
            word[strcspn(word, "\n")] = '\0'; // remove o \n

            normalize_string(word, normalized_word);
            
            printf("'-1' - Negativa\n'0' - Neutra\n'1' - Positiva\nDigite a polaridade: ");
            scanf("%d", &feel);

            modifyFeel(head, normalized_word, feel);
            printf("\n");

            break;
        
        case 3:
            saveTrieToFile(head, "teste.txt");
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
            insertWord(&head, normalized_word, feel, word);
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