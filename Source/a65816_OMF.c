/***********************************************************************/
/*                                                                     */
/*  a65816_OMF.c : Module pour la gestion du fichier OMF.              */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>

#include "Dc_Library.h"
#include "a65816_Line.h"
#include "a65816_File.h"
#include "a65816_Lup.h"
#include "a65816_Macro.h"
#include "a65816_Cond.h"
#include "a65816_Code.h"
#include "a65816_Data.h"
#include "a65816_OMF.h"

static void mem_init_omfsegment(struct omf_segment *);

/*********************************************************/
/*  BuildOMFFile() :  Création du fichier au format OMF. */
/*********************************************************/
int BuildOMFFile(char *output_folder_path, struct omf_project *current_omfproject)
{
  FILE *fd;
  int offset, nb_write;
  char *file_name;
  char file_path[1024];
  struct omf_segment *current_omfsegment;

  /*************************************/
  /***  Création du fichier Project  ***/
  /*************************************/
  /* Taille du fichier Project (on prend large) */
  current_omfproject->project_buffer_length = 1024;
  for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
    current_omfproject->project_buffer_length += (current_omfsegment->header_length + current_omfsegment->body_length);

  /** Allocation mémoire Project (on prend plus large) **/
  current_omfproject->project_buffer_file = (unsigned char *) calloc(current_omfproject->project_buffer_length,sizeof(unsigned char));
  if(current_omfproject->project_buffer_file == NULL)
    {
      printf("    Error : Can't allocate memory to build OMF File buffer.\n");
      return(1);
    }

  /** Ajout des Data (Header+Body) des Segments **/
  for(offset=0,current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
      /* Header */
      memcpy(&current_omfproject->project_buffer_file[offset],current_omfsegment->segment_header_file,current_omfsegment->header_length);
      offset += current_omfsegment->header_length;

      /* Body */
      memcpy(&current_omfproject->project_buffer_file[offset],current_omfsegment->segment_body_file,current_omfsegment->body_length);
      offset += current_omfsegment->body_length;
    }
  current_omfproject->project_file_length = offset;

  /*******************************************/
  /***  Création du fichier sur le disque  ***/
  /*******************************************/
  /* Nom du fichier */
  if(current_omfproject->dsk_name_tab == NULL)
    file_name = current_omfproject->first_segment->object_name;          /* Mono Segment */
  else
    file_name = current_omfproject->dsk_name_tab[0];
  
  /* Chemin du fichier */
  sprintf(file_path,"%s%s",output_folder_path,file_name);

  /* Information */
  printf("        => Creating OMF file '%s'\n",file_path);

  /* Création du fichier */
#if defined(WIN32) || defined(WIN64) 
  fd = fopen(file_path,"wb+");
#else
  fd = fopen(file_path,"w+");
#endif
  if(fd == NULL)
    {
      printf("    Error : Can't create OMF file '%s'.\n",file_path);
      return(1);
    }

  /* Ecriture des structures OMF dans le fichier */
  nb_write = (int) fwrite(current_omfproject->project_buffer_file,1,current_omfproject->project_file_length,fd);
  if(nb_write != current_omfproject->project_file_length)
    printf("    Error : Can't write OMF file '%s' data (%d bytes / %d bytes).\n",file_path,nb_write,(int)current_omfproject->project_file_length);

  /* Fermeture du fichier */
  fclose(fd);

  /*** Mise à jour du fichier _FileInformation.txt ***/
  UpdateFileInformation(output_folder_path,file_name,current_omfproject);

  /* OK */
  return(0);
}


