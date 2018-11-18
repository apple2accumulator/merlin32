/***********************************************************************/
/*                                                                     */
/*  a65816_Cond.c : Module pour la gestion des Conditional.            */
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
#include "a65816_Cond.h"

/****************************************************************************/
/*  ProcessConditionalDirective() :  Traite les lignes avec des conditions. */
/****************************************************************************/
int ProcessConditionalDirective(struct omf_segment *current_omfsegment)
{
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  int64_t value_64;
  int value, level, is_reloc;
  struct source_file *first_file;
  struct source_line *begin_line;
  struct source_line *end_line;
  struct source_line *current_line;
  struct source_line *next_line;
  struct external *current_external;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  level = 0;

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Passe en revue toutes les lignes pour calculer le niveau des IF/ELSE/FIN DO/ELSE/FIN ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ne va ignorer les lignes invalides et les Macros définies dans le fichier Source */
      if(current_line->is_valid == 0 || current_line->is_inside_macro == 1)
        continue;

      /** On va devoir gérer les variables en dehors de Lup pour calculer leur valeur **/
      if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"DO") || !my_stricmp(current_line->opcode_txt,"IF")))
        {
          /* Augmente le niveau */
          current_line->cond_level = level;
          level++;
        }
      else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"ELSE"))
        {
          /* Erreur */
          if(level == 0)
            {
              sprintf(param->buffer_error,"Error : Conditional ELSE without IF or DO before (line %d from file '%s')",current_line->file_line_number,current_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* On est dans une condition */
          current_line->cond_level = level-1;
        }
      else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"FIN"))
        {
          /* Erreur */
          if(level == 0)
            printf("        Warning : Conditional FIN without IF or DO before (line %d from file '%s')\n",current_line->file_line_number,current_line->file->file_name);
          else
            {
              /* Baisse le niveau */
              level--;
              current_line->cond_level = level;
            }
        }
      else
        current_line->cond_level = level;
    }

  /* On vérifie qu'on est bien à 0 */
  if(level != 0)
    {
      sprintf(param->buffer_error,"Error : Missing %d FIN conditional in source code",level);
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

  /** On va évaluer les Conditions **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ne va ignorer les lignes invalides */
      if(current_line->is_valid == 0 || current_line->is_inside_macro == 1)
        continue;

      /*** On traite un block de IF/DO ELSE FIN pour déterminer la partie valide ***/
      if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"DO") || !my_stricmp(current_line->opcode_txt,"IF")))
        {
          /* Evaluation du DO */
          if(!my_stricmp(current_line->opcode_txt,"DO"))
            {
              /* On évalue la condition à 0 ou 1 */
              value_64 = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
              if(strlen(buffer_error) > 0)
                {
                  sprintf(param->buffer_error,"Impossible to evaluate DO conditional part '%s' (line %d from file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

              /* Conversion en 0 / 1 */
              value = (value_64 == 0) ? 0 : 1;
            }
          else if(!my_stricmp(current_line->opcode_txt,"IF"))
            {
              /* MX */
              if(!my_strnicmp(current_line->operand_txt,"MX",2))
                {
                  /* Evaluation de l'expression */
                  value_64 = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
                  if(strlen(buffer_error) > 0)
                    {
                      sprintf(param->buffer_error,"Impossible to evaluate IF conditional part '%s' (line %d from file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                      my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }
                  /* Conversion en 0 / 1 */
                  value = (value_64 == 0) ? 0 : 1;
                }
              else  /* First character */
                {
                  /* Evaluation de l'expression */
                  value = 0;
                  if(strlen(current_line->operand_txt) >= 3)
                    if(current_line->operand_txt[0] == current_line->operand_txt[2])
                      value = 1;
                } 
            }

          /** On va marquer les lignes suivantes **/
          current_line->is_valid = 1;                  /* La ligne IF/DO est valide */
          for(next_line=current_line->next; next_line; next_line=next_line->next)
            {
              if(next_line->type == LINE_DIRECTIVE && !my_stricmp(next_line->opcode_txt,"ELSE") && next_line->cond_level == current_line->cond_level)
                {
                  /* Inverse */
                  value = (value == 0) ? 1 : 0;
                  next_line->is_valid = 1;             /* La ligne ELSE du IF/DO est valide */
                }
              else if(next_line->type == LINE_DIRECTIVE && !my_stricmp(next_line->opcode_txt,"FIN") && next_line->cond_level == current_line->cond_level)
                {
                  /* Fin du block */
                  next_line->is_valid = 1;             /* La ligne FIN du IF/DO est valide */
                  break;
                }
              else
                next_line->is_valid = value;           /* La validité des lignes à l'intérieur dépend de l'évaluation de la condition */
            }

          /* On va passer à la condition suivante (on va évaluer les IF présent dans la partie valide de ce IF) */
        }
      else
        current_line->is_valid = 1;                    /* Par défaut les lignes sont valides */
    }

  /** On va marquer les zones Lup comme non valides **/
  for(begin_line=first_file->first_line; begin_line; begin_line=begin_line->next)
    {
      /* On ignore les lignes invalides */
      if(begin_line->is_valid == 0 || begin_line->is_inside_macro == 1)
        continue;

      if(begin_line->type == LINE_DIRECTIVE && !my_stricmp(begin_line->opcode_txt,"LUP"))
        {
          /** Recherche la fin de la Lup **/
          for(end_line=begin_line->next; end_line; end_line=end_line->next)
            if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"--^"))
              break;
            else if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"LUP"))
              {
                /* Erreur : On commence une nouvelle alors que la précédente n'est pas terminé */
                sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
              }

          /* Rien trouvé ? */
          if(end_line == NULL)
            {
              sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /** Marques les lignes de la Lup comme invalides (sauf celles présentes dans les Macros du Code) **/
          for(current_line=begin_line; current_line != end_line->next; current_line=current_line->next)
            current_line->is_valid = 0;

          /* On continue à la fin de la zone */
          begin_line = end_line;
        }
    }

  /* OK */
  return(0);
}

/***********************************************************************/