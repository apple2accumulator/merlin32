/***********************************************************************/
/*                                                                     */
/*  a65816_Macro.c : Module pour la gestion des Macros  .              */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/timeb.h>

#include "Dc_Library.h"
#include "a65816_File.h"
#include "a65816_Line.h"
#include "a65816_Macro.h"

void LoadOneMacroFile(char *,char *,struct source_line *,struct omf_segment *);
static struct source_line *BuildMacroLine(struct source_line *,struct omf_segment *,struct omf_project *);
static struct source_line *BuildSourceMacroLine(struct source_line *,struct macro_line *,int,char **,struct omf_segment *);
static void BuildSubstituteValue(char *,int,char **,char *);
struct macro *mem_alloc_macro(char *,char *,int);
struct macro_line *mem_alloc_macroline(char *,char *,char *,char *);

/******************************************************************/
/*  LoadAllMacroFile() :  Chargement de tous les fichiers Macros. */
/******************************************************************/
void LoadAllMacroFile(char *folder_path, struct omf_segment *current_omfsegment)
{
  int i, nb_file, is_error;
  char **tab_file_name;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /** Récupère les fichiers du répertoire **/  
  tab_file_name = GetFolderFileList(folder_path,&nb_file,&is_error);
  if(tab_file_name == NULL && is_error == 0)
    return;
  if(tab_file_name == NULL && is_error == 1)
    {
      printf("    => Error, Can't get files list from Macro folder '%s'.\n",folder_path);
      return;
    }

  /* Prépare le nom du dossier */
  strcpy(param->buffer_folder_path,folder_path);
  if(strlen(param->buffer_folder_path) > 0)
    if(param->buffer_folder_path[strlen(param->buffer_folder_path)-1] != '\\' && param->buffer_folder_path[strlen(param->buffer_folder_path)-1] != '/')
      strcat(param->buffer_folder_path,FOLDER_SEPARATOR);

  /** On charge tous les fichiers .s présents **/
  for(i=0; i<nb_file; i++)
    {
      /* On vérifie le .s à la fin */
      if(strlen(tab_file_name[i]) < 3)
        continue;
      if(my_stricmp(&(tab_file_name[i][strlen(tab_file_name[i])-2]),".s"))
        continue;
        
      /** Charge le fichier **/
      LoadOneMacroFile(param->buffer_folder_path,tab_file_name[i],NULL,current_omfsegment);
    }
    
  /* Libération mémoire */
  mem_free_list(nb_file,tab_file_name);
}


/***********************************************************************/
/*  LoadSourceMacroFile() :  Chargement des fichiers Macros du Source. */
/***********************************************************************/
void LoadSourceMacroFile(char *macro_folder_path, struct omf_segment *current_omfsegment)
{
  int i;
  struct source_file *first_file;
  struct source_line *current_line;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /*** Passe toutes les lignes en revue ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* Ligne déjà connue */
      if(current_line->type != LINE_UNKNOWN)
        continue;

      /** On recherche les lignes USE **/
      if(!my_stricmp(current_line->opcode_txt,"USE") && strlen(current_line->operand_txt) > 0)
        {
          /* Ce fichier était peut être un fichier contenant du code */
          if(!IsMacroFile(current_line->operand_txt,param->source_folder_path,macro_folder_path))
            continue;

          /* On va extraire le nom du fichier : 4/Locator.Macs => Locator.Macs.s */
          for(i=(int)strlen(current_line->operand_txt); i>=0; i--)
            if(current_line->operand_txt[i] == '/' || current_line->operand_txt[i] == ':')
              break;
          strcpy(param->buffer_file_name,&current_line->operand_txt[i+1]);

          /* Ajoute le .s final */
          if(my_stricmp(&param->buffer_file_name[strlen(param->buffer_file_name)-2],".s"))
            strcat(param->buffer_file_name,".s");

          /** Charge le fichier Macro **/
          printf("        - %s\n",param->buffer_file_name);
          LoadOneMacroFile(macro_folder_path,param->buffer_file_name,current_line,current_omfsegment);
        }
    }
}


