#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

typedef struct {
    int id;
    double x;
    double y;
} City;

typedef struct {
    int num_cities;
    double** distances;
} GraphMatrix;

double distance(City c1, City c2) {
    return sqrt(pow(c1.x - c2.x, 2) + pow(c1.y - c2.y, 2));
}

GraphMatrix* create_graph_matrix(int n) {
    // Aloca a estrutura em si
    GraphMatrix* graph = (GraphMatrix*)malloc(sizeof(GraphMatrix));
    graph->num_cities = n;

    // Aloca as linhas de cidades
    graph->distances = (double**)malloc(sizeof(double*) * n);

    // Aloca as colunas pra cada linha
    for (int i = 0; i < n; i++) {
        graph->distances[i] = (double*)malloc(sizeof(double) * n);
    }

    return graph;
}

double search_matrix(GraphMatrix* matrixGraph, int id_city1, int id_city2) {
    //Eu posso adicionar uma verificação pra ver se os ids são válidos, se for inválido retorna -1 (afinal n existe distância negativa)
    return matrixGraph->distances[id_city1][id_city2];
}

void changeDist_matrix(GraphMatrix* matrixGraph, int id_city1, int id_city2, double distance) {
    matrixGraph->distances[id_city1][id_city2] = distance;
    return;
}

void insert_matrix(GraphMatrix* matrixGraph, City** cities_ptr, City new_city) {
    
    //Coloca a nova cidade dentro do vetor de cities
    
    *cities_ptr = (City*)realloc(*cities_ptr, sizeof(City) * (matrixGraph->num_cities + 1));
    
    if ((*cities_ptr) == NULL) {
        printf("Erro ao realocar vetor de cidades");
        exit(1);
    }

    (*cities_ptr)[matrixGraph->num_cities] = new_city;

    int old_n = matrixGraph->num_cities;
    int new_n = matrixGraph->num_cities + 1;
    
    double** new_distances = (double**) realloc(matrixGraph->distances, new_n * sizeof(double*));

    if (new_distances == NULL) {
        printf("Realocação falhou!\n");
        exit(1);
    }

    matrixGraph->distances = new_distances;

    //Realocando só as colunas que existem (a nova vai precisar de malloc)
    for(int i = 0; i < old_n; i++) {
        // realoca as colunas
        matrixGraph->distances[i] = (double*)realloc(matrixGraph->distances[i], new_n * sizeof(double));
        
        //n sei se é necessário fazer esse if. Pensando em performance
        if (new_distances[i] == NULL) {
            printf("Realocação falhou!\n");
            exit(1);
        }
        
    }
    //Realocando a nova coluna
    new_distances[old_n] = (double*)malloc(new_n * sizeof(double));

    // Recalcular a distância
    for(int i = 0; i < new_n; i++) {
        // 0/3 e o 3/0.
        // 1/3 e o 3/1.
        if(old_n == i) {
            matrixGraph->distances[old_n][i] = 0.0;
        } else {
            double dist = distance(new_city, (*cities_ptr)[i]);
            matrixGraph->distances[old_n][i] = dist;
            matrixGraph->distances[i][old_n] = dist;
        }

    }

    matrixGraph->num_cities = new_n;
    return;

}

void remove_matrix(GraphMatrix* matrixGraph, int city_id, City** cities_ptr) {
    int remove_index = city_id - 1;
    int n = matrixGraph->num_cities;

    // Libera a linha que vamos revoler
    free(matrixGraph->distances[remove_index]);

    //puxa todas as linhas 1 "pra cima" com os espaços
    for(int i = remove_index; i < n - 1 ; i++) {
        matrixGraph->distances[i] = matrixGraph->distances[i + 1];
    }   

    //puxa todas as colunas 1 "pro lado"
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1; j++) {
            matrixGraph->distances[i][j] = matrixGraph->distances[i][j+1];
        }
    }

    //Realloc pra liberar a memória extra que ficou
    matrixGraph->distances = (double**)realloc(matrixGraph->distances, (n-1) * sizeof(double*));

    for (int i = 0; i < n - 1; i++) {
        matrixGraph->distances[i] = (double*)realloc(matrixGraph->distances[i], (n - 1) * sizeof(double));
    }
    
    //Shift de tudo no cities pra esquerda, cobrindo o buraco da remoção
    for (int i = remove_index; i < n - 1; i++) {
        //Usamos ponteiro pra ponteiro pra mudar a estrutura original
        (*cities_ptr)[i] = (*cities_ptr)[i+1];
    }

    // Atualiza o ponteiro original do cities, pois o realloc que limpa o espaço pode mudar o endereço
    *cities_ptr = (City*)realloc(*cities_ptr, sizeof(City) * n - 1);
    
    matrixGraph->num_cities--;
}