/************************************************/
/*  BuildOMFBody() :  Construction du Body OMF. */
/************************************************/
DWORD BuildOMFBody(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment)
{
  DWORD offset, offset_length, offset_data, length;
  BYTE one_byte, current_page, address_page;
  WORD one_word;
  DWORD one_dword;
  DWORD address_patch;
  BYTE address_path_byte[4];
  unsigned char *buffer_body;
  int i, nb_address, nb_creloc, nb_super_creloc2, nb_super_creloc3, nb_cinterseg, nb_super_interseg1;
  int nb_super_interseg_13_24[12];
  int nb_super_interseg_25_36[12];
  struct relocate_address *current_address;
  struct relocate_address *next_address;

  /* Init */
  offset = 0;
  buffer_body = current_omfsegment->segment_body_file;

  /****************/
  /***  LCONST  ***/
  /****************/
  /* Record Type */
  one_byte = 0xF2;
  bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* Number of byte */
  one_dword = current_omfsegment->object_length;
  bo_memcpy(&buffer_body[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);

  /* Conserve l'offset des Data pour l'ExpressLoad */
  current_omfsegment->xpress_data_offset = offset;

  /* Data */
  offset_data = offset;
  memcpy(&buffer_body[offset],current_omfsegment->object_code,current_omfsegment->object_length);
  offset += current_omfsegment->object_length;

  /* Conserve la longueur des Data pour l'ExpressLoad */
  current_omfsegment->xpress_data_length = current_omfsegment->object_length;

  /********************************************/
  /** Stat sur les Types d'adresse à reloger **/
  /********************************************/
  nb_address = 0;
  nb_super_creloc2 = 0;
  nb_super_creloc3 = 0;
  nb_creloc = 0;
  nb_cinterseg = 0;
  nb_super_interseg1 = 0;
  for(i=0; i<12; i++)
    {
      nb_super_interseg_13_24[i] = 0;
      nb_super_interseg_25_36[i] = 0;
    }
  for(current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
    {
      /* Nombre total d'adresses */
      nb_address++;

      /* Internes */
      if(current_address->external == NULL)
        {
          if(current_address->ByteCnt == 2 && current_address->BitShiftCnt == 0x00)
            nb_super_creloc2++;
          else if(current_address->ByteCnt == 3 && current_address->BitShiftCnt == 0x00)
            nb_super_creloc3++;
          else
            nb_creloc++;        /* TO DO : pour les segement Data > 64 KB, prévoir des RELOC */
        }
      else   /* Externes */
        {
          if(current_address->ByteCnt == 3 && current_address->BitShiftCnt == 0x00)
            nb_super_interseg1++;
          else if(current_address->external->external_segment->segment_number <= 12 && current_address->ByteCnt == 2 && current_address->BitShiftCnt == 0x00)
            nb_super_interseg_13_24[current_address->external->external_segment->segment_number-1] += 1;
          else if(current_address->external->external_segment->segment_number <= 12 && current_address->ByteCnt == 2 && current_address->BitShiftCnt == 0xF0)   /* -16 */
            nb_super_interseg_25_36[current_address->external->external_segment->segment_number-1] += 1;
          else
            nb_cinterseg++;    /* To Do : Pour les Segments Data > 64 KB ou si le nb de Segment > 256, utiliser les INTERSEG */
        }
    }

  /*****************************************************************/
  /*** SUPER, CRELOC2 : Adresses Internes, 2 Bytes, pas de Shift ***/
  /*****************************************************************/
  if(nb_super_creloc2 > 0)
    {
      /* Conserve l'offset des Reloc pour l'ExpressLoad */
      if(current_omfsegment->xpress_reloc_offset == 0)
        current_omfsegment->xpress_reloc_offset = offset;

      /* Record Type */
      one_byte = 0xF7;              /* SUPER : Super Compress Relocation Dictionary */
      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);
      /* Length */
      offset_length = offset;
      length = 0;
      offset += sizeof(DWORD);
      /* Type */
      one_byte = 0x00;              /* Super Reloc2 */
      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);

      /* Conserve la longueur des Reloc pour l'ExpressLoad */
      current_omfsegment->xpress_reloc_length += (sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE));

      /** On va y placer toutes les adresses par Page **/
      current_page = 0x00;
      for(nb_address = 0,current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
        if(current_address->processed == 0 && current_address->external == NULL && current_address->ByteCnt == 2 && current_address->BitShiftCnt == 0x00)
          {
            /* Page de cette addresse */
            address_page = (BYTE) (current_address->OffsetPatch >> 8);

            /* Doit t'on sauter des Pages ? */
            if(address_page != current_page)
              {
                /* Skip # Pages */
                if(address_page - current_page <= 0x7F)
                  {
                    /* 1 seul Byte de Skip Page suffit */
                    one_byte = 0x80 | (address_page - current_page);
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);
                  }
                else
                  {
                    /* il faut 2 Bytes de Skip Page */
                    one_byte = 0xFF;
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);

                    one_byte = 0x80 | (address_page - current_page - 0x7F);
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);
                  }

                current_page = address_page;
              }

            /** On détermine le nombre d'adresse sur cette Page **/
            for(nb_address = 1,next_address = current_address->next; next_address; next_address=next_address->next)
              if(next_address->processed == 0 && next_address->external == NULL && next_address->ByteCnt == 2 && next_address->BitShiftCnt == 0x00)
                {
                  if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                    nb_address++;
                  else
                    break;
                }

            /* Nombre d'adresse */
            one_byte = (BYTE) (nb_address-1);
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            length += sizeof(BYTE);

            /** Liste des addresses de cette Page **/
            for(next_address = current_address; next_address; next_address=next_address->next)
              if(next_address->processed == 0 && next_address->external == NULL && next_address->ByteCnt == 2 && next_address->BitShiftCnt == 0x00)
                {
                  if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                    {
                      /* Partie basse de l'adresse */
                      one_byte = (BYTE) (0x00FF & next_address->OffsetPatch);
                      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                      offset += sizeof(BYTE);
                      length += sizeof(BYTE);

                      /* Cette addresse est traitée */
                      next_address->processed = 1;
                    }
                  else
                    break;
                }

            /* Page suivante */
            current_page++;
          }

      /* Conserve la longueur des Reloc pour l'ExpressLoad */
      current_omfsegment->xpress_reloc_length += length;

      /** Longueur du Super **/
      length += sizeof(BYTE);  /* Type */
      bo_memcpy(&buffer_body[offset_length],&length,sizeof(DWORD));
    }

  /*****************************************************************/
  /*** SUPER, CRELOC3 : Adresses Internes, 3 Bytes, pas de Shift ***/
  /*****************************************************************/
  if(nb_super_creloc3 > 0)
    {
      /* Conserve l'offset des Reloc pour l'ExpressLoad */
      if(current_omfsegment->xpress_reloc_offset == 0)
        current_omfsegment->xpress_reloc_offset = offset;

      /* Record Type */
      one_byte = 0xF7;              /* SUPER : Super Compress Relocation Dictionary */
      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);
      /* Length */
      offset_length = offset;
      length = 0;
      offset += sizeof(DWORD);
      /* Type */
      one_byte = 0x01;              /* Super Reloc3 */
      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);

      /* Conserve la longueur des Reloc pour l'ExpressLoad */
      current_omfsegment->xpress_reloc_length += (sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE));

      /** On va y placer toutes les adresses par Page **/
      current_page = 0x00;
      for(nb_address = 0,current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
        if(current_address->processed == 0 && current_address->external == NULL && current_address->ByteCnt == 3 && current_address->BitShiftCnt == 0x00)
          {
            /* Page de cette addresse */
            address_page = (BYTE) (current_address->OffsetPatch >> 8);

            /* Doit t'on sauter des Pages ? */
            if(address_page != current_page)
              {
                /* Skip # Pages */
                if(address_page - current_page <= 0x7F)
                  {
                    /* 1 seul Byte de Skip Page suffit */
                    one_byte = 0x80 | (address_page - current_page);
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);
                  }
                else
                  {
                    /* il faut 2 Bytes de Skip Page */
                    one_byte = 0xFF;
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);

                    one_byte = 0x80 | (address_page - current_page - 0x7F);
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);
                  }

                current_page = address_page;
              }

            /** On détermine le nombre d'adresse sur cette Page **/
            for(nb_address = 1,next_address = current_address->next; next_address; next_address=next_address->next)
              if(next_address->processed == 0 && next_address->external == NULL && next_address->ByteCnt == 3 && next_address->BitShiftCnt == 0x00)
                {
                  if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                    nb_address++;
                  else
                    break;
                }

            /* Nombre d'adresse */
            one_byte = (BYTE) (nb_address-1);
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            length += sizeof(BYTE);

            /** Liste des addresses de cette Page **/
            for(next_address = current_address; next_address; next_address=next_address->next)
              if(next_address->processed == 0 && next_address->external == NULL && next_address->ByteCnt == 3 && next_address->BitShiftCnt == 0x00)
                {
                  if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                    {
                      /* Partie basse de l'adresse */
                      one_byte = (BYTE) (0x00FF & next_address->OffsetPatch);
                      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                      offset += sizeof(BYTE);
                      length += sizeof(BYTE);

                      /* Cette addresse est traitée */
                      next_address->processed = 1;
                    }
                  else
                    break;
                }

            /* Page suivante */
            current_page++;
          }

      /* Conserve la longueur des Reloc pour l'ExpressLoad */
      current_omfsegment->xpress_reloc_length += length;

      /** Longueur du Super **/
      length += sizeof(BYTE);      /* Type */
      bo_memcpy(&buffer_body[offset_length],&length,sizeof(DWORD));
    }

  /**********************************/
  /*** cRELOC : Adresses Internes ***/
  /**********************************/
  if(nb_creloc > 0)
    {
      for(current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
        if(current_address->processed == 0 && current_address->external == NULL)
          {
            /* Conserve l'offset des Reloc pour l'ExpressLoad */
            if(current_omfsegment->xpress_reloc_offset == 0)
              current_omfsegment->xpress_reloc_offset = offset;

            /* Record Type */
            one_byte = 0xF5;              /* cRELOC */
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Number of Byte to be relocated */
            one_byte = current_address->ByteCnt;
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Bit Shift Operator */
            one_byte = current_address->BitShiftCnt;
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Offset of the first Byte to be Patched */
            one_word = current_address->OffsetPatch;
            bo_memcpy(&buffer_body[offset],&one_word,sizeof(WORD));
            offset += sizeof(WORD);
            /* Address */
            one_word = current_address->OffsetReference;
            bo_memcpy(&buffer_body[offset],&one_word,sizeof(WORD));
            offset += sizeof(WORD);

            /* Cette addresse est traitée */
            current_address->processed = 1;

            /* Conserve la longueur des Reloc pour l'ExpressLoad */
            current_omfsegment->xpress_reloc_length += (3*sizeof(BYTE) + 2*sizeof(WORD));
          }
    }

  /********************************************************************/
  /*** SUPER, INTERSEG 1 : Adresses Externes, 3 Bytes, pas de Shift ***/
  /********************************************************************/
  if(nb_super_interseg1 > 0)
    {
      /* Conserve l'offset des Reloc pour l'ExpressLoad */
      if(current_omfsegment->xpress_reloc_offset == 0)
        current_omfsegment->xpress_reloc_offset = offset;

      /* Record Type */
      one_byte = 0xF7;              /* SUPER : Super Compress Relocation Dictionary */
      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);
      /* Length */
      offset_length = offset;
      length = 0;
      offset += sizeof(DWORD);
      /* Type */
      one_byte = 0x02;              /* Super Interseg 1 */
      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);

      /* Conserve la longueur des Reloc pour l'ExpressLoad */
      current_omfsegment->xpress_reloc_length += (sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE));

      /** On va y placer toutes les adresses par Page **/
      current_page = 0x00;
      for(nb_address = 0,current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
        if(current_address->processed == 0 && current_address->external != NULL && current_address->ByteCnt == 3 && current_address->BitShiftCnt == 0x00)
          {
            /* Page de cette addresse */
            address_page = (BYTE) (current_address->OffsetPatch >> 8);

            /* Doit t'on sauter des Pages ? */
            if(address_page != current_page)
              {
                /* Skip # Pages */
                if(address_page - current_page <= 0x7F)
                  {
                    /* 1 seul Byte de Skip Page suffit */
                    one_byte = 0x80 | (address_page - current_page);
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);
                  }
                else
                  {
                    /* il faut 2 Bytes de Skip Page */
                    one_byte = 0xFF;
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);

                    one_byte = 0x80 | (address_page - current_page - 0x7F);
                    bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                    offset += sizeof(BYTE);
                    length += sizeof(BYTE);
                  }

                current_page = address_page;
              }

            /** On détermine le nombre d'adresse sur cette Page **/
            for(nb_address = 1,next_address = current_address->next; next_address; next_address=next_address->next)
              if(next_address->processed == 0 && next_address->external != NULL && next_address->ByteCnt == 3 && next_address->BitShiftCnt == 0x00)
                {
                  if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                    nb_address++;
                  else
                    break;
                }

            /* Nombre d'adresse */
            one_byte = (BYTE) (nb_address-1);
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            length += sizeof(BYTE);

            /** Liste des addresses de cette Page **/
            for(next_address = current_address; next_address; next_address=next_address->next)
              if(next_address->processed == 0 && next_address->external != NULL && next_address->ByteCnt == 3 && next_address->BitShiftCnt == 0x00)
                {
                  if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                    {
                      /* Partie basse de l'adresse */
                      one_byte = (BYTE) (0x00FF & next_address->OffsetPatch);
                      bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                      offset += sizeof(BYTE);
                      length += sizeof(BYTE);

                      /** Modifie les Data au niveau de l'offset en y plaçant l'adresse (on ajoute le delta apporté par l'opération sur le label) **/
                      address_patch = next_address->external->external_label->line->address + next_address->OffsetReference;
                      if(next_address->BitShiftCnt == 0x00)
                        address_patch |= (((DWORD)(next_address->external->external_segment->segment_number)) << 16);
                      if(next_address->BitShiftCnt == 0xF0)
                        address_patch = (address_patch >> 16);
                      else if(next_address->BitShiftCnt == 0xF8)
                        address_patch = (address_patch >> 8);
                      bo_memcpy(&address_path_byte[0],&address_patch,sizeof(DWORD));
                      /* Copie à 3 endroits */
                      memcpy(&buffer_body[offset_data+next_address->OffsetPatch],&address_path_byte[0],next_address->ByteCnt);           /* Dans le LCONST de l'OMF */
                      memcpy(&current_omfsegment->object_code[next_address->OffsetPatch],&address_path_byte[0],next_address->ByteCnt);   /* Dans le Code Objet du Segment */
                      memcpy(next_address->object_line,&address_path_byte[0],next_address->ByteCnt);                                     /* Dans la line (operand_byte ou data) */

                      /* Cette addresse est traitée */
                      next_address->processed = 1;
                    }
                  else
                    break;
                }

            /* Page suivante */
            current_page++;
          }

      /* Conserve la longueur des Reloc pour l'ExpressLoad */
      current_omfsegment->xpress_reloc_length += length;

      /** Longueur du Super **/
      length += sizeof(BYTE);      /* Type */
      bo_memcpy(&buffer_body[offset_length],&length,sizeof(DWORD));
    }

  /************************************************************************/
  /*** SUPER, INTERSEG 13-24 : Adresses Externes, 2 Bytes, pas de Shift ***/
  /************************************************************************/
  for(i=0; i<12; i++)
    if(nb_super_interseg_13_24[i] > 0)
      {
        /* Conserve l'offset des Reloc pour l'ExpressLoad */
        if(current_omfsegment->xpress_reloc_offset == 0)
          current_omfsegment->xpress_reloc_offset = offset;

        /* Record Type */
        one_byte = 0xF7;              /* SUPER : Super Compress Relocation Dictionary */
        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
        offset += sizeof(BYTE);
        /* Length */
        offset_length = offset;
        length = 0;
        offset += sizeof(DWORD);
        /* Type */
        one_byte = 14+i;              /* Super Interseg 13-24 / code 14-25 / Segment 1-12 */
        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
        offset += sizeof(BYTE);

        /* Conserve la longueur des Reloc pour l'ExpressLoad */
        current_omfsegment->xpress_reloc_length += (sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE));

        /** On va y placer toutes les adresses par Page **/
        current_page = 0x00;
        for(nb_address = 0,current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
          if(current_address->processed == 0 && current_address->external != NULL && current_address->ByteCnt == 2 && current_address->BitShiftCnt == 0x00)
            if(current_address->external->external_segment->segment_number == i+1)
              {
                /* Page de cette addresse */
                address_page = (BYTE) (current_address->OffsetPatch >> 8);

                /* Doit t'on sauter des Pages ? */
                if(address_page != current_page)
                  {
                    /* Skip # Pages */
                    if(address_page - current_page <= 0x7F)
                      {
                        /* 1 seul Byte de Skip Page suffit */
                        one_byte = 0x80 | (address_page - current_page);
                        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                        offset += sizeof(BYTE);
                        length += sizeof(BYTE);
                      }
                    else
                      {
                        /* il faut 2 Bytes de Skip Page */
                        one_byte = 0xFF;
                        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                        offset += sizeof(BYTE);
                        length += sizeof(BYTE);

                        one_byte = 0x80 | (address_page - current_page - 0x7F);
                        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                        offset += sizeof(BYTE);
                        length += sizeof(BYTE);
                      }

                    current_page = address_page;
                  }

                /** On détermine le nombre d'adresse sur cette Page **/
                for(nb_address = 1,next_address = current_address->next; next_address; next_address=next_address->next)
                  if(next_address->processed == 0 && next_address->external != NULL && next_address->ByteCnt == 2 && next_address->BitShiftCnt == 0x00)
                    if(next_address->external->external_segment->segment_number == i+1)
                      {
                        if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                          nb_address++;
                        else
                          break;
                      }

                /* Nombre d'adresse */
                one_byte = (BYTE) (nb_address-1);
                bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                offset += sizeof(BYTE);
                length += sizeof(BYTE);

                /** Liste des addresses de cette Page **/
                for(next_address = current_address; next_address; next_address=next_address->next)
                  if(next_address->processed == 0 && next_address->external != NULL && next_address->ByteCnt == 2 && next_address->BitShiftCnt == 0x00)
                    if(next_address->external->external_segment->segment_number == i+1)
                      {
                        if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                          {
                            /* Partie basse de l'adresse */
                            one_byte = (BYTE) (0x00FF & next_address->OffsetPatch);
                            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                            offset += sizeof(BYTE);
                            length += sizeof(BYTE);

                            /** Modifie les Data au niveau de l'offset en y plaçant l'adresse (on ajoute le delta apporté par l'opération sur le label) **/
                            address_patch = next_address->external->external_label->line->address + next_address->OffsetReference;
                            if(next_address->BitShiftCnt == 0x00)
                              address_patch |= (((DWORD)(next_address->external->external_segment->segment_number)) << 16);
                            if(next_address->BitShiftCnt == 0xF0)
                              address_patch = (address_patch >> 16);
                            else if(next_address->BitShiftCnt == 0xF8)
                              address_patch = (address_patch >> 8);
                            bo_memcpy(&address_path_byte[0],&address_patch,sizeof(DWORD));
                            /* Copie à 3 endroits */
                            memcpy(&buffer_body[offset_data+next_address->OffsetPatch],&address_path_byte[0],next_address->ByteCnt);           /* Dans le LCONST de l'OMF */
                            memcpy(&current_omfsegment->object_code[next_address->OffsetPatch],&address_path_byte[0],next_address->ByteCnt);   /* Dans le Code Objet du Segment */
                            memcpy(next_address->object_line,&address_path_byte[0],next_address->ByteCnt);                                     /* Dans la line (operand_byte ou data) */

                            /* Cette addresse est traitée */
                            next_address->processed = 1;
                          }
                        else
                          break;
                      }

                /* Page suivante */
                current_page++;
              }

        /* Conserve la longueur des Reloc pour l'ExpressLoad */
        current_omfsegment->xpress_reloc_length += length;

        /** Longueur du Super **/
        length += sizeof(BYTE);      /* Type */
        bo_memcpy(&buffer_body[offset_length],&length,sizeof(DWORD));
      }

  /*****************************************************************/
  /*** SUPER, INTERSEG 25-36 : Adresses Externes, 2 Bytes, >> 16 ***/
  /*****************************************************************/
  for(i=0; i<12; i++)
    if(nb_super_interseg_25_36[i] > 0)
      {
        /* Conserve l'offset des Reloc pour l'ExpressLoad */
        if(current_omfsegment->xpress_reloc_offset == 0)
          current_omfsegment->xpress_reloc_offset = offset;

        /* Record Type */
        one_byte = 0xF7;              /* SUPER : Super Compress Relocation Dictionary */
        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
        offset += sizeof(BYTE);
        /* Length */
        offset_length = offset;
        length = 0;
        offset += sizeof(DWORD);
        /* Type */
        one_byte = 26+i;              /* Super Interseg 25-36 / code 26-37 / Segment 1-12 */
        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
        offset += sizeof(BYTE);

        /* Conserve la longueur des Reloc pour l'ExpressLoad */
        current_omfsegment->xpress_reloc_length += (sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE));

        /** On va y placer toutes les adresses par Page **/
        current_page = 0x00;
        for(nb_address = 0,current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
          if(current_address->processed == 0 && current_address->external != NULL && current_address->ByteCnt == 2 && current_address->BitShiftCnt == 0xF0)
            if(current_address->external->external_segment->segment_number == i+1)
              {
                /* Page de cette addresse */
                address_page = (BYTE) (current_address->OffsetPatch >> 8);

                /* Doit t'on sauter des Pages ? */
                if(address_page != current_page)
                  {
                    /* Skip # Pages */
                    if(address_page - current_page <= 0x7F)
                      {
                        /* 1 seul Byte de Skip Page suffit */
                        one_byte = 0x80 | (address_page - current_page);
                        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                        offset += sizeof(BYTE);
                        length += sizeof(BYTE);
                      }
                    else
                      {
                        /* il faut 2 Bytes de Skip Page */
                        one_byte = 0xFF;
                        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                        offset += sizeof(BYTE);
                        length += sizeof(BYTE);

                        one_byte = 0x80 | (address_page - current_page - 0x7F);
                        bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                        offset += sizeof(BYTE);
                        length += sizeof(BYTE);
                      }

                    current_page = address_page;
                  }

                /** On détermine le nombre d'adresse sur cette Page **/
                for(nb_address = 1,next_address = current_address->next; next_address; next_address=next_address->next)
                  if(next_address->processed == 0 && next_address->external != NULL && next_address->ByteCnt == 2 && next_address->BitShiftCnt == 0xF0)
                    if(next_address->external->external_segment->segment_number == i+1)
                      {
                        if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                          nb_address++;
                        else
                          break;
                      }

                /* Nombre d'adresse */
                one_byte = (BYTE) (nb_address-1);
                bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                offset += sizeof(BYTE);
                length += sizeof(BYTE);

                /** Liste des addresses de cette Page **/
                for(next_address = current_address; next_address; next_address=next_address->next)
                  if(next_address->processed == 0 && next_address->external != NULL && next_address->ByteCnt == 2 && next_address->BitShiftCnt == 0xF0)
                    if(next_address->external->external_segment->segment_number == i+1)
                      {
                        if((BYTE) (next_address->OffsetPatch >> 8) == address_page)
                          {
                            /* Partie basse de l'adresse */
                            one_byte = (BYTE) (0x00FF & next_address->OffsetPatch);
                            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
                            offset += sizeof(BYTE);
                            length += sizeof(BYTE);

                            /** Modifie les Data au niveau de l'offset en y plaçant les 2 octets bas de l'adresse (on ajoute le delta apporté par l'opération sur le label) **/
                            address_patch = next_address->external->external_label->line->address + next_address->OffsetReference;
                            bo_memcpy(&address_path_byte[0],&address_patch,sizeof(DWORD));
                            /* Copie à 3 endroits */
                            memcpy(&buffer_body[offset_data+next_address->OffsetPatch],&address_path_byte[0],next_address->ByteCnt);           /* Dans le LCONST de l'OMF */
                            memcpy(&current_omfsegment->object_code[next_address->OffsetPatch],&address_path_byte[0],next_address->ByteCnt);   /* Dans le Code Objet du Segment */
                            memcpy(next_address->object_line,&address_path_byte[0],next_address->ByteCnt);                                     /* Dans la line (operand_byte ou data) */

                            /* Cette addresse est traitée */
                            next_address->processed = 1;
                          }
                        else
                          break;
                      }

                /* Page suivante */
                current_page++;
              }

        /* Conserve la longueur des Reloc pour l'ExpressLoad */
        current_omfsegment->xpress_reloc_length += length;

        /** Longueur du Super **/
        length += sizeof(BYTE);      /* Type */
        bo_memcpy(&buffer_body[offset_length],&length,sizeof(DWORD));
    }

  /*************************************/
  /*** cINTERSEG : Adresses Externes ***/
  /*************************************/
  if(nb_cinterseg > 0)
    {
      for(current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
        if(current_address->processed == 0 && current_address->external != NULL)
          {
            /* Conserve l'offset des Reloc pour l'ExpressLoad */
            if(current_omfsegment->xpress_reloc_offset == 0)
              current_omfsegment->xpress_reloc_offset = offset;

            /* Record Type */
            one_byte = 0xF6;              /* cINTERSEG */
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Number of Byte to be relocated */
            one_byte = current_address->ByteCnt;
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Bit Shift Operator */
            one_byte = current_address->BitShiftCnt;
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Offset of the first Byte to be Patched */
            one_word = current_address->OffsetPatch;
            bo_memcpy(&buffer_body[offset],&one_word,sizeof(WORD));
            offset += sizeof(WORD);
            /* Segment Number */
            one_byte = (BYTE) current_address->external->external_segment->segment_number;
            bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
            offset += sizeof(BYTE);
            /* Address dans un autre Segment */
            one_word = current_address->external->external_label->line->address;
            bo_memcpy(&buffer_body[offset],&one_word,sizeof(WORD));
            offset += sizeof(WORD);

            /** Modifie les Data au niveau de l'offset en y plaçant l'adresse (on ajoute le delta apporté par l'opération sur le label) **/
            address_patch = current_address->external->external_label->line->address + current_address->OffsetReference;
            if(current_address->BitShiftCnt == 0x00)
              address_patch |= (((DWORD)(current_address->external->external_segment->segment_number)) << 16);
            if(current_address->BitShiftCnt == 0xF0)
              address_patch = (address_patch >> 16);
            else if(current_address->BitShiftCnt == 0xF8)
              address_patch = (address_patch >> 8);
            bo_memcpy(&address_path_byte[0],&address_patch,sizeof(DWORD));
            /* Copie à 3 endroits */
            memcpy(&buffer_body[offset_data+current_address->OffsetPatch],&address_path_byte[0],current_address->ByteCnt);           /* Dans le LCONST de l'OMF */
            memcpy(&current_omfsegment->object_code[current_address->OffsetPatch],&address_path_byte[0],current_address->ByteCnt);   /* Dans le Code Objet du Segment */
            memcpy(current_address->object_line,&address_path_byte[0],current_address->ByteCnt);                                     /* Dans la line (operand_byte ou data) */

            /* Cette addresse est traitée */
            current_address->processed = 1;

            /* Conserve la longueur des Reloc pour l'ExpressLoad */
            current_omfsegment->xpress_reloc_length += (4*sizeof(BYTE) + 2*sizeof(WORD));
          }
    }
  
  /*** END ***/
  one_byte = 0x00;
  bo_memcpy(&buffer_body[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);

  /* Renvoie la taille du Body */
  return(offset);
}


/***************************************************************************************/
/*  RelocateExternalFixedAddress() :  On va reloger les valeurs des adresses externes. */
/***************************************************************************************/
void RelocateExternalFixedAddress(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment)
{
  DWORD address_patch, org_address, offset_file;
  BYTE address_path_byte[4];
  struct relocate_address *current_address;

  /* ORG Adresse du Segment */
  org_address = (current_omfsegment->has_org_address == 1) ? current_omfsegment->org_address : current_omfsegment->org;

  /** On va modifier les adresses faisant référence aux adresses externes **/  
  for(current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
    {
      /* Internes */
      if(current_address->external == NULL)
        continue;

      /** Modifie les Data au niveau de l'offset en y plaçant l'adresse (on ajoute le delta apporté par l'opération sur le label) **/
      address_patch = (current_address->external->external_label->line->bank << 16) | (current_address->external->external_label->line->address + current_address->OffsetReference);
      if(current_address->BitShiftCnt == 0xF0)
        address_patch = (address_patch >> 16);
      else if(current_address->BitShiftCnt == 0xF8)
        address_patch = (address_patch >> 8);
      bo_memcpy(&address_path_byte[0],&address_patch,sizeof(DWORD));

      /* Vérifie les dépassements */
      if(current_address->OffsetPatch < (0x0000FFFF & org_address))
        {
          printf("     => Error : Can't relocate Address at %04X (Object Org Address is %04X).\n",(int)current_address->OffsetPatch,(int)(0xFFFF & org_address));
          continue;
        }
      if(current_address->OffsetPatch - (0x0000FFFF & org_address) > (DWORD) current_omfsegment->object_length)
        {
          printf("     => Error : Can't relocate Address at %04X (Object Length is %04X with Org Address %04X).\n",(int)current_address->OffsetPatch,(int)current_omfsegment->object_length,(int)(0x0000FFFF & org_address));
          continue;
        }

      /* Offset depuis le debut du fichier / object_code */
      offset_file = current_address->OffsetPatch - (0x0000FFFF & org_address);

      /* Copie à 2 endroits */
      memcpy(&current_omfsegment->object_code[offset_file],&address_path_byte[0],current_address->ByteCnt);          /* Dans le Code Objet du Segment */
      memcpy(current_address->object_line,&address_path_byte[0],current_address->ByteCnt);                           /* Dans la line (operand_byte ou data) */        
      
      /* Information */
      //printf("     => Relocate Address at %04X : %06X (Offset File is %04X).\n",current_address->OffsetPatch,address_patch,offset_file);
    }
}


/****************************************************/
/*  BuildOMFHeader() :  Construction du Header OMF. */
/****************************************************/
DWORD BuildOMFHeader(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment)
{
  int i, length;
  WORD lab_length;
  DWORD offset;
  BYTE one_byte;
  WORD one_word;
  DWORD one_dword;

  /* Init */
  offset = 0;

  /** Remplissage du Header **/
  /* Byte Count (mis à la fin pour tenir compte de la longueur de seg_name */
  offset += sizeof(DWORD);
  /* Res Spc */
  one_dword = 0;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* Length : Taille du Segment une fois en mémoire = LCONST + DS */
  one_dword = current_omfsegment->object_length;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* undefined #1 */
  one_byte = 0x00;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /** LABLEN (0 ou 10) **/
  if(strlen(current_omfsegment->segment_name) <= 10)
    one_byte = (BYTE) 0x0A;
  else
    one_byte = 0x00;            /* La longueur su seg_name est codée au début du texte sur 1 octet */
  lab_length = (WORD) one_byte;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* NUMLEN */
  one_byte = 0x04;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* VERSION : 2.1 */
  one_byte = 0x02;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* BANKSIZE */
  one_dword = current_omfsegment->bank_size;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* kind */
  one_word = current_omfsegment->type_attributes;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /* undefined #2 */
  one_byte = 0x00;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* undefined #3 */
  one_byte = 0x00;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* ORG */
  one_dword = current_omfsegment->org;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* Align */
  one_dword = current_omfsegment->alignment;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* numsex */
  one_byte = 0x00;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* undefined #4 */
  one_byte = 0x00;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* segnum : 1->N (ou 2->N si ExpressLoad) */
  one_word = current_omfsegment->segment_number;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /* entry */
  one_dword = 0;
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* dispname */
  one_word = 0x002C;   /* 44 */
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /* dispdata */
  one_word = 0x002C + 0x000A + (WORD) ((lab_length == 0) ? (1+strlen(current_omfsegment->segment_name)) : 0x000A);   /* 44 + 10 + (10 ou 1+strlen(seg_name)) */
  bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);

  /* LOAD NAME : 10 bytes */
  length = (int) ((strlen(current_omfsegment->load_name) > 10) ? 10 : strlen(current_omfsegment->load_name));   /* On ne dépasse pas plus de 10 */
  memset(&current_omfsegment->segment_header_file[offset],0x20,10);
  for(i=0; i<length; i++)
    current_omfsegment->segment_header_file[offset+i] = current_omfsegment->load_name[i];
  offset += 10;

  /* SEG NAME */
  if(lab_length == 0)
    {
      /* La longueur est codée sur 1 byte au début */
      one_byte = (BYTE) strlen(current_omfsegment->segment_name);
      bo_memcpy(&current_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
      offset += sizeof(BYTE);
      memcpy(&current_omfsegment->segment_header_file[offset],current_omfsegment->segment_name,strlen(current_omfsegment->segment_name));
      offset += (DWORD) strlen(current_omfsegment->segment_name);
    }
  else
    {
      /* lab_length = 10 bytes */
      memset(&current_omfsegment->segment_header_file[offset],0x20,lab_length);
      for(i=0; i<(int)strlen(current_omfsegment->segment_name); i++)
        current_omfsegment->segment_header_file[offset+i] = current_omfsegment->segment_name[i];
      offset += lab_length;
    }

  /* Byte Count */
  one_dword = offset + current_omfsegment->body_length;
  bo_memcpy(&current_omfsegment->segment_header_file[0],&one_dword,sizeof(DWORD));

  /** On fait une copie partielle pour l'ExpressLoad **/
  current_omfsegment->header_xpress_length = offset - (3*sizeof(DWORD));
  memcpy(current_omfsegment->header_xpress_file,&current_omfsegment->segment_header_file[3*sizeof(DWORD)],current_omfsegment->header_xpress_length);

  /* Renvoie la taille du Header */
  return(offset);
}


/*******************************************************************/
/*  BuildExpressLoadSegment() :  Création du Segment ~ExpressLoad. */ 
/*******************************************************************/
int BuildExpressLoadSegment(struct omf_project *current_omfproject)
{
  int i, offset, file_offset, body_lconst_length;
  BYTE one_byte;
  WORD one_word;
  DWORD one_dword;
  char *xpress_name = "~ExpressLoad";
  struct omf_segment *xpress_omfsegment;
  struct omf_segment *current_omfsegment;

  /** Allocation mémoire **/
  xpress_omfsegment = mem_alloc_omfsegment();
  if(xpress_omfsegment == NULL)
    return(1);
  xpress_omfsegment->segment_name = strdup(xpress_name);
  if(xpress_omfsegment->segment_name == NULL)  
    {
      /* Libération mémoire */
      mem_free_omfsegment(xpress_omfsegment);
      return(1);
    }

  /* Attachement en 1ère position */
  current_omfproject->nb_segment++;
  xpress_omfsegment->next = current_omfproject->first_segment;
  current_omfproject->first_segment = xpress_omfsegment;

  /* ExpressLoad est toujours en 1ère position */
  xpress_omfsegment->segment_number = 1;

  /* Taille des Data du Body de l'ExpressLoad */
  body_lconst_length = sizeof(DWORD) + sizeof(WORD);
  for(current_omfsegment=xpress_omfsegment->next; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
      body_lconst_length += 2*sizeof(WORD) + sizeof(DWORD);                                  /* Header entry table */
      body_lconst_length += sizeof(WORD);                                                    /* Segment Number Conversion Table */
      body_lconst_length += (4*sizeof(DWORD) + current_omfsegment->header_xpress_length);    /* Segment Header Table */
    }

  /* Allocation mémoire du Body */
  xpress_omfsegment->segment_body_length = 1024 + body_lconst_length;         /* On prend large car on stocke aussi le code du LCONST et le END */
  xpress_omfsegment->segment_body_file = (unsigned char *) calloc(1,xpress_omfsegment->segment_body_length);
  if(xpress_omfsegment->segment_body_file == NULL)
    return(1);

  /* On connait les tailles du Header et du Body avant de les remplir */
  xpress_omfsegment->header_length = 67;    /* 44 + 10 + (1+12) = 67 bytes */
  xpress_omfsegment->body_length = (sizeof(BYTE) + sizeof(DWORD)) + body_lconst_length + sizeof(BYTE);

  /* Offset depuis le début du fichier */
  file_offset = (xpress_omfsegment->header_length + xpress_omfsegment->body_length);

  /*******************************/
  /***  Body de l'ExpressLoad  ***/
  /*******************************/
  offset = 0;

  /** LCONST **/
  /* Record Type */
  one_byte = 0xF2;
  bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* Number of byte */
  one_dword = body_lconst_length;
  bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);

  /** Data **/
  /* Reserved */
  one_dword = 0;
  bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* Number of segments - 1 */
  one_word = current_omfproject->nb_segment - 2;
  bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /** Header Entry Table **/
  for(current_omfsegment=xpress_omfsegment->next; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
      /* Offset relative to segment Header Entry Table */
      one_word = 0;    /* Place la vraie valeur plus bas */
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_word,sizeof(WORD));
      offset += sizeof(WORD);
      /* Reserved */
      one_word = 0;
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_word,sizeof(WORD));
      offset += sizeof(WORD);
      /* Reserved */
      one_dword = 0;
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_dword,sizeof(DWORD));
      offset += sizeof(DWORD);
    }
  /** Segment Number Conversion Table **/
  for(current_omfsegment=xpress_omfsegment->next; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
      /* Actual file segment number for original segment */
      one_word = current_omfsegment->segment_number;
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_word,sizeof(WORD));
      offset += sizeof(WORD);
    }
  /** Segment Header Table **/
  for(i=0,current_omfsegment=xpress_omfsegment->next; current_omfsegment; current_omfsegment=current_omfsegment->next,i++)
    {
      /* Finalise la valeur : Offset relative to segment Header Entry Table */
      one_word = (WORD) (offset - (sizeof(BYTE) + sizeof(DWORD) + sizeof(DWORD) + sizeof(WORD) + (i*(2*sizeof(WORD) + sizeof(DWORD)))));
      bo_memcpy(&xpress_omfsegment->segment_body_file[sizeof(BYTE) + sizeof(DWORD) + sizeof(DWORD) + sizeof(WORD) + i*(2*sizeof(WORD) + sizeof(DWORD))],&one_word,sizeof(WORD));

      /** Offset et Length des Data et Reloc **/
      current_omfsegment->xpress_data_offset += (current_omfsegment->header_length + file_offset);
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&current_omfsegment->xpress_data_offset,sizeof(DWORD));
      offset += sizeof(DWORD);
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&current_omfsegment->xpress_data_length,sizeof(DWORD));
      offset += sizeof(DWORD);
      current_omfsegment->xpress_reloc_offset += (current_omfsegment->header_length + file_offset);
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&current_omfsegment->xpress_reloc_offset,sizeof(DWORD));
      offset += sizeof(DWORD);
      bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&current_omfsegment->xpress_reloc_length,sizeof(DWORD));
      offset += sizeof(DWORD);

      /* Segment #n Header Info */
      memcpy(&xpress_omfsegment->segment_body_file[offset],current_omfsegment->header_xpress_file,current_omfsegment->header_xpress_length);
      offset += current_omfsegment->header_xpress_length;

      /* Offset depuis le début du fichier */
      file_offset += (current_omfsegment->header_length + current_omfsegment->body_length);
    }

  /** END **/
  one_byte = 0x00;
  bo_memcpy(&xpress_omfsegment->segment_body_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);

  /*********************************/
  /***  Header de l'ExpressLoad  ***/
  /*********************************/
  offset = 0;
  /* Segment Header size (ici 67 bytes) + Segment Body size */
  one_dword = 67 + xpress_omfsegment->body_length;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* Res Spc */
  one_dword = 0;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* Length once in memory = LCONST Size */
  one_dword = xpress_omfsegment->body_length - (sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE));   /* On enlève la taille de la structure LCONST + END */
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* undefined #1 */
  one_byte = 0x00;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* LABLEN => on met à zéro et on va utiliser un byte en début de chaine de caractère pour indiquer la longueur */
  one_byte = 0;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* NUMLEN */
  one_byte = 0x04;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* VERSION */
  one_byte = 0x02;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* BANKSIZE => 0 pour Data */
  one_dword = 0;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* kind = Data  (Dynamic + Can be loaded in Special Memory) */
  one_word = 0x8001;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /* undefined #2 */
  one_byte = 0x00;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* undefined #3 */
  one_byte = 0x00;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* ORG */
  one_dword = 0;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* Align */
  one_dword = 0;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* numsex */
  one_byte = 0x00;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* undefined #4 */
  one_byte = 0x00;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  /* segnum */
  one_word = 0x0001;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /* entry */
  one_dword = 0;
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_dword,sizeof(DWORD));
  offset += sizeof(DWORD);
  /* dispname */
  one_word = 0x002C;   /* 44 */
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);
  /* dispdata = Header Size */
  one_word = 0x0043;   /* 44 + 10 + 1 + 12=strlen("~ExpressLoad") = 67 */
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_word,sizeof(WORD));
  offset += sizeof(WORD);

  /* LOAD NAME : 10 bytes à 0x00 */
  for(i=0; i<10; i++)
    xpress_omfsegment->segment_header_file[offset+i] = 0x00;
  offset += 10;

  /** SEG NAME : 1 byte_length + "~ExpressLoad" **/
  one_byte = (BYTE) strlen(xpress_name);
  bo_memcpy(&xpress_omfsegment->segment_header_file[offset],&one_byte,sizeof(BYTE));
  offset += sizeof(BYTE);
  memcpy(&xpress_omfsegment->segment_header_file[offset],xpress_name,strlen(xpress_name));
  offset += (int) strlen(xpress_name);

  /* OK */
  return(0);
}


