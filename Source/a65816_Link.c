/***********************************************************************/
/*                                                                     */
/*  a65816_Link.c : Module d'Assemblage / Linkage d'un source 65c816.  */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>

/** Platform dependent code **/
#if defined(WIN32) || defined(WIN64)
/* Windows */
#else
/* Linux + MacOS */
#include <unistd.h>                     /* unlink() */
#endif

#include "Dc_Library.h"
#include "a65816_Line.h"
#include "a65816_File.h"
#include "a65816_Lup.h"
#include "a65816_Macro.h"
#include "a65816_Cond.h"
#include "a65816_Code.h"
#include "a65816_Data.h"
#include "a65816_OMF.h"
#include "a65816_Link.h"

static int Assemble65c816Segment(struct omf_project *,struct omf_segment *,char *);
static int Link65c816Segment(struct omf_project *, struct omf_segment *);
static int IsLinkFile(struct source_file *);
static struct omf_project *BuildSingleSegment(char *);
static struct omf_project *BuildLinkFile(struct source_file *);

/**************************************************************************/
/*  AssembleLink65c816() :  Assemble et Links des fichiers source 65c816. */
/**************************************************************************/
int AssembleLink65c816(char *master_file_path, char *macro_folder_path, int verbose_mode)
{
  DWORD org_offset;
  int i, error, is_link_file;
  char file_name[1024] = "";
  char file_error_path[1024];
  struct source_file *master_file;
  struct omf_project *current_omfproject;
  struct omf_segment *current_omfsegment;
  struct omf_segment *tmp_omfsegment;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Nom du fichier */
  strcpy(file_name,master_file_path);
  for(i=(int)strlen(master_file_path); i>=0; i--)
    if(master_file_path[i] == '\\' || master_file_path[i] == '/')
      {
        strcpy(file_name,&master_file_path[i+1]);
        break;
      }
  
  /* Fichier Error_Output.txt */
  sprintf(file_error_path,"%serror_output.txt",param->current_folder_path);
  unlink(file_error_path);

  /* Allocation d'un OMF Segment temporaire */
  tmp_omfsegment = mem_alloc_omfsegment();
  if(tmp_omfsegment == NULL)
    {
      strcpy(param->buffer_error,"Impossible to allocate memory to build temporary segment structure");
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
  my_Memory(MEMORY_SET_OMFSEGMENT,tmp_omfsegment,NULL,NULL);

  /** Charge en mémoire le fichier principal **/
  master_file = LoadOneSourceFile(master_file_path,file_name,0);
  if(master_file == NULL)
    {
      sprintf(param->buffer_error,"Impossible to load Master Source file '%s'",master_file_path);
      my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

  /** Est-ce un fichier Link ou un ficher Source **/
  is_link_file = IsLinkFile(master_file);
  if(is_link_file == 0)
    {
      /* Libération mémoire du fichier Link */
      mem_free_sourcefile(master_file,1);
      mem_free_omfsegment(tmp_omfsegment);
      my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);

      /* Init File */
      my_File(FILE_INIT,NULL);

      /** Création d'1 omf_header + 1 omf_segment **/
      current_omfproject = BuildSingleSegment(master_file_path);
      if(current_omfproject == NULL)
        {
          sprintf(param->buffer_error,"Impossible to allocate memory to process Master Source file '%s'",master_file_path);
          my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

      /* Déclare le Segment OMF courrant */
      my_Memory(MEMORY_SET_OMFSEGMENT,current_omfproject->first_segment,NULL,NULL);

      /*** Mono Segment : Assemble les fichiers Source + Création du fichier Output.txt pour 1 Segment ***/
      printf("  + Assemble project files...\n");
      error = Assemble65c816Segment(current_omfproject,current_omfproject->first_segment,macro_folder_path);
      if(error)
        {
          /* Création du fichier Output */
          CreateOutputFile(file_error_path,current_omfproject->first_segment,current_omfproject);
          
          /* Libération mémoire */
          mem_free_omfproject(current_omfproject);
          my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
          my_File(FILE_FREE,NULL);
          
          /* Code Erreur */
          return(error);
        }

      /*** Mono Segment : Link les fichiers Source pour 1 Segment ***/
      printf("  + Link project files...\n");
      error = Link65c816Segment(current_omfproject,current_omfproject->first_segment);
      if(error)
        {
          /* Libération mémoire : OMF Project + OMF Segment */
          mem_free_omfproject(current_omfproject);
          my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
          my_File(FILE_FREE,NULL);
  
          /* Code Erreur */
          return(error);
        }

      /** Create OMF File / Binary File **/
      if(current_omfproject->first_segment->is_omf == 1)
        {
          /* OMF Mono Segment */
          printf("    o Build OMF output file...\n");
          BuildOMFFile(param->current_folder_path,current_omfproject);
        }
      else
        {
          /* Binaire à adresse fixe */
          printf("    o Build Binary output file...\n");
          BuildObjectFile(param->current_folder_path,current_omfproject->first_segment,current_omfproject);
        }

      /** Dump Code as Output Text File **/
      if(verbose_mode)
        {
          /* Nom du fichier _Output.txt */
          sprintf(param->output_file_path,"%s%s_Output.txt",param->current_folder_path,current_omfproject->first_segment->object_name);         /* Mono Segment */

          /* Création du fichier Output */
          printf("  + Create Output Text file...\n");
          CreateOutputFile(param->output_file_path,current_omfproject->first_segment,current_omfproject);
        }

      /* Libération mémoire */
      mem_free_omfproject(current_omfproject);
      my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
      my_File(FILE_FREE,NULL);
    }
  else
    {
      /** Multi Segments : Assemble + Link tous les Segments **/
      printf("  + Loading Link file...\n");

      /** Chargement du fichier Link : 1 omh_hearder + N omf_segment **/
      current_omfproject = BuildLinkFile(master_file);
      if(current_omfproject == NULL)
        {
          mem_free_sourcefile(master_file,1);
          sprintf(param->buffer_error,"Impossible to load Link file '%s'",master_file_path);
          my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

      /* Libération mémoire */
      mem_free_sourcefile(master_file,1);

      /*********************************************************/
      /*** On va enchainer l'assemblage de tous les Segments ***/
      /*********************************************************/
      for(i=0,org_offset = 0,current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next,i++)
        {
          /* Init File */
          my_File(FILE_INIT,NULL);

          /* Déclare le Segment OMF courrant */
          my_Memory(MEMORY_SET_OMFSEGMENT,current_omfsegment,NULL,NULL);

          /* Donne un numéro au Segment (décale en cas d'ExpressLoad) */
          current_omfsegment->segment_number = i + 1 + ((current_omfproject->express_load == 1 && current_omfproject->nb_segment > 1) ? 1 : 0);

          /* Donne une adresse ORG pour les Fixed-Address Single-Binary */
          if(current_omfproject->is_single_binary == 1)
            {
              current_omfsegment->has_org_address = 1;
              current_omfsegment->org_address = current_omfproject->org_address_tab[current_omfsegment->file_number-1] + org_offset;
            }

          /*** Segment #n : Assemble les fichiers Source + Création du fichier Output.txt pour 1 Segment ***/
          printf("  + Assemble project files for Segment #%02X :\n",current_omfsegment->segment_number);
          error = Assemble65c816Segment(current_omfproject,current_omfsegment,macro_folder_path);
          if(error)
            {
              /* Création du fichier Output */
              CreateOutputFile(file_error_path,current_omfsegment,current_omfproject);
              
              /* Libération mémoire : OMF Project + OMF Segment */
              mem_free_omfproject(current_omfproject);
              my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
              my_File(FILE_FREE,NULL);
  
              /* Code Erreur */
              return(error);
            }
            
          /* Met à jour la ORG Address du Segment suivant (pour les Fixed-Address SingleBinary), puis remis à zero entre les fichiers */
          org_offset += current_omfsegment->object_length;
          if(current_omfsegment->next != NULL)
            if(current_omfsegment->file_number != current_omfsegment->next->file_number)
              org_offset = 0;
        }

      /***********************************************************************/
      /*** On va générer les Segments OMF (Header + Body) ou Fixed Address ***/
      /***********************************************************************/
      for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
        {
          /*** Segment #n : Link les fichiers Source pour 1 Segment ***/
          printf("  + Link project files for Segment #%02X...\n",current_omfsegment->segment_number);
          error = Link65c816Segment(current_omfproject,current_omfsegment);
          if(error)
            {
              /* Libération mémoire : OMF Project + OMF Segment */
              mem_free_omfproject(current_omfproject);
              my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
              my_File(FILE_FREE,NULL);
  
              /* Code Erreur */
              return(error);
            }
        }

      /** Création du Segment ~ExpressLoad **/
      if(current_omfproject->is_multi_fixed != 1 && current_omfproject->express_load == 1 && current_omfproject->nb_segment > 1)
        {
          printf("  + Build ExpressLoad into Segment #01...\n");
          error = BuildExpressLoadSegment(current_omfproject);
          if(error)
            {
              /* Libération mémoire : OMF Project + OMF Segment */
              mem_free_omfproject(current_omfproject);
              my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
              my_File(FILE_FREE,NULL);
              return(error);
            }
        }

      /***************************************************************************************/
      /** Create OMF File Multi-Segments / OMF File Mono-Segment / Binary Multi-Fixed files **/
      /***************************************************************************************/
      if(current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 0)
        {
          /** On va créer les fichiers Binaire de tous les Segments **/
          printf("  + Build Binary output files...\n");
          for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
            {
              /* Création du fichier Binaire à adresse fixe */
              BuildObjectFile(param->current_folder_path,current_omfsegment,current_omfproject);
            }
        }
      else if(current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 1)
        {
          /** On va créer le fichier Binaire de tous les Segments concaténés **/
          printf("  + Build Binary output file%s...\n",(current_omfproject->nb_file==1)?"":"s");

          /* Création du/des fichiers Binaire à adresse fixe groupés ensemble */
          for(i=0; i<current_omfproject->nb_file; i++)
            BuildSingleObjectFile(param->current_folder_path,i,current_omfproject);
        }
      else
        {
          /** Create OMF File Multi-Segments (ou Mono Segment mais disposant de son Link File) **/
          printf("  + Build OMF output file...\n");
          BuildOMFFile(param->current_folder_path,current_omfproject);
        }

      /******************************/
      /** Dump as Output Text File **/
      /******************************/
      if(verbose_mode)
        {
          /* Création du/des fichiers Output Text */
          printf("  + Create Output Text file%s...\n",(current_omfproject->nb_segment == 1)?"":"s");
                        
          /** On va créer les fichiers Output.txt de tous les Segments (sauf l'ExpressLoad) **/
          for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
            {
              /* On ne Dump pas l'ExpressLoad */
              if(current_omfsegment->segment_number == 1 && !my_stricmp(current_omfsegment->segment_name,"~ExpressLoad"))
                continue;

              /* Nom du fichier _Output.txt */
              if(current_omfproject->nb_segment == 1)                                                                                                 /* Mono Segment */
                sprintf(param->output_file_path,"%s%s_Output.txt",param->current_folder_path,current_omfsegment->object_name);
              else if(current_omfproject->nb_segment > 1 && current_omfproject->is_multi_fixed == 0)                                                  /* Multi Segment OMF */
                sprintf(param->output_file_path,"%s%s_S%02X_%s_Output.txt",param->current_folder_path,current_omfproject->dsk_name_tab[0],current_omfsegment->segment_number,current_omfsegment->object_name);
              else if(current_omfproject->nb_segment > 1 && current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 0)     /* Multi Segment Fixed */
                sprintf(param->output_file_path,"%s%s_S%02X_Output.txt",param->current_folder_path,(strlen(current_omfsegment->segment_name)==0)?current_omfsegment->object_name:current_omfsegment->segment_name,current_omfsegment->segment_number);
              else if(current_omfproject->nb_segment > 1 && current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 1)     /* Multi Segment Fixed Single Binary */
                sprintf(param->output_file_path,"%s%s_S%02X_%s_Output.txt",param->current_folder_path,current_omfproject->dsk_name_tab[current_omfsegment->file_number-1],current_omfsegment->segment_number,(strlen(current_omfsegment->segment_name)==0)?current_omfsegment->object_name:current_omfsegment->segment_name);

              /* Création du fichier Output */
              CreateOutputFile(param->output_file_path,current_omfsegment,current_omfproject);
            }
        }

      /* Libération mémoire : OMF Project + OMF Segment */
      mem_free_omfproject(current_omfproject);
      my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
      my_File(FILE_FREE,NULL);
    }

  /* Code Erreur */
  return(error);
}


/*********************************************************************************/
/*  Assemble65c816Segment() :  Assemble des fichiers source 65c816 d'un Segment. */
/*********************************************************************************/
static int Assemble65c816Segment(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment, char *macro_folder_path)
{
  int modified, error, first_time, has_error;
  struct source_file *first_file;
  struct source_line *current_line;
  struct relocate_address *current_address;
  struct relocate_address *next_address;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
  my_Memory(MEMORY_FREE_EQUIVALENCE,NULL,NULL,current_omfsegment);

  /* Initialisation du compteur des labels uniques */
  GetUNID("INIT=1");

  /* Récupère le répertoire des fichiers source */
  GetFolderFromPath(current_omfsegment->master_file_path,param->source_folder_path);

  /* Création des tables de recherche rapides */
  BuildReferenceTable(current_omfsegment);

  /** Chargement de tous les fichiers Source / Identifie les lignes en Commentaire + Vide **/
  printf("    o Loading Sources files...\n");
  LoadAllSourceFile(current_omfsegment->master_file_path,macro_folder_path,current_omfsegment);

  /** Chargement des fichiers Macro **/
  printf("    o Loading Macro files...\n");
  LoadSourceMacroFile(macro_folder_path,current_omfsegment);

  /** Recherche des Macro supplémentaires définies directement dans le Source **/
  GetMacroFromSource(current_omfsegment);

  /** Tri toutes les Macro **/
  my_Memory(MEMORY_SORT_MACRO,NULL,NULL,current_omfsegment);

  /** Recherche des Macro en double **/
  printf("    o Check for duplicated Macros...\n");
  CheckForDuplicatedMacro(current_omfsegment);

  /** Détermine le type des lignes (Code, Macro, Directive, Equivalence...) **/
  printf("    o Decoding lines types...\n");
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);  
  error = DecodeLineType(first_file->first_line,NULL,current_omfsegment,current_omfproject);
  if(error)
    return(1);

  /** Remplace les labels :locaux/]variable par un unid_ dans le Code et les Macros **/
  printf("    o Process local/variable Labels...\n");
  ProcessAllLocalLabel(current_omfsegment);
  ProcessAllVariableLabel(current_omfsegment);

  /** Remplace les * par des Labels dans le Code et les Macros **/
  printf("    o Process Asterisk lines...\n");
  error = ProcessAllAsteriskLine(current_omfsegment);
  if(error)
    return(1);

  /** Création de la table des External **/
  printf("    o Build External table...\n");
  BuildExternalTable(current_omfsegment);

  /** Création de la table des Equivalences **/
  printf("    o Build Equivalence table...\n");
  BuildEquivalenceTable(current_omfsegment);

  /** Création de la table des variables ]LP **/
  printf("    o Build Variable table...\n");
  BuildVariableTable(current_omfsegment);

  /** Remplace les Equivalences **/
  printf("    o Process Equivalence values...\n");
  ProcessEquivalence(current_omfsegment);

  /** Remplace les Macro avec leur code **/
  printf("    o Replace Macros with Code...\n");
  error = ReplaceMacroWithContent(current_omfsegment,current_omfproject);
  if(error)
    return(1);

  /** Remplace les LUP par la séquence de code (cela vient après les Macro car le param de LUP peut être un des param d'appel de la Macro) **/
  printf("    o Replace Lup with code...\n");
  ReplaceLupWithCode(current_omfsegment);

  /** On refait les Equivalences, les Variables et les Remplacements dans le code amené par les Macro et les Loop **/
  BuildEquivalenceTable(current_omfsegment);
  BuildVariableTable(current_omfsegment);
  ProcessEquivalence(current_omfsegment);

  /** On va traiter les MX **/
  printf("    o Process MX directives...\n");
  ProcessMXDirective(current_omfsegment);

  /** On va traiter les Conditions **/
  printf("    o Process Conditional directives...\n");
  ProcessConditionalDirective(current_omfsegment);

  /** Création de la table des Labels **/
  printf("    o Build Label table...\n");
  BuildLabelTable(current_omfsegment);

  /** Recherche de Labels/Equivalence déclarés plusieurs fois **/
  printf("    o Check for duplicated Labels...\n");
  error = CheckForDuplicatedLabel(current_omfsegment);
  if(error)
    return(1);

  /** Détecte la liste des lignes inconnues **/
  printf("    o Check for unknown Source lines...\n");
  error = CheckForUnknownLine(current_omfsegment);
  if(error)
    return(1);

  /** Traite les lignes Dum **/
  printf("    o Check for Dum lines...\n");
  error = CheckForDumLine(current_omfsegment);
  if(error)
    return(1);

  /**** La détection automatique du Direct Page nous oblige à itérer plusieurs fois ****/
  first_time = 1;
  modified = 1;
  has_error = 0;
  while(modified == 1 || has_error == 1)
    {
      /* Erreur lors de la génération du code */
      strcpy(param->buffer_latest_error,"");
      has_error = 0;

      /*** Génération du code Opcode + Calcul de la Taille pour chaque ligne de Code ***/
      if(first_time)
        printf("    o Compute Operand Code size...\n");
      BuildAllCodeLineSize(current_omfsegment);

      /*** Calcul de la taille pour chaque ligne de Data ***/
      if(first_time)
        printf("    o Compute Operand Data size...\n");
      BuildAllDataLineSize(current_omfsegment);

      /*** Calcul les addresses de chaque ligne + Analyse les ORG / OBJ / REL / DUM ***/
      if(first_time)
        printf("    o Compute Line address...\n");
      ComputeLineAddress(current_omfsegment,current_omfproject);

      /*** Génération du binaire pour les lignes Code (LINE_CODE) ***/
      if(first_time)
        printf("    o Build Code Line...\n");
      BuildAllCodeLine(&has_error,current_omfsegment,current_omfproject);

      /** Compact Code for Direct Page (sauf si l'adresse est relogeable OMF ) **/
      if(first_time)
        printf("    o Compact Code for Direct Page Lines...\n");
      modified = CompactDirectPageCode(current_omfsegment);

      /* Le compact de donne rien et on a une erreur => On sort */
      if(has_error == 1 && modified == 0)
        my_RaiseError(ERROR_RAISE,param->buffer_latest_error);

      /** Il faut tout refaire => on va supprimer toutes les adresses à reloger **/
      if(modified == 1 || has_error == 1)
        {
          for(current_address=current_omfsegment->first_address; current_address; )
            {
              next_address = current_address->next;
              free(current_address);
              current_address = next_address;
            }
          current_omfsegment->first_address = NULL;
          current_omfsegment->last_address = NULL;
          current_omfsegment->nb_address = 0;
        }

      /* On a déjà fait un tour */
      first_time = 0;
    }

  /** On va évaluer les lignes ERR **/
  printf("    o Check for Err lines...\n");
  error = CheckForErrLine(current_omfsegment);
  if(error)
    return(1);

  /** On va vérifier les adressages Page Direct **/
  printf("    o Check for Direct Page Lines...\n");
  error = CheckForDirectPageLine(current_omfsegment);
  if(error)
    return(1);

  /*** Génération du binaire pour les lignes Data (LINE_DATA) ***/
  printf("    o Build Data Line...\n");
  BuildAllDataLine(current_omfsegment);

  /** Create Object Code (LINE_CODE + LINE_DATA) **/
  printf("    o Build Object Code...\n");
  BuildObjectCode(current_omfsegment);

  /** Transforme les lignes directive avec Label en ligne Vide **/
  ProcessDirectiveWithLabelLine(current_omfsegment);

  /** Si on n'a pas de Segment Name ou de Load Name, on utilise les directive du Master Source **/
  if(current_omfsegment->segment_name == NULL)
    current_omfsegment->segment_name = strdup(current_omfsegment->object_name);
  if(current_omfsegment->load_name == NULL)
    current_omfsegment->load_name = strdup(current_omfsegment->object_name);
  if(current_omfsegment->segment_name == NULL || current_omfsegment->load_name == NULL)
    {
      printf("      => Error, Can't allocate memory...\n");
      return(1);
    }

  /** Si on est en multi-segment Fixed, il faut faire remonter l'org address du segment **/
  if(current_omfproject->is_multi_fixed == 1)
    {
      /* On recherche la première ligne avec qqchise dedans */
      for(current_line=current_omfsegment->first_file->first_line; current_line; current_line=current_line->next)
        if(current_line->nb_byte > 0)
          {
            current_omfsegment->org = current_line->address;
            break;
          }
    }

  /* OK */
  return(0);
}


/*************************************************************************/
/*  Link65c816Segment() :  Link les fichiers source 65c816 d'un Segment. */
/*************************************************************************/
static int Link65c816Segment(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment)
{
  int nb_external;
  struct label *external_label;
  struct relocate_address *current_address;
  struct omf_segment *external_omfsegment;
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /** On va vérifier que tous les External utilisés de ce segments sont résolus **/
  my_Memory(MEMORY_GET_EXTERNAL_NB,&nb_external,NULL,current_omfsegment);
  if(nb_external > 0)
    {
      /** On vérifie toutes les adresses relogeables **/
      for(current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
        if(current_address->external != NULL)
          if(current_address->external->external_segment == NULL)
            {
              /** On passe tous les Segments en revue pour trouver le label Global associé à ce External **/
              for(external_omfsegment = current_omfproject->first_segment; external_omfsegment; external_omfsegment = external_omfsegment->next)
                {
                  /* Recherche un Label Global */
                  my_Memory(MEMORY_SEARCH_LABEL,current_address->external->name,&external_label,external_omfsegment);
                  if(external_label != NULL)
                    if(external_label->is_global == 1)
                      {
                        /* Si on en a déjà trouvé un, c'est qu'il existe au moins 2 Label Global s'appelant pareil => Erreur */
                        if(current_address->external->external_segment != NULL)
                          {
                            printf("     => Error : We have found 2 External Labels with the same name '%s' (File '%s', Line %d).\n",current_address->external->name,external_label->line->file->file_path,external_label->line->file_line_number);
                            return(1);
                          }
                        
                        /* On conserve celui là */
                        current_address->external->external_segment = external_omfsegment;
                        current_address->external->external_label = external_label;
                      }
                }
  
              /** On n'a pas pu trouver le Segment contenant ce Label externe :-( **/
              if(current_address->external->external_segment == NULL)
                {
                  printf("     => Error : Can't find External Label named '%s' (File '%s', Line %d).\n",current_address->external->name,current_address->external->source_line->file->file_path,current_address->external->source_line->file_line_number);
                  return(1);
                }            
            }
    }

  /** Taille du Body du Segment (on prend plus large pour l'OMF) **/
  if(current_omfproject->is_multi_fixed == 1)
    current_omfsegment->segment_body_length = current_omfsegment->object_length;
  else
    current_omfsegment->segment_body_length = 1024 + current_omfsegment->object_length + CRECORD_SIZE*current_omfsegment->nb_address + END_SIZE;

  /** Allocation mémoire Segment Body **/
  current_omfsegment->segment_body_file = (unsigned char *) calloc(current_omfsegment->segment_body_length,sizeof(unsigned char));
  if(current_omfsegment->segment_body_file == NULL)
    {
      printf("     => Error : Can't allocate memory to build Body File buffer.\n");
      return(1);
    }

  /** Création du Segment Body **/
  if(current_omfproject->is_multi_fixed == 1)
    {
      /* Reloge les adresses externes fixed */
      RelocateExternalFixedAddress(current_omfproject,current_omfsegment);
      
      /* Conserve le code objet */
      memcpy(current_omfsegment->segment_body_file,current_omfsegment->object_code,current_omfsegment->object_length);
    }
  else
    current_omfsegment->body_length = BuildOMFBody(current_omfproject,current_omfsegment);

  /* Création du Segment Header */
  if(current_omfproject->is_multi_fixed == 1)
    current_omfsegment->header_length = 0;
  else
    current_omfsegment->header_length = BuildOMFHeader(current_omfproject,current_omfsegment);

  /* OK */
  return(0);
}


/************************************************************************/
/*  IsLinkFile() :  Détermine si un fichier Source est un fichier Link. */
/************************************************************************/
static int IsLinkFile(struct source_file *master_file)
{
  int i, found;
  struct source_line *current_line;
  char *opcode_link[] = {"DSK","TYP","AUX","XPL","ASM","DS","KND","ALI","LNA","SNA","ORG","BSZ",NULL};      /* Opcode exclusifs au fichier Link */

  /* Fichier vide ? */
  if(master_file->first_line == NULL)
    return(0);

  /** On passe toutes les lignes en revue **/
  for(current_line = master_file->first_line; current_line; current_line = current_line->next)
    {
      /* Commentaire / Vide */
      if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY)
        continue;

      /* Reconnait les Opcode du Link */
      for(i=0,found=0; opcode_link[i]!=NULL; i++)
        if(!my_stricmp(current_line->opcode_txt,opcode_link[i]))
          {
            found = 1;
            break;
          }
      if(found == 0)
        return(0);
    }

  /* OK */
  return(1);
}


/************************************************************/
/*  BuildSingleSegment() :  Création d'un OMF mono-Segment. */
/************************************************************/
static struct omf_project *BuildSingleSegment(char *master_file_path)
{
  struct omf_project *current_omfproject;
  
  /* OMF Header */
  current_omfproject = (struct omf_project *) calloc(1,sizeof(struct omf_project));
  if(current_omfproject == NULL)
    return(NULL);

  /** OMF Segment **/
  current_omfproject->nb_segment = 1;
  current_omfproject->first_segment = mem_alloc_omfsegment();
  if(current_omfproject->first_segment == NULL)
    {
      mem_free_omfproject(current_omfproject);
      return(NULL);
    }
  current_omfproject->first_segment->master_file_path = strdup(master_file_path);
  if(current_omfproject->first_segment->master_file_path == NULL)
    {
      mem_free_omfproject(current_omfproject);
      return(NULL);
    }

  /* En mono-Segment, il n'y a pas d'ExpressLoad, le premier segment a le numéro 1 */
  current_omfproject->first_segment->segment_number = 1;  

  /* Renvoi la structure */
  return(current_omfproject);
}


/*************************************************************/
/*  BuildLinkFile() :  Récupère les données du fichier Link. */
/*************************************************************/
static struct omf_project *BuildLinkFile(struct source_file *link_file)
{
  struct omf_project *current_omfproject;
  struct omf_segment *new_omfsegment;  
  struct omf_segment *current_omfsegment;
  struct source_line *current_line;
  int i;
  char **dsk_name_tab;
  DWORD *org_address_tab;
  DWORD *file_size_tab;  
  char file_path[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Allocation mmoire OMF Heaer */
  current_omfproject = (struct omf_project *) calloc(1,sizeof(struct omf_project));
  if(current_omfproject == NULL)
    {
      printf("     => Error, Impossible to allocate memory to process Link file.\n");
      return(NULL);
    }

  /* Valeurs par défaut */
  current_omfproject->type = 0xB3;          /* GS/OS Application */
  current_omfproject->aux_type = 0x0000;

  /*****************************************************/
  /*** Passe toutes les lignes du Link File en revue ***/
  /*****************************************************/
  for(current_line = link_file->first_line; current_line; current_line = current_line->next)
    {
      /* Commentaire / Vide */
      if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY)
        continue;

      /********************/
      /** Nouveau Header **/
      /********************/
      /** TYP : Type du fchier **/
      if(!my_stricmp(current_line->opcode_txt,"TYP"))
        {
          /* Décode la valeur */
          current_omfproject->type = GetByteValue(current_line->operand_txt);
          continue;
        }
        
      /** AUX : AuxType du fichier **/
      if(!my_stricmp(current_line->opcode_txt,"AUX"))
        {
          /* Décode la valeur */
          current_omfproject->aux_type = GetWordValue(current_line->operand_txt);
          continue;
        }
        
      /** XPL : Express Load **/
      if(!my_stricmp(current_line->opcode_txt,"XPL"))
        {
          current_omfproject->express_load = 1;
          continue;
        }
      
      /**********************************/
      /** DSK : Nouveau Projet/Fichier **/
      /**********************************/
      if(!my_stricmp(current_line->opcode_txt,"DSK"))
        {
          /*** Table des nom de fichier + Table des ORG ***/
          /* Allocation */
          dsk_name_tab = (char **) calloc(current_omfproject->nb_file+1,sizeof(char *));
          if(dsk_name_tab == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }
          org_address_tab = (DWORD *) calloc(current_omfproject->nb_file+1,sizeof(DWORD));
          if(org_address_tab == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              free(dsk_name_tab);
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }
          file_size_tab = (DWORD *) calloc(current_omfproject->nb_file+1,sizeof(DWORD));
          if(file_size_tab == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              free(dsk_name_tab);
              free(org_address_tab);
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }            
          /* Remplissage */
          for(i=0; i<current_omfproject->nb_file; i++)
            {
              dsk_name_tab[i] = current_omfproject->dsk_name_tab[i];
              org_address_tab[i] = current_omfproject->org_address_tab[i];
            }      
          /* Remplacement */
          if(current_omfproject->dsk_name_tab != NULL)
            free(current_omfproject->dsk_name_tab);
          current_omfproject->dsk_name_tab = dsk_name_tab;
          if(current_omfproject->org_address_tab != NULL)
            free(current_omfproject->org_address_tab);
          current_omfproject->org_address_tab = org_address_tab;
          if(current_omfproject->file_size_tab != NULL)
            free(current_omfproject->file_size_tab);
          current_omfproject->file_size_tab = file_size_tab;
                    
          /* Un fichier de plus */
          current_omfproject->nb_file++;
          
          /* Nouvelles valeurs */
          current_omfproject->org_address_tab[current_omfproject->nb_file-1] = 0xFFFFFFFF;
          current_omfproject->dsk_name_tab[current_omfproject->nb_file-1] = strdup(current_line->operand_txt);
          if(current_omfproject->dsk_name_tab[current_omfproject->nb_file-1] == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }

          /* Ligne suivante */
          continue;
        }

      /** ORG : Adresse d'assemblage du fichier Fixed Address **/
      if(!my_stricmp(current_line->opcode_txt,"ORG") && current_omfproject->nb_file > 0)
        {
          /* Décode la valeur */
          current_omfproject->org_address_tab[current_omfproject->nb_file-1] = GetDwordValue(current_line->operand_txt);
          
          /* Vérfie la plage */
          if(current_omfproject->org_address_tab[current_omfproject->nb_file-1] > 0xFFFFFF)
            {
              printf("     => Error, Invalid ORG value : %X.\n",(int)current_omfproject->org_address_tab[current_omfproject->nb_file-1]);
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }
          continue;
        }


      /***************************/
      /** ASM : Nouveau Segment **/
      /***************************/
      if(!my_stricmp(current_line->opcode_txt,"ASM"))
        {
          /* Allocation mémoire du Segment */
          new_omfsegment = mem_alloc_omfsegment();
          if(new_omfsegment == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }

          /* Nom du fichier Source */
          BuildAbsolutePath(current_line->operand_txt,param->current_folder_path,file_path);
          new_omfsegment->master_file_path = strdup(file_path);
          if(new_omfsegment->master_file_path == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }

          /* Numéro du fichier */
          new_omfsegment->file_number = current_omfproject->nb_file;

          /* Attachement du Segment à la liste */
          if(current_omfproject->first_segment == NULL)
            current_omfproject->first_segment = new_omfsegment;
          else
            current_omfproject->last_segment->next = new_omfsegment;
          current_omfproject->last_segment = new_omfsegment;
          current_omfproject->nb_segment++;

          /* Ligne suivante */
          continue;
        }

      /** DS : Nombre de 0 à ajouter à la fin du segment **/
      if(!my_stricmp(current_line->opcode_txt,"DS") && current_omfproject->last_segment != NULL)
        {
          current_omfproject->last_segment->ds_end = atoi(current_line->operand_txt);
          continue;
        }

      /** KND : Type et Attributs du Segment **/
      if(!my_stricmp(current_line->opcode_txt,"KND") && current_omfproject->last_segment != NULL)
        {
          /* Décode la valeur */
          current_omfproject->last_segment->type_attributes = GetWordValue(current_line->operand_txt);

          /* Vérifie les valeurs : Type */
          if(!(((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0000) ||
               ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0001) ||
               ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0002) ||
               ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0004) ||
               ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0008) ||
               ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0010) ||
               ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0012)))
            {
              printf("     => Error, Invalid Link file : Unknown Type value for Directive KND (%04X) at line %d.\n",current_omfproject->last_segment->type_attributes,current_line->file_line_number);
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }

          /* Valeur suivante */
          continue;
        }

      /** ALI : Alignement **/
      if(!my_stricmp(current_line->opcode_txt,"ALI") && current_omfproject->last_segment != NULL)
        {
          if(!my_stricmp(current_line->operand_txt,"BANK"))
            current_omfproject->last_segment->alignment = ALIGN_BANK;
          else if(!my_stricmp(current_line->operand_txt,"PAGE"))
            current_omfproject->last_segment->alignment = ALIGN_PAGE;
          else if(!my_stricmp(current_line->operand_txt,"NONE"))
            current_omfproject->last_segment->alignment = ALIGN_NONE;
          else
            {
              printf("     => Error, Invalid Link file : Unknown value for Directive ALI (%s) at line %d.\n",current_line->operand_txt,current_line->file_line_number);
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }

          /* Ligne suivante */
          continue;
        }

      /** LNA : Load Name **/
      if(!my_stricmp(current_line->opcode_txt,"LNA") && current_omfproject->last_segment != NULL)
        {
          if(current_omfproject->last_segment->load_name == NULL)
            {
              current_omfproject->last_segment->load_name = strdup(current_line->operand_txt);
              if(current_omfproject->last_segment->load_name == NULL)
                {
                  printf("     => Error, Impossible to allocate memory to process Link file.\n");
                  mem_free_omfproject(current_omfproject);
                  return(NULL);
                }

              /* Enlève les " ou ' */
              CleanUpName(current_omfproject->last_segment->load_name);
            }
          else
            {
              /* On a déjà un LNA */
              if(!my_stricmp(current_omfproject->last_segment->load_name,current_line->operand_txt))
                continue;   /* Même valeur, on ne dit rien */
              else
                {
                  printf("     => Error, Invalid Link file : Two LNA directives found for Segment '%s'.\n",current_omfproject->last_segment->master_file_path);
                  mem_free_omfproject(current_omfproject);
                  return(NULL);
                }
            }

          /* Ligne suivante */
          continue;
        }

      /** SNA : Segment Name **/
      if(!my_stricmp(current_line->opcode_txt,"SNA") && current_omfproject->last_segment != NULL)
        {
          if(current_omfproject->last_segment->segment_name == NULL)
            {
              current_omfproject->last_segment->segment_name = strdup(current_line->operand_txt);
              if(current_omfproject->last_segment->segment_name == NULL)
                {
                  printf("     => Error, Impossible to allocate memory to process Link file.\n");
                  mem_free_omfproject(current_omfproject);
                  return(NULL);
                }

              /* Enlève les " ou ' */
              CleanUpName(current_omfproject->last_segment->segment_name);
            }
          else
            {
              /* On a déjà un SNA */
              if(!my_stricmp(current_omfproject->last_segment->segment_name,current_line->operand_txt))
                continue;   /* Même valeur, on ne dit rien */
              else
                {
                  printf("     => Error, Invalid Link file : Two SNA directives found for Segment '%s'.\n",current_omfproject->last_segment->master_file_path);
                  mem_free_omfproject(current_omfproject);
                  return(NULL);
                }
            }

          /* Ligne suivante */
          continue;
        }

      /** La commande est reconnue, mais on n'a pas de Segment comme support **/
      if(current_omfproject->last_segment == NULL && 
         (!my_stricmp(current_line->opcode_txt,"DS") || !my_stricmp(current_line->opcode_txt,"KND") || !my_stricmp(current_line->opcode_txt,"ALI") || 
          !my_stricmp(current_line->opcode_txt,"LNA") || !my_stricmp(current_line->opcode_txt,"SNA")))
        {
          printf("     => Error, The directive %s is valid but a previous directive ASM is missing in the Link file.\n",current_line->opcode_txt);
          mem_free_omfproject(current_omfproject);
          return(NULL);
        }

      /** Commande inconnue **/
      printf("     => Error, Invalid Link file : Unknown directive found (%s) at line %d.\n",current_line->opcode_txt,current_line->file_line_number);
      mem_free_omfproject(current_omfproject);
      return(NULL);
    }

  /****************************************************************************/
  /** On valide les valeurs du Header, des Projects/Fichiers et des Segments **/
  /****************************************************************************/
  /* A t'on un Multi-Segment non OMF */
  if(current_omfproject->type == 0x0006)     /* BIN = Fixed Address : MultiBinary or SingleBinary */
    {            
      if(current_omfproject->dsk_name_tab != NULL)
        {
          /** Single Binary : Tous les Segments seront collés les uns derrière les autres, dans 1 ou plusieurs fichiers **/
          current_omfproject->is_omf = 0;
          current_omfproject->is_multi_fixed = 1;
          current_omfproject->is_single_binary = 1;
          current_omfproject->express_load = 0;
          
          /* Vérifie les Noms */
          for(i=0; i<current_omfproject->nb_file; i++)
            {
              if(IsProdosName(current_omfproject->dsk_name_tab[i]) == 0)
                {
                  printf("     => Error, Bad Link file name '%s' : Invalid Prodos file name (15 chars max, letters/numbers/. allowed).\n",current_omfproject->dsk_name_tab[i]);
                  mem_free_omfproject(current_omfproject);
                  return(NULL);
                }
            }
            
          /* Vérifie les Org Address */
          for(i=0; i<current_omfproject->nb_file; i++)
            {
              if(current_omfproject->org_address_tab[i] == 0xFFFFFFFF)
                {
                  printf("     => Error, Invalid Link file : Directive ORG is missing for File #%d.\n",i+1);
                  mem_free_omfproject(current_omfproject);
                  return(NULL);
                }
            }            
        }
      else if(current_omfproject->dsk_name_tab == NULL)           /* Pas de DSK ni de ORG dans le Header */
        {
          /** Multi Binary : On va générer autant de fichier Binary qu'il y a de Segments **/
          current_omfproject->is_omf = 0;
          current_omfproject->is_multi_fixed = 1;
          current_omfproject->is_single_binary = 0;
          current_omfproject->express_load = 0;        
        }
    }
  else
    {
      /** OMF : On va générer un Program Relogeable OMF v2.1 **/
      current_omfproject->is_omf = 1;
      current_omfproject->is_multi_fixed = 0;
      current_omfproject->is_single_binary = 0;
              
      /** On élimine les cas douteux **/
      if(current_omfproject->nb_file > 1)
        {
          printf("     => Error, Invalid Link file : Too many DSK directives (only one for an OMF File).\n");
          mem_free_omfproject(current_omfproject);
          return(NULL);        
        }
      if(current_omfproject->dsk_name_tab == NULL)
        {
          printf("     => Error, Invalid Link file : Directive DSK is missing (Target OMF File name).\n");
          mem_free_omfproject(current_omfproject);
          return(NULL);
        }
      if(current_omfproject->org_address_tab[0] != 0xFFFFFFFF)
        {
          printf("     => Error, Invalid Link file : Directive ORG is not allowed (Target is a Relocatable OMF File).\n");
          mem_free_omfproject(current_omfproject);
          return(NULL);
        }
        
      /* Vérifie le nom de fichier */
      if(IsProdosName(current_omfproject->dsk_name_tab[0]) == 0)
        {
          printf("     => Error, Bad Link file name '%s' : Invalid Prodos file name (15 chars max, letters/numbers/. allowed).\n",current_omfproject->dsk_name_tab[0]);
          mem_free_omfproject(current_omfproject);
          return(NULL);
        }
    }

  /** Segments **/
  if(current_omfproject->nb_segment == 0)
    {
      printf("     => Error, Invalid Link file : No Segment defined (use ASM directive).\n");
      mem_free_omfproject(current_omfproject);
      return(NULL);
    }

  /** Vérifie tous les Segments :on met une chaine vide pour les paramètres manquant **/
  for(current_omfsegment = current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
      /* Load Name */
      if(current_omfsegment->load_name == NULL)
        {
          current_omfsegment->load_name = strdup("");
          if(current_omfsegment->load_name == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }
        }

      /* Segment Name */
      if(current_omfsegment->segment_name == NULL)
        {
          current_omfsegment->segment_name = strdup("");
          if(current_omfsegment->segment_name == NULL)
            {
              printf("     => Error, Impossible to allocate memory to process Link file.\n");
              mem_free_omfproject(current_omfproject);
              return(NULL);
            }
        }
    }

  /* OK */
  return(current_omfproject);
}

/***********************************************************************/