/*********************************************************/
/*  LoadOneMacroFile() :  Chargement d'un fichier Macro. */
/*********************************************************/
void LoadOneMacroFile(char *folder_path, char *file_name, struct source_line *macro_line, struct omf_segment *current_omfsegment)
{
  int data_size, macro_level, line_number;
  char *data;
  char *begin_line;
  char *end_line;
  char *next_line;
  struct macro *first_macro;
  struct macro *last_macro;
  struct macro *current_macro;
  struct macro_line *current_line;
  struct equivalence *current_equivalence;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
  
  /* Init */
  first_macro = NULL;
  macro_level = 0;

  /* Chemin du fichier */
  sprintf((char *)param->buffer,"%s%s",folder_path,file_name);

  /* Chargement du fichier en mémoire (on recherche dans le dossier Library) */
  data = (char *) LoadTextFileData((char *)param->buffer,&data_size);
  if(data == NULL)
    {
      /* On va regarder dans le dossier des sources */
      sprintf((char *)param->buffer,"%s%s",param->source_folder_path,file_name);
      data = (char *) LoadTextFileData((char *)param->buffer,&data_size);
      if(data == NULL)
        {
          if(macro_line != NULL)
            sprintf(param->buffer_error,"Macro file '%s' not found (Source file '%s', line %d : use %s)",file_name,macro_line->file->file_name,macro_line->file_line_number,macro_line->operand_txt);
          else
            sprintf(param->buffer_error,"Impossible to load Macro file '%s'",param->buffer);
          my_RaiseError(ERROR_RAISE,param->buffer_error);
        }
    }
  my_Memory(MEMORY_DECLARE_ALLOC,data,NULL,current_omfsegment);

  /** Traite toutes les lignes du fichier **/
  line_number = 0;
  begin_line = data;
  while(begin_line)
    {
      /* Fin de la ligne */
      end_line = strchr(begin_line,'\n');
      if(end_line)
        *end_line = '\0';
      next_line = (end_line == NULL) ? NULL : end_line + 1;

      /* Ligne vide */
      if(strlen(begin_line) == 0 || begin_line[0] == ';' || begin_line[0] == '*')
        {
          begin_line = next_line;
          line_number++;
          continue;
        }

      /** On recherche les lignes MAC **/
      DecodeLine(begin_line,param->buffer_label,param->buffer_opcode,param->buffer_operand,param->buffer_comment);
      if(!my_stricmp(param->buffer_opcode,"MAC"))
        {
          /* Macro Level */
          macro_level++;

          /* Nouvelle Macro */
          current_macro = mem_alloc_macro(file_name,param->buffer_label,line_number);
          if(current_macro == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to register macro '%s' from file '%s'",param->buffer_label,file_name);
              mem_free_macro_list(first_macro);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Attache la macro */
          if(first_macro == NULL)
            first_macro = current_macro;
          else
            last_macro->next = current_macro;
          last_macro = current_macro;
        }
      else if((!my_stricmp(param->buffer_opcode,"<<<") || !my_stricmp(param->buffer_opcode,"EOM")) && macro_level > 0)
        {
          /** Si cette ligne contient un Label, on l'intègre dans la macro comme ligne vide **/
          if(strlen(param->buffer_label) > 0)
            {
              for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
                {
                  /* Création de la ligne */
                  current_line = mem_alloc_macroline(param->buffer_label,"","",param->buffer_comment);
                  if(current_line == NULL)
                    {
                      sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,param->buffer_label,param->buffer_opcode,param->buffer_operand);
                      mem_free_macro_list(first_macro);
                      my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }

                  /* Attachement */
                  if(current_macro->first_line == NULL)
                    current_macro->first_line = current_line;
                  else
                    current_macro->last_line->next = current_line;
                  current_macro->last_line = current_line;
                }
            }
            
          /** On va terminer toutes les macros en cours **/
          for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
            my_Memory(MEMORY_ADD_MACRO,current_macro,NULL,current_omfsegment);

          /* Init */
          first_macro = NULL;

          /* Macro Level */
          macro_level = 0;
        }
      else if(macro_level > 0)
        {
          /** Ajoute cette macro_line aux macros enregistrées **/
          for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
            {
              /* Création de la ligne */
              current_line = mem_alloc_macroline(param->buffer_label,param->buffer_opcode,param->buffer_operand,param->buffer_comment);
              if(current_line == NULL)
                {
                  sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,param->buffer_label,param->buffer_opcode,param->buffer_operand);
                  mem_free_macro_list(first_macro);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

              /* Attachement */
              if(current_macro->first_line == NULL)
                current_macro->first_line = current_line;
              else
                current_macro->last_line->next = current_line;
              current_macro->last_line = current_line;
            }
        }
      else
        {
          /** On est à l'extérieur de la définition d'une Macro, il peut y avoir des EQU qui trainent (Util.Macs.s) **/
          if((!my_stricmp(param->buffer_opcode,"=") || !my_stricmp(param->buffer_opcode,"EQU")) && strlen(param->buffer_label) > 0)
            {
              /** Allocation de la structure Equivalence **/
              current_equivalence = (struct equivalence *) calloc(1,sizeof(struct equivalence));
              if(current_equivalence == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure equivalence");
              current_equivalence->name = strdup(param->buffer_label);
              current_equivalence->value = strdup(param->buffer_operand);
              if(current_equivalence->name == NULL || current_equivalence->value == NULL)
                {
                  mem_free_equivalence(current_equivalence);
                  my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure equivalence");
                }

              /* Déclaration de la structure */
              my_Memory(MEMORY_ADD_EQUIVALENCE,current_equivalence,NULL,current_omfsegment);
            }
        }

      /* Ligne suivante */
      begin_line = next_line;
      line_number++;
    }

  /* Libération mémoire */
  my_Memory(MEMORY_FREE_ALLOC,data,NULL,current_omfsegment);
}


/*********************************************************************/
/*  GetMacroFromSource() :  Récupère les macros des fichiers Source. */
/*********************************************************************/
void GetMacroFromSource(struct omf_segment *current_omfsegment)
{
  struct macro *first_macro;
  struct macro *last_macro;
  struct macro *current_macro;
  struct source_file *first_file;
  struct source_line *current_line;
  struct macro_line *current_macroline;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  first_macro = NULL;

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return;

  /*** Passe en revue toutes les lignes ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      if(!my_stricmp(current_line->opcode_txt,"MAC"))
        {
          /* Nouvelle Macro */
          current_macro = mem_alloc_macro(current_line->file->file_name,current_line->label_txt,current_line->file_line_number);
          if(current_macro == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to register macro '%s' from file '%s'",current_line->label_txt,current_line->file->file_name);
              mem_free_macro_list(first_macro);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Attache la macro */
          if(first_macro == NULL)
            first_macro = current_macro;
          else
            last_macro->next = current_macro;
          last_macro = current_macro;

          /* Cette ligne est de type définition de macro */
          current_line->type = LINE_DIRECTIVE;
          current_line->type_aux = LINE_MACRO_DEF;

          /* Il s'agit d'une Macro déclarée dans le Source */
          current_line->is_inside_macro = 1;
        }
      else if(!my_stricmp(current_line->opcode_txt,"<<<") || !my_stricmp(current_line->opcode_txt,"EOM"))
        {
          /** Si cette ligne contient un Label, on l'intègre dans la macro comme ligne vide **/
          if(strlen(current_line->label_txt) > 0)
            {
              for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
                {
                  /* Création de la ligne */
                  current_macroline = mem_alloc_macroline(current_line->label_txt,"","",current_line->comment_txt);
                  if(current_macroline == NULL)
                    {
                      sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,current_line->label_txt,current_line->opcode_txt,current_line->operand_txt);
                      mem_free_macro_list(first_macro);
                      my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }

                  /* Attachement */
                  if(current_macro->first_line == NULL)
                    current_macro->first_line = current_macroline;
                  else
                    current_macro->last_line->next = current_macroline;
                  current_macro->last_line = current_macroline;
                }
            }

          /** On va terminer toutes les macros en cours **/
          for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
            my_Memory(MEMORY_ADD_MACRO,current_macro,NULL,current_omfsegment);

          /* Init */
          first_macro = NULL;

          /* Cette ligne est de type définition de macro */
          current_line->type = LINE_DIRECTIVE;
          current_line->type_aux = LINE_MACRO_DEF;

          /* Il s'agit d'une Macro déclarée dans le Source */
          current_line->is_inside_macro = 1;
        }
      else if(first_macro != NULL)
        {
          /** Ajoute cette macro_line aux macros enregistrées **/
          for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
            {
              /* Création de la ligne */
              current_macroline = mem_alloc_macroline(current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->comment_txt);
              if(current_line == NULL)
                {
                  sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,current_line->label_txt,current_line->opcode_txt,current_line->operand_txt);
                  mem_free_macro_list(first_macro);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

              /* Attachement */
              if(current_macro->first_line == NULL)
                current_macro->first_line = current_macroline;
              else
                current_macro->last_line->next = current_macroline;
              current_macro->last_line = current_macroline;
            }

          /* Cette ligne est de type définition de macro */
          current_line->type = LINE_DIRECTIVE;
          current_line->type_aux = LINE_MACRO_DEF;

          /* Il s'agit d'une Macro déclarée dans le Source */
          current_line->is_inside_macro = 1;
        }
    }
}


