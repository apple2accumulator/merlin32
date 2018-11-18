/***********************************************************************/
/*                                                                     */
/*  a65816_Line.c : Module pour la gestion des lignes  .               */
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
#include "a65816_Macro.h"
#include "a65816_File.h"
#include "a65816_Code.h"
#include "a65816_Line.h"
#include "a65816_OMF.h"


char *opcode_list[] = 
{
  "ADC","ADCL","AND","ANDL","ASL",
  "BCC","BLT","BCS","BGE","BEQ","BIT","BMI","BNE","BPL","BRA","BRK","BRL","BVC","BVS",
  "CLC","CLD","CLI","CLV","CMP","CMPL","COP","CPX","CPY",
  "DEC","DEX","DEY",
  "EOR","EORL",
  "INC","INX","INY",
  "JMP","JML","JMPL","JSR","JSL",
  "LDA","LDAL","LDX","LDY","LSR",
  "MVN","MVP",
  "NOP",
  "ORA","ORAL",
  "PEA","PEI","PER","PHA","PHB","PHD","PHK","PHP","PHX","PHY","PLA","PLB","PLD","PLP","PLX","PLY",
  "REP","ROL","ROR","RTI","RTL","RTS",
  "SBC","SBCL","SEC","SED","SEI","SEP","STA","STAL","STP","STX","STY","STZ",
  "TAX","TAY","TCD","TCS","TDC","TRB","TSB","TSC","TSX","TXA","TXS","TXY","TYA","TYX",
  "WAI","WDM",
  "XBA","XCE",
  NULL
};

/* DA   : Define Address (2 Bytes) */
/* DW   : Define Word (2 Bytes) */
/* DDB  : Define Double Byte (2 Bytes) */
/* DFB  : DeFine Byte (1 Byte) */
/* DB   : Define Byte (1 Byte) */
/* ADR  : Define Long Address (3 Bytes) */
/* ADRL : Define Long Address (4 Bytes) */
/* HEX  : Define HEX data (1 Byte) */
/* DS   : Define Storage (X Bytes) = DS 10 (put $00 in 10 bytes), DS 10,$80 (put $80 in 10 bytes), DS \,$80 (put $80 until next page) */

/* ASC  : Ascii ("" positive, '' negative) */
/* DCI  : Dextral Character Inverted */
/* INV  : Inverse Text */
/* FLS  : Flasing Text */
/* REV  : Reverse */
/* STR  : String with leading length (1 byte) */
/* STRL : String with leading length (2 bytes) */

char *data_list[] = 
{
  "DA","DW","DDB","DFB","DB","ADR","ADRL","HEX","DS",
  "DC","DE",  /* ? */
  "ASC","DCI","INV","FLS","REV","STR","STRL",
  "CHK",                                           /* Remplace par 1 Byte de Checksum */
  NULL
};

char *directive_list[] = 
{
  "ANOP","ORG","PUT","PUTBIN",         /* PUTBIN n'existe pas dans Merlin 16+ */
  "START","END",
  "DUM","DEND",
  "MX","XC","LONGA","LONGI",
  "USE","USING",
  "REL","DSK","LNK","SAV",
  "TYP",
  "IF","DO","ELSE","FIN",
  "LUP","--^",
  "ERR","DAT",
  "AST","CYC","EXP","LST","LSTDO","PAG","TTL","SKP","TR","KBD","PAU","SW","USR",   /* On ne fait rien avec ces Directives */
  NULL
};

char *equivalence_list[] =    /* Equivalence ou ]Variable */
{
  "EQU","=",
  NULL
};

/** Address Mode **
A          Implicit
addr2      Absolute
(addr2,X)  Absolute Indexed,X Indirect    
addr2,X    Absolute Indexed,X           
addr2,Y    Absolute Indexed,Y           
(addr2)    Absolute Indirect            
[addr2]    Absolute Indirect Long       
addr3      Absolute Long                
addr3,X    Absolute Long Indexed,X                         
dp         Direct Page                  
dp,X       Direct Page Indexed,X        
dp,Y       Direct Page Indexed,Y        
(dp)       Direct Page Indirect         
[dp]       Direct Page Indirect Long    
(dp,X)     Direct Page Indexed Indirect,X        
(dp),Y     Direct Page Indirect Indexed,Y        
[dp],Y     Direct Page Indirect Long Indexed,Y   
#const     Immediate                    
relative1  Program Counter Relative     
relative2  Program Counter Relative Long
(sr,S),Y   Stack Relative Indirect Indexed,Y             
label      Stack PC Relative Long
sr,S       Stack Relative
*/

static int IsLocalLabel(char *,struct omf_segment *);
static int ProcessSourceAsteriskLine(struct source_line *,struct source_line *,struct source_file *,struct omf_segment *);
static int ProcessMacroAsteriskLine(struct macro_line *,struct macro_line *,struct macro *,struct omf_segment *);
static int ProcessSourceLineLocalLabel(struct source_line *,struct source_line *,struct omf_segment *);
static int ProcessMacroLineLocalLabel(struct macro_line *,struct macro_line *,struct macro *,struct omf_segment *);
static int ProcessSourceLineVariableLabel(struct source_line *,struct source_line *,struct omf_segment *);
static int ProcessMacroLineVariableLabel(struct macro_line *,struct macro_line *,struct macro *,struct omf_segment *);
static void AddDateLine(struct source_line *,struct omf_segment *);