/*******************************************************************************/
/*  UpdateFileInformation() :  MAJ / Création du fichier _FileInformation.txt. */
/*******************************************************************************/
void UpdateFileInformation(char *output_folder_path, char *output_file_name, struct omf_project *current_omfproject)
{
  FILE *fd;
  char *next_sep;
  int i, nb_line;
  BYTE version_created, min_version, access;
  char **line_tab;
  char file_information_path[1024];
  char file_name[1024];
  char local_buffer[1024];
  char folder_info1[256] = "000000000000000000000000000000000000";
  char folder_info2[256] = "000000000000000000000000000000000000";
  
  /* Init */
  version_created = 0x70;
  min_version = 0xBE;
  access = 0xE3;           /* Access Flags : Delete + Rename + Backup + Write + Read */

  /* Chemin du fichier _FileInformation.txt */
  sprintf(file_information_path,"%s_FileInformation.txt",output_folder_path);

  /** Prépare la ligne du fichier **/
  sprintf(local_buffer,"%s=Type(%02X),AuxType(%04X),VersionCreate(%02X),MinVersion(%02X),Access(%02X),FolderInfo1(%s),FolderInfo2(%s)",output_file_name,
          current_omfproject->type,current_omfproject->aux_type,version_created,min_version,access,folder_info1,folder_info2);

  /** Charge en mémoire le fichier **/
  line_tab = BuildUniqueListFromFile(file_information_path,&nb_line);
  if(line_tab == NULL)
    {
      /* Créer le fichier FileInformation */
      CreateBinaryFile(file_information_path,(unsigned char *)local_buffer,(int)strlen(local_buffer));

      /* Rendre le fichier invisible */
      my_SetFileAttribute(file_information_path,SET_FILE_HIDDEN);
      return;
    }

  /* Rendre le fichier visible */
  my_SetFileAttribute(file_information_path,SET_FILE_VISIBLE);

  /** Création du fichier **/
  fd = fopen(file_information_path,"w");
  if(fd == NULL)
    {
      mem_free_list(nb_line,line_tab);
      return;
    }

  /** Ajouts des lignes existantes **/
  for(i=0; i<nb_line; i++)
    {
      /* Isole le nom du fichier */
      next_sep = strchr(line_tab[i],'=');
      if(next_sep == NULL)
        continue;

      /* Recherche le fichier actuel */
      memcpy(file_name,line_tab[i],next_sep-line_tab[i]);
      file_name[next_sep-line_tab[i]] = '\0';

      /* On ne recopie pas la ligne du fichier */
      if(my_stricmp(file_name,output_file_name))
        fprintf(fd,"%s\n",line_tab[i]);
    }
        
  /* Nouvelle ligne */
  fprintf(fd,"%s\n",local_buffer);

  /* Fermeture */
  fclose(fd);

  /* Libération mémoire */
  mem_free_list(nb_line,line_tab);

  /* Rendre le fichier invisible */
  my_SetFileAttribute(file_information_path,SET_FILE_HIDDEN);
}