/***********************************************************************/
/*  CheckForDuplicatedMacro() :  Recherche de Macro ayant le même nom. */
/***********************************************************************/
void CheckForDuplicatedMacro(struct omf_segment *current_omfsegment)
{
  int i, nb_macro;
  struct macro *previous_macro = NULL;
  struct macro *current_macro;

  /** Passe en revue toutes les Macro **/
  my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
  for(i=1; i<=nb_macro; i++)
    {
      my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);
      if(current_macro != NULL && previous_macro != NULL)
        if(!strcmp(current_macro->name,previous_macro->name))
          printf("    [Warning] Macro : '%s' found in '%s' and '%s'\n",current_macro->name,previous_macro->file_name,current_macro->file_name);

      previous_macro = current_macro;
    }
}


/********************************************************************/
/*  ReplaceMacroWithContent() :  Remplace les Macros par leur code. */
/********************************************************************/
int ReplaceMacroWithContent(struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *first_macro_line; 
  struct source_line *last_macro_line; 
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Passe en revue toutes les lignes ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes invalides */
      if(current_line->is_valid == 0)
        continue;

      /** Recherche les appels de Macro **/
      if(current_line->type == LINE_MACRO)
        {
          /* Construit les lignes de codes de cette Macro en y intégrant les paramètres et en remplaçant les Labels */
          first_macro_line = BuildMacroLine(current_line,current_omfsegment,current_omfproject);
          if(first_macro_line == NULL)
            {
              /* Erreur */
              return(1);
            }
          /* Dernière ligne de la Macro */
          for(last_macro_line = first_macro_line; last_macro_line->next != NULL; last_macro_line=last_macro_line->next)
            ;

          /** Insère les lignes de Code derrière l'appel de la Macro **/
          last_macro_line->next = current_line->next;
          current_line->next = first_macro_line;
          
          /* Ligne suivante */
          current_line = last_macro_line;
        }
    }

  /* OK */
  return(0);
}