/****************************************************/
/*  DecodeLineType() :  Détermine le type de ligne. */
/****************************************************/
int DecodeLineType(struct source_line *first_line, struct macro *current_macro, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
  int i, nb_error, nb_label, do_level, do_status, nb_global, found;
  int64_t value_wdc;
  char *str_temp;
  char *new_label;
  char **tab_label;
  struct item *current_item;
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *new_line;
  struct source_line *last_line;
  struct source_line *do_line;
  struct source_line *else_line;
  struct source_line *fin_line;
  char *new_opcode;
  struct global *current_global;
  char opcode[1024];
  char macro_name[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /* Init */
  nb_error = 0;

  /*** Passe toutes les lignes en revue ***/
  for(current_line=first_line; current_line; current_line=current_line->next)
    {
      /* Ligne déjà connue */
      if(current_line->type != LINE_UNKNOWN)
        continue;

      /** Ligne vide **/
     if(strlen(current_line->label_txt) == 0 && strlen(current_line->opcode_txt) == 0 && strlen(current_line->operand_txt) == 0)
        {
          current_line->type = LINE_EMPTY;
          continue;
        }
      if(strlen(current_line->label_txt) > 0 && strlen(current_line->opcode_txt) == 0)
        {
          current_line->type = LINE_EMPTY;
          continue;
        }

      /** Ligne ERR Expression dont l'évaluation se fait à la fin **/
      if(!my_stricmp(current_line->opcode_txt,"ERR") && strlen(current_line->operand_txt))
        {
          current_line->type = LINE_CODE;
          continue;
        }

      /** Ligne DAT à laquelle ont ajouter la Date sous forme de Texte **/
      if(!my_stricmp(current_line->opcode_txt,"DAT"))
        {
          current_line->type = LINE_DIRECTIVE;
          
          /* On doit ajouter une ligne de Texte avec la date */
          if(strlen(current_line->operand_txt) > 0)
            AddDateLine(current_line,current_omfsegment);
          continue;
        }
        
      /** Ligne définissant un point d'entrée Global pour les InterSeg **/
      if(strlen(current_line->label_txt) > 0 && !my_stricmp(current_line->opcode_txt,"ENT"))
        {
          current_line->type = LINE_GLOBAL;
          continue;
        }

      /** Ligne définissant une ou plusieurs entrées Global pour les InterSeg **/
      if(strlen(current_line->label_txt) == 0 && !my_stricmp(current_line->opcode_txt,"ENT") && strlen(current_line->operand_txt) > 0)
        {
          /* Plusieurs Labels EXT sous la forme Label1,Label2,Label3... => Création de nouvelles lignes */
          tab_label = BuildUniqueListFromText(current_line->operand_txt,',',&nb_label);
          if(tab_label == NULL)
            {
              /* Error */
              printf("        => Error : Can't allocate memory to process line.\n");
              return(1);
            }
                    
          /* On va conserver ces Labels pour les traiter après le chargement des lignes */
          for(i=0; i<nb_label; i++)
            my_Memory(MEMORY_ADD_GLOBAL,tab_label[i],current_line,current_omfsegment);
          current_line->type = LINE_EMPTY;
          mem_free_list(nb_label,tab_label);
          continue;
        }

      /** Ligne définissant un label Externe à ce segment **/
      if(strlen(current_line->label_txt) > 0 && !my_stricmp(current_line->opcode_txt,"EXT"))
        {
          current_line->type = LINE_EXTERNAL;
          continue;
        }

      /** Ligne définissant un|plusieurs label Externe à ce segment **/
      if(strlen(current_line->label_txt) == 0 && !my_stricmp(current_line->opcode_txt,"EXT") && strlen(current_line->operand_txt) > 0)
        {
          /* On va inverser le Label et l'Operand : EXT Label => Label EXT */
          if(strchr(current_line->operand_txt,',') == NULL)
            {
              /* Un seul label EXT */
              str_temp = current_line->label_txt;
              current_line->label_txt = current_line->operand_txt;
              current_line->operand_txt = str_temp;
              current_line->type = LINE_EXTERNAL;
              continue;
            }
          else
            {
              /* Plusieurs Labels EXT sous la forme Label1,Label2,Label3... => Création de nouvelles lignes */
              tab_label = BuildUniqueListFromText(current_line->operand_txt,',',&nb_label);
              if(tab_label == NULL)
                {
                  /* Error */
                  printf("        => Error : Can't allocate memory to process line.\n");
                  return(1);
                }
              
              /* Finalement aucun Label */
              if(nb_label == 0)
                {
                  mem_free_list(nb_label,tab_label);
                  current_line->type = LINE_EMPTY;
                  continue;
                }
                
              /** On va devoir créer des lignes pour chacun des Labels **/
              /* Ligne courrante */
              new_label = strdup(tab_label[0]);
              if(new_label == NULL)
                {
                  /* Error */
                  printf("        => Error : Can't allocate memory to process line.\n");
                  return(1);
                }
              free(current_line->label_txt);
              current_line->label_txt = new_label;
              strcpy(current_line->operand_txt,"");
              current_line->type = LINE_EXTERNAL;
              
              /* Lignes Suivantes */
              for(i=1; i<nb_label; i++)
                {
                  /* Nouvelle ligne = Duplique la ligne */
                  new_line = DuplicateSourceLine(current_line);
                  if(new_line == NULL)
                    {
                      /* Error */
                      mem_free_list(nb_label,tab_label);
                      printf("        => Error : Can't allocate memory to process line.\n");
                      return(1);
                    }
                  
                  /* Nouveau Label (on part du fond du tableau) */
                  new_line->label_txt = strdup(tab_label[nb_label-i]);
                  if(new_line->label_txt == NULL)
                    {
                      /* Error */
                      mem_free_sourceline(new_line);
                      mem_free_list(nb_label,tab_label);
                      printf("        => Error : Can't allocate memory to process line.\n");
                      return(1);
                    }
                    
                  /* Attache la ligne */
                  new_line->next = current_line->next;
                  current_line->next = new_line;
                }
              
              /* Libération mémoire */
              mem_free_list(nb_label,tab_label);
              continue;
            }
        }

      /** Identification du type de ligne en utilisant l'opcode **/
      if(strlen(current_line->opcode_txt) > 0)
        {
          /*** Macro (on place la détection de Macro avant la détection des Opcodes car un WAIT en macro pourrait être interpretté comme un WAI du 65c816) ***/
          /* Appel via PMC ou >>> */
          if((!my_stricmp(current_line->opcode_txt,"PMC") || !my_stricmp(current_line->opcode_txt,">>>")) && strlen(current_line->operand_txt) > 0)
            {
              /* On va isoler le nom de la macro (car il peut être collé aux paramètres) */
              strcpy(macro_name,current_line->operand_txt);
              for(i=0; i<(int)strlen(macro_name); i++)
                if(macro_name[i] == ',' || macro_name[i] == '.' || macro_name[i] == '/' || macro_name[i] == '-' || macro_name[i] == '(' || macro_name[i] == ' ')
                  {
                    macro_name[i] = '\0';
                    break;
                  }

              /* Recherche cette Macro */
              my_Memory(MEMORY_SEARCH_MACRO,macro_name,&current_item,current_omfsegment);
              if(current_item != NULL)
                {
                  current_line->type = LINE_MACRO;
                  current_line->macro = (struct macro *) current_item;
                  continue;
                }
            }
          else
            {
              /* Appel avec le nom de la macro */
              my_Memory(MEMORY_SEARCH_MACRO,current_line->opcode_txt,&current_item,current_omfsegment);
              if(current_item != NULL)
                {
                  current_line->type = LINE_MACRO;
                  current_line->macro = (struct macro *) current_item;
                  continue;
                }
            }

          /*** Opcode ***/
          if(current_item == NULL)
            {
              my_Memory(MEMORY_SEARCH_OPCODE,current_line->opcode_txt,&current_item,current_omfsegment);
              if(current_item != NULL)
                {
                  current_line->type = LINE_CODE;
                  continue;
                }

              /*** Opcode avec une lettre derrière : LDA\ ou LDA: (ni D ni L) ***/
              if(strlen(current_line->opcode_txt) == 4)
                {
                  if(toupper(current_line->opcode_txt[3]) != 'L' && toupper(current_line->opcode_txt[3]) != 'D')
                    {
                      strcpy(opcode,current_line->opcode_txt);
                      opcode[3] = '\0';
                      my_Memory(MEMORY_SEARCH_OPCODE,opcode,&current_item,current_omfsegment);
                      if(current_item != NULL)
                        {
                          current_line->opcode_txt[3] = '\0';
                          current_line->type = LINE_CODE;
                          current_line->no_direct_page = 1;   /* Il y a un caractère derrière l'opcode pour empêcher le Page Direct */
                          continue;
                        }
                    }
                }
            }

          /*** Data ***/
          if(current_item == NULL)
            {
              my_Memory(MEMORY_SEARCH_DATA,current_line->opcode_txt,&current_item,current_omfsegment);
              if(current_item != NULL)
                {
                  current_line->type = LINE_DATA;
                  continue;
                }
            }

          /*** Directive ***/
          if(current_item == NULL)
            {
              my_Memory(MEMORY_SEARCH_DIRECTIVE,current_line->opcode_txt,&current_item,current_omfsegment);
              if(current_item != NULL)
                {
                  current_line->type = LINE_DIRECTIVE;

                  /* On repère les REL (mais on entiet pas compte si on est sur un project multi-fixed) */
                  if(!my_stricmp(current_line->opcode_txt,"REL") && current_omfproject->is_multi_fixed != 1)
                    current_omfsegment->is_relative = 1;

                  continue;
                }
            }

          /*** Equivalence ou Variable ***/
          if(current_item == NULL)
            {
              my_Memory(MEMORY_SEARCH_DIREQU,current_line->opcode_txt,&current_item,current_omfsegment);
              if(current_item != NULL)
                {
                  if(current_line->label_txt[0] == ']')
                    current_line->type = LINE_VARIABLE;
                  else
                    current_line->type = LINE_EQUIVALENCE;
                  continue;
                }
            }
        }

      /* Erreur : Ligne toujours inconnue :-( */
      if(current_line->type == LINE_UNKNOWN)
        {
          if(current_macro != NULL)
            printf("      => [Error] Unknown Macro line '%s' from Macro file '%s' (line %d), inserted in source file '%s' (line %d).\n",current_line->line_data,current_macro->file_name,current_macro->file_line_number,current_line->file->file_name,current_line->file_line_number);
          else
            printf("      => [Error] Unknown line '%s' in source file '%s' (line %d).\n",current_line->line_data,current_line->file->file_name,current_line->file_line_number);          
          nb_error++;
        }
    }

  /*** On ne fait pas le travail d'analyse des ENT dans les Macro ***/
  if(current_macro == NULL)
    {
      /********************************************************************/
      /** Y a t'il des ENT qui ont été déclarées en ENT Label1,Label2... **/
      /********************************************************************/
      my_Memory(MEMORY_GET_GLOBAL_NB,&nb_global,NULL,current_omfsegment);
      for(i=1; i<=nb_global; i++)
        {
          /* On récupère un Label ENT */
          my_Memory(MEMORY_GET_GLOBAL,&i,&current_global,current_omfsegment);
          
          /* Cherche une ligne avec ce Label */
          for(found=0,current_line = first_file->first_line; current_line; current_line = current_line->next)
            {
              if(!strcmp(current_line->label_txt,current_global->name) && current_line->type == LINE_GLOBAL)
                {
                  /* On a déjà le ENT sur la ligne du label => rien à faire */
                  found = 1;
                  break;
                }
              else if(!strcmp(current_line->label_txt,current_global->name))
                {
                  /* On crée une ligne ENT avec le label */
                  new_line = DuplicateSourceLine(current_line);
                  if(new_line == NULL)
                    {
                      sprintf(param->buffer_error,"Impossible to allocate memory to Duplicate Line %d in Source file '%s'",current_line->file_line_number,current_line->file->file_path);
                      my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }
                  new_opcode = strdup("ENT");
                  if(new_opcode == NULL)
                    {
                      mem_free_sourceline(new_line);
                      sprintf(param->buffer_error,"Impossible to allocate memory to Duplicate Line %d in Source file '%s'",current_line->file_line_number,current_line->file->file_path);
                      my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }
                   
                  /* Modifie les valeurs */
                  strcpy(current_line->operand_txt,"");    /* On vide l'Operande de la ligne ENT */
                  free(current_line->opcode_txt);
                  current_line->opcode_txt = new_opcode;   /* Le nouvel Opcode est ENT */
                  current_line->type = LINE_GLOBAL;        /* Cette ligne est désormais une GLOBAL */
                  strcpy(new_line->label_txt,"");          /* On vide le label pour éviter les doublons */
                
                  /* Attache la nouvelle ligne */
                  new_line->next = current_line->next;
                  current_line->next = new_line;
                
                  /* Label ENT traité */
                  found = 1;
                  break;
                }
            }
                        
          /* On a pas trouvé de ligne avec ce Label => on la place au niveau du ENT Label1,Label2... */
          if(found == 0)
            {
              /* On crée une ligne ENT avec le label en la placant apreès la ligne ENT Label1,Label2...*/
              new_line = DuplicateSourceLine(current_global->source_line);
              if(new_line == NULL)
                {
                  sprintf(param->buffer_error,"Impossible to allocate memory to Duplicate Line %d in Source file '%s'",current_global->source_line->file_line_number,current_global->source_line->file->file_path);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
              new_label = strdup(current_global->name);
              if(new_label == NULL)
                {
                  mem_free_sourceline(new_line);
                  sprintf(param->buffer_error,"Impossible to allocate memory to Duplicate Line %d in Source file '%s'",current_global->source_line->file_line_number,current_global->source_line->file->file_path);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
                    
              /* Modifie les valeurs */
              strcpy(new_line->operand_txt,"");      /* On vide l'Operande de la ligne ENT */
              free(new_line->label_txt);
              new_line->label_txt = new_label;       /* Place le Label */
              new_line->type = LINE_GLOBAL;          /* Cette ligne est désormais une GLOBAL */
            
              /* Attache la nouvelle ligne en dessous */
              new_line->next = current_global->source_line->next;
              current_global->source_line->next = new_line;          
            }
        }
    }
    
  /*** On ne fait pas le travail d'analyse des DO dans les Macro ***/
  if(current_macro == NULL)
    {
      /************************************************************************************/
      /*** 1ère passe pour marquer les niveaux des DO-ELSE-FIN et valider l'imbrication ***/
      /************************************************************************************/
      do_level = 0;
      for(current_line=first_line; current_line; current_line=current_line->next)
        {
          /* On conserve la dernière ligne du fichier pour le message d'erreur */
          last_line = current_line;
          
          /* Ligne définissant une nouvelle condition DO-FIN */
          if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"DO") || !my_stricmp(current_line->opcode_txt,"IF")))
            {
              do_level++;
              current_line->do_level = do_level;
              continue;
            }

          /* Ligne définissant l'inversion d'une condition DO-ELSE_FIN */
          if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"ELSE"))
            {
              current_line->do_level = do_level;
              if(do_level == 0)
                {
                  printf("      => [Error] ELSE directive found in source file '%s' at line %d doesn't match with a previous (missing) DO or IF.\n",current_line->file->file_name,current_line->file_line_number);
                  return(1);            
                }
              continue;        
            }
                    
          /* Ligne définissant la fin d'un DO-FIN */
          if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"FIN"))
            {
              current_line->do_level = do_level;
              do_level--;
              if(do_level < 0)
                {
                  printf("      => [Error] FIN directive found in source file '%s' at line %d doesn't match with a previous (missing) DO or IF.\n",current_line->file->file_name,current_line->file_line_number);
                  return(1);
                }
              continue;
            }
        
          /* Ligne normale */
          current_line->do_level = do_level;
        }
      if(do_level > 0) 
        {
          printf("      => [Error] Missing FIN directive in source file '%s' (due to previous usage of a DO or IF directive).\n",last_line->file->file_name);
          return(1);
        }

      /***********************************************************************************/
      /*** 2ème passe pour évaluer les conditions DO et invalider les lignes du source ***/
      /***********************************************************************************/
      for(do_line=first_line; do_line; do_line=do_line->next)
        {             
          /* Ligne définissant une première condition DO */
          if(do_line->type == LINE_DIRECTIVE && !my_stricmp(do_line->opcode_txt,"DO"))
            {
              /* Evaluation de la condition */
              do_status = QuickConditionEvaluate(do_line,&value_wdc,current_omfsegment);
              
              /* Cherche la ligne FIN */
              for(fin_line=do_line; fin_line; fin_line=fin_line->next)
                if(fin_line->type == LINE_DIRECTIVE && !my_stricmp(fin_line->opcode_txt,"FIN") && do_line->do_level == fin_line->do_level)
                  break;
              
              /* Cherche une éventuelle ligne ELSE */
              for(else_line=do_line; else_line!=fin_line; else_line=else_line->next)
                if(else_line->type == LINE_DIRECTIVE && !my_stricmp(else_line->opcode_txt,"ELSE") && do_line->do_level == else_line->do_level)
                  break;
              if(else_line == fin_line)
                else_line = NULL;
                
              /** On invalide DO - ELSE/FIN **/
              if(do_status == STATUS_DONT)
                {
                  for(current_line=do_line->next; current_line!=((else_line!=NULL)?else_line:fin_line); current_line=current_line->next)
                    current_line->is_valid = 0;
                }
              /** On invalide ELSE - FIN **/
              else if(do_status == STATUS_DO && else_line != NULL)
                {
                  for(current_line=else_line->next; current_line!=fin_line; current_line=current_line->next)
                    current_line->is_valid = 0;            
                }
                
              /** On ne sait pas : On continue à partie du FIN **/
              do_line = fin_line;
            }
        }
    }
    
  /* Renvoie le nombre d'erreur */
  return(nb_error);
}


/****************************************************************************************/
/*  ProcessAllAsteriskLine() :  Remplace les '*' dans les lignes de Code et les Macros. */
/****************************************************************************************/
int ProcessAllAsteriskLine(struct omf_segment *current_omfsegment)
{
  int i, error, nb_macro;
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *last_line = NULL;
  struct macro *current_macro;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** On traite les lignes du Source **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    if(current_line->next == NULL)
      last_line = current_line;
  error = ProcessSourceAsteriskLine(first_file->first_line,last_line,first_file,current_omfsegment);
  if(error)
    return(1);

  /** On traite les Macros **/
  my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
  for(i=1; i<=nb_macro; i++)
    {
      my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);

      /** Traite les lignes de cette Macro **/
      error = ProcessMacroAsteriskLine(current_macro->first_line,current_macro->last_line,current_macro,current_omfsegment);
      if(error)
        return(1);
    }

  /* OK */
  return(0);
}


/*****************************************************************************/
/*  ProcessSourceAsteriskLine() :  Remplace les '*' dans les lignes de Code. */
/*****************************************************************************/
static int ProcessSourceAsteriskLine(struct source_line *first_line, struct source_line *last_line, struct source_file *first_file, struct omf_segment *current_omfsegment)
{
  int use_address;
  struct source_line *previous_line;  
  struct source_line *current_line;
  struct source_line *new_line;
  char label_name[1024];  
  char buffer_error[1024];

  /*** Passe toutes les lignes en revue ***/
  for(previous_line=NULL, current_line=first_line; current_line; previous_line=current_line, current_line=current_line->next)
    {
      /* On ne traite pas les commentaires */
      if(current_line->type == LINE_EMPTY || current_line->type == LINE_COMMENT || current_line->type == LINE_GLOBAL || current_line->is_valid == 0)
        continue;

      /** Cas particulier des ]Label = * ou des Label = * **/        
      if(strlen(current_line->label_txt) > 0 && !strcmp(current_line->opcode_txt,"=") && !strcmp(current_line->operand_txt,"*"))
        {
          /* On convertit cette ligne en ligne vide */
          strcpy(current_line->opcode_txt,"");
          strcpy(current_line->operand_txt,"");
          current_line->type = LINE_EMPTY;
          continue;
        }

      /** On ne cherche que dans l'Operande **/
      if(strchr(current_line->operand_txt,'*') != NULL)
        {
          /** Peut t'on détecter un * utilisé comme valeur ? **/
          use_address = UseCurrentAddress(current_line->operand_txt,&buffer_error[0],current_line);
          if(strlen(buffer_error) > 0)
            {
              printf("    Error : Impossible to analyze Operand '%s' in source file '%s' (line %d) : %s.\n",
                     current_line->operand_txt,current_line->file->file_name,current_line->file_line_number,buffer_error);
              return(1);
            }
          if(use_address == 0)
            continue;

          /** On va remplacer le * par un label unique **/
          /* Création d'un Label unique ANOP */
          GetUNID(&label_name[0]);

          /** Création d'un Ligne vide ANOP avec le label **/
          new_line = (struct source_line *) calloc(1,sizeof(struct source_line));
          if(new_line == NULL)
            {
              printf("    Error : Impossible to allocate memory to create new Empty line.\n");
              return(1);
            }
          new_line->file = current_line->file;
          new_line->file_line_number = current_line->file_line_number;
          new_line->line_number = current_line->line_number;
          new_line->is_in_source = 1;
          new_line->is_valid = 1;
          strcpy(new_line->reloc,"         ");
          new_line->label_txt = strdup(label_name);
          new_line->opcode_txt = strdup("");
          new_line->operand_txt = strdup("");
          new_line->comment_txt = strdup("");
          strcpy(new_line->m,current_line->m);
          strcpy(new_line->x,current_line->x);
          new_line->address = current_line->address;
          new_line->operand_value = current_line->operand_value;
          new_line->operand_address_long = current_line->operand_address_long;
          new_line->bank = current_line->bank;
          new_line->type_aux = current_line->type_aux;        /* On conserve l'information d'inclusion dans le corps d'une Macro */
          new_line->is_inside_macro = current_line->is_inside_macro;
          if(new_line->label_txt == NULL || new_line->opcode_txt == NULL || new_line->operand_txt == NULL || new_line->comment_txt == NULL)
            {
              printf("    Error : Impossible to allocate memory to populate new Empty line.\n");
              mem_free_sourceline(new_line);
              return(1);
            }
          new_line->type = LINE_EMPTY;
          
          /* Attachement de la ligne au dessus */
          new_line->next = current_line;
          if(previous_line == NULL)
            first_file->first_line = new_line;
          else
            previous_line->next = new_line;
            
          /** Remplace le * par un Label unique **/
          ReplaceCurrentAddressInOperand(&current_line->operand_txt,label_name,&buffer_error[0],current_line);
          if(strlen(buffer_error) > 0)
            {
              printf("    Error : Impossible to replace '*' in Operand '%s' in source file '%s' (line %d) : %s.\n",
                     current_line->operand_txt,current_line->file->file_name,current_line->file_line_number,buffer_error);
              return(1);
            }
        }
    }

  /* OK */
  return(0);
}