/*****************************************************************************/
/*  mem_free_omfproject() :  Libération mémoire de la structure omf_project. */
/*****************************************************************************/
void mem_free_omfproject(struct omf_project *current_omfproject)
{
  int i;
  struct omf_segment *current_omfsegment;
  struct omf_segment *next_omfsegment;

  if(current_omfproject)
    {
      if(current_omfproject->dsk_name_tab)
        {
          for(i=0; i<current_omfproject->nb_file; i++)
            if(current_omfproject->dsk_name_tab[i])
              free(current_omfproject->dsk_name_tab[i]);
          free(current_omfproject->dsk_name_tab);
        }

      if(current_omfproject->org_address_tab)
        free(current_omfproject->org_address_tab);

      if(current_omfproject->file_size_tab)
        free(current_omfproject->file_size_tab);

      for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; )
        {
          next_omfsegment = current_omfsegment->next;
          mem_free_omfsegment(current_omfsegment);
          current_omfsegment = next_omfsegment;
        }

      if(current_omfproject->project_buffer_file)
        free(current_omfproject->project_buffer_file);

      free(current_omfproject);
    }
}


/******************************************************************************/
/*  mem_alloc_omfsegment() :  Allocation mémoire de la structure omf_segment. */
/******************************************************************************/
struct omf_segment *mem_alloc_omfsegment(void)
{
  struct omf_segment *current_omfsegment;
  
