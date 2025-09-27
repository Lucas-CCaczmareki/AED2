//tentativa trie

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct node {
    char c;
    bool end;
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
bool search(node* head, char* word) {
    
    //Confere se word[0] - 'a' == NULL (ai temos certeza que a palavra não tá)
    if(head->next[(word[0] - 'a')] == NULL) {
        return false;
    }
    
    //Se chegou aqui, temos a possibilidade da palavra estar dentro da trie
    int size = strlen(word);
    node* p_node = NULL;

    //A ideia vai ser ir movendo com o cálculo de índice até a última letra, depois conferir a flag
    for(int i = 0; i < size; i++) {
        //Avança o p_node
        p_node = head->next[(word[i] - 'a')];
        
        //Confere se a próxima letra da palavra tá na trie;
        if(p_node == NULL) {
            //isso significa que a palavra não está contida
            return false;
        }
    }

    //Se não retornou false no loop, significa que o p_node tá apontando pra última letra da palavra
    return p_node->end;
}

/* OK */
bool insertWord(node* head, char* word) {
    int size = strlen(word); 
    int i;
    node* p_node = NULL;

    //Vai precisar conferir se já existe
    if(search(head, word)) {
        //se retornar true:
        printf("Essa palavra já está na trie!\n");
    } else {
        //se retornou false, ou a palavra não tá lá 0 letras, ou tem um pedaço, ou tá toda e não ta marcada
        p_node = head;
        for(i = 0; i < size; i++) {
            //Avança o p_node
            p_node = p_node->next[(word[i] - 'a')];
            
            //Confere se a próxima letra da palavra tá na trie;
            printf("%c", word[i+1]);

            //p_node->next[(word[i+1] - 'a')] == NULL (anterior)
            if(p_node == NULL) {
                //isso significa que chegamos na última letra.
                break;
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
            p_node = createNode();    //Cria um nodo novo
            //p_node = p_node->next[(word[i] - 'a')];          //Move o ponteiro pra esse novo nodo
            p_node->c = word[i];                             //Coloca o caracter lá
            i++;
        }        
    }
    //Quando sair desse while, ta inserida
    return true;
}

int main () {
    node* head = createNode();
    char word[50];

    printf("Digite uma palavra para inserir na Trie: ");
    scanf("%s", word);

    if(insertWord(head, word)) {
        printf("Palavra inserida com sucesso!");
    }

}