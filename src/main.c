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

typedef struct Node {
    int dest_id;        // id da próxima cidade
    double distance;    // é o "peso"
    struct Node* next;  // ponteiro pra próxima cidade
} Node;

typedef struct {
    int num_cities;   // total de cidades
    Node** adj_lists;   // um vetor de ponteiros pra listas
    // a struct guarda uma lista pra cada cidade. Ou seja, cada cidade armazena todas suas conexões (com todas outras pq o grafo é completo)
} GraphList;

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

void edit_matrix(GraphMatrix* matrixGraph, int id_city1, int id_city2, double distance) {
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

    // Verifica se é válido
    if (remove_index < 0 || remove_index >= n) return;

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

Node* create_node(int dest, double distance) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->dest_id = dest;
    new_node->distance = distance;
    new_node->next = NULL;
    return new_node;
}

GraphList* create_graph_list(int n) {
    GraphList* graph = (GraphList*)malloc(sizeof(GraphList));
    graph->num_cities = n;

    // Aloca n cabeças de listas (1 pra cada cidade)
    graph->adj_lists = (Node**)calloc(n, sizeof(Node*));    //aloca tudo como null com o calloc
    
    return graph;
}

void add_connection_list(GraphList* listGraph, int src, int dest, double distance) {
    // Adiciona um elemento na lista da cidade src
    Node* new_node = create_node(dest, distance);
    new_node->next = listGraph->adj_lists[src];
    listGraph->adj_lists[src] = new_node;

    // Faz a simetria (se foi A->B, faz B->A) pra poupar operações. Já que todos grafos aqui são não direcionados
    new_node = create_node(src, distance);
    new_node->next = listGraph->adj_lists[dest];
    listGraph->adj_lists[dest] = new_node;
}

double search_list(GraphList* listGraph, int src, int dest) {
    //Seleciona a cidade de partida
    Node* current = listGraph->adj_lists[src];

    // Navega pela lista dela até encontrar a cidade destino. Como o grafo é completo, O(n)
    while (current != NULL) {
        if (current->dest_id == dest) { // procura 1 por 1 na lista
            return current->distance;   // retorna a distância se encontrou
        }
        current = current->next;        //avança próx
    }

    return -1.0; //retorna -1 se não encontrar a conexão
}

void edit_list(GraphList* listGraph, int src, int dest, double distance) {
    bool found = false;
    Node* current = listGraph->adj_lists[src];

    // Custo se mantém O(n), embora na real seja (2*O(n), por fazer 2 vezes, mas na simplificação fica tudo O(n))

    // Confere a ida
    while (current != NULL) {
        if (current->dest_id == dest) {
            current->distance = distance;
            found = true;
            break; //para de procurar
        }
    }

    // SE achou a ida, muda a volta também.
    if (found) {
        current = listGraph->adj_lists[dest];
        while (current != NULL) {
            if (current->dest_id == src) {
                current->distance = distance;
                break; //para de procurar
            }
        }
    } else {
        printf("Erro: conexão inexistente entre cidade %d e cidade %d.\n", src, dest);
    }

    return;
}

void insert_list(GraphList* listGraph, City** cities_ptr, City new_city) {
    //Coloca a nova cidade dentro do vetor de cities
    *cities_ptr = (City*)realloc(*cities_ptr, sizeof(City) * (listGraph->num_cities + 1));
    
    if ((*cities_ptr) == NULL) {
        printf("Erro ao realocar vetor de cidades");
        exit(1);
    }
    (*cities_ptr)[listGraph->num_cities] = new_city;

    // Aumenta o índice de listas do grafo
    int old_n = listGraph->num_cities;
    int new_n = listGraph->num_cities + 1;

    listGraph->adj_lists = (Node**)realloc(listGraph->adj_lists, sizeof(Node*) * new_n);

    if (listGraph->adj_lists == NULL) {
        printf("Erro ao realocar vetor de índice das listas!\n");
        exit(1);
    }

    listGraph->adj_lists[old_n] = NULL;     // novo ponteiro começa em null
    listGraph->num_cities = new_n;          // atualiza o tamanho

    for (int i = 0; i < old_n; i++) {
        double dist = distance((*cities_ptr)[i], new_city);
        add_connection_list(listGraph, old_n, i, dist); // já cria a ida e a volta
    }

}

