#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <time.h> 
#include "aux.h" 

// cliente
typedef struct {
	int NUMERO;
	int INSTANTE_CHEGADA_GUICHE;
	int INSTANTE_CHEGADA_FILA_EMBARQUE;
	int INSTANTE_EMBARQUE;
	int INSTANTE_PARTIDA;
	int INSTANTE_CHEGADA;
	int PROB_DESIS_FILA_GUICHE;
	int PROB_DESIS_FILA_EMBARQUE; 
}cliente_stru; 

// configuracao
struct configuracao_stru {
	int TAMANHO_MAX_FILA_GUICHE;
	int TAMANHO_MAX_FILA_EMBARQUE;
	int TEMPO_MEDIO_VIAGEM;
	int TEMPO_ESPERA_FILA_GUICHE;
	int TEMPO_ESPERA_FILA_EMBARQUE;
	int TEMPO_CHEGADA_CLIENTES;
	int TEMPO_CHEGADA_CARROS;
	int NUMERO_DE_CARROS;
	int NUMERO_DE_PASSAGEIROS_CARRO;
	int PROB_DESISTENCIA_GUICHE;
	int PROB_DESISTENCIA_EMBARQUE;
	int TEMPO_SIMULACAO; 
};

// leitura do ficheiro
int* LerConfiguracao(char* ficheiro) {
	// criacao de array
	int* aux = (int*) malloc(sizeof(int) * 13);
	// abertura da configuracao
	FILE* ficheiro_conf = fopen(ficheiro, "r");
	// ficheiro encontrado
	if(ficheiro_conf == NULL) 
	{
		printf("Nao e possivel abrir o ficheiro.\n");
		return 0;
	}
	
	int num, i = 0;
	char name[100], buff[256];
	// percorre o ficheiro
	while(fgets(buff, sizeof buff, ficheiro_conf) != NULL)
	{
		if(sscanf(buff, "%[^=]=%d", name, &num) == 2)
			aux[i++] = num;
	}
	// fecha o ficheiro
	fclose(ficheiro_conf);
	return aux; 
}