  /* Allocation mémoire */
  current_omfsegment = (struct omf_segment *) calloc(1,sizeof(struct omf_segment));
  if(current_omfsegment == NULL)
    return(NULL);
  
  /* Valeurs par defaut */
  current_omfsegment->alignment = ALIGN_NONE;
  current_omfsegment->bank_size = 0x10000;        /* 64 KB */
  current_omfsegment->org = 0;
  current_omfsegment->ds_end = 0;
  current_omfsegment->type_attributes = 0x1000;   /* Static, Code */

  /* Initialisation */
  mem_init_omfsegment(current_omfsegment);
  
  /* Renvoie la structure */
  return(current_omfsegment);
}


/*********************************************************************************/
/*  mem_init_omfsegment() :  Initialisation mémoire de la structure omf_segment. */
/*********************************************************************************/
static void mem_init_omfsegment(struct omf_segment *current_omfsegment)
{
  int i;
  
  for(i=0; i<1024; i++)
    current_omfsegment->alloc_table[i] = NULL;
     
  current_omfsegment->first_file = NULL;
  
  current_omfsegment->nb_opcode = 0;
  current_omfsegment->first_opcode = NULL;
  current_omfsegment->last_opcode = NULL;
  current_omfsegment->tab_opcode = NULL;
  
  current_omfsegment->nb_data = 0;
  current_omfsegment->first_data = NULL;
  current_omfsegment->last_data = NULL;
  current_omfsegment->tab_data = NULL;
  
  current_omfsegment->nb_directive = 0;
  current_omfsegment->first_directive = NULL;
  current_omfsegment->last_directive = NULL;
  current_omfsegment->tab_directive = NULL;
  
  current_omfsegment->nb_direqu = 0;
  current_omfsegment->first_direqu = NULL;
  current_omfsegment->last_direqu = NULL;
  current_omfsegment->tab_direqu = NULL;
  memset(&current_omfsegment->local_item,0,sizeof(struct item));
  current_omfsegment->local_item_ptr = &current_omfsegment->local_item;
  
  current_omfsegment->nb_macro = 0;
  current_omfsegment->first_macro = NULL;
  current_omfsegment->last_macro = NULL;
  current_omfsegment->tab_macro = NULL;
  memset(&current_omfsegment->local_macro,0,sizeof(struct macro));
  current_omfsegment->local_macro_ptr = &current_omfsegment->local_macro;
  
  current_omfsegment->nb_label = 0;
  current_omfsegment->first_label = NULL;
  current_omfsegment->last_label = NULL;
  current_omfsegment->tab_label = NULL;
  memset(&current_omfsegment->local_label,0,sizeof(struct label));
  current_omfsegment->local_label_ptr = &current_omfsegment->local_label;
  
  current_omfsegment->nb_equivalence = 0;
  current_omfsegment->first_equivalence = NULL;
  current_omfsegment->last_equivalence = NULL;
  current_omfsegment->tab_equivalence = NULL;
  memset(&current_omfsegment->local_equivalence,0,sizeof(struct equivalence));
  current_omfsegment->local_equivalence_ptr = &current_omfsegment->local_equivalence;
  
  current_omfsegment->nb_variable = 0;
  current_omfsegment->first_variable = NULL;
  current_omfsegment->last_variable = NULL;
  current_omfsegment->tab_variable = NULL;
  memset(&current_omfsegment->local_variable,0,sizeof(struct variable));
  current_omfsegment->local_variable_ptr = &current_omfsegment->local_variable;

  current_omfsegment->nb_external = 0;
  current_omfsegment->first_external = NULL;
  current_omfsegment->last_external = NULL;
  current_omfsegment->tab_external = NULL;
  memset(&current_omfsegment->local_external,0,sizeof(struct external));
  current_omfsegment->local_external_ptr = &current_omfsegment->local_external;
}