/******************************************************************************/
/*  ProcessMacroAsteriskLine() :  Remplace les '*' dans les lignes des Macro. */
/******************************************************************************/
static int ProcessMacroAsteriskLine(struct macro_line *first_line, struct macro_line *last_line, struct macro *current_macro, struct omf_segment *current_omfsegment)
{
  int use_address;
  struct macro_line *previous_line;  
  struct macro_line *current_line;
  struct macro_line *new_line;
  char label_name[1024];  
  char buffer_error[1024];

  /*** Passe toutes les lignes en revue ***/
  for(previous_line=NULL, current_line=first_line; current_line; previous_line=current_line, current_line=current_line->next)
    {
      /** Cas particulier des ]Label = * ou des Label = * **/        
      if(strlen(current_line->label) > 0 && !strcmp(current_line->opcode,"=") && !strcmp(current_line->operand,"*"))
        {
          /* On convertit cette ligne en ligne vide */
          strcpy(current_line->opcode,"");
          strcpy(current_line->operand,"");
          continue;
        }

      /** On ne cherche quand dans l'Operande **/
      if(strchr(current_line->operand,'*') != NULL)
        {
          /** Peut t'on détecter un * utilisé comme valeur ? **/
          use_address = UseCurrentAddress(current_line->operand,&buffer_error[0],NULL);
          if(strlen(buffer_error) > 0)
            {
              printf("    Error : Impossible to analyze Operand '%s' in Macro '%s' : %s.\n",
                     current_line->operand,current_macro->name,buffer_error);
              return(1);
            }
          if(use_address == 0)
            continue;

          /** On va remplacer le * par un label unique **/
          /* Création d'un Label unique ANOP */
          GetUNID(&label_name[0]);

          /** Création d'un Ligne vide ANOP avec le label **/
          new_line = (struct macro_line *) calloc(1,sizeof(struct macro_line));
          if(new_line == NULL)
            {
              printf("    Error : Impossible to allocate memory to create new Empty Macro line.\n");
              return(1);
            }
          new_line->label = strdup(label_name);
          new_line->opcode = strdup("");
          new_line->operand = strdup("");
          new_line->comment = strdup("");
          if(new_line->label == NULL || new_line->opcode == NULL || new_line->operand == NULL || new_line->comment == NULL)
            {
              printf("    Error : Impossible to allocate memory to populate new Empty Macro line.\n");
              mem_free_macroline(new_line);
              return(1);
            }
          
          /* Attachement de la ligne au dessus */
          new_line->next = current_line;
          if(previous_line == NULL)
            current_macro->first_line = new_line;
          else
            previous_line->next = new_line;
            
          /** Remplace le * par un Label unique **/
          ReplaceCurrentAddressInOperand(&current_line->operand,label_name,&buffer_error[0],NULL);
          if(strlen(buffer_error) > 0)
            {
              printf("    Error : Impossible to replace '*' in Operand '%s' in Macro '%s' : %s.\n",
                     current_line->operand,current_macro->name,buffer_error);
              return(1);
            }
        }
    }

  /* OK */
  return(0);
}


/*************************************************************/
/*  BuildLabelTable() :  Construction des tables des Labels. */
/*************************************************************/
int BuildLabelTable(struct omf_segment *current_omfsegment)
{
  struct label *current_label;
  struct source_line *current_line;
  struct source_file *first_file;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On ne prend que le lignes avec label */
      if(strlen(current_line->label_txt) == 0)
        continue;

      /* On ne prend pas les Label locaux ou les Variables */
      if(current_line->label_txt[0] == ':' || current_line->label_txt[0] == ']')
        continue;

      /* On ne prend pas les Equivalence */
      if(current_line->type == LINE_EQUIVALENCE)
        continue;

      /* On ne prend pas les External */
      if(current_line->type == LINE_EXTERNAL)
        continue;

      /* On ne prend pas les Label dans les Macro */
      if(current_line->type == LINE_DIRECTIVE && current_line->type_aux == LINE_MACRO_DEF)
        continue;
      if(current_line->is_inside_macro == 1)
        continue;

      /* On ne prend pas les Labels dans les LUP */
      if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"LUP"))
        continue;
      if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"--^"))
        continue;

      /** Allocation de la structure Label **/
      current_label = (struct label *) calloc(1,sizeof(struct label));
      if(current_label == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure label");
      current_label->name = strdup(current_line->label_txt);
      if(current_label->name == NULL)
        {
          free(current_label);
          my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure label");
        }
      current_label->line = current_line;

      /* Un label Global est un label comme un autre */
      if(current_line->type == LINE_GLOBAL)
        current_label->is_global = 1;

      /* Déclaration de la structure */
      my_Memory(MEMORY_ADD_LABEL,current_label,NULL,current_omfsegment);
    }

  /* Tri les Labels */
  my_Memory(MEMORY_SORT_LABEL,NULL,NULL,current_omfsegment);

  /* OK */
  return(0);
}


