#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int *configuracao(char *file)
{
	int *a = (int *)malloc(sizeof(int) * 20);
	FILE *fp = fopen (file, "r");
	if(fp == NULL )
	{
		printf("erro: abertura do ficheiro de configuracao\n");
		abort();
	}
	int num, i=1;
	char name[100], buff[500];
	while (fgets( buff, sizeof buff, fp) != NULL) 
	{
		if(sscanf(buff, "%[^=]=%d", name, &num) == 2)
			a[i++]=num;
	}
	a[0] = i;
	fclose(fp);
	return a;
}