/*****************************************************************************/
/*  mem_free_omfsegment() :  Libération mémoire de la structure omf_segment. */
/*****************************************************************************/
void mem_free_omfsegment(struct omf_segment *current_omfsegment)
{
  int i;
  struct relocate_address *current_address;
  struct relocate_address *next_address;

  if(current_omfsegment)
    {
      /** Header **/
      if(current_omfsegment->master_file_path)
        free(current_omfsegment->master_file_path);

      if(current_omfsegment->load_name)
        free(current_omfsegment->load_name);

      if(current_omfsegment->segment_name)
        free(current_omfsegment->segment_name);

      /** Code Objet **/
      for(current_address=current_omfsegment->first_address; current_address; )
        {
          next_address = current_address->next;
          free(current_address);
          current_address = next_address;
        }
      
      if(current_omfsegment->object_code)
        free(current_omfsegment->object_code);

      if(current_omfsegment->segment_body_file)
        free(current_omfsegment->segment_body_file);

      /** Zones mémoires **/
      my_Memory(MEMORY_FREE_OPCODE,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_DATA,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_DIRECTIVE,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_DIREQU,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_MACRO,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_LABEL,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_EQUIVALENCE,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_VARIABLE,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_EXTERNAL,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_GLOBAL,NULL,NULL,current_omfsegment);
      my_Memory(MEMORY_FREE_FILE,NULL,NULL,current_omfsegment);
        
      /* Libère toutes les allocations temporaires */
      for(i=0; i<1024; i++)
        if(current_omfsegment->alloc_table[i] != NULL)
          {
            free(current_omfsegment->alloc_table[i]);
            current_omfsegment->alloc_table[i] = NULL;
          }      

      /* Libère la structure */
      free(current_omfsegment);
    }
}

/***********************************************************************/
