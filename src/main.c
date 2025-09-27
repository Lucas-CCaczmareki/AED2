/*Trie case INSENSITIVE

Usando ela para interpretar

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct node {
    char c;
    bool end;
    int feels;
    struct node* next[26];
} node;

/* OK */
node* createNode() {
    //Cria a cabeça da Trie
    node* head = (node *)malloc(sizeof(node));

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
bool insertWord(node** head, char* word) {
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
                }
            //Se a letra existe
            } else {
                //E é fim de palavra
                if(word[i+1] == '\0') {
                    p_node->end = true;
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

int main () {
    node* head = createNode();
    char word[50];

    int opt;

    while (true) {
        printf("-------- MENU --------\n");
        printf("1. Inserir palavra\n");
        printf("2. Buscar palavra\n");
        printf("3. Remover palavra\n");
        printf("4. Sair\n");
        printf("----------------------\n");
        printf("Digite sua opção: ");
        scanf("%d", &opt);

        switch (opt)
        {
        case 1: // Inserir palavra
            printf("Digite uma palavra para inserir na Trie: ");
            scanf("%s", word);
            strlwr(word);

            if(insertWord(&head, word)) {
                printf("Palavra inserida com sucesso!\n");
            }
            break;

        case 2:
            printf("Digite a palavra para buscar na Trie: ");
            scanf("%s", word);
            strlwr(word);

            if(search(&head, word)) {
                printf("Palavra encontrada: '%s'\n", word);
            } else {
                printf("Palavra não encontrada!\n");
            }

            break;
        
        case 3:
            printf("Digite a palavra para remover na Trie: ");
            scanf("%s", word);
            strlwr(word);

            if(removeWord(&head, word)) {
                printf("Palavra '%s' removida com sucesso!\n", word);
            }

            break;

        case 4:
            exit(1);
            break;

        default:
            printf("Opcao invalida!");
            break;
        }
    }
    

    

}