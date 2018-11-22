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
#include "version.h"

#ifndef MACRO_DIR
#error MACRO_DIR must be defined to the default macro library directory.
#endif

#define STR_SIZE 2048

int Assemble65c816(char *,char *,int);
void ParseArguments(int,char **,int *,char *,char *);
void Usage(void);
void FailWithUsage(char *,char *);

char program_name[STR_SIZE];

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
  char file_error_path[STR_SIZE];
  char source_file_path[STR_SIZE];
  char macro_folder_path[STR_SIZE];
  size_t len;

  /* Remplir la variable de nom de programme global */
  CopyString(program_name,argv[0],STR_SIZE);

  /* Message Information */
  printf("%s %s, (c) Brutal Deluxe 2011-2015\n",argv[0],MERLIN_VERSION);
 
  /* Analyser les arguments en ligne de commande */
  ParseArguments(argc,argv,&verbose_mode,macro_folder_path,source_file_path);

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
  len = strlen(macro_folder_path);
  if(len > 0)
    if(macro_folder_path[len-1] != '\\' && macro_folder_path[len-1] != '/')
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
  return EXIT_SUCCESS;
}


/*****************************************************/
/*  ParseArguments() :  Analyser les arguments       */
/*****************************************************/
void ParseArguments(
        /* entrée de données */  int   argc,
        /* entrée de données */  char *argv[],
        /* sortie */             int  *verbose,
        /* sortie */             char *macro_dir,
        /* sortie */             char *source_file)
{
  int i;

  *verbose = 0;
  ClearString(macro_dir);
  ClearString(source_file);

  for(i = 1; i < argc; i++)
    if(my_stricmp(argv[i],"-v")==0 || my_stricmp(argv[i],"--verbose")==0)
      *verbose = 1;
    else if(my_stricmp(argv[i],"-h")==0 || my_stricmp(argv[i],"--help")==0)
      {
        Usage();
        exit(EXIT_SUCCESS);
      }
    else if(IsDirectory(argv[i])) /* Dir arg is macro lib */
      if(IsEmpty(macro_dir) && IsEmpty(source_file))
        CopyString(macro_dir,argv[i],STR_SIZE);
      else
        FailWithUsage(argv[i],"Too many macro directories");
    else                          /* Non-dir arg is source file */
      if(IsEmpty(source_file))    /* Accept only if not yet provided */
        CopyString(source_file,argv[i],STR_SIZE);
      else
        FailWithUsage(argv[i],"Too many source files");

  if(IsEmpty(macro_dir))
    CopyString(macro_dir,MACRO_DIR,STR_SIZE);
  if(IsEmpty(source_file))
    FailWithUsage(NULL,"Missing source file parameter");
}


/*************************************************************/
/*  Usage() :  Affiche les arguments de la ligne de commande */
/*************************************************************/
void Usage(void)
{
  printf("Usage:\n");
  printf("  %s [-V|--verbose] [<macro_dir>] <source_file>\n",program_name);
  printf("  %s -v|--version\n",program_name);
}


/************************************************************************/
/*  FailWithUsage() :  Imprimer l'utilisation et quitter en cas d'échec */
/************************************************************************/
void FailWithUsage(char *object_name,char *message)
{
  if (IsEmpty(object_name))
    fprintf(stderr, "%s: %s\n", program_name, message);
  else
    fprintf(stderr, "%s: %s: %s\n", program_name, message, object_name);
  Usage();
  exit(EXIT_FAILURE);
}

/***********************************************************************/

/* vim: tabstop=2 shiftwidth=2 expandtab
*/