/************************************************************************/
/*  BuildEquivalenceTable() :  Construction des tables des Equivalence. */
/************************************************************************/
int BuildEquivalenceTable(struct omf_segment *current_omfsegment)
{
  int i, j, nb_equivalence, nb_element, modified, nb_modified;
  struct equivalence *current_equivalence;
  struct equivalence *replace_equivalence;
  struct source_line *current_line;
  struct source_file *first_file;
  char *new_value;
  char **tab_element;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue (certaines equivalences provenant des fichiers macro sont déjà enregistrées) **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On ne prend que les Equivalence */
      if(current_line->type != LINE_EQUIVALENCE)
        continue;

      /* On ne prend que le lignes avec label */
      if(strlen(current_line->label_txt) == 0)
        continue;

      /** Allocation de la structure Equivalence **/
      current_equivalence = (struct equivalence *) calloc(1,sizeof(struct equivalence));
      if(current_equivalence == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure equivalence");
      current_equivalence->name = strdup(current_line->label_txt);
      current_equivalence->value = strdup(current_line->operand_txt);
      if(current_equivalence->name == NULL || current_equivalence->value == NULL)
        {
          mem_free_equivalence(current_equivalence);
          my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure equivalence");
        }
      current_equivalence->source_line = current_line;   /* Cette equivalence vient d'un fichier source */

      /* Déclaration de la structure */
      my_Memory(MEMORY_ADD_EQUIVALENCE,current_equivalence,NULL,current_omfsegment);
    }

  /* Tri les Equivalence */
  my_Memory(MEMORY_SORT_EQUIVALENCE,NULL,NULL,current_omfsegment);

  /** On va repasser sur les équivalences pour résoudre celles dépendent d'autres équivalences **/
  modified = 1;
  nb_modified = 0;
  while(modified)
    {
      /* Init */
      modified= 0;
      my_Memory(MEMORY_GET_EQUIVALENCE_NB,&nb_equivalence,NULL,current_omfsegment);
      for(i=1; i<=nb_equivalence; i++)
        {
          my_Memory(MEMORY_GET_EQUIVALENCE,&i,&current_equivalence,current_omfsegment);

          /** Découpe l'expression **/
          tab_element = DecodeOperandeAsElementTable(current_equivalence->value,&nb_element,SEPARATOR_REPLACE_LABEL,current_equivalence->source_line);
          if(tab_element == NULL)
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'tab_element' table");

          /** On passe en revue les valeurs **/
          for(j=0,param->buffer_operand[0]='\0'; j<nb_element; j++)
            {
              my_Memory(MEMORY_SEARCH_EQUIVALENCE,tab_element[j],&replace_equivalence,current_omfsegment);
              if(replace_equivalence != NULL)
                {
                  /* L'ajout des {} permet de certifier l'ordre d'évaluation */
                  strcat(param->buffer_operand,"{");
                  strcat(param->buffer_operand,replace_equivalence->value);   /* Equivalence */
                  strcat(param->buffer_operand,"}");

                  /* La valeur a été modifiée */
                  modified = 1;
                }
              else
                strcat(param->buffer_operand,tab_element[j]);
            }

          /* Libération mémoire */
          mem_free_table(nb_element,tab_element);

          /** Si la valeur a été modifiée, on la remplace **/
          if(modified == 1)
            {
              new_value = strdup(param->buffer_operand);
              if(new_value == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory to replace an Equivalence");

              /* Libère l'ancienne */
              free(current_equivalence->value);

              /* Positionne la nouvelle */
              current_equivalence->value = new_value;
            }
        }

      /* On se protège des récursivités sans fin */
      if(modified)
        nb_modified++;
      if(nb_modified > 10)
        my_RaiseError(ERROR_RAISE,"Recursivity detected in Equivalence replacement");
    }

  /* OK */
  return(0);
}


/******************************************************************/
/*  BuildExternalTable() :  Construction des tables des External. */
/******************************************************************/
int BuildExternalTable(struct omf_segment *current_omfsegment)
{
  struct external *current_external;
  struct source_file *first_file;
  struct source_line *current_line;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /* Init */
  my_Memory(MEMORY_FREE_EXTERNAL,NULL,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On ne prend que les External */
      if(current_line->type != LINE_EXTERNAL)
        continue;

      /** Allocation de la structure External **/
      current_external = (struct external *) calloc(1,sizeof(struct external));
      if(current_external == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure external");
      current_external->name = strdup(current_line->label_txt);
      if(current_external->name == NULL)
        {
          mem_free_external(current_external);
          my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure external");
        }
      current_external->source_line = current_line;

      /* Déclaration de la structure */
      my_Memory(MEMORY_ADD_EXTERNAL,current_external,NULL,current_omfsegment);
    }

  /* Tri les External */
  my_Memory(MEMORY_SORT_EXTERNAL,NULL,NULL,current_omfsegment);

  /* OK */
  return(0);
}


/*****************************************************************/
/*  CheckForDuplicatedLabel() :  Recherche les Labels en double. */
/*****************************************************************/
int CheckForDuplicatedLabel(struct omf_segment *current_omfsegment)
{
  int i, nb_label, nb_equivalence, nb_error;
  struct label *previous_label;
  struct label *current_label;
  struct equivalence *previous_equivalence;
  struct equivalence *current_equivalence;

  /* Init */
  nb_error = 0;

  /** Recherche de doublons dans les Labels **/
  previous_label = NULL;
  my_Memory(MEMORY_GET_LABEL_NB,&nb_label,NULL,current_omfsegment);
  for(i=1; i<=nb_label; i++)
    {
      my_Memory(MEMORY_GET_LABEL,&i,&current_label,current_omfsegment);
      if(previous_label != NULL && current_label != NULL)
        if(!strcmp(previous_label->name,current_label->name))
          {
            printf("      => [Error] Found label name '%s' in both source files '%s' (line %d) and '%s' (line %d).\n",current_label->name,
                   previous_label->line->file->file_name, previous_label->line->file_line_number, 
                   current_label->line->file->file_name, current_label->line->file_line_number);
            nb_error++;
          }

      previous_label = current_label;
    }

  /** Recherche de doublons dans les Equivalence (mais pas dans les variables) **/
  previous_equivalence = NULL;
  my_Memory(MEMORY_GET_EQUIVALENCE_NB,&nb_equivalence,NULL,current_omfsegment);
  for(i=1; i<=nb_equivalence; i++)
    {
      my_Memory(MEMORY_GET_EQUIVALENCE,&i,&current_equivalence,current_omfsegment);
      if(previous_equivalence != NULL && current_equivalence != NULL)
        if(!strcmp(previous_equivalence->name,current_equivalence->name))
          {
            printf("      => [Error] Found label equivalence '%s' in both source files '%s' (line %d) and '%s' (line %d).\n",current_equivalence->name,
                   previous_equivalence->source_line->file->file_name, previous_equivalence->source_line->file_line_number,
                   current_equivalence->source_line->file->file_name, current_equivalence->source_line->file_line_number);
            nb_error++;
          }

      previous_equivalence = current_equivalence;
    }

  /** Recherche de doublons entre les Label et les Equivalence **/
  for(i=1; i<=nb_equivalence; i++)
    {
      my_Memory(MEMORY_GET_EQUIVALENCE,&i,&current_equivalence,current_omfsegment);

      /* Recherche un Label portant le même nom */
      my_Memory(MEMORY_SEARCH_LABEL,current_equivalence->name,&current_label,current_omfsegment);
      if(current_label != NULL)
        {
          printf("      => [Error] Found equivalence and label '%s' in both source files '%s' (line %d) and '%s' (line %d).\n",current_equivalence->name,
                 current_equivalence->source_line->file->file_name,current_equivalence->source_line->file_line_number,
                 current_label->line->file->file_name,current_label->line->file_line_number);
          nb_error++;
        }
    }

  /* OK */
  return(nb_error);
}


/***************************************************************************/
/*  ProcessAllLocalLabel() :  On remplace les labels locaux par des unid_. */
/***************************************************************************/
int ProcessAllLocalLabel(struct omf_segment *current_omfsegment)
{
  int i, error, nb_macro;
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *last_line = NULL;
  struct macro *current_macro;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** On traite les lignes du Source **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    if(current_line->next == NULL)
      last_line = current_line;
  error = ProcessSourceLineLocalLabel(first_file->first_line,last_line,current_omfsegment);

  /** On traite les Macros **/
  my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
  for(i=1; i<=nb_macro; i++)
    {
      my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);

      /* Traite les lignes de cette Macro */
      error = ProcessMacroLineLocalLabel(current_macro->first_line,current_macro->last_line,current_macro,current_omfsegment);
    }

  /* OK */
  return(0);
}


/*******************************************************************************************************/
/*  ProcessSourceLineLocalLabel() :  On remplace les labels locaux par des unid_ des lignes du Source. */
/*******************************************************************************************************/
static int ProcessSourceLineLocalLabel(struct source_line *first_line, struct source_line *last_line, struct omf_segment *current_omfsegment)
{
  struct source_line *current_line;
  struct source_line *other_line;
  struct source_line *begin_global_line;
  struct source_line *end_global_line;
  struct source_line *replace_line;
  int found, nb_local;
  char *new_label;
  char *new_operand;
  char previous_label[256];
  char unique_label[256];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /*************************************************************************************/
  /** Si un Label :Local n'existe qu'en 1 seul exemplaire, on va le globaliser _Local **/
  /*************************************************************************************/
  for(current_line=first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes commentaires / celles sans labels / Label autres que :Label */
      if(current_line->type == LINE_COMMENT || current_line->is_valid == 0)
        continue;
      if(strlen(current_line->label_txt) == 0)
        continue;
      if(current_line->label_txt[0] != ':')
        continue;

      /** Ce Label est t'il unique ? **/
      for(found=0,other_line=first_line; other_line; other_line=other_line->next)
        {
          /* On ignore les lignes commentaires / celles sans labels / Label autres que :Label */
          if(other_line->type == LINE_COMMENT || other_line->is_valid == 0)
            continue;
          if(strlen(other_line->label_txt) == 0)
            continue;
          if(other_line->label_txt[0] != ':')
            continue;
          if(other_line == current_line)
            continue;

          /* Compare le Label */
          if(!strcmp(current_line->label_txt,other_line->label_txt))
            {
              found = 1;
              break;
            }
        }

      /** Il est unique, on va essayer de trouver un nom autre que oz_unid... **/
      if(found == 0)
        {
          /* On remplace le : par un _ */
          strcpy(unique_label,current_line->label_txt);
          unique_label[0] = '_';

          /* Ce _Label est t'il unique ? */
          for(found=0,other_line=first_line; other_line; other_line=other_line->next)
            {
              /* On ignore les lignes commentaires / celles sans labels / Label autres que :Label */
              if(other_line->type == LINE_COMMENT || other_line->is_valid == 0)
                continue;
              if(strlen(other_line->label_txt) == 0)
                continue;

              /* Compare le Label */
              if(!strcmp(unique_label,other_line->label_txt))
                {
                  found = 1;
                  break;
                }
            }

          /* Il est unique, on l'utilise pour remplacer */
          if(found == 0)
            {
              /* Remplace :Label par _Label dans tout le fichier */
              strcpy(previous_label,current_line->label_txt);
              for(other_line=first_line; other_line; other_line=other_line->next)
                {
                  /* On remplace dans le label */
                  if(!strcmp(other_line->label_txt,previous_label))
                    {
                      other_line->label_txt[0] = '_';
                      other_line->was_local_label = 1;     /* Ce label a été un label local */
                    }

                  /** Remplace le Label dans l'Operand **/
                  if(other_line->type == LINE_CODE || other_line->type == LINE_DATA || other_line->type == LINE_MACRO)
                    {
                      new_operand = ReplaceInOperand(other_line->operand_txt,previous_label,unique_label,SEPARATOR_REPLACE_LABEL,other_line);
                      if(new_operand != other_line->operand_txt)
                        {
                          free(other_line->operand_txt);
                          other_line->operand_txt = new_operand;
                        }
                    }                                              
                }
            }
        }
    }

  /******************************************/
  /** On recherche le premier Label global **/
  /******************************************/
  for(begin_global_line=first_line; begin_global_line; begin_global_line=begin_global_line->next)
    {
      /* On ignore les lignes commentaires / celles sans labels / Celles avec des labels Variable ] */
      if(begin_global_line->type == LINE_COMMENT || begin_global_line->is_valid == 0)
        continue;
      if(strlen(begin_global_line->label_txt) == 0)
        continue;
      if(begin_global_line->label_txt[0] == ']')
        continue;
      if(begin_global_line->was_local_label == 1)
        continue;
      
      /* Erreur : On ne peut pas commencer son source par un Label Local */
      if(begin_global_line->label_txt[0] == ':')
        {
          printf("      => [Error] Wrong Local Label : '%s' in file '%s' (line %d).\n",begin_global_line->data,begin_global_line->file->file_name,begin_global_line->file_line_number);
          return(1);
        }
        
      /* On est sur le 1er Label global */
      break;
    }
    
  /* Aucun Label global dans le source => On prend le début du fichier comme référence */
  if(begin_global_line == NULL)
    begin_global_line = first_line;

  /**************************************************************/
  /** On traite les Label locaux situés entre 2 labels globaux **/
  /**************************************************************/
  while(begin_global_line)
    {
      /* Recherche le label global suivant */
      for(nb_local=0,end_global_line = begin_global_line->next; end_global_line; end_global_line=end_global_line->next)
        {
          /* On saute */
          if(end_global_line->type == LINE_COMMENT || end_global_line->is_valid == 0)
            continue;
          if(strlen(end_global_line->label_txt) == 0)
             continue;
          if(end_global_line->label_txt[0] == ']')
             continue;
          if(end_global_line->was_local_label == 1)
            continue;
            
          /* Comptabilise */ 
          if(end_global_line->label_txt[0] == ':')
            {
              nb_local++;
              continue;
            }
            
          /* Nouveau label global */
          break;
        }

      /** Pas de Label Global => On prend la dernière ligne du Source **/
      if(end_global_line == NULL)
        end_global_line = last_line;

      /** On a fini **/
      if(nb_local == 0 && end_global_line == last_line)
        return(0);

      /** Aucun label local dans l'intervalle, on va au suivant **/
      if(nb_local == 0 && end_global_line != NULL)
        {
          begin_global_line = end_global_line;
          continue;
        }

      /** il y a du label local dans l'intervalle, on traite **/
      if(nb_local > 0)
        {
          /** On va passer toutes les lignes de l'intervalle en revue pour y corriger tous les labels locaux **/
          for(current_line=begin_global_line; current_line; current_line=current_line->next)
            {
              /* Saute les lignes non valides */
              if(current_line->is_valid == 0)
                continue;

              /** On recherche un Label local **/
              if(current_line->label_txt[0] == ':')
                {
                  /* Création d'un label unique */
                  GetUNID(unique_label);
                  
                  /** On effectue le remplacement dans tout l'intervalle **/
                  for(replace_line=begin_global_line; replace_line; replace_line=replace_line->next)
                    {
                      /** Remplace le Label dans l'Operand **/
                      if(replace_line->type == LINE_CODE || replace_line->type == LINE_DATA || replace_line->type == LINE_MACRO)
                        {
                          new_operand = ReplaceInOperand(replace_line->operand_txt,current_line->label_txt,unique_label,SEPARATOR_REPLACE_LABEL,replace_line);
                          if(new_operand != replace_line->operand_txt)
                            {
                              free(replace_line->operand_txt);
                              replace_line->operand_txt = new_operand;
                            }
                        }
                                              
                      /* Fin de zone */
                      if(replace_line == end_global_line)
                        break;
                    }
                    
                  /** Remplace le label de la ligne **/
                  new_label = strdup(unique_label);
                  if(new_label == NULL)
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new local label");
                  free(current_line->label_txt);
                  current_line->label_txt = new_label;
                  current_line->was_local_label = 1;     /* Ce label a été un label local */
                }
                
              /* Fin de la zone */
              if(current_line == end_global_line)
                break;
            }
        }
    }

  /* OK */
  return(0);
}


/*******************************************************************************************************/
/*  ProcessMacroLineLocalLabel() :  On remplace les labels locaux par des unid_ des lignes des Macros. */
/*******************************************************************************************************/
static int ProcessMacroLineLocalLabel(struct macro_line *first_line, struct macro_line *last_line, struct macro *current_macro, struct omf_segment *current_omfsegment)
{
  struct macro_line *current_line;
  struct macro_line *begin_global_line;
  struct macro_line *end_global_line;
  struct macro_line *replace_line;
  int nb_local;
  char *new_label;
  char *new_operand;
  char unique_label[256];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /** On recherche le premier Label global **/
  for(begin_global_line=first_line; begin_global_line; begin_global_line=begin_global_line->next)
    {
      /* On ignore les lignes commentaires / celles sans labels / Celles avec des labels Variable ] */
       if(strlen(begin_global_line->label) == 0)
         continue;
       if(begin_global_line->label[0] == ']')
         continue;
        
      /* On est sur le 1er Label global */
      break;
    }
    
  /* Aucun Label global dans le source => On prend la 1ère ligne comme référence */
  if(begin_global_line == NULL)
    begin_global_line = first_line;
    
  /** On traite les Label locaux situées entre 2 labels globaux **/
  while(begin_global_line)
    {
      /* Recherche le label global suivant */
      for(nb_local=0,end_global_line = begin_global_line->next; end_global_line; end_global_line=end_global_line->next)
        {
          /* On saute */
          if(strlen(end_global_line->label) == 0)
             continue;
          if(end_global_line->label[0] == ']')
             continue;
            
          /* Comptabilise */ 
          if(end_global_line->label[0] == ':')
            {
              nb_local++;
              continue;
            }
            
          /* Nouveau label global */
          break;
        }

      /* On a atteind la fin sans rencontrer de Label Global */
      if(end_global_line == NULL)
        end_global_line = last_line;

      /** On a fini **/
      if(nb_local == 0 && end_global_line == last_line)
        return(0);

      /** Aucun label local dans l'intervalle, on va au suivant **/
      if(nb_local == 0 && end_global_line != NULL)
        {
          begin_global_line = end_global_line;
          continue;
        }

      /** il y a du label local dans l'intervalle, on traite **/
      if(nb_local > 0)
        {
          /** On va passer toutes les lignes de l'intervalle en revue pour y corriger tous les labels locaux **/
          for(current_line=begin_global_line; current_line; current_line=current_line->next)
            {
              /** On recherche un Label local **/
              if(current_line->label[0] == ':')
                {
                  /* Création d'un label unique */
                  GetUNID(unique_label);
                  
                  /** On effectue le remplacement dans tout l'intervalle **/
                  for(replace_line=begin_global_line; replace_line; replace_line=replace_line->next)
                    {
                      /** Remplace le Label dans l'Operand **/
                      new_operand = ReplaceInOperand(replace_line->operand,current_line->label,unique_label,SEPARATOR_REPLACE_LABEL,NULL);
                      if(new_operand != replace_line->operand)
                        {
                          free(replace_line->operand);
                          replace_line->operand = new_operand;
                        }
                                              
                      /* Fin de zone */
                      if(replace_line == end_global_line)
                        break;
                    }
                    
                  /** Remplace le label de la ligne **/
                  new_label = strdup(unique_label);
                  if(new_label == NULL)
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new local label in Macro");
                  free(current_line->label);
                  current_line->label = new_label;
                }
                
              /* Fin de la zone */
              if(current_line == end_global_line)
                break;
            }
        }
    }

  /* OK */
  return(0);
}


/*********************************************************************************/
/*  ProcessAllVariableLabel() :  On remplace les ]labels Variable par des unid_. */
/*********************************************************************************/
int ProcessAllVariableLabel(struct omf_segment *current_omfsegment)
{
  int i, error, nb_macro;
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *last_line = NULL;
  struct macro *current_macro;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** On traite les lignes du Source **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    if(current_line->next == NULL)
      last_line = current_line;
  error = ProcessSourceLineVariableLabel(first_file->first_line,last_line,current_omfsegment);

  /** On traite les Macros **/
  my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
  for(i=1; i<=nb_macro; i++)
    {
      my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);

      /** Traite les lignes de cette Macro **/
      error = ProcessMacroLineVariableLabel(current_macro->first_line,current_macro->last_line,current_macro,current_omfsegment);
    }

  /* OK */
  return(0);
}