/************************************************************************/
/*  BuildMacroLine() :  Création des lignes de code venant d'une Macro. */
/************************************************************************/
static struct source_line *BuildMacroLine(struct source_line *current_source_line, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
  struct macro *current_macro;
  struct source_line *new_source_line = NULL;
  struct source_line *label_source_line = NULL;
  struct source_line *first_source_line = NULL;
  struct source_line *last_source_line = NULL;
  struct source_line *first_source_macro_line;
  struct source_line *last_source_macro_line;
  struct macro_line *current_macro_line = NULL;  
  char *next_sep;
  char *new_operand;
  char *var_list = NULL;
  int i, j, nb_error, nb_var = 0;
  char **var_tab;
  char label_unique[256];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  current_macro = current_source_line->macro;

  /****************************************/
  /**  Extrait les variables de l'appel  **/
  /****************************************/
  /* Init */
  var_tab = (char **) calloc(9,sizeof(char *));
  if(var_tab == NULL)
    {
      sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Build Line]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
  for(i=0; i<9; i++)
    {
      var_tab[i] = (char *) calloc(256,sizeof(char));
      if(var_tab[i] == NULL)
        {
          for(j=0; j<i; j++)
            free(var_tab[j]);
          free(var_tab);
          sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Build Line]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
          my_RaiseError(ERROR_RAISE,param->buffer_error);
        }
    }
      
  /** On se positionne au début des paramètres **/
  if(!my_stricmp(current_source_line->opcode_txt,"PMC") || !my_stricmp(current_source_line->opcode_txt,">>>"))
    {
      /* Le nom de la Macro et les paramètres sont collés dans l'Operande */
      for(i=0; i<(int)strlen(current_source_line->operand_txt); i++)
        if(current_source_line->operand_txt[i] == '.' || current_source_line->operand_txt[i] == '/' || current_source_line->operand_txt[i] == ',' || 
           current_source_line->operand_txt[i] == '-' || current_source_line->operand_txt[i] == '(' || current_source_line->operand_txt[i] == ' ')
          {
            var_list = &current_source_line->operand_txt[i+1];
            break;
          }
    }
  else
    var_list = strlen(current_source_line->operand_txt) == 0 ? NULL : current_source_line->operand_txt;
  
  /** Il n'y a que 8 variables maximum séparées par des ; **/
  while(var_list)
    {
      next_sep = strchr(var_list,';');
      
      /* Dernière entrées */
      if(next_sep == NULL)
        {
          strcpy(var_tab[nb_var+1],var_list);
          nb_var++;
          break;
        }
        
      /* Copie l'entrée */
      memcpy(var_tab[nb_var+1],var_list,next_sep-var_list);
      var_tab[nb_var+1][next_sep-var_list] = '\0';
      nb_var++;
      
      /* On avance */
      var_list += (next_sep-var_list+1);
    }
  sprintf(var_tab[0],"%d",nb_var);

  /*** Duplication des lignes de la Macro ***/
  for(current_macro_line=current_macro->first_line; current_macro_line; current_macro_line=current_macro_line->next)
    {
      /* Création de la ligne Source */
      new_source_line = BuildSourceMacroLine(current_source_line,current_macro_line,nb_var,var_tab,current_omfsegment);
      if(new_source_line == NULL)
        {
          for(i=0; i<9; i++)
            free(var_tab[i]);
          free(var_tab);
          sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Build Line]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
          my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

      /* Attachement de la ligne Source */
      if(first_source_line == NULL)
        first_source_line = new_source_line;
      else
        last_source_line->next = new_source_line;
      last_source_line = new_source_line;
    }
  
  /* Libération mémoire */
  for(i=0; i<9; i++)
    free(var_tab[i]);
  free(var_tab);
  
  /*** On va modifier les Labels Globaux pour obtenir qqchose d'unique ***/
  for(new_source_line=first_source_line; new_source_line; new_source_line=new_source_line->next)
    {
      /* On ne va traiter que les Labels globaux */
      if(strlen(new_source_line->label_txt) > 0 && new_source_line->label_txt[0] != ':' && new_source_line->label_txt[0] != ']')
        {
          /* Création d'un label unique */
          GetUNID(&label_unique[0]);

          /** On passe toutes les lignes en revue **/
          for(label_source_line=first_source_line; label_source_line; label_source_line=label_source_line->next)
            {
              /** Remplace le Label dans l'Operand **/
              new_operand = ReplaceInOperand(label_source_line->operand_txt,new_source_line->label_txt,label_unique,SEPARATOR_REPLACE_VARIABLE,label_source_line);
              if(new_operand != label_source_line->operand_txt)
                {
                  free(label_source_line->operand_txt);
                  label_source_line->operand_txt = new_operand;
                }
            }
          
          /* On le remplace dans la ligne l'ayant définie */
          free(new_source_line->label_txt);
          new_source_line->label_txt = strdup(label_unique);
          if(new_source_line->label_txt == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Update Label]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }
  
  /** On va analyser les nouvelles lignes pour déterminer leurs Types **/
  nb_error = DecodeLineType(first_source_line,current_macro,current_omfsegment,current_omfproject);
  if(nb_error > 0)
    {
      /* Erreur : On efface tout et on sort en erreur */      
      mem_free_sourceline_list(first_source_line);
      return(NULL);
    }
  
  /** On a détecté une macro => Il faut substituer cet appel avec les lignes de code (récursivité) **/
  for(new_source_line=first_source_line; new_source_line; new_source_line=new_source_line->next)
    {
      if(new_source_line->type == LINE_MACRO)
        {
          /** Création des lignes substituées **/
          first_source_macro_line = BuildMacroLine(new_source_line,current_omfsegment,current_omfproject);
          if(first_source_macro_line == NULL)
            {
              /* Erreur : On efface tout et on sort en erreur */      
              mem_free_sourceline_list(first_source_line);
              return(NULL);
            }
          /* On se positionne à la fin */
          for(last_source_macro_line=first_source_macro_line; last_source_macro_line->next != NULL; last_source_macro_line = last_source_macro_line->next)
            ;
          
          /** Insère les lignes substituées à leur place **/
          last_source_macro_line->next = new_source_line->next;
          new_source_line->next = first_source_macro_line;
          
          /* Ligne suivante */
          new_source_line = last_source_macro_line;
        }
    }
    
  /* Renvoi les lignes */
  return(first_source_line);
}


