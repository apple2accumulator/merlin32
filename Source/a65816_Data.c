/***********************************************************************/
/*                                                                     */
/*  a65816_Code.c : Module pour la génération du code objet.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "Dc_Library.h"
#include "a65816_Line.h"
#include "a65816_File.h"
#include "a65816_Macro.h"
#include "a65816_Data.h"


static void BuildOneDataLineSize(struct source_line *,char *,struct omf_segment *);
static void BuildOneDataLineOperand(struct source_line *,char *,struct omf_segment *);

/*************************************************************************/
/*  BuildAllDataLineSize() :  Calcule de la taille pour les lignes Data. */
/*************************************************************************/
int BuildAllDataLineSize(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Passe en revue toutes les lignes Data pour calculer leur taille ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* Cette ligne n'est pas valide */
      if(current_line->is_valid == 0)
        {
          current_line->nb_byte = 0;
          continue;
        }

      /** Lignes de Data **/
      if(current_line->type == LINE_DATA)
        {
          /** Détermine la taille de Operand **/
          BuildOneDataLineSize(current_line,buffer_error,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Impossible to decode Data format for instruction '%s  %s' (line %d, file '%s') : %s",
                      current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /** Allocation mémoire pour les Data **/
          current_line->data = (unsigned char *) calloc(current_line->nb_byte+1,sizeof(unsigned char));
          if(current_line->data == NULL)
            {
              sprintf(param->buffer_error,"Impossible to allocate memory to store Data line '%s  %s' (line %d, file '%s')",
                      current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }

  /* OK */
  return(0);
}


/*************************************************************************/
/*  BuildAllDataLineSize() :  Calcule de la taille pour les lignes Data. */
/*************************************************************************/
int BuildAllDataLine(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Passe en revue toutes les lignes Data pour créer l'Operand ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* Cette ligne n'est pas valide */
      if(current_line->is_valid == 0 || current_line->is_dum == 1)
        continue;

      /** Lignes de Data **/
      if(current_line->type == LINE_DATA)
        {
          /** Création des données de l'Operand **/
          BuildOneDataLineOperand(current_line,buffer_error,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Impossible to build Data line '%s  %s' (line %d, file '%s') : %s",
                      current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
      else if(current_line->type == LINE_VARIABLE)
        {
          /** Evaluation de la variable **/
          EvaluateVariableLine(current_line,current_omfsegment);
        }
    }

  /* OK */
  return(0);
}


/********************************************************************/
/*  BuildOneDataLineSize() :  Détermine la taille d'une ligne DATA. */
/********************************************************************/
static void BuildOneDataLineSize(struct source_line *current_line, char *buffer_error_rtn, struct omf_segment *current_omfsegment)
{
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  int i, nb_element, nb_valid_element, nb_byte, nb_nibble, length, is_reloc;
  char *next_char;
  char **tab_element;
  struct external *current_external;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  strcpy(buffer_error_rtn,"");

  /*** On va reconnaitre les différents types de Data ***/
  if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB") ||
     !my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB") ||
     !my_stricmp(current_line->opcode_txt,"ADR") || !my_stricmp(current_line->opcode_txt,"ADRL"))
    {
      /* Découpe en plusieurs éléments */
      tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
      if(tab_element == NULL)
        {
          sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
          return;
        }

      /** On va analyser les éléments **/
      for(i=0,nb_valid_element=0; i<nb_element; i++)
        if(strlen(tab_element[i]) == 0)
          {
            mem_free_table(nb_element,tab_element);
            sprintf(buffer_error_rtn,"Empty Data in Operand '%s'",current_line->operand_txt);
            return;
          }
        else if(!strcmp(tab_element[i],","))
          ;                     /* On ne fait rien */
        else
          nb_valid_element++;   /* On ne comptabilise que les éléments valides */

      /** Taille de la partie Data **/
      if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB"))
        current_line->nb_byte = 2*nb_valid_element;
      else if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
        current_line->nb_byte = nb_valid_element;
      else if(!my_stricmp(current_line->opcode_txt,"ADR"))
        current_line->nb_byte = 3*nb_valid_element;
      else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
        current_line->nb_byte = 4*nb_valid_element;

      /* Libération mémoire */
      mem_free_table(nb_element,tab_element);
    }
  else if(!my_stricmp(current_line->opcode_txt,"HEX"))
    {
      /** Compte le nombre de caractère / valide les données **/
      for(i=0,nb_byte=0,nb_nibble=0; i<(int)strlen(current_line->operand_txt); i++)
        {
          if((current_line->operand_txt[i] >= '0' && current_line->operand_txt[i] <= '9') || (toupper(current_line->operand_txt[i]) >= 'A' && toupper(current_line->operand_txt[i]) <= 'F'))
            {
              nb_nibble++;
              if(nb_nibble == 2)
                {
                  nb_byte++;
                  nb_nibble = 0;
                }
            }
          else if(current_line->operand_txt[i] == ',')
            {
              if(nb_nibble == 1)
                {
                  sprintf(buffer_error_rtn,"Wrong Hex Format Data in Operand '%s'",current_line->operand_txt);
                  return;
                }
            }
          else
            {
              sprintf(buffer_error_rtn,"Wrong Hex Format Data in Operand '%s'",current_line->operand_txt);
              return;
            }
        }
      if(nb_nibble == 1)
        {
          sprintf(buffer_error_rtn,"Wrong Hex Format Data in Operand '%s'",current_line->operand_txt);
          return;
        }

      /** Taille de la partie Data **/
      current_line->nb_byte = nb_byte;
    }
  else if(!my_stricmp(current_line->opcode_txt,"DS"))
    {
      /* Découpe en plusieurs éléments */
      tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
      if(tab_element == NULL)
        {
          sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
          return;
        }
      /* DS 2 ou DS 2,FF */
      if(nb_element != 1 && nb_element != 3)
        {
          mem_free_table(nb_element,tab_element);
          sprintf(buffer_error_rtn,"Wrong DS Format Data in Operand '%s'",current_line->operand_txt);
          return;
        }

      /** Cas particulier du \ **/;
      if(!strcmp(tab_element[0],"\\"))
        {
          /** Taille de la partie Data => il faudra complèter jusqu'à la Page suivante **/
          current_line->nb_byte = 0xFFFF;   /* 4 F */
        }
      else
        {
          /* On évalue comme entier la première valeur */
          nb_byte = (int) EvalExpressionAsInteger(tab_element[0],buffer_error,current_line,1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0 && nb_byte == 0xFFFF)
            {
              /** Taille de la partie Data => il faudra refaire l'évaluation plus tard lors du calcul des adresses **/
              current_line->nb_byte = 0xFFFFF;  /* 5 F */
            }
          else if(strlen(buffer_error) > 0)
            {
              mem_free_table(nb_element,tab_element);
              sprintf(buffer_error_rtn,"Wrong DS Format Data in Operand '%s' (%s)",current_line->operand_txt,buffer_error);
              return;
            }
          else if(nb_byte < 0)
            {
              mem_free_table(nb_element,tab_element);
              sprintf(buffer_error_rtn,"Wrong DS Value in Operand '%s' : Negative value (%d)",current_line->operand_txt,nb_byte);
              return;
            }
          else
            {
              /** Taille de la partie Data **/
              current_line->nb_byte = nb_byte;
            }
        }
    }
  /**************************/
  /** Chaine de caractères **/
  /**************************/
  else if(!my_stricmp(current_line->opcode_txt,"ASC") || !my_stricmp(current_line->opcode_txt,"DCI") || !my_stricmp(current_line->opcode_txt,"INV") || !my_stricmp(current_line->opcode_txt,"FLS") ||
          !my_stricmp(current_line->opcode_txt,"STR") || !my_stricmp(current_line->opcode_txt,"STRL"))
    {
      /** Comptabilise les caractères dans les "" et les Hex à l'extérieur **/
      for(i=0,nb_nibble=0,nb_byte=0; i<(int)strlen(current_line->operand_txt); i++)
        {
          /* Début de zone */
          if(current_line->operand_txt[i] == '\'' || current_line->operand_txt[i] == '"')
            {
              /* Hexa précédent non terminé */
              if(nb_nibble == 1)
                {
                  sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
                  return;
                }

              /* Fin de zone */
              next_char = strchr(&current_line->operand_txt[i+1],current_line->operand_txt[i]);
              if(next_char == NULL)
                {
                  sprintf(buffer_error_rtn,"Wrong String Format Data in Operand '%s' : End-of-String character is missing",current_line->operand_txt);
                  return;
                }

              /* Taille de la zone */
              length = (int) ((next_char - &current_line->operand_txt[i]) - 1);
              nb_byte += length;

              /* On continue */
              i += (length+1);
              continue;
            }
          else if(current_line->operand_txt[i] == ',')
            {
              if(nb_nibble == 1)
                {
                  sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
                  return;
                }
            }
          else if((current_line->operand_txt[i] >= '0' && current_line->operand_txt[i] <= '9') || (toupper(current_line->operand_txt[i]) >= 'A' && toupper(current_line->operand_txt[i]) <= 'F'))
            {
              nb_nibble++;
              if(nb_nibble == 2)
                {
                  nb_byte++;
                  nb_nibble = 0;
                }
            }
        }
      if(nb_nibble == 1)
        {
          sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
          return;
        }

      /** Taille de la partie Data **/
      if(!my_stricmp(current_line->opcode_txt,"STR"))
        {
          if(nb_byte > 255)
            {
              sprintf(buffer_error_rtn,"STR String is too long in Operand '%s'",current_line->operand_txt);
              return;
            }
          current_line->nb_byte = 1 + nb_byte;
        }
      else if(!my_stricmp(current_line->opcode_txt,"STRL"))
        current_line->nb_byte = 2 + nb_byte;
      else
        current_line->nb_byte = nb_byte;
    }
  else if(!my_stricmp(current_line->opcode_txt,"REV"))
    {
      /** Pas de Hex dans la chaine **/
      /* Début de zone */
      if(current_line->operand_txt[0] != '\'' && current_line->operand_txt[0] != '"')
        {
          sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
          return;
        }
      /* Fin de zone */
      if(current_line->operand_txt[strlen(current_line->operand_txt)-1] != current_line->operand_txt[0])
        {
          sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
          return;
        }

      /** Taille de la partie Data **/
      current_line->nb_byte = (int) (strlen(current_line->operand_txt) - 2);
    }
  else if(!my_stricmp(current_line->opcode_txt,"CHK"))
    {
      /* 1 Byte de Checksum */
      current_line->nb_byte = 1;
    }
  else
    {
      /* Data inconnu */
      sprintf(param->buffer_error,"Impossible to decode Data mode for instruction '%s  %s' (line %d, file '%s')",
              current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
}


/************************************************************************************************/
/*  BuildOneDataLineOperand() :  Création des octets de la partie Operand pour les lignes Data. */
/************************************************************************************************/
static void BuildOneDataLineOperand(struct source_line *current_line, char *buffer_error_rtn, struct omf_segment *current_omfsegment)
{
  BYTE one_byte, byte_count, bit_shift;
  WORD one_word, offset_patch, offset_reference;
  DWORD one_dword, address_long;
  int i, j, nb_element, nb_valid_element, nb_byte, nb_nibble, length, value, is_reloc, operand_size, line_address;
  struct relocate_address *current_address_1;
  struct relocate_address *current_address_2;
  struct external *current_external;
  char *next_char;
  char **tab_element;
  unsigned char data[5];
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  strcpy(buffer_error_rtn,"");

  /*** On va reconnaitre les différents types de Data ***/
  if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB") ||
     !my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB") ||
     !my_stricmp(current_line->opcode_txt,"ADR") || !my_stricmp(current_line->opcode_txt,"ADRL"))
    {
      /** Taille de l'Operand **/
      if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
        operand_size = 1;
      else if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB"))
        operand_size = 2;
      else if(!my_stricmp(current_line->opcode_txt,"ADR"))
        operand_size = 3;
      else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
        operand_size = 4;

      /* Découpe en plusieurs éléments */
      tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
      if(tab_element == NULL)
        {
          sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
          return;
        }

      /** On convertir les valeurs **/
      for(i=0,nb_valid_element=0; i<nb_element; i++)
        {
          if(!strcmp(tab_element[i],",") || strlen(tab_element[i]) == 0)
            continue;
          else
            {
              /** Conversion en nombre **/
              one_dword = (DWORD) EvalExpressionAsInteger(tab_element[i],buffer_error,current_line,operand_size,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
              if(strlen(buffer_error) > 0)
                {
                  sprintf(buffer_error_rtn,"Impossible to evaluate %s Data '%s' (%s)",current_line->opcode_txt,tab_element[i],buffer_error);
                  mem_free_table(nb_element,tab_element);
                  return;
                }
                
              /* L'adresse de la ligne tient compte des [ORG $Addr ORG] */
              if(current_line->is_fix_address == 1 && current_line->address != current_line->global_address)
                line_address = current_line->global_address;
              else
                line_address = current_line->address;
                
              /** Découpage en Byte (copie respectant le Byte Order) **/
              bo_memcpy(&data[0],&one_dword,sizeof(DWORD));
              
              /** Stockage dans L'opérande **/
              if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW"))
                {
                  current_line->data[2*nb_valid_element+0] = data[0];
                  current_line->data[2*nb_valid_element+1] = data[1];

                  /* Adresse Relogeable */
                  offset_patch = line_address + 2*nb_valid_element + 0;     /* 2 Bytes */
                }
              else if(!my_stricmp(current_line->opcode_txt,"DDB"))
                {
                  current_line->data[2*nb_valid_element+0] = data[1];
                  current_line->data[2*nb_valid_element+1] = data[0];

                  /* Adresse Relogeable */
                  offset_patch = line_address + 2*nb_valid_element + 0;    /* 2 Bytes */
                }
              else if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
                {
                  current_line->data[nb_valid_element] = data[0];

                  /* Adresse Relogeable */
                  offset_patch = line_address + nb_valid_element + 0;    /* 1 Byte */
                }
              else if(!my_stricmp(current_line->opcode_txt,"ADR"))
                {
                  current_line->data[3*nb_valid_element+0] = data[0];
                  current_line->data[3*nb_valid_element+1] = data[1];
                  current_line->data[3*nb_valid_element+2] = data[2];

                  /* Adresse Relogeable */
                  offset_patch = line_address + 3*nb_valid_element + 0;   /* 3 Bytes */
                }
              else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
                {
                  current_line->data[4*nb_valid_element+0] = data[0];
                  current_line->data[4*nb_valid_element+1] = data[1];
                  current_line->data[4*nb_valid_element+2] = data[2];
                  current_line->data[4*nb_valid_element+3] = data[3];

                  /* Adresse Relogeable */
                  offset_patch = line_address + 4*nb_valid_element + 0;   /* 4 Bytes */
                }

              /** Adresse Relogeable (interne ou externe au Segment) **/
              if(is_reloc)
                {
                  /* Cas particulier du DDB qui reloge 2 adresses sur 1 byte, plutot qu'une adresse sur 2 bytes ! */
                  if(!my_stricmp(current_line->opcode_txt,"DDB"))
                    {
                      /* Adresse 1 : >> 8 */
                      current_address_1 = BuildRelocateAddress(1,0xF8,offset_patch,offset_reference,current_external,current_omfsegment);
                      current_address_1->object_line = &current_line->data[2*nb_valid_element+0];

                      /* Adresse 2 */
                      current_address_2 = BuildRelocateAddress(1,0,offset_patch+1,offset_reference,current_external,current_omfsegment);
                      current_address_2->object_line = &current_line->data[2*nb_valid_element+1];

                      /* Information pour le fichier Output.txt (on n'indique qu'un seul relogeage) */
                      sprintf(current_line->reloc,"%c 1 >> 8 ",(current_external == NULL)?' ':'E');
                    }
                  else
                    {
                      /* 1 seule adresse à reloger */
                      current_address_1 = BuildRelocateAddress(byte_count,bit_shift,offset_patch,offset_reference,current_external,current_omfsegment);

                      /* Adresse à pacther dans la line */
                      if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
                        current_address_1->object_line = &current_line->data[1*nb_valid_element+0];                    /* 1 byte */
                      else if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW"))
                        current_address_1->object_line = &current_line->data[2*nb_valid_element+0];                    /* 2 bytes */
                      else if(!my_stricmp(current_line->opcode_txt,"ADR"))
                        current_address_1->object_line = &current_line->data[3*nb_valid_element+0];                    /* 3 bytes */
                      else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
                        current_address_1->object_line = &current_line->data[4*nb_valid_element+0];                    /* 4 bytes */

                      /* Version Texte pour le Output.txt */
                      sprintf(current_line->reloc,"%c %d",(current_external == NULL)?' ':'E',byte_count);
                      if(bit_shift == 0xF0)
                        strcat(current_line->reloc," >>16 ");
                      else if(bit_shift == 0xF8)
                        strcat(current_line->reloc," >> 8 ");
                      else
                        strcat(current_line->reloc,"      ");
                    }
                }

              /* Element suivant */
              nb_valid_element++;
            }
        }
        
      /* Libération mémoire */
      mem_free_table(nb_element,tab_element);
    }
  else if(!my_stricmp(current_line->opcode_txt,"HEX"))
    {
      /** Conversion de l'Hexadécimal **/
      for(i=0,nb_byte=0,nb_nibble=0; i<(int)strlen(current_line->operand_txt); i++)
        {
          if((current_line->operand_txt[i] >= '0' && current_line->operand_txt[i] <= '9') || (toupper(current_line->operand_txt[i]) >= 'A' && toupper(current_line->operand_txt[i]) <= 'F'))
            {
              nb_nibble++;
              if(nb_nibble == 2)
                {
                  data[1] = current_line->operand_txt[i];
                  data[2] = '\0';                  
                  sscanf((char *)data,"%X",&value);
                  current_line->data[nb_byte++] = (unsigned char) value;
                  nb_nibble = 0;
                }
              else
                data[0] = current_line->operand_txt[i];
            }
        }
    }
  else if(!my_stricmp(current_line->opcode_txt,"DS"))
    {
      /* Découpe en plusieurs éléments */
      tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
      if(tab_element == NULL)
        {
          sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
          return;
        }

      /** On vérifie si le remplissage se fait avec une valeur particulière **/
      if(nb_element == 3)
        {
          /* On évalue comme entier la deuxième valeur */
          one_dword = (DWORD) EvalExpressionAsInteger(tab_element[2],buffer_error,current_line,1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(buffer_error_rtn,"Impossible to evaluate DS Data '%s' in Operand '%s' (%s)",tab_element[2],current_line->operand_txt,buffer_error);
              mem_free_table(nb_element,tab_element);
              return;
            }

          /** Découpage en Byte (copie respectant le Byte Order) **/
          bo_memcpy(&data[0],&one_dword,sizeof(DWORD));
          
          /** On remplit les Data **/
          for(i=0; i<current_line->nb_byte; i++)
            current_line->data[i] = data[0];
        }
    }
  /**************************/
  /** Chaine de caractères **/
  /**************************/
  else if(!my_stricmp(current_line->opcode_txt,"ASC") || !my_stricmp(current_line->opcode_txt,"DCI") || !my_stricmp(current_line->opcode_txt,"INV") || !my_stricmp(current_line->opcode_txt,"FLS") ||
          !my_stricmp(current_line->opcode_txt,"STR") || !my_stricmp(current_line->opcode_txt,"STRL"))
    {
      /** Extraction de la chaine de caractère en indiquant où sont les parties Hexa **/
      for(i=0,nb_nibble=0,nb_byte=0; i<(int)strlen(current_line->operand_txt); i++)
        {
          /* Début de zone */
          if(current_line->operand_txt[i] == '\'' || current_line->operand_txt[i] == '"')
            {
              /* Fin de zone */
              next_char = strchr(&current_line->operand_txt[i+1],current_line->operand_txt[i]);

              /* Taille de la zone */
              length = (int) ((next_char - &current_line->operand_txt[i]) - 1);
              for(j=0; j<length; j++)
                {
                  /* Est-ce un caractère valide de la plage ? */
                  if(!my_stricmp(current_line->opcode_txt,"INV") || !my_stricmp(current_line->opcode_txt,"FLS"))
                    next_char = (char *) strchr(INVFLS_TABLE,current_line->operand_txt[i+1+j]);
                  else
                    next_char = (char *) strchr(ASCII_TABLE,current_line->operand_txt[i+1+j]);
                  if(next_char == NULL)
                    {
                      sprintf(buffer_error_rtn,"The character '%c' is not valid with Directive %s",current_line->operand_txt[i+1+j],current_line->opcode_txt);
                      return;                    
                    }
                
                  /* Ajuste le bit 7 */
                  if(current_line->operand_txt[i] == '"')
                    param->buffer_string[nb_byte+j] = (0x80 | current_line->operand_txt[i+1+j]);
                  else
                    param->buffer_string[nb_byte+j] = (0x7F & current_line->operand_txt[i+1+j]);
                }

              /* Conserve le rang */
              nb_byte += length;

              /* On continue */
              i += (length+1);
              continue;
            }
          else if((current_line->operand_txt[i] >= '0' && current_line->operand_txt[i] <= '9') || (toupper(current_line->operand_txt[i]) >= 'A' && toupper(current_line->operand_txt[i]) <= 'F'))
            {
              nb_nibble++;
              if(nb_nibble == 2)
                {
                  /* Récupération du Byte */
                  data[1] = current_line->operand_txt[i];
                  data[2] = '\0';                  
                  sscanf((char *)data,"%X",&value);

                  /* Conserve le caractère */
                  param->buffer_string[nb_byte] = (unsigned char) value;
                  nb_byte++;
                  nb_nibble = 0;
                }
              else
                data[0] = current_line->operand_txt[i];
            }
        }

      /** Stockage de la chaine en fonction de l'Opcode **/
      if(!my_stricmp(current_line->opcode_txt,"ASC"))
        {
          /* Conserve la chaine telle qu'elle */  
          memcpy(current_line->data,param->buffer_string,nb_byte);
        }
      else if(!my_stricmp(current_line->opcode_txt,"DCI"))
        {
          memcpy(current_line->data,param->buffer_string,nb_byte);
          if((current_line->data[nb_byte-1] & 0x80) == 0x00)
            current_line->data[nb_byte-1] = (0x80 | current_line->data[nb_byte-1]);
          else
            current_line->data[nb_byte-1] = (0x7F & current_line->data[nb_byte-1]);
        }
      else if(!my_stricmp(current_line->opcode_txt,"INV"))
        {
          /* On va passer en codage 0x00-0x3F */
          for(i=0; i<nb_byte; i++)
            current_line->data[i] = (param->buffer_string[i] & 0x3F);
        }
      else if(!my_stricmp(current_line->opcode_txt,"FLS"))
        {
          /* On va passer en codage 0x40-0x7F */        
          for(i=0; i<nb_byte; i++)
            current_line->data[i] = (param->buffer_string[i] & 0x7F);
        }
      else if(!my_stricmp(current_line->opcode_txt,"STR"))
        {
          one_byte = (BYTE) nb_byte;
          bo_memcpy(&current_line->data[0],&one_byte,1);                  /* Byte Order */
          memcpy(&current_line->data[1],param->buffer_string,nb_byte);
        }
      else if(!my_stricmp(current_line->opcode_txt,"STRL"))
        {
          one_word = (WORD) nb_byte;
          bo_memcpy(&current_line->data[0],&one_word,2);                  /* Byte Order */
          memcpy(&current_line->data[2],param->buffer_string,nb_byte);
        }
    }
  else if(!my_stricmp(current_line->opcode_txt,"REV"))
    {
      /** Pas de Hex dans la chaine **/
      for(i=0; i<current_line->nb_byte; i++)
        {
          if(current_line->operand_txt[0] == '"')
            current_line->data[i] = (0x80 | current_line->operand_txt[current_line->nb_byte-i]);
          else
            current_line->data[i] = (0x7F & current_line->operand_txt[current_line->nb_byte-i]);
        }
    }
  else if(!my_stricmp(current_line->opcode_txt,"CHK"))
    {
      /* 1 Byte de Checksum : On met un 0x00 pour l'instant */
      current_line->data[0] = 0x00;
    }
}

/***********************************************************************/