/**************************************************************************************************/
/*  ProcessSourceLineVariableLabel() :  On remplace les ]labels variable du source par des unid_. */
/**************************************************************************************************/
int ProcessSourceLineVariableLabel(struct source_line *first_line, struct source_line *last_line, struct omf_segment *current_omfsegment)
{
  struct source_line *replace_line;
  struct source_line *begin_variable_line;
  struct source_line *end_variable_line;
  int use_address;
  char *new_label;
  char *new_operand;
  char unique_label[256];
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /*** Traite tous les Labels Variable ]label ***/
  for(begin_variable_line=first_line; begin_variable_line; begin_variable_line=begin_variable_line->next)
    {
      /* On ignore les lignes commentaires / celles sans labels */
      if(begin_variable_line->type == LINE_COMMENT || strlen(begin_variable_line->label_txt) == 0 || begin_variable_line->is_valid == 0)
        continue;
      if(begin_variable_line->type == LINE_VARIABLE)
        {
          /* Il faut différencer une vrai Equivalence d'un label ]LP = * */
          if(strchr(begin_variable_line->operand_txt,'*') == NULL)
            continue;

          /** Peut t'on détecter un * utilisé comme valeur ? **/
          use_address = UseCurrentAddress(begin_variable_line->operand_txt,&buffer_error[0],begin_variable_line);
          if(strlen(buffer_error) > 0)
            {
              printf("    Error : Impossible to analyze Operand '%s' in source file '%s' (line %d) : %s.\n",
                     begin_variable_line->operand_txt,begin_variable_line->file->file_name,begin_variable_line->file_line_number,buffer_error);
              return(1);
            }
          if(use_address == 0)
            continue;
         }

      /** Début de la zone de recherche **/
      if(begin_variable_line->label_txt[0] == ']')
        {
          /** Cherche la fin de la zone de recherche = Autre Label variable avec le même nom **/
          for(end_variable_line=begin_variable_line->next; end_variable_line; end_variable_line=end_variable_line->next)
            {
              /* On ignore les lignes commentaires / celles sans labels */
               if(end_variable_line->type == LINE_COMMENT || strlen(end_variable_line->label_txt) == 0 || end_variable_line->is_valid == 0)
                 continue;

               /* On recherche le même Label (Case sensitive) */
               if(!strcmp(begin_variable_line->label_txt,end_variable_line->label_txt))
                 break;
            }

          /* Création d'un label unique */
          GetUNID(unique_label);

          /** On remplace sur la zone **/
          for(replace_line=begin_variable_line; replace_line != end_variable_line; replace_line=replace_line->next)
            {
              /* On ne traite pas les lignes invalides */
              if(replace_line->is_valid == 0)
                continue;

              /** Remplace le Label dans l'Operand **/
              new_operand = ReplaceInOperand(replace_line->operand_txt,begin_variable_line->label_txt,unique_label,SEPARATOR_REPLACE_VARIABLE,replace_line);
              if(new_operand != replace_line->operand_txt)
                {
                  free(replace_line->operand_txt);
                  replace_line->operand_txt = new_operand;
                }
            }

          /** Remplace le label de la ligne **/
          new_label = strdup(unique_label);
          if(new_label == NULL)
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new variable label");
          free(begin_variable_line->label_txt);
          begin_variable_line->label_txt = new_label;
        }
    }

  /* OK */
  return(0);
}


/****************************************************************************************************/
/*  ProcessMacroLineVariableLabel() :  On remplace les ]labels variable d'une Macro par des unid_. */
/****************************************************************************************************/
int ProcessMacroLineVariableLabel(struct macro_line *first_line, struct macro_line *last_line, struct macro *current_macro, struct omf_segment *current_omfsegment)
{
  struct macro_line *replace_line;
  struct macro_line *begin_variable_line;
  struct macro_line *end_variable_line;
  int use_address;
  char *new_label;
  char *new_operand;
  char buffer_error[1024];
  char unique_label[256];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /*** Traite tous les Labels Variable ]label ***/
  for(begin_variable_line=first_line; begin_variable_line; begin_variable_line=begin_variable_line->next)
    {
      /* On ignore les lignes commentaires / celles sans labels */
      if(strlen(begin_variable_line->label) == 0)
        continue;
      if(!strcmp(begin_variable_line->opcode,"="))
        {
          /* Il faut différencer une vrai Equivalence d'un label ]LP = * */
          if(strchr(begin_variable_line->operand,'*') == NULL)
            continue;

          /** Peut t'on détecter un * utilisé comme valeur ? **/
          use_address = UseCurrentAddress(begin_variable_line->operand,&buffer_error[0],NULL);
          if(strlen(buffer_error) > 0)
            {
              printf("    Error : Impossible to analyze Operand '%s' in Macro %s : %s.\n",begin_variable_line->operand,current_macro->name,buffer_error);
              return(1);
            }
          if(use_address == 0)
            continue;
        }

      /** Début de la zone de recherche **/
      if(begin_variable_line->label[0] == ']')
        {
          /** Cherche la fin de la zone de recherche = Autre Label variable avec le même nom **/
          for(end_variable_line=begin_variable_line->next; end_variable_line; end_variable_line=end_variable_line->next)
            {
              /* On ignore les lignes commentaires / celles sans labels */
               if(strlen(end_variable_line->label) == 0)
                 continue;

               /* On recherche le même Label (Case sensitive) */
               if(!strcmp(begin_variable_line->label,end_variable_line->label))
                 break;
            }

          /* Création d'un label unique */
          GetUNID(unique_label);

          /** On remplace sur la zone **/
          for(replace_line=begin_variable_line; replace_line != end_variable_line; replace_line=replace_line->next)
            {
              /** Remplace le Label dans l'Operand **/
              new_operand = ReplaceInOperand(replace_line->operand,begin_variable_line->label,unique_label,SEPARATOR_REPLACE_VARIABLE,NULL);
              if(new_operand != replace_line->operand)
                {
                  free(replace_line->operand);
                  replace_line->operand = new_operand;
                }
            }

          /** Remplace le label de la ligne **/
          new_label = strdup(unique_label);
          if(new_label == NULL)
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new variable label in Macro");
          free(begin_variable_line->label);
          begin_variable_line->label = new_label;
        }
    }

  /* OK */
  return(0);
}


/*******************************************************/
/*  ProcessEquivalence() :  Remplace les Equivalences. */
/*******************************************************/
int ProcessEquivalence(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;
  int nb_modified;

  /* Init */
  nb_modified = 0;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /*** Traite tous les Operands pouvant contenir des Equivalence ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On ignore les lignes commentaires / celles sans labels */
      if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY || current_line->type == LINE_GLOBAL || strlen(current_line->operand_txt) == 0)
        continue;

      /* On ne fait pas de remplacement sur les lignes de Data HEX */
      if(current_line->type == LINE_DATA && !my_stricmp(current_line->opcode_txt,"HEX"))
        continue;

      /* Remplace les Equivalences sur la Ligne */
      nb_modified += ProcessLineEquivalence(current_line,current_omfsegment);
    }

  /* OK */
  return(0);
}


/************************************************************************/
/*  ProcessLineEquivalence() :  Remplace les équivalences pour 1 ligne. */
/************************************************************************/
int ProcessLineEquivalence(struct source_line *current_line, struct omf_segment *current_omfsegment)
{
  int i, modified, nb_element, is_variable, is_label, is_hexa, nb_byte;
  char **tab_element;
  char *new_operand;
  char variable_name[1024];
  struct equivalence *current_equivalence;
  struct variable *current_variable;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  modified = 0;

  /** On va découper l'opérande en plusieurs éléments unitaires **/
  tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_REPLACE_LABEL,current_line);
  if(tab_element == NULL)
    my_RaiseError(ERROR_RAISE,"Impossible to decode Operand as element table");
    
  /** On reconstruit la chaine en remplaçant les valeurs (case sensitive) **/
  for(i=0,param->buffer_operand[0]='\0'; i<nb_element; i++)
    {
      my_Memory(MEMORY_SEARCH_EQUIVALENCE,tab_element[i],&current_equivalence,current_omfsegment);
      if(current_equivalence != NULL)
        {
          /* Cas particulier d'une variable ]var ou d'un label ]var et d'une équivalence var */
          is_variable = 0;
          is_label = 0;
          is_hexa = 0;
          if(i > 0)
            {
              if(!strcmp(tab_element[i-1],"]"))
                {
                  sprintf(variable_name,"]%s",tab_element[i]);
                  my_Memory(MEMORY_SEARCH_VARIABLE,variable_name,&current_variable,current_omfsegment);
                  if(current_variable != NULL)
                    is_variable = 1;
                  else
                    {
                      /* On regarde s'il n'existerait pas un label local portant ce nom */
                      if(IsLocalLabel(variable_name,current_omfsegment))
                        is_label = 1;
                    }
                }
              else if(!strcmp(tab_element[i-1],"$"))
                {
                  /* Peut t'on interpretter $variable comme un nombre Hexa ? */
                  sprintf(variable_name,"$%s",tab_element[i]);
                  if(IsHexaDecimal(variable_name,&nb_byte))
                    is_hexa = 1;
                }
            }

          /* On ne va pas remplacer si c'est finalement une Variable / Label / une forme hexa */
          if(is_variable || is_label || is_hexa)
            strcat(param->buffer_operand,tab_element[i]);               /* Variable */
          else
            {
              strcat(param->buffer_operand,"{");
              strcat(param->buffer_operand,current_equivalence->value);   /* Equivalence */
              strcat(param->buffer_operand,"}");
            }
        }
      else
        strcat(param->buffer_operand,tab_element[i]);
    }
  
  /* Libération mémoire du tableau de valeurs */
  mem_free_table(nb_element,tab_element);

  /** Remplace l'Operande (si qqchose a été changé) **/
  if(strcmp(param->buffer_operand,current_line->operand_txt))
    {
      /* Nouvelle chaine */
      new_operand = strdup(param->buffer_operand);
      if(new_operand == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory to replace an Equivalence");

      /* Libère l'ancienne */
      free(current_line->operand_txt);

      /* ositionne la nouvelle */
      current_line->operand_txt = new_operand;

      /* Modification */
      modified = 1;
    }

  /* At t'on modifié la ligne ? */
  return(modified);
}


/**********************************************/
/*  IsLocalLabel() :  Est-ce un label local ? */
/**********************************************/
static int IsLocalLabel(char *label_name, struct omf_segment *current_omfsegment)
{
  struct source_line *current_line;
  struct source_file *first_file;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ne prend pas les Variables dans les Macro */
      if(current_line->type != LINE_CODE && current_line->type_aux != LINE_DATA)
        continue;

      /* Recherche le ]label */
      if(strcmp(current_line->label_txt,label_name))
        return(1);
    }

  /* Pas trouvé */
  return(0);
}


/*******************************************************************/
/*  BuildVariableTable() :  Construction des tables des Variables. */
/*******************************************************************/
int BuildVariableTable(struct omf_segment *current_omfsegment)
{
  struct variable *current_variable;
  struct source_line *current_line;
  struct source_file *first_file;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /* Init */
  my_Memory(MEMORY_SORT_VARIABLE,NULL,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On ne prend pas les Variables dans les Macro */
      if(current_line->type == LINE_DIRECTIVE && current_line->type_aux == LINE_MACRO_DEF)
        continue;

      /* On ne prend que les lignes avec label */
      if(strlen(current_line->label_txt) == 0)
        continue;

      /* On ne prend que les Variables ]XX = */
      if(current_line->type != LINE_VARIABLE)
        continue;

      /** Recherche d'une variable de même nom déjà existante **/
      my_Memory(MEMORY_SEARCH_VARIABLE,current_line->label_txt,&current_variable,current_omfsegment);

      /** Allocation d'une nouvelle structure Variable **/
      if(current_variable == NULL)
        {
          current_variable = (struct variable *) calloc(1,sizeof(struct variable));
          if(current_variable == NULL)
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure variable");
          current_variable->name = strdup(current_line->label_txt);
          if(current_variable->name == NULL)
            {
              free(current_variable);
              my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure variable");
            }

          /* Initialisation à 0 */
          current_variable->value = 0;

          /* Format $, #$ */
          if(current_line->operand_txt[0] == '$')
            current_variable->is_dollar = 1;
          else if(current_line->operand_txt[0] == '#')
            {
              if(current_line->operand_txt[1] == '$')
                current_variable->is_pound_dollar = 1;
              else
                current_variable->is_pound = 1;
            }

          /* Déclaration de la structure */
          my_Memory(MEMORY_ADD_VARIABLE,current_variable,NULL,current_omfsegment);
        }

      /* On fait pointer cette ligne vers la variable */
      current_line->variable = current_variable;
    }

  /* Tri les Variables */
  my_Memory(MEMORY_SORT_VARIABLE,NULL,NULL,current_omfsegment);

  /* OK */
  return(0);
}