/****************************************************************************************/
/*  BuildSourceMacroLine() :  Construction d'une ligne de Source provenant d'une Macro. */
/****************************************************************************************/
static struct source_line *BuildSourceMacroLine(struct source_line *current_source_line, struct macro_line *current_macro_line, int nb_var, char **var_tab, struct omf_segment *current_omfsegment)
{
  int is_modified;
  struct source_line *new_source_line = NULL;
  char buffer[1024];

  /* Allocation de la nouvelle ligne */
  new_source_line = (struct source_line *) calloc(1,sizeof(struct source_line));
  if(new_source_line == NULL)
    return(NULL);
    
  /** Transfert les caractéristiques de la ligne Source (numéro de la ligne...) **/
  new_source_line->file_line_number = current_source_line->file_line_number;
  new_source_line->file = current_source_line->file;
  new_source_line->type = LINE_UNKNOWN;
  new_source_line->address = -1;
  new_source_line->nb_byte = -1;
  new_source_line->is_valid = 1;           /* la ligne est valide */
  new_source_line->operand_value = 0xFFFFFFFF;
  new_source_line->operand_address_long = 0xFFFFFFFF;
  strcpy(new_source_line->m,"?");
  strcpy(new_source_line->x,"?");
  strcpy(new_source_line->reloc,"         ");

