/*
Ainda deve ter alguns bugs e melhorias mas no mais ta feito. 

Só faltaria comentar melhor e organizar o código, comentar os print de depuração e tal
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX 10

typedef struct {
    char c;
    bool used;
} celula;

typedef struct {
    char word[MAX];
    int direction;  //direção (horizontal, vertical, diagonal)
    int sense;      //sentido (normal, ao contrário)
    int row_pos;
    int col_pos;
    bool found;
} word;

/* OK */
bool verifyPosition(int opt, int* row, int* col, int size, celula** table) {
    switch (opt)
    {
    case 0: // vertical
        /* code */
        for(int j = 0; j < 100; j++) {
            *row = rand() % MAX;
            *col = rand() % MAX;

            //se não cabe, pula
            if ((*row + size) > MAX) {
                continue;
            }

            bool espacoLivre = true;
            //Confere se as posições de row até MAX estão livres
            for(int i = *row; i < (*row + size); i++) {
                if(table[i][*col].used == true) {
                    //vai pra próxima iteração e sorteia de novo
                    espacoLivre = false;
                    break;
                }
            }

            if(espacoLivre) {
                return true;
            }
        }
        //Se loopou 100x aleatório e n achou, retorna false depois do break
        break;
    
    case 1: //horizontal
        for(int j = 0; j < 100; j++) {
            *row = rand() % MAX;
            *col = rand() % MAX;

            if ((*col + size) > MAX) {
                continue;
            }

            bool espacoLivre = true;

            for(int i = *col; i < (*col + size); i++) {
                if(table[*row][i].used == true) {
                    espacoLivre = false;
                    break;
                }
            }
            if(espacoLivre) {
                return true;
            }
        }
        break;

    case 2: //diagonal descendente
        for(int j = 0; j < 100; j++) {
            *row = rand() % MAX;
            *col = rand() % MAX;
            
            if (((*col + size) > MAX) || ((*row + size) > MAX)) {
                continue;
            }

            bool espacoLivre = true;
            int k = *col;
        
            for(int i = *row; i < (*row + size); i++) {
                if(table[i][k].used == true) {
                    espacoLivre = false;
                    break;
                }
                k++;
            }
            if(espacoLivre) {
                return true;
            }

        }

    default:
        break;
    }

    return false;
}

/* OK */
celula** createTable() {
    //Cria uma matriz de células
    celula** table = (celula**) malloc(sizeof(celula*) * MAX);

    for(int i = 0; i < MAX; i++) {
        table[i] = (celula*) malloc(sizeof(celula) * MAX);
    }
    
    //Cria a seed pra letras aleatórias
    srand(time(NULL));

    //Preenche a matriz com letras aleatórias
    for(int i = 0; i < MAX; i++) {
        for(int j = 0; j < MAX; j++) {
            //Gera um número entre 0 e 25, que ao somar com 'a' gera uma letra aleatória do alfabeto
            int letra = rand() % 26;
            table[i][j].c = 'a' + letra;
            table[i][j].used = false;
        }
    }

    return table;
}

/* OK */
void showTable(int n, celula** matriz) {
    //Visualiza a matriz
    printf("\n");
    for(int i = 0; i < n; i++){
        printf("\n");
        for(int j = 0; j < n; j++) {
            printf("%c ", matriz[i][j].c);
        }
    }
    printf("\n");
}

/* OK */
word** readWords() {
    const char *nome_arq = "words.txt";
    FILE *f = fopen(nome_arq, "r");

    if(f == NULL) {
        printf("\nErro ao abrir o arquivo");
        exit(1);
    }

    char str_aux[MAX+2]; //+2 pra conseguir caber os \n e os \0

    //Words é um ponteiro que aponta pra ponteiros (os ponteiros das nossas strings)
    //Aloca espaço pra 10 ponteiros para string (que é um ponteiro pra char)
    word** words = (word**) malloc(sizeof(word*) * MAX);
    
    //Cada ponteiro agora vai guardar o espaço de 10 caracteres
    for(int i = 0; i < MAX; i++) {
        words[i] = (word*) malloc(sizeof(word));
    }

    int wordCount = 0;

    //wordCount < MAX garante que não vamos estourar o array acessando algo que não existe
    while (wordCount < MAX && fgets(str_aux, MAX, f) != NULL) {
        //Se o tamanho da string lida for maior que o do tabuleiro, pula. Se não, lê e armazena

        /* A função strchr() retorna um ponteiro para a primeira ocorrência do caractere especificado 
        dentro de uma string, ou um ponteiro NULL (ou nulo) se o caractere não for encontrado na string */
        
        //Se não leu a string toda, e não é final de arquivo (significa que a palavra era maior que o permitido)
        //feof(f) vai retornar TRUE se for o final. Por isso tá negada.
        if(strchr(str_aux, '\n') == NULL && !feof(f)) {
            //Limpa o buffer de entrada do arquivo
            int ch;
            printf("Aviso: Palavra '%s' é muito longa e será ignorada.\n" ,str_aux);

            /* fgetc lê um caracter e avança o ponteiro de leitura pra frente */
            while ((ch = fgetc(f)) != '\n' && ch != EOF); //consome caracteres até achar um \n ou EOF

            continue; //pula pra próxima iteração do loop while, que é a próxima palavra.
        }
        //Se chegou aqui, significa que a linha é válida

        /* strcspn conta a str_aux até achar os caracteres do segundo parâmetro, 
        nesse caso, ele para exatamente na posição do \n */

        //Remove o \n do final da string e troca por \0 (código dele em ascii é 0)
        str_aux[strcspn(str_aux, "\n")] = 0;

        //Copia a palavra limpa pro nosso array
        strcpy(words[wordCount]->word, str_aux);

        //Seta a posição -1, -1 (não inserida) para verificações futuras
        words[wordCount]->row_pos = -1;
        words[wordCount]->col_pos = -1;
        words[wordCount]->found = false;

        wordCount++;
    }
    fclose(f);

    // Se lemos menos de MAX palavras faz os ponteiros restantes apontarem para strings vazias
    for (int i = wordCount; i < MAX; i++) {
        words[i]->word[0] = '\0';
    }

    return words;
};

