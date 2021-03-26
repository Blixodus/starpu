#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	char str1[15];
	int x = 0; int y = 0;
	char str2[15];
    FILE *fichier_hfp = fopen(argv[1], "r");
    FILE *fichier_effectif = fopen(argv[2], "r");
    FILE *fichier_out = fopen(argv[3], "w");
    if (fichier_hfp != NULL && fichier_effectif != NULL)
    {
		/* Print beggining */
		fprintf(fichier_out, "	x	y\n");
		while (fscanf(fichier_hfp, "%s", str1) == 1) //expect 1 successful conversion
		{
		  printf("ok0 %s\n", str1);
		  //process buffer
		  fprintf(fichier_out, "%s	%d", str1, x);
		  x++;
		  rewind(fichier_effectif);
		  fscanf(fichier_effectif, "%s", str2);
		  y = 0;
		  while (strcmp(str1, str2) != 0)
		  {
			 fscanf(fichier_effectif, "%s", str2);
			 y++;
			 printf("ok1, %d\n", y);
		 }
		 printf("ok2\n");
		 fprintf(fichier_out, "	%d\n", y); 
		}
		if (feof(fichier_hfp)) 
		{
		  //hit end of file
		  fclose(fichier_hfp);
		  fclose(fichier_effectif);
		  fclose(fichier_out);
		  return 0;
		}
		else
		{
		  //some other error interrupted the read
		  printf("Error while reading\n"); exit(0);
		}
		//~ while(str1 != EOF)
		//~ {
		//~ rewind(fichier_in);
		//~ for (j = 0; j < NOMBRE_DE_TAILLES_DE_MATRICES; j++) {
			//~ fprintf(fichier_out,"%d",ECHELLE_X*(j+1)+START_X);
			//~ for (i = 0; i < NOMBRE_DE_TAILLES_DE_MATRICES*NOMBRE_ALGO_TESTE; i++) {
				//~ if (i%NOMBRE_DE_TAILLES_DE_MATRICES == j) {
					//~ for (k = 0; k < count; k++) {
						//~ fscanf(fichier_in,"%s",str1);
					//~ }
					//~ fscanf(fichier_in, "%s",GFlops);
					//~ fprintf(fichier_out,"	%s",GFlops);
				//~ }
				//~ else {
					//~ for (k = 0; k < count + 1; k++) {
						//~ fscanf(fichier_in,"%s",str1);
					//~ }
				//~ }
			//~ }
			//~ fprintf(fichier_out,"\n"); 
			//~ rewind(fichier_in);
		//~ }	
		//~ fclose(fichier_in);
		//~ fclose(fichier_out);
    }
    else
    {
        printf("Impossible d'ouvrir au moins 1 fichier d'ordre\n"); 
        exit(0);
    }
    return 0;
}

