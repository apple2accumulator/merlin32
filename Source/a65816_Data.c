/***********************************************************************/
/*                                                                     */
/*  a65816_Code.c : Module for generation of Data.                     */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include "Dc_Library.h"

#include "a65816_Data.h"

#include "a65816_Macro.h"
#include "a65816_Line.h"
#include "a65816_File.h"


static void BuildOneDataLineSize(struct source_line *,char *,struct omf_segment *);
static void BuildOneDataLineOperand(struct source_line *,char *,struct omf_segment *);

/**************************************************************/
/*  BuildAllDataLineSize() :  Calculates size for Data lines. */
/**************************************************************/
int BuildAllDataLineSize(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Recover the 1st source file */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Process all lines Data to calculate their size ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* This line is invalid */
      if(current_line->is_valid == 0)
        {
          current_line->nb_byte = 0;
          continue;
        }

      /** Lines of Data **/
      if(current_line->type == LINE_DATA)
        {
          /** Determine the size of Operand **/
          BuildOneDataLineSize(current_line,buffer_error,current_omfsegment);
          if(strlen(buffer_error) > 0)
            {
              sprintf(param->buffer_error,"Impossible to decode Data format for instruction '%s  %s' (line %d, file '%s') : %s",
                      current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
              my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

          /** Allocate memory for the Data **/
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


/*********************************************************************/
/*  BuildAllDataLineSize() :  Calculate the size for the Data lines. */
/*********************************************************************/
int BuildAllDataLine(struct omf_segment *current_omfsegment)
{
  struct source_file *first_file;
  struct source_line *current_line;
  char buffer_error[1024];
  struct parameter *param;
  my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

  /* Recover the 1st source file */
  my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
  if(first_file == NULL)
    return(0);

  /*** Process all lines Data to create the Operand ***/
  for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
      /* This line is invalid */
      if(current_line->is_valid == 0 || current_line->is_dum == 1)
        continue;

      /** Lines of Data **/
      if(current_line->type == LINE_DATA)
        {
          /** Creation of Operand data **/
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
          /** Evaluation of the variable **/
          EvaluateVariableLine(current_line,current_omfsegment);
        }
    }

  /* OK */
  return(0);
}


/*****************************************************************/
/*  BuildOneDataLineSize() :  Determine the size of a DATA line. */
/*****************************************************************/
static void BuildOneDataLineSize(struct source_line *current_line, char *buffer_error_rtn, struct omf_segment *current_omfsegment)
{
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    int nb_element = 0, nb_valid_element = 0, nb_byte = 0, nb_nibble = 0, length = 0, is_reloc = 0;
    char *next_char = NULL;
    char **tab_element = NULL;
    struct external *current_external = NULL;
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    strcpy(buffer_error_rtn,"");

    /*** We will recognize the different types of Data ***/
    if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB") ||
       !my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB") ||
       !my_stricmp(current_line->opcode_txt,"ADR") || !my_stricmp(current_line->opcode_txt,"ADRL"))
    {
        /* Cut into individual elements */
        tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
        if(tab_element == NULL)
        {
            sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
            return;
        }

        /** Analyze the elements **/
        for(int i=0; i<nb_element; i++)
            if(strlen(tab_element[i]) == 0)
            {
                mem_free_table(nb_element,tab_element);
                sprintf(buffer_error_rtn,"Empty Data in Operand '%s'",current_line->operand_txt);
                return;
            }
            else if(!strcmp(tab_element[i],","))
                ;                     /* Nothing to do */
            else
                nb_valid_element++;   /* We count matches only valid elements */

        /** Get the size of the Data Part **/
        if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB"))
            current_line->nb_byte = 2*nb_valid_element;
        else if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
            current_line->nb_byte = nb_valid_element;
        else if(!my_stricmp(current_line->opcode_txt,"ADR"))
            current_line->nb_byte = 3*nb_valid_element;
        else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
            current_line->nb_byte = 4*nb_valid_element;

        /* Memory release */
        mem_free_table(nb_element,tab_element);
    }
    else if(!my_stricmp(current_line->opcode_txt,"HEX"))
    {
        /** Counts the number of characters / validates the data **/
        for(int i=0; i<(int)strlen(current_line->operand_txt); i++)
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

        /** Get the size of the Data Part **/
        current_line->nb_byte = nb_byte;
    }
    else if(!my_stricmp(current_line->opcode_txt,"DS"))
    {
        /* Cut into individual elements */
        tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
        if(tab_element == NULL)
        {
            sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
            return;
        }
        /* DS 2 or DS 2,FF */
        if(nb_element != 1 && nb_element != 3)
        {
            mem_free_table(nb_element,tab_element);
            sprintf(buffer_error_rtn,"Wrong DS Format Data in Operand '%s'",current_line->operand_txt);
            return;
        }

        /** Particular case of \ **/;
        if(!strcmp(tab_element[0],"\\"))
        {
            /** Get the size of the Data Part => it will be necessary to complete until the next page **/
            current_line->nb_byte = 0xFFFF;   /* 4 F */
        }
        else
        {
            /* The first value is evaluated as integer */
            nb_byte = (int) EvalExpressionAsInteger(tab_element[0],buffer_error,current_line,1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
            if(strlen(buffer_error) > 0 && nb_byte == 0xFFFF)
            {
                /** Get the size of the Data Part => the evaluation will have to be repeated later when calculating the addresses **/
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
                /** Get the size of the Data Part **/
                current_line->nb_byte = nb_byte;
            }
        }
    }
    /**************************/
    /** Character strings **/
    /**************************/
    else if(!my_stricmp(current_line->opcode_txt,"ASC") || !my_stricmp(current_line->opcode_txt,"DCI") || !my_stricmp(current_line->opcode_txt,"INV") || !my_stricmp(current_line->opcode_txt,"FLS") ||
            !my_stricmp(current_line->opcode_txt,"STR") || !my_stricmp(current_line->opcode_txt,"STRL"))
    {
        /** Count matches the characters in the "" and Hex outside **/
        for(int i=0; i<(int)strlen(current_line->operand_txt); i++)
        {
            /* Start of zone */
            if(current_line->operand_txt[i] == '\'' || current_line->operand_txt[i] == '"')
            {
                /* Hex precedes w/o completion */
                if(nb_nibble == 1)
                {
                    sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
                    return;
                }

                /* End of zone */
                next_char = strchr(&current_line->operand_txt[i+1],current_line->operand_txt[i]);
                if(next_char == NULL)
                {
                    sprintf(buffer_error_rtn,"Wrong String Format Data in Operand '%s' : End-of-String character is missing",current_line->operand_txt);
                    return;
                }

                /* Size of the zone */
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

        /** Get the size of the Data Part **/
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
        /** No Hex in the chain **/
        /* Start of zone */
        if(current_line->operand_txt[0] != '\'' && current_line->operand_txt[0] != '"')
        {
            sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
            return;
        }
        /* End of zone */
        if(current_line->operand_txt[strlen(current_line->operand_txt)-1] != current_line->operand_txt[0])
        {
            sprintf(buffer_error_rtn,"Wrong Data String format in Operand '%s'",current_line->operand_txt);
            return;
        }

        /** Get the size of the Data Part **/
        current_line->nb_byte = (int) (strlen(current_line->operand_txt) - 2);
    }
    else if(!my_stricmp(current_line->opcode_txt,"CHK"))
    {
        /* 1 Byte of Checksum */
        current_line->nb_byte = 1;
    }
    else
    {
        /* Data Unknown */
        sprintf(param->buffer_error,"Impossible to decode Data mode for instruction '%s  %s' (line %d, file '%s')",
                current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
}


/***************************************************************************************/
/*  BuildOneDataLineOperand() :  Byte Creation of the Operand for the Lines Data Part. */
/***************************************************************************************/
static void BuildOneDataLineOperand(struct source_line *current_line, char *buffer_error_rtn, struct omf_segment *current_omfsegment)
{
    BYTE one_byte = 0, byte_count = 0, bit_shift = 0;
    WORD one_word = 0, offset_patch = 0, offset_reference = 0;
    DWORD one_dword = 0, address_long = 0;
    int nb_element = 0, nb_byte = 0, nb_nibble = 0, length = 0, value = 0, is_reloc = 0, operand_size = 0, line_address = 0;
    struct relocate_address *current_address_1 = NULL;
    struct relocate_address *current_address_2 = NULL;
    struct external *current_external = NULL;
    char *next_char = NULL;
    char **tab_element = NULL;
    unsigned char data[5];
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    strcpy(buffer_error_rtn,"");

    /*** We will recognize the different types of Data ***/
    if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB") ||
       !my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB") ||
       !my_stricmp(current_line->opcode_txt,"ADR") || !my_stricmp(current_line->opcode_txt,"ADRL"))
    {
        /** Size of the Operand **/
        if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
            operand_size = 1;
        else if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW") || !my_stricmp(current_line->opcode_txt,"DDB"))
            operand_size = 2;
        else if(!my_stricmp(current_line->opcode_txt,"ADR"))
            operand_size = 3;
        else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
            operand_size = 4;

        /* Cut into individual elements */
        tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
        if(tab_element == NULL)
        {
            sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
            return;
        }

        /** Convert the values **/
        for(int i=0, nb_valid_element=0; i<nb_element; i++)
        {
            if(!strcmp(tab_element[i],",") || strlen(tab_element[i]) == 0)
                continue;
            else
            {
                /** Conversion to number **/
                one_dword = (DWORD) EvalExpressionAsInteger(tab_element[i],buffer_error,current_line,operand_size,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
                if(strlen(buffer_error) > 0)
                {
                    sprintf(buffer_error_rtn,"Impossible to evaluate %s Data '%s' (%s)",current_line->opcode_txt,tab_element[i],buffer_error);
                    mem_free_table(nb_element,tab_element);
                    return;
                }
                
                /* Address of the line takes into account [ORG $ Addr ORG] */
                if(current_line->is_fix_address == 1 && current_line->address != current_line->global_address)
                    line_address = current_line->global_address;
                else
                    line_address = current_line->address;
                
                /** Cutting in Byte (copy respecting the Byte Order) **/
                bo_memcpy(&data[0],&one_dword,sizeof(DWORD));

                /** Storage in the operand **/
                if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW"))
                {
                    current_line->data[2*nb_valid_element+0] = data[0];
                    current_line->data[2*nb_valid_element+1] = data[1];

                    /* Relocatable Address */
                    offset_patch = (WORD)(line_address + 2*nb_valid_element + 0);     /* 2 Bytes */
                }
                else if(!my_stricmp(current_line->opcode_txt,"DDB"))
                {
                    current_line->data[2*nb_valid_element+0] = data[1];
                    current_line->data[2*nb_valid_element+1] = data[0];

                    /* Relocatable Address */
                    offset_patch = (WORD)(line_address + 2*nb_valid_element + 0);    /* 2 Bytes */
                }
                else if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
                {
                    current_line->data[nb_valid_element] = data[0];

                    /* Relocatable Address */
                    offset_patch = (WORD)(line_address + nb_valid_element + 0);    /* 1 Byte */
                }
                else if(!my_stricmp(current_line->opcode_txt,"ADR"))
                {
                    current_line->data[3*nb_valid_element+0] = data[0];
                    current_line->data[3*nb_valid_element+1] = data[1];
                    current_line->data[3*nb_valid_element+2] = data[2];

                    /* Relocatable Address */
                    offset_patch = (WORD)(line_address + 3*nb_valid_element + 0);   /* 3 Bytes */
                }
                else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
                {
                    current_line->data[4*nb_valid_element+0] = data[0];
                    current_line->data[4*nb_valid_element+1] = data[1];
                    current_line->data[4*nb_valid_element+2] = data[2];
                    current_line->data[4*nb_valid_element+3] = data[3];

                    /* Relocatable Address */
                    offset_patch = (WORD)(line_address + 4*nb_valid_element + 0);   /* 4 Bytes */
                }

                /** Relocatable Address (internal or external Segment) **/
                if(is_reloc)
                {
                    /* Particular case of DDB which relocates 2 addresses on 1 byte, rather than an address on 2 bytes! */
                    if(!my_stricmp(current_line->opcode_txt,"DDB"))
                    {
                        /* Address 1 : >> 8 */
                        current_address_1 = BuildRelocateAddress(1,0xF8,offset_patch,offset_reference,current_external,current_omfsegment);
                        current_address_1->object_line = &current_line->data[2*nb_valid_element+0];

                        /* Address 2 */
                        current_address_2 = BuildRelocateAddress(1,0,offset_patch+1,offset_reference,current_external,current_omfsegment);
                        current_address_2->object_line = &current_line->data[2*nb_valid_element+1];

                        /* Information for the File Output.txt (only one relocation is indicated) */
                        sprintf(current_line->reloc,"%c 1 >> 8 ",(current_external == NULL)?' ':'E');
                    }
                    else
                    {
                        /* Only 1 address to relocate */
                        current_address_1 = BuildRelocateAddress(byte_count,bit_shift,offset_patch,offset_reference,current_external,current_omfsegment);

                        /* Address patch in the line */
                        if(!my_stricmp(current_line->opcode_txt,"DFB") || !my_stricmp(current_line->opcode_txt,"DB"))
                            current_address_1->object_line = &current_line->data[1*nb_valid_element+0];                    /* 1 byte */
                        else if(!my_stricmp(current_line->opcode_txt,"DA") || !my_stricmp(current_line->opcode_txt,"DW"))
                            current_address_1->object_line = &current_line->data[2*nb_valid_element+0];                    /* 2 bytes */
                        else if(!my_stricmp(current_line->opcode_txt,"ADR"))
                            current_address_1->object_line = &current_line->data[3*nb_valid_element+0];                    /* 3 bytes */
                        else if(!my_stricmp(current_line->opcode_txt,"ADRL"))
                            current_address_1->object_line = &current_line->data[4*nb_valid_element+0];                    /* 4 bytes */

                        /* Text Version for the Output.txt */
                        sprintf(current_line->reloc,"%c %d",(current_external == NULL)?' ':'E',byte_count);
                        if(bit_shift == 0xF0)
                            strcat(current_line->reloc," >>16 ");
                        else if(bit_shift == 0xF8)
                            strcat(current_line->reloc," >> 8 ");
                        else
                            strcat(current_line->reloc,"      ");
                    }
                }

                /* Next element */
                nb_valid_element++;
            }
        }
        
        /* Memory release */
        mem_free_table(nb_element,tab_element);
    }
    else if(!my_stricmp(current_line->opcode_txt,"HEX"))
    {
        /** Conversion of Hexadecimal **/
        for(int i=0; i<(int)strlen(current_line->operand_txt); i++)
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
        /* Cut into individual elements */
        tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_DATA_VALUES,current_line);
        if(tab_element == NULL)
        {
            sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",current_line->operand_txt);
            return;
        }

        /** Check if the Fill out structure is done with a particular value **/
        if(nb_element == 3)
        {
            /* Evaluate the second value as integer */
            one_dword = (DWORD) EvalExpressionAsInteger(tab_element[2],buffer_error,current_line,1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
            if(strlen(buffer_error) > 0)
            {
                sprintf(buffer_error_rtn,"Impossible to evaluate DS Data '%s' in Operand '%s' (%s)",tab_element[2],current_line->operand_txt,buffer_error);
                mem_free_table(nb_element,tab_element);
                return;
            }

            /** Cutting in Byte (copy respecting the Byte Order) **/
            bo_memcpy(&data[0],&one_dword,sizeof(DWORD));

            /** We fill the data **/
            for(int i=0; i<current_line->nb_byte; i++)
                current_line->data[i] = data[0];
        }
    }
    /**************************/
    /** Character strings **/
    /**************************/
    else if(!my_stricmp(current_line->opcode_txt,"ASC") || !my_stricmp(current_line->opcode_txt,"DCI") || !my_stricmp(current_line->opcode_txt,"INV") || !my_stricmp(current_line->opcode_txt,"FLS") ||
            !my_stricmp(current_line->opcode_txt,"STR") || !my_stricmp(current_line->opcode_txt,"STRL"))
    {
        /** Extraction of the string indicating where are the Hex parts **/
        for(int i=0; i<(int)strlen(current_line->operand_txt); i++)
        {
            /* Start of zone */
            if(current_line->operand_txt[i] == '\'' || current_line->operand_txt[i] == '"')
            {
                /* End of zone */
                next_char = strchr(&current_line->operand_txt[i+1],current_line->operand_txt[i]);

                /* Size of the zone */
                length = (int) ((next_char - &current_line->operand_txt[i]) - 1);
                for(int j=0; j<length; j++)
                {
                    /* Is it a valid character of the directive? */
                    if(!my_stricmp(current_line->opcode_txt,"INV") || !my_stricmp(current_line->opcode_txt,"FLS"))
                        next_char = (char *) strchr(INVFLS_TABLE,current_line->operand_txt[i+1+j]);
                    else
                        next_char = (char *) strchr(ASCII_TABLE,current_line->operand_txt[i+1+j]);
                    if(next_char == NULL)
                    {
                        sprintf(buffer_error_rtn,"The character '%c' is not valid with Directive %s",current_line->operand_txt[i+1+j],current_line->opcode_txt);
                        return;
                    }

                    /* Adjust bit 7 */
                    if(current_line->operand_txt[i] == '"')
                        param->buffer_string[nb_byte+j] = (0x80 | current_line->operand_txt[i+1+j]);
                    else
                        param->buffer_string[nb_byte+j] = (0x7F & current_line->operand_txt[i+1+j]);
                }

                /* Adjust the length */
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
                    /* Recovery from Byte */
                    data[1] = current_line->operand_txt[i];
                    data[2] = '\0';
                    sscanf((char *)data,"%X",&value);

                    /* Keep the character */
                    param->buffer_string[nb_byte] = (unsigned char) value;
                    nb_byte++;
                    nb_nibble = 0;
                }
                else
                    data[0] = current_line->operand_txt[i];
            }
        }

        /** Storage of the string according to Opcode **/
        if(!my_stricmp(current_line->opcode_txt,"ASC"))
        {
            /* Keep the string as it */
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
            /* We will go into coding 0x00-0x3F */
            for(int i=0; i<nb_byte; i++)
                current_line->data[i] = (param->buffer_string[i] & 0x3F);
        }
        else if(!my_stricmp(current_line->opcode_txt,"FLS"))
        {
            /* We will go into coding 0x40-0x7F */
            for(int i=0; i<nb_byte; i++)
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
        /** No Hex in the string **/
        for(int i=0; i<current_line->nb_byte; i++)
        {
            if(current_line->operand_txt[0] == '"')
                current_line->data[i] = (0x80 | current_line->operand_txt[current_line->nb_byte-i]);
            else
                current_line->data[i] = (0x7F & current_line->operand_txt[current_line->nb_byte-i]);
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CHK"))
    {
        /* 1 Byte of Checksum : We put a 0x00 for the moment */
        current_line->data[0] = 0x00;
    }
}

/***********************************************************************/
