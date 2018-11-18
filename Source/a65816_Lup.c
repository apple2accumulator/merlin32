/***********************************************************************/
/*                                                                     */
/*  a65816_Lup.c : Module pour la gestion des Lup.                     */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <sys/timeb.h>

#include "Dc_Library.h"
#include "a65816_File.h"
#include "a65816_Line.h"
#include "a65816_Lup.h"

static struct source_line *BuildLupLine(struct source_line *,struct source_line *,struct omf_segment *);
static struct source_line *BuildSourceLupOneIterationLine(struct source_line *,struct source_line *,int,struct omf_segment *);
static struct source_line *BuildSourceLupLine(struct source_line *,int,struct omf_segment *);

/************************************************************/
/*  ReplaceLupWithCode() :  Remplace les Lup par leur code. */
/************************************************************/
int ReplaceLupWithCode(struct omf_segment *current_omfsegment)
{
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  int is_reloc;
  int64_t value;
  struct source_file *first_file;
  struct source_line *begin_line;
  struct source_line *end_line;
  struct source_line *first_lup_line; 
  struct source_line *last_lup_line;
  struct variable *current_variable;
  struct external *current_external;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Passe en revue toutes les lignes ***/
  for(begin_line=first_file->first_line; begin_line; begin_line=begin_line->next)
    {
      /* On ignore les lignes non valides */
      if(begin_line->is_valid == 0)
        continue;

      /** On va devoir gérer les variables en dehors de Lup pour calculer leur valeur **/
      if(begin_line->type == LINE_VARIABLE)
        {
          /* Récupère la variable */
          my_Memory(MEMORY_SEARCH_VARIABLE,begin_line->label_txt,&current_variable,current_omfsegment);
          if(current_variable == NULL)
            {
              /* Erreur : On commence une nouvelle alors que la précédente n'est pas terminé */
              sprintf(param->buffer_error,"Impossible to locate Variable '%s' (line %d from file '%s')",begin_line->label_txt,begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Calcule sa nouvelle valeur */
          value = EvalExpressionAsInteger(begin_line->operand_txt,buffer_error,begin_line,begin_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              /* Erreur : On commence une nouvelle alors que la précédente n'est pas terminé */
              sprintf(param->buffer_error,"Impossible to evaluate Variable '%s' value '%s' (line %d from file '%s')",begin_line->label_txt,begin_line->operand_txt,begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Stocke la valeur */
          current_variable->value = value;
        }

      /** Recherche les appels de Lup (dans le code, pas dans le corps des Macro présentes dans le code) **/
      if(begin_line->type == LINE_DIRECTIVE && begin_line->is_inside_macro == 0 && !my_stricmp(begin_line->opcode_txt,"LUP"))
        {
          /** Recherche la fin de la Lup **/
          for(end_line=begin_line->next; end_line; end_line=end_line->next)
            {
              /* On ignore les lignes non valides */
              if(end_line->is_valid == 0)
                continue;

              if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"--^"))
                break;
              else if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"LUP"))
                {
                  /* Erreur : On commence une nouvelle alors que la précédente n'est pas terminé */
                  sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
            }

          /* Rien trouvé ? */
          if(end_line == NULL)
            {
              sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /** Construit les lignes de codes de cette Lup (on remplace les labels par des nouveaux) **/
          first_lup_line = BuildLupLine(begin_line,end_line,current_omfsegment);
          if(first_lup_line == NULL)
            {
              /* Rien à insérer : Ligne suivante */
              begin_line = end_line;
            }
          else
            {
              /* Dernière ligne de la Lup */
              for(last_lup_line = first_lup_line; last_lup_line->next != NULL; last_lup_line=last_lup_line->next)
                ;

              /** Insère les lignes de Code derrière la fin de la Lup **/
              last_lup_line->next = end_line->next;
              end_line->next = first_lup_line;
          
              /* Ligne suivante */
              begin_line = last_lup_line;
            }
        }
    }

  /* OK */
  return(0);
}


/*****************************************************************/
/*  BuildLupLine() :  Construction des lignes de code de la Lup. */
/*****************************************************************/
static struct source_line *BuildLupLine(struct source_line *begin_line, struct source_line *end_line, struct omf_segment *current_omfsegment)
{
  int64_t nb_iter_64;
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  char *new_label;
  int i, nb_iter, is_reloc;
  struct source_line *first_line = NULL;
  struct source_line *last_line = NULL;
  struct source_line *first_new_line;
  struct source_line *last_new_line;
  struct source_line *label_line;
  struct source_line *current_line;
  struct external *current_external;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /** Décodage du nombre de tour **/
  if(strlen(begin_line->operand_txt) == 0)
    nb_iter_64 = 0;            /* On a un PB car on a un LUP mais sans Nb, on va donc considérer qu'on LOOP 0 fois (arrive dans le cas de Macro avec paramètres variables) */
  else
    {
      nb_iter_64 = EvalExpressionAsInteger(begin_line->operand_txt,buffer_error,begin_line,begin_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
      if(strlen(buffer_error) > 0)
        {
          sprintf(param->buffer_error,"Impossible to get Lup value '%s' (line %d from file '%s') : %s",begin_line->operand_txt,begin_line->file_line_number,begin_line->file->file_name,buffer_error);
          my_RaiseError(ERROR_RAISE,param->buffer_error);
        }
    }

  /* Il n'y aura pas d'itération */
  if(nb_iter_64 <= 0 || nb_iter_64 > 0x8000)
    {
      /* Aucun Label à gérer, on sort */
      if(strlen(begin_line->label_txt) == 0 && strlen(end_line->label_txt) == 0)
        return(NULL);

      /* On va devoir créer des Lignes vides pour héberger les Labels */
      if(strlen(begin_line->label_txt) > 0)
        {
          /* On va devoir créer une ligne vide pour héberger le label du début */
          label_line = BuildEmptyLabelLine(begin_line->label_txt,begin_line);
          if(label_line == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Ajoute la ligne */
          first_line = label_line;
          last_line = label_line;
        }
      if(strlen(end_line->label_txt) > 0)
        {
          /* On va devoir créer une ligne vide pour héberger le label de fin */
          label_line = BuildEmptyLabelLine(end_line->label_txt,end_line);
          if(label_line == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",end_line->file_line_number,end_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Met la ligne contenant le label de fin en deuxième position */
          if(first_line == NULL)
            first_line = label_line;
          else
            last_line->next = label_line;
          last_line = label_line;
        }

      /* Renvoi les lignes Label */
      return(first_line);
    }
  nb_iter = (int) nb_iter_64;

  /*** On va produire les X itérations de code ***/
  for(i=0; i<nb_iter; i++)
    {
      /** Produit les lignes d'une itération + Changement des libellés **/
      first_new_line = BuildSourceLupOneIterationLine(begin_line,end_line,i+1,current_omfsegment);
      if(first_new_line == NULL)
        break;
      /* Dernière ligne */
      for(current_line=first_new_line; current_line; current_line=current_line->next)
        if(current_line->next == NULL)
          last_new_line = current_line;

      /* Attache les lignes aux précédentes */
      if(first_line == NULL)
        first_line = first_new_line;
      else
        last_line->next = first_new_line;
      last_line = last_new_line;
    }

  /** Il y a un label sur la ligne du LUP **/
  if(strlen(begin_line->label_txt) > 0)
    {
      /* On essaye de le placer sur la 1ère ligne des lignes de substitution */
      if(strlen(first_line->label_txt) == 0)
        {
          new_label = strdup(begin_line->label_txt);
          if(new_label == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
          free(first_line->label_txt);
          first_line->label_txt = new_label;
        }
      else
        {
          /* On va devoir créer une ligne vide pour héberger le label */
          label_line = BuildEmptyLabelLine(begin_line->label_txt,first_line);
          if(label_line == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Met la ligne avant toutes les autres */
          label_line->next = first_line;
          first_line = label_line;
        }
    }

  /** Il y a un label sur la ligne du --^ **/
  if(strlen(end_line->label_txt) > 0)
    {
      /* On essaye de le placer sur la dernière ligne des lignes de substitution */
      if(strlen(last_line->label_txt) == 0)
        {
          new_label = strdup(end_line->label_txt);
          if(new_label == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
          free(last_line->label_txt);
          last_line->label_txt = new_label;
        }
      else
        {
          /* On va devoir créer une ligne vide pour héberger le label */
          label_line = BuildEmptyLabelLine(end_line->label_txt,last_line);
          if(label_line == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",end_line->file_line_number,end_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Met la ligne après toutes les autres */
          last_line->next = label_line;
          last_line = label_line;
        }
    }

  /* Renvoi la liste des instructions générées */
  return(first_line);
}


/********************************************************************************************************/
/*  BuildSourceLupOneIterationLine() :  Construction des ligne de Source provenant d'une boucle de Lup. */
/********************************************************************************************************/
static struct source_line *BuildSourceLupOneIterationLine(struct source_line *begin_line, struct source_line *end_line, int iter, struct omf_segment *current_omfsegment)
{
  int is_reloc;
  int64_t value;
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  char *new_operand;
  struct source_line *current_line;
  struct source_line *new_line;
  struct source_line *first_line = NULL;
  struct source_line *last_line = NULL;
  struct source_line *label_line;
  struct variable *current_variable;
  struct external *current_external;
  char label_unique[512];
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /*** La première ligne est le LUP, la dernière est le --^ ***/
  for(current_line = begin_line->next; current_line != end_line; current_line = current_line->next)
    {
      /** Ligne de code / data / directive (on exclu les Global) **/
      if(current_line->type == LINE_CODE || current_line->type == LINE_EMPTY || current_line->type == LINE_DATA || 
         current_line->type == LINE_DIRECTIVE || current_line->type == LINE_MACRO)
        {
          /** On va dupliquer la ligne en adaptant le Label et l'Operande **/
          new_line = BuildSourceLupLine(current_line,iter,current_omfsegment);                   /* iter : 1 -> nb_iter */

          /* Ajoute cette ligne à la liste */
          if(first_line == NULL)
            first_line = new_line;
          else
            last_line->next = new_line;
          last_line = new_line;
        }
      /** Variable **/
      else if(current_line->type == LINE_VARIABLE)
        {
          /** Met à jour la valeur de la variable **/
          /* Récupère la variable */
          my_Memory(MEMORY_SEARCH_VARIABLE,current_line->label_txt,&current_variable,current_omfsegment);
          if(current_variable == NULL)
            {
              sprintf(param->buffer_error,"Impossible to find Variable '%s' declaration (line %d from file '%s') : %s",current_line->label_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Calcule sa nouvelle valeur */
          value = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Impossible to evaluate Variable '%s' value '%s' (line %d from file '%s') : %s",current_line->label_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Stocke la valeur */
          current_variable->value = value;
        }
    }

  /*** On va modifier les Labels Globaux pour obtenir qqchose d'unique ***/
  for(current_line=first_line; current_line; current_line=current_line->next)
    {
      /* On ne va traiter que les Labels globaux */
      if(strlen(current_line->label_txt) > 0 && current_line->label_txt[0] != ':' && current_line->label_txt[0] != ']')
        {
          /* Création d'un label unique */
          GetUNID(&label_unique[0]);

          /** On passe toutes les lignes en revue **/
          for(label_line=first_line; label_line; label_line=label_line->next)
            {
              /** Remplace le Label dans l'Operand **/
              new_operand = ReplaceInOperand(label_line->operand_txt,current_line->label_txt,label_unique,SEPARATOR_REPLACE_VARIABLE,label_line);
              if(new_operand != label_line->operand_txt)
                {
                  free(label_line->operand_txt);
                  label_line->operand_txt = new_operand;
                }
            }
          
          /* On le remplace dans la ligne l'ayant définie */
          free(current_line->label_txt);
          current_line->label_txt = strdup(label_unique);
          if(current_line->label_txt == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to replace Lup at line %d from file '%s' [Update Label]",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }

  /* Renvoie cet ensemble de lignes */
  return(first_line);
}


/************************************************************************************/
/*  BuildSourceLupLine() :  Construction d'une ligne de Source provenant d'une Lup. */
/************************************************************************************/
static struct source_line *BuildSourceLupLine(struct source_line *current_source_line, int iter, struct omf_segment *current_omfsegment)
{
  int i, j, k, l;
  struct source_line *new_source_line = NULL;
  char new_operand_txt[1024];
  char value_txt[256];
  char variable_name[1024];
  struct variable *current_variable;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Allocation de la nouvelle ligne */
  new_source_line = (struct source_line *) calloc(1,sizeof(struct source_line));
  if(new_source_line == NULL)
    return(NULL);
    
  /** Transfert les caractéristiques de la ligne Source (numéro de la ligne...) **/
  new_source_line->file_line_number = current_source_line->file_line_number;
  new_source_line->file = current_source_line->file;
  new_source_line->type = current_source_line->type;
  new_source_line->no_direct_page = current_source_line->no_direct_page;
  new_source_line->address = -1;
  new_source_line->nb_byte = -1;
  new_source_line->is_valid = 1;           /* la ligne est valide */
  strcpy(new_source_line->m,"?");
  strcpy(new_source_line->x,"?");
  strcpy(new_source_line->reloc,"         ");
  new_source_line->operand_value = 0xFFFFFFFF;
  new_source_line->operand_address_long = 0xFFFFFFFF;
  new_source_line->macro = current_source_line->macro;

  /** On duplique les premiers éléments **/
  new_source_line->label_txt = strdup(current_source_line->label_txt);
  new_source_line->opcode_txt = strdup(current_source_line->opcode_txt);
  new_source_line->comment_txt = strdup(current_source_line->comment_txt);
  if(new_source_line->label_txt == NULL || new_source_line->opcode_txt == NULL || new_source_line->comment_txt == NULL)
    {
      mem_free_sourceline(new_source_line);
      sprintf(param->buffer_error,"Impossible to allocate memory to process Lup at line %d from file '%s'",current_source_line->file_line_number,current_source_line->file->file_name);
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

  /** On modifie le Label : @ -> ASC iter **/
  for(i=0; i<(int)strlen(new_source_line->label_txt); i++)
    if(new_source_line->label_txt[i] == '@')
      {
        if(iter > 26)
          {
            mem_free_sourceline(new_source_line);
            sprintf(param->buffer_error,"Impossible to update Label '%s' (iter=%d > 26) in Lup at line %d from file '%s'",current_source_line->label_txt,iter+1,current_source_line->file_line_number,current_source_line->file->file_name);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
          }
        else
          new_source_line->label_txt[i] = 'A' + iter - 1;    /* A-Z */
      }

  /** On effectue les remplacements de variables dans l'Operande **/
  for(i=0,l=0; i<(int)strlen(current_source_line->operand_txt); i++)
    {
      if(current_source_line->operand_txt[i] == ']')
        {
          /* Isole le nom de la variable */
          variable_name[0] = ']';
          for(j=i+1,k=1; j<(int)strlen(current_source_line->operand_txt); j++)
            if(IsSeparator(current_source_line->operand_txt[j],0))
              break;
            else
              variable_name[k++] = current_source_line->operand_txt[j];
          variable_name[k] = '\0';

          /* Recherche la variable */
          my_Memory(MEMORY_SEARCH_VARIABLE,variable_name,&current_variable,current_omfsegment);
          if(current_variable == NULL)
            {
              mem_free_sourceline(new_source_line);
              sprintf(param->buffer_error,"Impossible to find Lup Variable '%s' declaration (line %d from file '%s')",current_source_line->label_txt,current_source_line->file_line_number,current_source_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Place la valeur sous la bonne forme : Les valeurs des variables sont limitées à 32 bits */
          if(current_variable->is_dollar)
            sprintf(value_txt,"$%X",(int)current_variable->value);
          else if(current_variable->is_pound_dollar)
            sprintf(value_txt,"#$%X",(int)current_variable->value);
          else if(current_variable->is_pound)
            sprintf(value_txt,"#%d",(int)current_variable->value);
          else
            sprintf(value_txt,"%d",(int)current_variable->value);
          strcpy(&new_operand_txt[l],value_txt);
          l += (int) strlen(value_txt);

          /* On continue d'explorer l'operand */
          i += (int) (strlen(variable_name) - 1);   /* i++ */
        }
      else
        new_operand_txt[l++] = current_source_line->operand_txt[i];
    }
  new_operand_txt[l] = '\0';

  /* Allocation mémoire */
  new_source_line->operand_txt = strdup(new_operand_txt);
  if(new_source_line->operand_txt == NULL)
    {
      mem_free_sourceline(new_source_line);
      sprintf(param->buffer_error,"Impossible to allocate memory to process Lup at line %d from file '%s'",current_source_line->file_line_number,current_source_line->file->file_name);
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

  /* Renvoi la ligne */
  return(new_source_line);
}

/***********************************************************************/