  /** Transfert les éléments de la ligne Macro **/
  BuildSubstituteValue(current_macro_line->label,nb_var,var_tab,buffer);
  new_source_line->label_txt = strdup(buffer);
  BuildSubstituteValue(current_macro_line->opcode,nb_var,var_tab,buffer);
  new_source_line->opcode_txt = strdup(buffer);
  BuildSubstituteValue(current_macro_line->operand,nb_var,var_tab,buffer);
  new_source_line->operand_txt = strdup(buffer);
  BuildSubstituteValue(current_macro_line->comment,nb_var,var_tab,buffer);
  new_source_line->comment_txt = strdup(buffer);
  if(new_source_line->label_txt == NULL || new_source_line->opcode_txt == NULL || new_source_line->operand_txt == NULL || new_source_line->comment_txt == NULL)
    {
      mem_free_sourceline(new_source_line);
      return(NULL);
    }

  /* Remplace les Equivalences sur la Ligne */
  is_modified = ProcessLineEquivalence(new_source_line,current_omfsegment);

  /* Renvoi la ligne */
  return(new_source_line);
}


/***************************************************************************************/
/*  BuildSubstituteValue() :  On remplace les ]x par les valeurs passées en paramètre. */
/***************************************************************************************/
static void BuildSubstituteValue(char *src_string, int nb_var, char **var_tab, char *dst_string_rtn)
{
  int i, j;

  /* On recherche un ] */
  for(i=0,j=0; i<(int)strlen(src_string); i++)
    if(src_string[i] == ']' && (src_string[i+1] >= '0' && src_string[i+1] <= '8'))
      {
        memcpy(&dst_string_rtn[j],var_tab[src_string[i+1]-'0'],strlen(var_tab[src_string[i+1]-'0']));
        j += (int) strlen(var_tab[src_string[i+1]-'0']);
        i++;
      }
    else
      dst_string_rtn[j++] = src_string[i];

  /* Fin de chaine */
  dst_string_rtn[j] = '\0';
}


