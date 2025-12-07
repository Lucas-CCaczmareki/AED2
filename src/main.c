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

double distance(City c1, City c2) {
    return sqrt(pow(c1.x - c2.x, 2) + pow(c1.y - c2.y, 2));
}

int main () {
    City* cities;
    bool isCoord = false;
    int i = 0;
    char line[500];
    FILE* fp = fopen("ja9847.tsp", "r");
    int dimension = 0;

    // Lê linha por linha do arquivo
    while(fgets(line, sizeof(line), fp) != NULL) {
        //Lê as coordenadas das cidades
        if(isCoord) {
            if(sscanf(line, "%d %lf %lf", &cities[i].id, &cities[i].x, &cities[i].y) != 3) {
                printf("Erro ao ler a coordenada da %da cidade!\n", i);
            } else {
                printf("Cidade %d: x: %.4lf y: %.4lf\n", cities[i].id, cities[i].x, cities[i].y);
            }
            i++;
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
        }
    }

    printf("Cidade %d: x: %.4f y: %.4f\n", cities[0].id, cities[0].x, cities[0].y);
}