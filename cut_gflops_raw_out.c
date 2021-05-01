#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc != 8)
	{
		fprintf(stderr, "Error: il manque 1 ou des arguments à ./cut_gflops_raw_out\n");
		exit(EXIT_FAILURE);
	}
	int NOMBRE_DE_TAILLES_DE_MATRICES = atoi(argv[1]);
	int NOMBRE_ALGO_TESTE = atoi(argv[2]);
	int ECHELLE_X = atoi(argv[3]);
	int START_X = atoi(argv[4]);
	long where_to_write = 0; int k = 0;
	long ligne[10] = {0};
	int index = 0;
	int first_loop = 0;
	int i = 0; int j = 0;
	char str1[10];
	int count = 0;
	char GFlops[10];
	char deviance[10];
	char c;
    FILE* fichier_in = NULL;
    FILE* fichier_out = NULL;
    fichier_in = fopen(argv[5], "r");
    fichier_out = fopen(argv[6], "w+");
    int ecart_type = atoi(argv[7]);
    if (ecart_type != 1 && ecart_type != 0)
    {
		fprintf(stderr, "Argument 7 (chose if we used deviance or not) of ./cut_gflops_raw_out must be 0 (no) or 1 (yes)\n");
		exit(EXIT_FAILURE);
	}
    if (fichier_in != NULL)
    {
		c = fgetc(fichier_in);
		while (c != '\n') {
			c = fgetc(fichier_in);
			if (c == '	') { count++; }
		}
		rewind(fichier_in);
		for (j = 0; j < NOMBRE_DE_TAILLES_DE_MATRICES; j++) {
			fprintf(fichier_out,"%d",ECHELLE_X*(j+1)+START_X);
			if (ecart_type == 0)
			{
				for (i = 0; i < NOMBRE_DE_TAILLES_DE_MATRICES*NOMBRE_ALGO_TESTE; i++) {
					if (i%NOMBRE_DE_TAILLES_DE_MATRICES == j) {
						for (k = 0; k < count; k++) {
							fscanf(fichier_in,"%s",str1);
						}
						fscanf(fichier_in, "%s",GFlops);
						fprintf(fichier_out,"	%s",GFlops);
					}
					else {
						for (k = 0; k < count + 1; k++) {
							fscanf(fichier_in,"%s",str1);
						}
					}
				}
			}
			else
			{
				for (i = 0; i < NOMBRE_DE_TAILLES_DE_MATRICES*NOMBRE_ALGO_TESTE; i++) {
					if (i%NOMBRE_DE_TAILLES_DE_MATRICES == j) {
						for (k = 0; k < count - 1; k++) {
							fscanf(fichier_in,"%s", str1);
						}
						fscanf(fichier_in, "%s	%s",GFlops, deviance);
						fprintf(fichier_out,"	%s	%s",GFlops, deviance);
					}
					else {
						for (k = 0; k < count + 1; k++) {
							fscanf(fichier_in,"%s",str1);
						}
					}
				}
			}
			
			fprintf(fichier_out,"\n"); 
			rewind(fichier_in);
		}	
		fclose(fichier_in);
		fclose(fichier_out);
    }
    else
    {
        printf("Impossible d'ouvrir le fichier RAW.txt");
    }
    return 0;
}