/******************************************************************/
/*  IsMacroFile() :  Détermine si ce fichier contient des Macros. */
/******************************************************************/
int IsMacroFile(char *file_name, char *source_folder_path, char *macro_folder_path)
{
  int i, found;
  struct source_file *macro_file;
  struct source_line *current_line;
  char file_path[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  found = 0;

  /* On va vite, on regarde le nom */
  if(strlen(file_name) > strlen(".Macs.s"))
    if(!my_stricmp(&file_name[strlen(file_name)-strlen(".Macs.s")],".Macs.s"))
      return(1);
  if(strlen(file_name) > strlen(".Macs"))
    if(!my_stricmp(&file_name[strlen(file_name)-strlen(".Macs")],".Macs"))
      return(1);
      
  /* On essaye d'ouvrir le fichier avec son nom */
  sprintf(file_path,"%s%s",source_folder_path,file_name);
  macro_file = LoadOneSourceFile(file_path,file_name,0);
  if(macro_file == NULL)
    {
      /** On ajoute un .S à la fin **/
      strcat(file_path,".s");
      macro_file = LoadOneSourceFile(file_path,file_name,0);

      /** On va nettoyer le nom du fichier **/
      if(macro_file == NULL)
        {
          /* On va extraire le nom du fichier : 4/Locator.Macs => Locator.Macs.s */
          for(i=(int)strlen(file_name); i>=0; i--)
            if(file_name[i] == '/' || file_name[i] == ':')
              break;
          strcpy(param->buffer_file_name,&file_name[i+1]);

          /* Ajoute le .s final */
          if(my_stricmp(&param->buffer_file_name[strlen(param->buffer_file_name)-2],".s"))
            strcat(param->buffer_file_name,".s");

          /* On essaye d'ouvrir le fichier avec son nom */
          sprintf(file_path,"%s%s",macro_folder_path,param->buffer_file_name);
          macro_file = LoadOneSourceFile(file_path,param->buffer_file_name,0);
        }
    }

  /* On a pas réussi à ouvrir le fichier, on le déclare comme un fichier Macro et on laisse le code suivant le déclarer non disponble */
  if(macro_file == NULL)
    return(1);

  /** On va analyser les lignes afin de trouver du MAC >>> **/
  /* Fichier vide ? */
  if(macro_file->first_line == NULL)
    {
      mem_free_sourcefile(macro_file,1);
      return(1);
    }

  /** On passe toutes les lignes en revue **/
  for(current_line = macro_file->first_line; current_line; current_line = current_line->next)
    {
      /* Commentaire / Vide */
      if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY)
        continue;

      /* Reconnait les Opcode des Macro */
      if(!my_stricmp(current_line->opcode_txt,"MAC"))
          {
            found = 1;
            break;
          }
    }

  /* Libération mémoire */
  mem_free_sourcefile(macro_file,1);

  /* Indique si on a trouvé des Macro */
  return(found);
}


