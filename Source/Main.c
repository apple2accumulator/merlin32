/***********************************************************************/
/*                                                                     */
/*  Main.c : Module Assembleur d'un code source 65c816.                */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>

#include "Dc_Library.h"
#include "a65816_Link.h"
#include "a65816_Line.h"
#include "a65816_File.h"
#include "a65816_Lup.h"
#include "a65816_Macro.h"
#include "a65816_Cond.h"
#include "a65816_Code.h"
#include "a65816_Data.h"
#include "a65816_OMF.h"

int Assemble65c816(char *,char *,int);

/****************************************************/
/*  main() :  Fonction principale de l'application. */
/****************************************************/
int main(int argc, char *argv[])
{
  jmp_buf context;
  int i, verbose_mode, context_value, error;
  char *error_string = NULL;
  struct omf_segment *current_omfsegment;
  struct parameter *param;
  char file_error_path[2048];
  char source_file_path[2048];
  char macro_folder_path[2048];

  /* Message Information */
  printf("%s v 1.0, (c) Brutal Deluxe 2011-2015\n",argv[0]);

  /* Vérification des paramètres */
  if(argc != 3 && argc != 4)
    {
      printf("  Usage : %s [-V] <macro_folder_path> <source_file_path>.\n",argv[0]);
      return(1);
    }
  if(argc == 3 && !my_stricmp(argv[1],"-V"))
    {
      printf("  Usage : %s [-V] <macro_folder_path> <source_file_path>.\n",argv[0]);
      return(1);
    }
  if(argc == 4 && my_stricmp(argv[1],"-V"))
    {
      printf("  Usage : %s [-V] <macro_folder_path> <source_file_path>.\n",argv[0]);
      return(1);
    }

  /* Décodage des paramètres */
  if(argc == 3)
    {
      verbose_mode = 0;
      strcpy(macro_folder_path,argv[1]);
      strcpy(source_file_path,argv[2]);
    }
  else
    {
      verbose_mode = 1;
      strcpy(macro_folder_path,argv[2]);
      strcpy(source_file_path,argv[3]);
    }

  /* Initialisation */
  my_Memory(MEMORY_INIT,NULL,NULL,NULL);
  my_File(FILE_INIT,NULL);

  /* Initialisation du mécanisme de gestion d'erreurs */
  my_RaiseError(ERROR_INIT,NULL);
  context_value = setjmp(context);
  if(context_value)
    {
      /* Récupération de la chaine contenant le message d'erreur */
      my_RaiseError(ERROR_GET_STRING,&error_string);

      /* Message d'erreur et fin */
      if(error_string)
        {
          printf("      => [Error] %s.\n",error_string);
          free(error_string);
        }

      /* On récupère le OMF Segment courant (s'il existe) */
      my_Memory(MEMORY_GET_OMFSEGMENT,&current_omfsegment,NULL,NULL);

      /** On essaye de Dumper qqchose dans le fichier Error_Output.txt **/
      if(current_omfsegment != NULL)
        {
          strcpy(file_error_path,"error_output.txt");
          my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
          if(param != NULL)
            if(strlen(param->current_folder_path) > 0)
              sprintf(file_error_path,"%serror_output.txt",param->current_folder_path);
          CreateOutputFile(file_error_path,current_omfsegment,NULL);
        }
        
      /* Libération des ressources */
      my_Memory(MEMORY_FREE,NULL,NULL,NULL);

      /* Error */
      return(1);
    }
  my_RaiseError(ERROR_INIT,&context);

  /* Allocation mémoire de la structure param */
  param = mem_alloc_param();
  if(param == NULL)
    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure parameter");
  my_Memory(MEMORY_SET_PARAM,param,NULL,NULL);

  /** Préparation du dossier Macro **/
  if(strlen(macro_folder_path) > 0)
    if(macro_folder_path[strlen(macro_folder_path)-1] != '\\' && macro_folder_path[strlen(macro_folder_path)-1] != '/')
      strcat(macro_folder_path,FOLDER_SEPARATOR);

  /** Prépare le chemin des fichiers Output **/
  strcpy(param->output_file_path,source_file_path);
  for(i=(int)strlen(param->output_file_path); i>=0; i--)
    if(param->output_file_path[i] == '\\' || param->output_file_path[i] == '/')
      {
        param->output_file_path[i+1] = '\0';
        break;
      }
  if(i < 0)
    strcpy(param->output_file_path,"");
  strcpy(param->current_folder_path,param->output_file_path);

  /*** Assemble et Link tous les fichiers du projet ***/
  error = AssembleLink65c816(source_file_path,macro_folder_path,verbose_mode);

  /* Libération des ressources */
  my_File(FILE_FREE,NULL);
  my_Memory(MEMORY_FREE,NULL,NULL,NULL);

  /* Fin de la gestion des erreurs */
  my_RaiseError(ERROR_END,NULL);

  /* OK */
  return(0);
}

/***********************************************************************/