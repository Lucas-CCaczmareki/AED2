#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX 10

void showTable(int n, char matriz[n][n]) {
    //Visualiza a matriz
    printf("\n");
    for(int i = 0; i < n; i++){
        printf("\n");
        for(int j = 0; j < n; j++) {
            printf("%c ", matriz[i][j]);
        }
    }
    printf("\n");
}

char** readWords() {
    const char *nome_arq = "words.txt";
    FILE *f = fopen(nome_arq, "r");

    if(f == NULL) {
        printf("\nErro ao abrir o arquivo");
        exit(1);
    }

    char str_aux[MAX];
    char** words = (char**) malloc(sizeof(char) * MAX+10);
    
    for(int i = 0; i < MAX; i++) {
        words[i] = (char*) malloc(sizeof(char) * MAX+10);
    }

    int wordCount = 0;

    while (fgets(str_aux, MAX, f) != NULL) {
        //destino - origem
        strcpy(words[wordCount], str_aux);
        wordCount++;
    }

    return words;
};

int main () {
    int n = MAX;
    
    //printf("Qual o tamanho da matriz? ");
    //scanf("%d", &n);

    char matriz[n][n];
    srand(time(NULL));

    //Preenche a matriz com letras aleatórias
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            //Gera um número entre 0 e 25, que ao somar com 'a' gera uma letra aleatória do alfabeto
            int letra = rand() % 26;
            matriz[i][j] = 'a' + letra;
        }
    }

    showTable(n, matriz);
    char **words = readWords();

    for(int i = 0; i < 10; i++) {
        printf("%s", words[i]);
    }


}


/* RASCUNHO

int main () {
    int n = 0;
    printf("Qual o tamanho da matriz voce deseja? ");
    scanf("%d", &n);

    // Semeia o gerador de números aleatórios com o tempo atual
    // Faça isso apenas uma vez no início do programa
    srand(time(NULL));

    char matriz [n][n];
    
    // Preenche a matriz com caracteres minúsculos aleatórios
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            // Gera um número de 0 a 25 e soma com o código ASCII de 'a'
            // a = 97 + 1 número aleatório que vai fazer ir de a-z
            matriz[i][j] = 'a' + (rand() % 26);
        }
    }

    // Imprime a matriz para visualização
    printf("\nMatriz com caracteres aleatorios:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%c ", matriz[i][j]);
        }
        printf("\n");
    }

    //Cria uma matriz pra armazenar as palavras que vão estar no caça palavra
    char palavras[n][n];
    char string_aux[100];
    printf("Digite 10 palavras\n");
    for(int i = 0; i < n; i++) {
        printf("%d: ", i+1);
        
        fgets(string_aux, sizeof(string_aux), stdin); // Lê a linha, incluindo o '\n'

        // Encontra a posição do '\n'
        char *pos = strchr(string_aux, '\n');
        if (pos != NULL) {
            *pos = '\0'; // Substitui o '\n' por um '\0'
        }

        if (strlen(string_aux) < n) {
            //destino, origem
            strcpy(palavras[i], string_aux);
        }
    }

    //Só printa as palavras pra ver se deu bom
    /*
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < strlen(palavras[i]); j++) {
            printf("%c", palavras[i][j]);
        }
        printf("\n");
    }
   

    //Vai precisar fazer isso 10 vezes pra cada palavra e inserir no tabuleiro
    //E de algum jeito eu preciso marcar onde que a palavra foi inserida
    //Pra colocar no caça palavras preciso saber a direção(rand de 0 a 2) e o sentido (rand de 0 a 1)
    //Direção
    //0 - horizontal
    //1 - vertical
    //2 - Diagonal
    
    //Sentido
    //0 - Normal
    //1 - Invertido

    for (int i = 0; i < 10; i++) {
        //Gera a orientação da palavra
        int direcao = rand() % 3;
        int sentido = rand() % 2;
        
        //Tenta inserir a palavra 100 vezes, se n der pula
        for(int x = 0; x < 100; x++) {
            //Gera a posição da matriz por enquanto fodase se sobrepor
            int linha = rand() % n;
            int coluna = rand() % n;
            
            //Horizontal
            if (direcao == 0) {
                //Se a palavra não couber horizontalmente na coluna selecionada, seleciona de novo até conseguir
                while(strlen(palavras[i]) > (n - coluna)) {
                    coluna = rand() % n;
                }

                int y = 0;

                //Coloca na matriz
                for(int j = coluna; j < n-1; j++) {
                    matriz[linha][j] = palavras[i][y];
                    y++;
                }
                break;
            }

            //Vertical
            if (direcao == 1) {
                //Se a palavra não couber verticalmente, sorteia até caber
                while(strlen(palavras[i]) > (n - linha)) {
                    linha = rand() % n;
                }

                int y = 0;

                for(int j = linha; j < n-1; j++) {
                    matriz[j][coluna] = palavras[i][y];
                    y++;
                }
                break;
            }

            //Diagonal
            if (direcao == 2) {
                //fodase por enquanto
            }

        }
    }

    // Imprime a matriz para visualização
    printf("\nMatriz com caracteres aleatorios:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%c ", matriz[i][j]);
        }
        printf("\n");
    }

}


*/