/*******************************************************************/
/*  mem_alloc_macro() :  Allocation mémoire de la structure macro. */
/*******************************************************************/
struct macro *mem_alloc_macro(char *file_name, char *name, int line_number)
{
  struct macro *current_macro;

  /* Allocation mémoire */
  current_macro = (struct macro *) calloc(1,sizeof(struct macro));
  if(current_macro == NULL)
    return(NULL);

  /* Remplissage */
  current_macro->file_line_number = line_number;
  current_macro->file_name = strdup(file_name);
  current_macro->name = strdup(name);
  if(current_macro->file_name == NULL || current_macro->name == NULL)
    {
      mem_free_macro(current_macro);
      return(NULL);
    }

  /* Retourne la structure */
  return(current_macro);
}


/****************************************************************************/
/*  mem_alloc_macroline() :  Allocation mémoire de la structure macro_line. */
/****************************************************************************/
struct macro_line *mem_alloc_macroline(char *label, char *opcode, char *operand, char *comment)
{
  struct macro_line *current_line;

  /* Allocation mémoire */
  current_line = (struct macro_line *) calloc(1,sizeof(struct macro_line));
  if(current_line == NULL)
    return(NULL);

  /* Remplissage */
  current_line->label = strdup(label);
  current_line->opcode = strdup(opcode);
  current_line->operand = strdup(operand);
  current_line->comment = strdup(comment);
  if(current_line->label == NULL || current_line->opcode == NULL || current_line->operand == NULL || current_line->comment == NULL)
    {
      mem_free_macroline(current_line);
      return(NULL);
    }

  /* Retourne la structure */
  return(current_line);
}


/***************************************************************************/
/*  mem_free_macroline() :  Libération mémoire de la structure macro_line. */
/***************************************************************************/
void mem_free_macroline(struct macro_line *current_line)
{
  if(current_line)
    {
      if(current_line->label)
        free(current_line->label);

      if(current_line->opcode)
        free(current_line->opcode);

      if(current_line->operand)
        free(current_line->operand);

      if(current_line->comment)
        free(current_line->comment);

      free(current_line);
    }
}


/******************************************************************/
/*  mem_free_macro() :  Libération mémoire de la structure macro. */
/******************************************************************/
void mem_free_macro(struct macro *current_macro)
{
  struct macro_line *current_line;
  struct macro_line *next_line;

  if(current_macro)
    {
      if(current_macro->name)
        free(current_macro->name);

      if(current_macro->file_name)
        free(current_macro->file_name);

      for(current_line=current_macro->first_line; current_line; )
        {
          next_line = current_line->next;
          mem_free_macroline(current_line);
          current_line = next_line;
        }

      free(current_macro);
    }
}


/**********************************************************************/
/*  mem_free_macro_list() :  Libération mémoire des structures macro. */
/**********************************************************************/
void mem_free_macro_list(struct macro *all_macro)
{
  struct macro *current_macro;
  struct macro *next_macro;

  for(current_macro=all_macro; current_macro; )
    {
      next_macro = current_macro->next;
      mem_free_macro(current_macro);
      current_macro = next_macro;
    }
}

/***********************************************************************/
