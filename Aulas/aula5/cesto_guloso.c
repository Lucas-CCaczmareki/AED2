// cesto_guloso.c
#include <stdio.h>

typedef struct {
    const char *nome;
    double peso;
    int valor;
} Item;

int main(void) {
    Item itens[] = {
        {"Folhas de Caraná", 1.8, 16},
        {"Yãkoana", 4.0, 34},
        {"Tucumã", 2.5, 15},
        {"Jenipapo", 4.5, 28},
        {"Fibra", 1.0, 10}
    };
    int n = sizeof(itens)/sizeof(itens[0]);
    double capacidade = 7.0;

    // ordenar por razão valor/peso (bubble sort simples)
    for (int i = 0; i < n-1; ++i) {
        for (int j = 0; j < n-i-1; ++j) {
            double r1 = (double)itens[j].valor / itens[j].peso;
            double r2 = (double)itens[j+1].valor / itens[j+1].peso;
            if (r1 < r2) {
                Item tmp = itens[j];
                itens[j] = itens[j+1];
                itens[j+1] = tmp;
            }
        }
    }

    double peso_total = 0.0;
    int valor_total = 0;
  

    printf("=== Solução Gulosa (valor/peso) ===\n");
    printf("Ordem usada (maior razão primeiro):\n");
    for (int i = 0; i < n; ++i) {
        printf(" - %s (peso %.1f, valor %d, razão %.2f)\n",
               itens[i].nome, itens[i].peso, itens[i].valor,
               (double)itens[i].valor / itens[i].peso);
    }

    printf("\nItens escolhidos:\n");
    for (int i = 0; i < n; ++i) {
        if (peso_total + itens[i].peso <= capacidade) {
            peso_total += itens[i].peso;
            valor_total += itens[i].valor;
          
            printf(" - %s (peso %.1f, valor %d)\n",
                   itens[i].nome, itens[i].peso, itens[i].valor);
        }
    }

    printf("Peso total: %.1f kg\n", peso_total);
    printf("Valor total: %d\n", valor_total);

    return 0;
}