/* OK */
void insertWords(celula** table, word** words) {
    //Imprime as palavras lidas só pra ter certeza
    printf("\n");
    for(int i = 0; i < 10; i++) {
        printf("%s\n", words[i]->word);
    }
    printf("\n");

    //Cria a seed pra letras aleatórias
    srand(time(NULL));

    /* As palavras podem ser inseridas na 
    0 - Vertical
    1 - Horizontal
    2 - Diagonal
    
    E também podem aparecer na ordem
    0 - Normal
    1 - Ao contrário
    */

    //Percorre todas as palavras e tenta colocar no tabuleiro
    for(int i = 0; i < 10; i++) {
        words[i]->direction = rand() % 3;
        words[i]->sense = rand() % 2;
        int size = strlen(words[i]->word);

        int row = rand() % MAX;
        int col = rand() % MAX;

        //Inverte a palavra se necessário
        if(words[i]->sense == 1) {
            strrev(words[i]->word);
        }

        switch (words[i]->direction)
        {
        case 0: //Vertical
            /*  */
            //verifica o tabuleiro até achar uma posição vertical válida
            if(verifyPosition(0, &row, &col, size, table)) {
                //Salva a posição de início da palavra na matriz
                words[i]->row_pos = row;
                words[i]->col_pos = col;
                
                printf("0");
                int k = 0;

                for(int j = row; j < (row + size); j++) {
                    table[j][col].c = words[i]->word[k];
                    table[j][col].used = true;
                    k++;
                }
            } else {
                printf("Essa palavra não coube no tabuleiro: '%s'\n", words[i]->word);
            }

            break;
        
        case 1: //horizontal
            if(verifyPosition(1, &row, &col, size, table)) {
                //Salva a posição de início na palavra da matriz
                words[i]->row_pos = row;
                words[i]->col_pos = col;

                printf("1");
                int k = 0;
                
                for(int j = col; j < (col + size); j++) {
                    table[row][j].c = words[i]->word[k];
                    table[row][j].used = true;
                    k++;
                }
            } else {
                printf("Essa palavra não coube no tabuleiro: '%s'\n", words[i]->word);
            }
            break;
        
        case 2: //diagonal descendente
            if(verifyPosition(2, &row, &col, size, table)) {
                //Salva a posição de início da palavra na matriz
                words[i]->row_pos = row;
                words[i]->col_pos = col;

                printf("2");
                int c = 0;

                int k = row;
                for(int j = col; j < (col + size); j++) {
                    table[k][j].c = words[i]->word[c];
                    table[k][j].used = true;
                    k++;
                    c++;
                }
            } else {
                printf("Essa palavra não coube no tabuleiro: '%s'\n", words[i]->word);
            }
            break;

        
        default:
            break;
        }

        //Desinverte a palavra pra ficar intuitivo de jogar
        if(words[i]->sense == 1) {
            strrev(words[i]->word);
        }
    }



}

word* searchWord(word** words, char* str) {
    for(int i = 0; i < MAX; i++) {
        
        if(strcmp(words[i]->word, str) == 0) {
            //Se a posição é negativa, ela tá em words mas não coube no tabuleiro
            if(words[i]->row_pos == -1) {
                return NULL;
            }

            //Se found é igual a true, significa que o usuário já marcou essa palavra antes
            if(words[i]->found == true) {
                //printf("Essa palavra já foi encontrada!\n");
                return NULL;
            }

                
            return words[i];
        }
    }
    return NULL;
}

void markWord(celula** table, word* w) {
    int size = strlen(w->word);
    w->found = true;

    switch (w->direction) {
        case 0: //Vertical
            for(int j = w->row_pos; j < (w->row_pos + size); j++) {
                table[j][w->col_pos].c = '*';
            }
            break;
        
        case 1: //horizontal
            for(int j = w->col_pos; j < (w->col_pos + size); j++) {
                table[w->row_pos][j].c = '*';
            }
            break;
        
        case 2: //diagonal descendente
            int k = w->row_pos;
            for(int j = w->col_pos; j < (w->col_pos + size); j++) {
                table[k][j].c = '*';
                k++;
            }
            break;
        default:
            break;
    }
}

int main () {
    int n = MAX;
    
    //printf("Qual o tamanho da matriz? ");
    //scanf("%d", &n);

    celula** matriz = createTable();
    showTable(n, matriz);
    word **words = readWords();
    insertWords(matriz, words);
    showTable(n, matriz);

    char str[MAX+2];
    int total_words = 0;
    int missing_words = 0;

    for(int i = 0; i < MAX; i++) {
        if(words[i]->word[0] != '\0') { //não é palavra vazia
            total_words++;
            missing_words++;
        }
    }
    
    //dps melhora esse while pro programa finalizar quando achar todas palavras
    while(missing_words != 0) {
        printf("Qual palavra voce acha que esta no tabuleiro? ");
        scanf("%s", str);
        str[strcspn(str, "\n")] = 0; //remove o \n

        word* w = searchWord(words, str);

        //se a palavra está contida
        if(w != NULL) {
            missing_words--;
            markWord(matriz, w);
            printf("Boa, você encontrou uma palavra!");
        } else {
            printf("Essa palavra não está no tabuleiro ou já foi encontrada!\n");
        }
        
        showTable(n, matriz);
    }
    printf("Parabéns! Você venceu o jogo!");
}