/*******************************************************************/
/*  BuildReferenceTable() :  Construction des tables de référence. */
/*******************************************************************/
void BuildReferenceTable(struct omf_segment *current_omfsegment)
{
  int i;

  /** Opcode **/
  for(i=0; opcode_list[i]!=NULL; i++)
    my_Memory(MEMORY_ADD_OPCODE,opcode_list[i],NULL,current_omfsegment);
  my_Memory(MEMORY_SORT_OPCODE,NULL,NULL,current_omfsegment);

  /** Data **/
  for(i=0; data_list[i]!=NULL; i++)
    my_Memory(MEMORY_ADD_DATA,data_list[i],NULL,current_omfsegment);
  my_Memory(MEMORY_SORT_DATA,NULL,NULL,current_omfsegment);

  /** Directive **/
  for(i=0; directive_list[i]!=NULL; i++)
    my_Memory(MEMORY_ADD_DIRECTIVE,directive_list[i],NULL,current_omfsegment);
  my_Memory(MEMORY_SORT_DIRECTIVE,NULL,NULL,current_omfsegment);

  /** DirectiveEqu **/
  for(i=0; equivalence_list[i]!=NULL; i++)
    my_Memory(MEMORY_ADD_DIREQU,equivalence_list[i],NULL,current_omfsegment);
  my_Memory(MEMORY_SORT_DIREQU,NULL,NULL,current_omfsegment);
}