void remove_list(GraphList* listGraph, int city_id, City** cities_ptr) {
    int remove_index = city_id - 1;
    int n = listGraph->num_cities;

    // Verifica se é válido
    if (remove_index < 0 || remove_index >= n) return;

    // Libera a lista da cidade que vai ser removida
    Node* current = listGraph->adj_lists[remove_index];

    // Libera 1 por 1
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }

    // Passa por todas outras listas e limpa as conexões com a cidade removida, ligando os ponteiros e atualizando os índices das cidades
    for (int i = 0; i < n; i++) {
        if (i == remove_index) continue;    //vai pra cidade que vamos remover
        
        Node* current = listGraph->adj_lists[i];
        Node* previous = NULL;

        while (current != NULL) {
            // Caso o nó atual aponte pra cidade que estamos removendo, deleta
            if (current->dest_id == remove_index) {
                if (previous == NULL) {
                    // Era o primeiro da lista
                    listGraph->adj_lists[i] = current->next;
                    free(current);
                    current = listGraph->adj_lists[i];
                } else {
                    // Era algum nó no meio
                    previous->next = current->next;
                    free(current);
                    current = previous->next;
                }

            // Caso o nó atual aponte pra uma cidade que tá depois da que foi removida, atualiza o ID dela
            } else if (current->dest_id > remove_index) {
                current->dest_id--;
                previous = current;
                current = current->next;
            
            // Muda nada, só atualiza os ponteiros
            } else {
                previous = current;
                current = current->next;
            }
        }
    }

    //Agora move todos os índices de lista "pra cima", tapando o buraco
    for (int i = remove_index; i < n - 1; i++) {
        listGraph->adj_lists[i] = listGraph->adj_lists[i+1];
    }
    listGraph->adj_lists = (Node**)realloc(listGraph->adj_lists, (n - 1) * sizeof(Node*));

    //Remove a cidade do cities também
    for (int i = remove_index; i < n - 1; i++) {
        (*cities_ptr)[i] = (*cities_ptr)[i + 1];
    }
    *cities_ptr = (City*)realloc(*cities_ptr, sizeof(City) * (n - 1));

    listGraph->num_cities--;    // atualiza o contador
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
    printf("Criando a matriz de adjacência...\t");
    for (int i = 0; i < dimension; i++) {

        // A diagonal principal (distancia de uma cidade até ela mesmo) é sempre 0
        matrixGraph->distances[i][i] = 0.0;

        // Esse loop evita passar por espaços simétricos (1/3 e 3/1) e as diagonais(1/1 e 3/3 por exemplo)
        for (int j = i + 1; j < dimension; j++) {
            double dist = distance(cities[i], cities[j]);
            
            // Já preenche os 2 lados (ida e volta, 1/3 e 3/1)
            matrixGraph->distances[i][j] = dist;
            matrixGraph->distances[j][i] = dist;
        }
    }
    printf("Criada com sucesso!\n");

    GraphList* listGraph = create_graph_list(dimension);

    //Preenche a lista de adjacência
    printf("Criando a lista de adjacência...\t");
    for (int i = 0; i < dimension; i++) {
        // Esse loop evita fazer 2x pra ida e volta (1/3 e 3/1)
        for (int j = i + 1; j < dimension; j++) {
            double dist = distance(cities[i], cities[j]);
            // a função insert já cria a ida e a volta nas listas
            add_connection_list(listGraph, i, j, dist);
        }
    }
    printf("Criada com sucesso!\n");
    
    // TESTES
    int c1 = cities[0].id - 1, c2 = cities[1].id - 1;

    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f\n", c1, c2, search_matrix(matrixGraph, c1, c2));
    printf("Mudando distância para 45.00...\n\n");

    edit_matrix(matrixGraph, c1, c2, 45.00);
    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f", c1, c2, search_matrix(matrixGraph, c1, c2));

    City new_city;
    new_city.id = matrixGraph->num_cities + 1;
    new_city.x = 24000.00;
    new_city.y = 15000.00;

    insert_matrix(matrixGraph, &cities, new_city);

    c1 = 0, c2 = matrixGraph->num_cities - 1;
    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f\n", c1, c2, search_matrix(matrixGraph, c1, c2));
    printf("Mudando distância para 95.00...\n\n");

    edit_matrix(matrixGraph, c1, c2, 95.00);
    printf("Acesso a matriz de adjacencia:\nDistancia entre cidade %d e cidade %d: %.4f\n", c1, c2, search_matrix(matrixGraph, c1, c2));

    printf("Removendo a cidade 10\n");
    remove_matrix(matrixGraph, cities[11].id, &cities);

    printf("%d", search_matrix(matrixGraph, 11, 0)); //isso aqui vai dar erro ou imprimir algo fodase


    

}