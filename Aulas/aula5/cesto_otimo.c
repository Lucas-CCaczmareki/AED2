#include <stdio.h>

typedef struct {
    const char *nome;
    double peso;
    int valor;
} Item;

/* ESSE É O ALGORITMO FORÇA BRUTA */

int main(void) {

    //Cada bit representa 1 item
    // 00111 (significa que tu tá tentando colocar no cesto o Tucumã, o Jenipapo e a Fibra)

    Item itens[] = {
        {"Casca de Carapanaúba", 1.8, 16},
        {"Yãkoana", 4.0, 34},
        {"Tucumã", 2.5, 15},
        {"Jenipapo", 4.5, 28},
        {"Fibra", 1.0, 10}
    };
    int n = sizeof(itens)/sizeof(itens[0]); //número de itens no vetor
    double capacidade = 7.0;

    //Total de combinações de itens é igual a 2^n (representado de 0 a 31 em binário)
    int total = 1 << n; //faz 1 shift à esquerda (= 2^n)
    
    double melhor_peso = 0.0;
    int melhor_valor = 0;
    int melhor_mask = 0;

    //1o for. faz pros 32 (0 a 31) números binários
    //Ou seja, faz pra todas combinmações de itens possível.
    for (int mask = 0; mask < total; mask++) { 
        double peso = 0.0;
        int valor = 0;

        //Repete 1x pra cada item
        for (int i = 0; i < n; i++) {
            //Mask é a combinação atual (ex: 00111, tem na cesta os 3 itens)

            //Isso aqui testa cada bit do maks pra saber qual item tá jogando pra cesta
            if (mask & (1 << i)) {
                peso += itens[i].peso;
                valor += itens[i].valor;
            }

        }

        //Se ta dentro da capacidade e o valor é maior que o maior (último maior)
        if (peso <= capacidade && valor > melhor_valor) {
            melhor_valor = valor;
            melhor_peso = peso;
            melhor_mask = mask;
        }
    }


    //Isso aq fodase
    printf("=== Solução Ótima (força bruta) ===\n");
    printf("Itens escolhidos:\n");
    for (int i = 0; i < n; ++i) {
        if (melhor_mask & (1 << i))
            printf(" - %s (peso %.1f, valor %d)\n", itens[i].nome, itens[i].peso, itens[i].valor);
    }
    printf("Peso total: %.1f kg\n", melhor_peso);
    printf("Valor total: %d\n", melhor_valor);

    return 0;
}