/***********************************************************************/
/*  ProcessMXDirective() :  On va reconnaitres les MX de chaque ligne. */
/***********************************************************************/
int ProcessMXDirective(struct omf_segment *current_omfsegment)
{
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  int is_reloc;
  int64_t value;
  char m, x;
  struct source_file *first_file;
  struct source_line *current_line;
  struct external *current_external;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  m = '1';
  x = '1';

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

      /** Nouvelle valeur de M et X **/
      if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"MX"))
        {
          /* Récupère la valeur */
          value = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Impossible to evaluate MX value '%s' (line %d from file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
          if(value < 0 || value > 3)
            {
              sprintf(param->buffer_error,"Bad value '%d' for MX directive '%s' (line %d from file '%s') : %s",(int)value,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Décode M et X */
          m = ((value & 0x02) == 0) ? '0' : '1';
          x = ((value & 0x01) == 0) ? '0' : '1';

          /** On place les valeurs MX sur la ligne **/
          current_line->m[0] = m;
          current_line->x[0] = x;
        }
      else if(current_line->type == LINE_CODE && (!my_stricmp(current_line->opcode_txt,"REP") || !my_stricmp(current_line->opcode_txt,"SEP")))
        {
          /* Récupère la valeur */
          value = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Impossible to evaluate %s value '%s' (line %d from file '%s') : %s",current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
          if(value < 0 || value > 256)
            {
              sprintf(param->buffer_error,"Bad value '%d' for %s '%s' (line %d from file '%s') : %s",(int)value,current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /* Décode M et X */
          if(!my_stricmp(current_line->opcode_txt,"REP"))
            {
              m = ((value & 0x20) == 0) ? m : '0';
              x = ((value & 0x10) == 0) ? x : '0';
            }
          else   /* SEP */
            {
              m = ((value & 0x20) == 0) ? m : '1';
              x = ((value & 0x10) == 0) ? x : '1';
            }

          /** On place les valeurs MX sur la ligne **/
          current_line->m[0] = m;
          current_line->x[0] = x;
        }
      else if(current_line->type == LINE_CODE && !my_stricmp(current_line->opcode_txt,"SEC") && current_line->next != NULL)
        {
          /** On place les valeurs MX sur la ligne **/
          current_line->m[0] = m;
          current_line->x[0] = x;

          /* On a un XCE qui suit => 8 bit */
          if(current_line->next->is_valid == 1 && current_line->next->type == LINE_CODE && !my_stricmp(current_line->next->opcode_txt,"XCE"))
            {
              m = '1';
              x = '1';
            }
        }
      else
        {
          /** On place les valeurs MX sur la ligne **/
          current_line->m[0] = m;
          current_line->x[0] = x;
        }
    }

  /* OK */
  return(0);
}


/**********************************************************************/
/*  EvaluateVariableLine() :  Evaluation de la variable sur sa ligne. */
/**********************************************************************/
int EvaluateVariableLine(struct source_line *current_line, struct omf_segment *current_omfsegment)
{
  int64_t value;
  int is_reloc;
  BYTE byte_count, bit_shift; 
  WORD offset_reference;
  DWORD address_long;
  char buffer_error[1024] = "";
  struct variable *current_variable;
  struct external *current_external;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /** On va rechercher la variable **/
  my_Memory(MEMORY_SEARCH_VARIABLE,current_line->label_txt,&current_variable,current_omfsegment);
  if(current_variable == NULL)
    return(0);

  /** On va évaluer la variable **/
  value = EvalExpressionAsInteger(current_line->operand_txt,&buffer_error[0],current_line,4,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
  if(strlen(buffer_error) > 0)
    {
      sprintf(param->buffer_error,"Impossible to evaluate Variable '%s' value '%s' (line %d, file '%s') : %s",
              current_variable->name,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
      my_RaiseError(ERROR_RAISE,param->buffer_error);      
    }

  /* Stocke la valeur */
  current_variable->value = value;

  /* OK */
  return(0);
}


/***********************************************************************/
/*  ComputeLineAddress() :  Détermine les adresses des lignes valides. */
/***********************************************************************/
int ComputeLineAddress(struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
  BYTE byte_count, bit_shift;
  WORD offset_reference;
  DWORD address_long;
  int64_t new_address_64, dum_address_64;
  int line_number, current_address, global_address, new_address, dum_address, nb_byte, has_previous_label, is_reloc, is_first_org, is_fix_address;
  int current_bank, global_bank, new_bank, dum_bank;
  struct source_file *first_file;
  struct source_line *current_line;
  struct external *current_external;
  char *next_sep;
  char operand[1024];
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  if(current_omfproject->is_omf == 1)
    {
      /* Le code OMF est relogeable */
      current_bank = 0;
      current_address = 0x0000;
      current_omfsegment->is_omf = 1;
      is_fix_address = 0;
    }
  else if(current_omfproject->is_single_binary == 1)
    {
      /* L'adresse ORG est transmise de Segment en Segment (qui s'enchainent) */
      current_bank = (current_omfsegment->org_address >> 16);
      current_address = 0xFFFF & current_omfsegment->org_address;
      is_fix_address = 0;
    }
  else
    {
      current_bank = 0;
      current_address = 0x8000;
      is_fix_address = 1;
    }
  global_bank = current_bank;
  global_address = current_address;
  has_previous_label = 0;
  line_number = 1;
  is_first_org = 1;

  /* Récupère le 1er fichier source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /** Numérote toutes les lignes **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes invalides */
      if(current_line->is_valid == 0)
        continue;

      /* Numéro de la ligne du Projet */
      current_line->line_number = line_number++;
    }

  /** Recherche un REL si on ne sait pas encore quel est le type de projet **/
  if(current_omfproject->is_omf == 0 && current_omfproject->is_single_binary == 0 && current_omfproject->is_multi_fixed == 0)
    {
      for(current_line=first_file->first_line; current_line; current_line=current_line->next)
        {
          /* On ignore les lignes invalides */
          if(current_line->is_valid == 0)
            continue;

          /** On cherche le 1er REL **/
          if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"REL"))
            {
              /* On ne doit rien y avoir avant */
              if(has_previous_label == 1)
                {
                  sprintf(param->buffer_error,"Error : The REL directive should be located at the top of the file (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

              /* L'assemblage va commence en $0000 */
              current_bank = 0;
              current_address = 0x0000;
              global_bank = 0;
              global_address = 0x0000;

              /* Le fichier est relogeable au format OMF */
              current_omfsegment->is_omf = 1;
              current_omfproject->is_omf = 1;
              is_fix_address = 0;
              break;
            }
          else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"USE"))
            has_previous_label = 1;
          else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"PUT"))
            has_previous_label = 1;
          else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"PUTBIN"))
            has_previous_label = 1;
          else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"OBJ") && current_omfsegment->is_omf == 1)
            {
              /* Never Here */
              sprintf(param->buffer_error,"Error : The OBJ directive is not allowed with source code having already define a REL directive (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
          else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"ORG") && current_omfsegment->is_omf == 1)
            {
              /* Never Here */
              sprintf(param->buffer_error,"Error : The ORG directive is not allowed with source code having already define a REL directive (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }            
          else if((current_line->type == LINE_CODE || current_line->type == LINE_DATA || current_line->type == LINE_MACRO || current_line->type == LINE_EMPTY || current_line->type == LINE_GLOBAL) && strlen(current_line->label_txt) > 0)
            has_previous_label = 1;
        }
    }
    
  /*** Passe en revue toutes les lignes ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /** Directive modifiant l'adresse : ORG+DUM **/
      if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"ORG"))
        {
          /** Org $Addr (pour les OMF et les SingleBinary, on a une zone [ORG $Addr - ORG] à adresse fixe) **/
          if(strlen(current_line->operand_txt) > 0)
            { 
              /* Récupère la nouvelle addresse */
              new_address_64 = EvalExpressionAsInteger(current_line->operand_txt,&buffer_error[0],current_line,2,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
              if(strlen(buffer_error) > 0)
                {
                  sprintf(param->buffer_error,"Error : Impossible to evaluate ORG Address : '%s' (line %d, file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
              /* On ne conserve que 32 bit */
              new_address = (int) (0xFFFFFFFF & new_address_64);

              /* On reste dans les 64 KB */
              new_bank = new_address >> 16;
              new_address = new_address & 0xFFFF;

              /* Nouvelle addresse */
              current_line->bank = current_bank;
              current_line->address = current_address;
              current_line->is_fix_address = is_fix_address;
              current_line->global_bank = global_bank;          /* Adresse sans tenir compte des [ORG $Addr ORG] */
              current_line->global_address = global_address;
              current_bank = new_bank;
              current_address = new_address;

              /* Le premier ORG nous sert à définir l'adresse globale (pour les binaires à adresse fixe) */
              if(is_first_org == 1 && current_omfproject->is_omf == 0 && current_omfproject->is_single_binary == 0)
                {
                  global_bank = new_bank;
                  global_address = new_address;
                  is_first_org = 0;
                }
                
              /* A partir de maintenant toutes les lignes sont en adresses Fixes => pas relogeable */
              is_fix_address = 1;
              continue;
            }
          else    /* ORG */
            {
              /** On rétablit l'adresse du fichier global **/
              current_bank = global_bank;
              current_address = global_address;
              
              /* A partir de maintenant toutes les lignes OMF / SingleBinary ne sont plus en adresses Fixes */
              if(current_omfproject->is_omf == 1 || current_omfproject->is_single_binary == 1)
                is_fix_address = 0;
            }
        }
      else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"DUM"))
        {
          /* Récupère la nouvelle addresse */
          dum_address_64 = EvalExpressionAsInteger(current_line->operand_txt,&buffer_error[0],current_line,2,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Error : Impossible to evaluate DUM Address : '%s' (line %d, file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
          /* On ne conserve que 32 bit */
          dum_address = (int) (0xFFFFFFFF & dum_address_64);

          /* On reste dans les 64 KB */
          dum_bank = dum_address >> 16;
          dum_address = 0xFFFF & dum_address;
        }

      /* On met l'adresse courrante à la ligne */
      if(current_line->is_dum == 1)
        {
          current_line->bank = dum_bank;
          current_line->address = dum_address;
          current_line->is_fix_address = is_fix_address;
          current_line->global_bank = global_bank;          /* Adresse sans tenir compte des [ORG $Addr ORG] */
          current_line->global_address = global_address;

          /* On définit l'adresse suivante */
          if(current_line->nb_byte == 0xFFFF)
            {
              /* Cas particulier des lignes DS \ : Alignement sur le prochain $100 */
              nb_byte = 0x100 - (dum_address & 0x0000FF);
              current_line->nb_byte = nb_byte;
              dum_address += nb_byte;
            }
          else if(current_line->nb_byte == 0xFFFFF)
            {
              /** Cas particulier des lignes DS avec des Labels dedans : On essaye de ré-évaluer **/
              /* Isole l'expression indiquant la longueur */
              strcpy(operand,current_line->operand_txt);
              next_sep = strchr(operand,',');
              if(next_sep)
                *next_sep = '\0';

              /* Calcule l'expression */
              nb_byte = (int) EvalExpressionAsInteger(operand,&buffer_error[0],current_line,3,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
              if(strlen(buffer_error) > 0)
                {
                  sprintf(param->buffer_error,"Error : Impossible to evaluate DS data size : '%s' (line %d, file '%s') : %s",operand,current_line->file_line_number,current_line->file->file_name,buffer_error);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
              if(nb_byte < 0)
                {
                  sprintf(param->buffer_error,"Error : Evaluation of DS data size ends up as negative value (%d) : '%d' (line %d, file '%s')",nb_byte,operand,current_line->file_line_number,current_line->file->file_name);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
                
              /* On a enfin la taille occupée par la ligne */
              current_line->nb_byte = nb_byte;
              dum_address += nb_byte;
            }
          else
            {
              /* On saute de la taille de l'instruction */
              dum_address += current_line->nb_byte;
            }

          /* Erreur : on dépasse 64 KB */
          if(dum_address > 0xFFFF)
            {
              sprintf(param->buffer_error,"Error : DUM Object code size > 64 KB (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
      else
        {
          /* Adresse de la ligne */
          current_line->bank = current_bank;
          current_line->address = current_address;
          current_line->is_fix_address = is_fix_address;
          current_line->global_bank = global_bank;          /* Adresse sans tenir compte des [ORG $Addr ORG] */
          current_line->global_address = global_address;

          /* On définit l'adresse suivante */
          if(current_line->nb_byte == 0xFFFF)
            {
              /* Cas particulier des lignes DS \ : Alignement sur le prochain $100 */
              nb_byte = 0x100 - (current_address & 0x0000FF);
              current_line->nb_byte = nb_byte;
              current_address += nb_byte;
              global_address += nb_byte;
            }
          else if(current_line->nb_byte == 0xFFFFF)
            {
              /** Cas particulier des lignes DS avec des Labels dedans : On essaye de ré-évaluer **/
              /* Isole l'expression indiquant la longueur */
              strcpy(operand,current_line->operand_txt);
              next_sep = strchr(operand,',');
              if(next_sep)
                *next_sep = '\0';

              /* Calcule l'expression */
              nb_byte = (int) EvalExpressionAsInteger(operand,&buffer_error[0],current_line,3,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
              if(strlen(buffer_error) > 0)
                {
                  sprintf(param->buffer_error,"Error : Impossible to evaluate DS data size : '%s' (line %d, file '%s') : %s",operand,current_line->file_line_number,current_line->file->file_name,buffer_error);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
              if(nb_byte < 0)
                {
                  sprintf(param->buffer_error,"Error : Evaluation of DS data size ends up as negative value (%d) : '%s' (line %d, file '%s')",nb_byte,operand,current_line->file_line_number,current_line->file->file_name);
                  my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
                
              /* On a enfin la taille occupée par la ligne */
              current_line->nb_byte = nb_byte;
              current_address += nb_byte;
              global_address += nb_byte;
            }
          else if(current_line->nb_byte > 0)
            {
              /* On saute de la taille de l'instruction */
              current_address += current_line->nb_byte;
              global_address += current_line->nb_byte;
            }

          /* Erreur : on dépasse 64 KB */
          if(current_address > 0x10000)     /* bug 0xFFFF */
            {
              sprintf(param->buffer_error,"Error : Object code size > 64 KB (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }

  /* OK */
  return(0);
}


/********************************************************************************/
/*  BuildRelocateAddress() :  On signale une adresse comme devant être patchée. */
/********************************************************************************/
struct relocate_address *BuildRelocateAddress(BYTE ByteCnt, BYTE BitShiftCnt, WORD OffsetPatch, WORD OffsetReference, struct external *current_external, struct omf_segment *current_omfsegment)
{
  struct relocate_address *current_address;
  struct relocate_address *next_address;

  /* Allocation mémoire */
  current_address = (struct relocate_address *) calloc(1,sizeof(struct relocate_address));
  if(current_address == NULL)
    my_RaiseError(ERROR_RAISE,"Error : Can't allocate memory for relocate_address structure.");

  /* Remplissage */
  current_address->ByteCnt = ByteCnt;
  current_address->BitShiftCnt = BitShiftCnt;
  current_address->OffsetPatch = OffsetPatch;
  current_address->OffsetReference = OffsetReference;

  /* Si on se référe à un label externe au Segment */
  current_address->external = current_external;

  /* Attache en triant les adresses OffsetPath */
  if(current_omfsegment->first_address == NULL)
    {
      current_omfsegment->first_address = current_address;
      current_omfsegment->last_address = current_address;
    }
  else
    {
      /* Ajoute en 1ère position */
      if(current_address->OffsetPatch < current_omfsegment->first_address->OffsetPatch)
        {
          current_address->next = current_omfsegment->first_address;
          current_omfsegment->first_address = current_address;
        }
      else if(current_address->OffsetPatch >= current_omfsegment->last_address->OffsetPatch)
        {
          /* Attache en dernière position */
          current_omfsegment->last_address->next = current_address;
          current_omfsegment->last_address = current_address;
        }
      else
        {
          /* Attache au milieu */
          for(next_address=current_omfsegment->first_address; ; next_address=next_address->next)
            if(next_address->next->OffsetPatch >= current_address->OffsetPatch)
              {
                current_address->next = next_address->next;
                next_address->next = current_address;
                break;
              }
        }
    }

  /* Une addresse de plus */
  current_omfsegment->nb_address++;

  return(current_address);
}


/**************************************************************************/
/*  CheckForUnknownLine() :  Recherche toutes les lignes non identifiées. */
/**************************************************************************/
int CheckForUnknownLine(struct omf_segment *current_omfsegment)
{
  int nb_error;
  struct source_line *current_line;
  struct source_file *first_file;

  /* Init */
  nb_error = 0;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On ne prend pas les Label dans les Macro */
      if(current_line->type == LINE_UNKNOWN)
        {
          printf("      => [Error] Unkown line : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
          nb_error++;
        }
    }

  /* Renvoi le nombre d'erreur détectés */
  return(nb_error);
}


/*************************************************************/
/*  CheckForDumLine() :  Vérifie toutes les lignes DUM-DEND. */
/*************************************************************/
int CheckForDumLine(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *dend_line;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /* On va rechercher les zones DUM-DEND */
      if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"DUM"))  
        {
          /* On vérifie la présence d'un Opérand */
          if(strlen(current_line->operand_txt) == 0)
            {
              printf("      => [Error] Empty DUM line : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
              return(1);
            }

          /* On va rechercher le DEND et on marque toutes les lignes entre */
          for(dend_line=current_line; dend_line; dend_line=dend_line->next)
            {
              /* On ignore les lignes non valides */
              if(dend_line->is_valid == 0)
                continue;

              /* On marque la ligne */
              dend_line->is_dum = 1;
              if(dend_line != current_line)
                dend_line->dum_line = current_line;

              if(dend_line->type == LINE_DIRECTIVE && !my_stricmp(dend_line->opcode_txt,"DEND"))
                break;

              /* On ne devrait pas retomber sur un DUM */
              if(current_line != dend_line && dend_line->type == LINE_DIRECTIVE && !my_stricmp(dend_line->opcode_txt,"DUM"))
                {
                  printf("      => [Error] DUM line with DUM found before DEND : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
                  return(1);
                }
            }

          /* Pas de DEND ? */
          if(dend_line == NULL)
            {
              printf("      => [Error] DUM line without DEND : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
              return(1);
            }

          /* On continue après */
          current_line = dend_line;
        }
    }

  /* OK */
  return(0);
}


/*******************************************************/
/*  CheckForErrLine() :  Evalue toutes les lignes ERR. */
/*******************************************************/
int CheckForErrLine(struct omf_segment *current_omfsegment)
{
  int64_t value;
  int is_reloc;
  BYTE byte_count, bit_shift; 
  WORD offset_reference;
  DWORD address_long;
  char buffer_error[1024] = "";
  struct external *current_external;
  struct source_file *first_file;
  struct source_line *current_line;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes ERR en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /** On va rechercher les lignes ERR **/
      if(current_line->type == LINE_CODE && !my_stricmp(current_line->opcode_txt,"ERR"))  
        {
          /* On repasse la ligne en Directive pour le fichier Output */
          current_line->type = LINE_DIRECTIVE;

          /* On vérifie la présence d'un Opérand, sinon on considère qu'il n'y a pas d'erreur */
          if(strlen(current_line->operand_txt) == 0)
            continue;

          /** On va évaluer l'Opérande **/
          value = EvalExpressionAsInteger(current_line->operand_txt,&buffer_error[0],current_line,4,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              /* Erreur dans l'évaluation */
              printf("      => [Error] Impossible to evaluate ERR expression '%s' (line %d, file '%s') : %s\n",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              return(1);
            }

          /** Si c'est différent de Zéro, c'est une erreur **/
          if((int) value != 0)
            {
              /* On force une erreur */
              printf("      => [Error] The evaluation of ERR expression '%s' is '0x%X' (line %d, file '%s')\n",current_line->operand_txt,(int)value,current_line->file_line_number,current_line->file->file_name);
              return(1);
            }
        }
    }

  /* OK */
  return(0);
}


/***********************************************************************/
/*  CheckForDirectPageLine() :  Vérifie toutes les lignes Page Direct. */
/***********************************************************************/
int CheckForDirectPageLine(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;

  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes Page Direct en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /** On va rechercher les lignes de code Page Direct **/
      if(current_line->type == LINE_CODE && IsPageDirectAddressMode(current_line->address_mode))  
        {
          /* On vérifie la présence d'un Opérande valide */
          if(current_line->operand_value == 0xFFFFFFFF)
            continue;

          /** Si l'opérande n'est pas une addresse Page Direct => Error **/
          if((current_line->operand_value & 0xFFFFFF00) != 0x00000000)
            {
              /* On force une erreur */
              printf("      => [Bad Address Mode] Operand address '%s' (=0x%X) is located outside of the Direct Page (line %d, file '%s')\n",current_line->operand_txt,current_line->operand_value,current_line->file_line_number,current_line->file->file_name);
              return(1);
            }
        }
    }

  /* OK */
  return(0);
}


/**************************************************************************************************/
/*  ProcessDirectiveWithLabelLine() :  Conversion des Lignes Directive avec Label en Ligne vides. */
/**************************************************************************************************/
int ProcessDirectiveWithLabelLine(struct omf_segment *current_omfsegment)
{
  int found;
  struct source_file *first_file;
  struct source_line *current_line;
  struct source_line *other_line;
  
  /* Récupère le fichier Source */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

  /** Passe toutes les lignes Page Direct en revue **/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* On ignore les lignes non valides */
      if(current_line->is_valid == 0)
        continue;

      /** On va rechercher les lignes DIRECTIVE **/
      if(current_line->type == LINE_DIRECTIVE && current_line->is_inside_macro == 0 && strlen(current_line->label_txt) > 0)  
        {
          /* On vérifie que le label est utilisé = pointé par une autre ligne */
          for(other_line=first_file->first_line,found=0; other_line; other_line=other_line->next)
            {
              /* On ignore les lignes non valides */
              if(other_line->is_valid == 0)
                continue;
                
              /* Cette ligne pointe vers la ligne DIRECTIVE */
              if(other_line->operand_address_long == current_line->address)
                {
                  found = 1;
                  break;
                }
            }

          /** On passe la ligne en ligne EMPTY pour qu'elle soit affichée dans l'output **/
          if(found == 1)
            current_line->type = LINE_EMPTY;
        }
    }

  /* OK */
  return(0);
}

/*********************************************************/
/*  BuildSourceLine() :  Décodage d'une ligne de Source. */
/*********************************************************/
struct source_line *BuildSourceLine(struct source_file *current_file, int line_number)
{
  struct source_line *current_line;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Allocation mémoire */
  current_line = (struct source_line *) calloc(1,sizeof(struct source_line));
  if(current_line == NULL)
    return(NULL);
  current_line->type = LINE_UNKNOWN;

  /* Remplissage */
  current_line->file_line_number = line_number+1;
  current_line->line_data = current_file->tab_line[line_number];
  current_line->file = current_file;
  
  /* Cette ligne était dans le fichier Source (contrairement aux lignes venant des Macro ou des Lup) */
  current_line->is_in_source = 1;

  /* Par défaut, toute ligne est valide */
  current_line->is_valid = 1;

  /* Valeurs de E, C, M et X */ 
  strcpy(current_line->m,"?");
  strcpy(current_line->x,"?");

  /* Reloc */
  strcpy(current_line->reloc,"         ");

  /* Adresse */
  current_line->address = -1;    /* Pas encore déterminée */

  /* Taille du code objet */
  current_line->nb_byte = -1;    /* Pas encore déterminé */

  /* La ligne interdit t'elle le Page Direct ? */
  current_line->no_direct_page = 0;

  /* Valeur de l'Opérande */
  current_line->operand_value = 0xFFFFFFFF;

  /* Adresse Longue du Label pointé par l'opérande  */
  current_line->operand_address_long = 0xFFFFFFFF;

  /** Lignes commentaire **/
  strcpy(param->buffer_line,current_line->line_data);
  CleanBuffer(param->buffer_line);
  if(strlen(param->buffer_line) == 0 || param->buffer_line[0] == ';' || param->buffer_line[0] == '*')
    {
      /* On met du vide dans les champs */
      current_line->label_txt = strdup("");
      current_line->opcode_txt = strdup("");
      current_line->operand_txt = strdup("");
      current_line->comment_txt = strdup("");
      if(current_line->label_txt == NULL || current_line->opcode_txt == NULL ||
         current_line->operand_txt == NULL || current_line->comment_txt == NULL)
        {
          mem_free_sourceline(current_line);
          return(NULL);
        }
      current_line->type = (strlen(param->buffer_line) == 0) ? LINE_EMPTY : LINE_COMMENT;
      return(current_line);
    }

  /*** Découpage de la ligne en 4 bloc : Label / Opcode / Operand / Commentaire ***/
  DecodeLine(current_line->line_data,param->buffer_label,param->buffer_opcode,param->buffer_operand,param->buffer_comment);
  current_line->label_txt = strdup(param->buffer_label);
  current_line->opcode_txt = strdup(param->buffer_opcode);
  current_line->operand_txt = strdup(param->buffer_operand);
  current_line->comment_txt = strdup(param->buffer_comment);
  if(current_line->label_txt == NULL || current_line->opcode_txt == NULL ||
     current_line->operand_txt == NULL || current_line->comment_txt == NULL)
    {
      mem_free_sourceline(current_line);
      return(NULL);
    }

  /* Renvoi la ligne */
  return(current_line);
}


/*********************************************************/
/*  DuplicateSourceLine() :  Duplique une ligne de Code. */
/*********************************************************/
struct source_line *DuplicateSourceLine(struct source_line *current_line)
{
  struct source_line *new_line;
  
  /* Allocation mémoire */
  new_line = (struct source_line *) calloc(1,sizeof(struct source_line));
  if(new_line == NULL)
    return(NULL);
  
  /* Recopie les valeurs */
  new_line->line_number = current_line->line_number;
  new_line->file_line_number = current_line->file_line_number;
  new_line->file = current_line->file;
  new_line->type = current_line->type;
  new_line->type_aux = current_line->type_aux;
  new_line->is_in_source = current_line->is_in_source;
  new_line->is_valid = current_line->is_valid;
  new_line->no_direct_page = current_line->no_direct_page;
  new_line->use_direct_page = current_line->use_direct_page;
  new_line->is_inside_macro = current_line->is_inside_macro;
  new_line->is_dum = current_line->is_dum;
  new_line->dum_line = current_line->dum_line;
  new_line->cond_level = current_line->cond_level;
  strcpy(new_line->m,current_line->m);
  strcpy(new_line->x,current_line->x);
  new_line->variable = current_line->variable;
  new_line->macro = current_line->macro;
  new_line->bank = current_line->bank;
  new_line->address = current_line->address;
  new_line->operand_value = current_line->operand_value;
  new_line->operand_address_long = current_line->operand_address_long;
  new_line->nb_byte = current_line->nb_byte;
  new_line->opcode_byte = current_line->opcode_byte;
  new_line->address_mode = current_line->address_mode;
  new_line->address_is_rel = current_line->address_is_rel;
  memcpy(new_line->operand_byte,current_line->operand_byte,4);
  strcpy(new_line->reloc,current_line->reloc);
  new_line->next = NULL;
  
  /* Duplique certaines valeurs */
  if(current_line->line_data != NULL)
    {
      new_line->line_data = strdup(current_line->line_data);
      if(new_line->line_data == NULL)
        {
          mem_free_sourceline(new_line);
          return(NULL);
        }
    }  
  if(current_line->data != NULL)
    {
      new_line->data = current_line->data = (unsigned char *) calloc(new_line->nb_byte+1,sizeof(unsigned char));
      if(new_line->data == NULL)
        {
          mem_free_sourceline(new_line);
          return(NULL);
        }
      memcpy(new_line->data,current_line->data,new_line->nb_byte+1);
    }
  new_line->label_txt = strdup(current_line->label_txt);
  new_line->opcode_txt = strdup(current_line->opcode_txt);
  new_line->operand_txt = strdup(current_line->operand_txt);
  new_line->comment_txt = strdup(current_line->comment_txt);
  if(new_line->label_txt == NULL || new_line->opcode_txt == NULL || new_line->operand_txt == NULL || new_line->comment_txt == NULL)
    {
      mem_free_sourceline(new_line);
      return(NULL);
    }
  
  /* Renvoie la ligne */
  return(new_line);
}


/************************************************************************/
/*  BuildEmptyLabelLine() :  Crée une ligne de Code vide avec un Label. */
/************************************************************************/
struct source_line *BuildEmptyLabelLine(char *label, struct source_line *current_line)
{
  struct source_line *new_line;
  
  /* Allocation mémoire */
  new_line = (struct source_line *) calloc(1,sizeof(struct source_line));
  if(new_line == NULL)
    return(NULL);
  
  /* Recopie les valeurs */
  new_line->line_number = current_line->line_number;
  new_line->file_line_number = current_line->file_line_number;
  new_line->file = current_line->file;
  new_line->type = LINE_EMPTY;
  new_line->type_aux = current_line->type_aux;
  new_line->is_in_source = current_line->is_in_source;
  new_line->is_valid = current_line->is_valid;
  new_line->no_direct_page = current_line->no_direct_page;
  new_line->use_direct_page = current_line->use_direct_page;
  new_line->is_inside_macro = current_line->is_inside_macro;
  new_line->is_dum = current_line->is_dum;
  new_line->dum_line = current_line->dum_line;
  new_line->cond_level = current_line->cond_level;
  strcpy(new_line->m,current_line->m);
  strcpy(new_line->x,current_line->x);
  new_line->variable = NULL;
  new_line->macro = NULL;
  new_line->bank = current_line->bank;
  new_line->address = current_line->address;
  new_line->operand_value = current_line->operand_value;
  new_line->operand_address_long = current_line->operand_address_long;
  new_line->nb_byte = 0;
  new_line->opcode_byte = 0x00;
  new_line->address_mode = current_line->address_mode;
  new_line->address_is_rel = current_line->address_is_rel;
  memset(new_line->operand_byte,0,4);
  strcpy(new_line->reloc,current_line->reloc);
  new_line->next = NULL;
  
  /* Duplique certaines valeurs */
  new_line->line_data = strdup("");
  if(new_line->line_data == NULL)
    {
      mem_free_sourceline(new_line);
      return(NULL);
    }
  new_line->label_txt = strdup(label);
  new_line->opcode_txt = strdup("");
  new_line->operand_txt = strdup("");
  new_line->comment_txt = strdup("");
  if(new_line->label_txt == NULL || new_line->opcode_txt == NULL || new_line->operand_txt == NULL || new_line->comment_txt == NULL)
    {
      mem_free_sourceline(new_line);
      return(NULL);
    }
  
  /* Renvoie la ligne */
  return(new_line);
}


/*****************************************************************/
/*  DecodeLine() :  Décode une ligne en séparant les 4 éléments. */
/*****************************************************************/
void DecodeLine(char *line_data, char *label_rtn, char *opcode_rtn, char *operand_rtn, char *comment_rtn)
{
  int has_data, nb_separator;
  struct item *all_item;
  struct item *current_item;
  struct item *opcode_item = NULL;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Init */
  strcpy(label_rtn,"");
  strcpy(opcode_rtn,"");
  strcpy(operand_rtn,"");
  strcpy(comment_rtn,"");

  /** Découpe la ligne en éléments séparés par des espaces ou des tab (en tenant compte des commentaires et des chaines '' ou "") **/
  all_item = ExtractAllIem(line_data);
  if(all_item == NULL)
    return;                 /* Ligne vide */

  /** Cas particulier : Que des sépérateurs **/
  for(has_data=0, current_item = all_item; current_item; current_item = current_item->next)
    if(current_item->type == TYPE_DATA)
      {
        has_data = 1;
        break;
      }
  if(has_data == 0)
    {
      mem_free_item_list(all_item);
      return;                 /* Ligne vide */
    }

  /** Cas particulier : Ligne de commentaire **/
  for(current_item = all_item; current_item; current_item = current_item->next)
    if(current_item->type == TYPE_DATA)
      {
        if(current_item->name[0] == '*' || current_item->name[0] == ';')
          {
            strcpy(comment_rtn,current_item->name);
            mem_free_item_list(all_item);
            return;
          }
        break;
      }

  /** Commentaire : Valeur qui commence par un ; **/
  for(current_item = all_item; current_item->next; current_item = current_item->next)
    if(current_item->next->type == TYPE_DATA && current_item->next->name[0] == ';')
      {
        /* Garde le commentaire, libère la suite */
        strcpy(comment_rtn,current_item->next->name);
        mem_free_item_list(current_item->next);
        current_item->next = NULL;
        break;
      }

  /** Label : Tout ce qui est collé à gauche **/
  if(all_item->type == TYPE_DATA)
    strcpy(label_rtn,all_item->name);

  /** Opcode : DATA qui est après le 1er séparateur **/
  for(nb_separator=0,current_item = all_item; current_item; current_item = current_item->next)
    {
      if(current_item->type == TYPE_SEPARATOR)
        nb_separator++;
      else if(current_item->type == TYPE_DATA && nb_separator == 1)
        {
          strcpy(opcode_rtn,current_item->name);
          opcode_item = current_item;
          break;
        }
    }

  /** Operand : Ce qu'il reste (la partie commentaire a déjà été supprimée) **/
  if(opcode_item != NULL)
    {
      for(current_item = opcode_item->next; current_item; current_item = current_item->next)
        {
          if(current_item->type == TYPE_SEPARATOR)
            {
              if(strlen(operand_rtn) > 0)
                if(operand_rtn[strlen(operand_rtn)-1] != ' ')
                  strcat(operand_rtn," ");
            }
          else
            strcat(operand_rtn,current_item->name);
        }
    }

  /* Libération mémoire */
  mem_free_item_list(all_item);
  
  /** On supprime les espaces et les \t entourant les valeurs **/
  CleanBuffer(label_rtn);
  CleanBuffer(opcode_rtn);
  CleanBuffer(operand_rtn);
  CleanBuffer(comment_rtn);
}


/*******************************************************************/
/*  AddDateLine() :  Ajout d'une ligne Date pour la directive DAT. */
/*******************************************************************/
static void AddDateLine(struct source_line *current_line, struct omf_segment *current_omfsegment)
{
  struct source_line *new_line;
  char buffer[256];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
  
  /* On va décoder le type de Date demandé */
  if(!my_stricmp(current_line->operand_txt,"1"))
    sprintf(buffer,"\"%s\"",param->date_1);
  else if(!my_stricmp(current_line->operand_txt,"2"))
    sprintf(buffer,"\"%s\"",param->date_2);
  else if(!my_stricmp(current_line->operand_txt,"3"))
    sprintf(buffer,"\"%s\"",param->date_3);
  else if(!my_stricmp(current_line->operand_txt,"4"))
    sprintf(buffer,"\"%s\"",param->date_4);
  else if(!my_stricmp(current_line->operand_txt,"5"))
    sprintf(buffer,"'%s'",param->date_1);
  else if(!my_stricmp(current_line->operand_txt,"6"))
    sprintf(buffer,"'%s'",param->date_2);
  else if(!my_stricmp(current_line->operand_txt,"7"))
    sprintf(buffer,"'%s'",param->date_3);
  else if(!my_stricmp(current_line->operand_txt,"8"))
    sprintf(buffer,"'%s'",param->date_4);
  else
    return;
    
  /** Création d'une Ligne DATA **/
  new_line = BuildEmptyLabelLine("",current_line);
  if(new_line == NULL)
    {
      printf("      => [Error] Impossible to allocate memory to insert DAT value (line %d, file '%s')\n",current_line->file_line_number,current_line->file->file_path);
      return;
    }
  free(new_line->opcode_txt);
  new_line->opcode_txt = NULL;
  free(new_line->operand_txt);
  new_line->operand_txt = NULL;

  /* Nouvelles valeurs */    
  new_line->type = LINE_DATA;
  new_line->opcode_txt = strdup("ASC");
  new_line->operand_txt = strdup(buffer);
  if(new_line->opcode_txt == NULL || new_line->operand_txt == NULL)
    {
      mem_free_sourceline(new_line);
      printf("      => [Error] Impossible to allocate memory to insert DAT value (line %d, file '%s')\n",current_line->file_line_number,current_line->file->file_path);
      return;
    }
  
  /* Insère la ligne après la ligne DAT */
  new_line->next = current_line->next;
  current_line->next = new_line;
}


/*****************************************************************************/
/*  mem_free_sourceline() :  Libération mémoire de la structure source_line. */
/*****************************************************************************/
void mem_free_sourceline(struct source_line *current_sourceline)
{
  if(current_sourceline)
    {
      if(current_sourceline->label_txt)
        free(current_sourceline->label_txt);

      if(current_sourceline->opcode_txt)
        free(current_sourceline->opcode_txt);

      if(current_sourceline->operand_txt)
        free(current_sourceline->operand_txt);

      if(current_sourceline->comment_txt)
        free(current_sourceline->comment_txt);

      if(current_sourceline->data)
        free(current_sourceline->data);

      free(current_sourceline);
    }
}


/*******************************************************************/
/*  mem_free_sourceline_list() :  Libère la liste de ligne source. */
/*******************************************************************/
void mem_free_sourceline_list(struct source_line *first_sourceline)
{
  struct source_line *current_sourceline;
  struct source_line *next_sourceline;
  
  /** Libère la liste chainée de structure **/
  for(current_sourceline = first_sourceline; current_sourceline; )
    {
      next_sourceline = current_sourceline->next;
      mem_free_sourceline(current_sourceline);
      current_sourceline = next_sourceline;
    }
}


/******************************************************************/
/*  mem_free_label() :  Libération mémoire de la structure label. */
/******************************************************************/
void mem_free_label(struct label *current_label)
{
  if(current_label)
    {
      if(current_label->name)
        free(current_label->name);

      free(current_label);
    }
}


/************************************************************************/
/*  mem_free_variable() :  Libération mémoire de la structure variable. */
/************************************************************************/
void mem_free_variable(struct variable *current_variable)
{
  if(current_variable)
    {
      if(current_variable->name)
        free(current_variable->name);

      free(current_variable);
    }
}


/******************************************************************************/
/*  mem_free_equivalence() :  Libération mémoire de la structure equivalence. */
/******************************************************************************/
void mem_free_equivalence(struct equivalence *current_equivalence)
{
  if(current_equivalence)
    {
      if(current_equivalence->name)
        free(current_equivalence->name);

      if(current_equivalence->value)
        free(current_equivalence->value);

      free(current_equivalence);
    }
}


/************************************************************************/
/*  mem_free_external() :  Libération mémoire de la structure external. */
/************************************************************************/
void mem_free_external(struct external *current_external)
{
  if(current_external)
    {
      if(current_external->name)
        free(current_external->name);

      free(current_external);
    }
}


/********************************************************************/
/*  mem_free_global() :  Libération mémoire de la structure global. */
/********************************************************************/
void mem_free_global(struct global *current_global)
{
  if(current_global)
    {
      if(current_global->name)
        free(current_global->name);

      free(current_global);
    }
}

/***********************************************************************/