int main () {
    City* cities;
    bool isCoord = false;
    int i = 0;
    char line[500];
    FILE* fp = fopen("ja_tests.tsp", "r");
    int dimension = 0;

    if (fp == NULL) {
        printf("Erro ao abrir o arquivo!\n");
        exit(1);
    }

    // Lê linha por linha do arquivo
    while(fgets(line, sizeof(line), fp) != NULL) {
        // Testa se EOF pra evitar ler coisa errada.
        if(strstr(line, "EOF") != NULL) {
            break;
        }
        
        //Lê as coordenadas das cidades
        if(isCoord) {
            if(sscanf(line, "%d %lf %lf", &cities[i].id, &cities[i].x, &cities[i].y) != 3) {
                printf("Erro ao ler a coordenada da %da cidade!\n", i);
            } else {
                // printf("Cidade %d: x: %.4lf y: %.4lf\n", cities[i].id, cities[i].x, cities[i].y);
                i++;
            }
        }
        
        // Pega quantas cidades existem no arquivo
        if(strstr(line, "DIMENSION") != NULL) {
            //retorna um ponteiro pra onde tá os : ou null se n tiver
            char* two_points = strchr(line, ':'); 

            if (two_points != NULL) {
                dimension = atoi(two_points + 1);
                printf("Dimensao: %d\n", dimension);
            }
        }
        
        // Marca quando chegar na parte das coordenadas e aloca um vetorzao de cidades
        if(strstr(line, "NODE_COORD_SECTION") != NULL) {
            //dps daqui vamos começar a ler as coordenadas, ent vamo setar uma flag pra isso e alocar o vetor
            cities = (City*)malloc(sizeof(City) * dimension);
            isCoord = true;
            continue;
        }
    }
    fclose(fp);
    // Teste pra ver se deu certo
    // printf("Cidade %d: x: %.4f y: %.4f\n", cities[0].id, cities[0].x, cities[0].y);

    // Início da parte que cria a matriz de adjacência

    GraphMatrix* matrixGraph = create_graph_matrix(dimension);

    // Preenche a matriz de adjacência
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            if (i == j) {
                matrixGraph->distances[i][j] = 0.0;
            } else {
                double dist = distance(cities[i], cities[j]);
                matrixGraph->distances[i][j] = dist;
            }
        }
    }


    
    // TESTES
    int c1 = cities[0].id - 1, c2 = cities[1].id - 1;

    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f\n", c1, c2, search_matrix(matrixGraph, c1, c2));
    printf("Mudando distância para 45.00...\n\n");

    changeDist_matrix(matrixGraph, c1, c2, 45.00);
    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f", c1, c2, search_matrix(matrixGraph, c1, c2));

    City new_city;
    new_city.id = matrixGraph->num_cities + 1;
    new_city.x = 24000.00;
    new_city.y = 15000.00;

    insert_matrix(matrixGraph, &cities, new_city);

    c1 = 0, c2 = matrixGraph->num_cities - 1;
    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f\n", c1, c2, search_matrix(matrixGraph, c1, c2));
    printf("Mudando distância para 95.00...\n\n");

    changeDist_matrix(matrixGraph, c1, c2, 95.00);
    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f\n", c1, c2, search_matrix(matrixGraph, c1, c2));

    printf("Removendo a cidade 10\n");
    remove_matrix(matrixGraph, cities[11].id, &cities);

    printf("%d", search_matrix(matrixGraph, 11, 0)); //isso aqui vai dar erro ou imprimir algo fodase


    

}