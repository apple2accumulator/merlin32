/***********************************************************************/
/*                                                                     */
/*  a65816_Code.c : Module for object code generation.                 */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
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
#include "a65816_OMF.h"
#include "a65816_Code.h"

static void BuildOneCodeLineOpcode(struct source_line *,struct omf_segment *);
static void BuildOneCodeLineOperand(struct source_line *,int *,struct omf_segment *,struct omf_project *);
static int DecodeAddressMode(struct source_line *,char *,struct omf_segment *);
static int GetOperandNbByte(char *,struct source_line *,int *,char *,struct omf_segment *);
static int GetBitMode(struct source_line *);

/************************************************************************/
/*  BuildAllCodeLineOpcode() :  Calculate Opcode + Size for Code Lines. */
/************************************************************************/
int BuildAllCodeLineSize(struct omf_segment *current_omfsegment)
{
    struct source_file *first_file;
    struct source_line *current_line;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
    
    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);
    
    /*** Process all lines to calculate their size ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* This line is invalid */
        if(current_line->is_valid == 0)
        {
            current_line->nb_byte = 0;
            continue;
        }
        
        /** code lines **/
        if(current_line->type == LINE_CODE)
        {
            /** Determines Opcode Byte, Addressing Mode and Operand Size **/
            BuildOneCodeLineOpcode(current_line,current_omfsegment);
        }
        else if(current_line->type == LINE_DATA)
            ;    /* We do nothing for the moment */
        else
            current_line->nb_byte = 0;
    }
    
    /* OK */
    return(0);
}


/************************************************************************/
/*  BuildAllCodeLineOpcode() :  Calculate Opcode + Size for Code Lines. */
/************************************************************************/
int BuildAllCodeLine(int *has_error_rtn, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    int has_output_name;
    struct source_file *first_file;
    struct source_line *current_line;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
    
    /* Init */
    has_output_name = 0;
    
    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);
    
    /*** Process all lines to create the binary code of the Operand ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* This line is invalid */
        if(current_line->is_valid == 0 || current_line->is_dum == 1)
            continue;
        
        /** code lines **/
        if(current_line->type == LINE_CODE)
        {
            /** Create the code object of the Operand **/
            BuildOneCodeLineOperand(current_line,has_error_rtn,current_omfsegment,current_omfproject);
        }
        else if(current_line->type == LINE_VARIABLE)
        {
            /** Evaluation of the variable **/
            EvaluateVariableLine(current_line,current_omfsegment);
        }
        else if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"dsk") || !my_stricmp(current_line->opcode_txt,"lnk") || !my_stricmp(current_line->opcode_txt,"sav")))
        {
            /** Keeps the first name found (if the name of the segment was not provided by the File Link) **/
            if(has_output_name == 0)
            {
                /* File name Object */
                strcpy(current_omfsegment->object_name,current_line->operand_txt);
                
                /* Deletes the final .L */
                if(strlen(current_omfsegment->object_name) > 2)
                    if(!my_stricmp(&current_omfsegment->object_name[strlen(current_omfsegment->object_name)-2],".L"))
                        current_omfsegment->object_name[strlen(current_omfsegment->object_name)-2] = '\0';
                
                /* We only take the 1st */
                has_output_name = 1;
            }
        }
    }
    
    /* OK */
    return(0);
}


/*********************************************************************/
/*  BuildOneCodeLineOpcode() :  Creating the Opcode for a Code Line. */
/*********************************************************************/
static void BuildOneCodeLineOpcode(struct source_line *current_line, struct omf_segment *current_omfsegment)
{
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);
    
    /* Init */
    strcpy(buffer_error,"");
    
    /** ERR does not produce any code but will have to be interpreted **/
    if(!my_stricmp(current_line->opcode_txt,"ERR"))
    {
        current_line->nb_byte = 0;
        current_line->opcode_byte = 0x00;
        current_line->address_mode = AM_IMMEDIATE_16;
        return;
    }
    
    /** Decoding of the addressing mode **/
    int address_mode = DecodeAddressMode(current_line,&buffer_error[0],current_omfsegment);
    if(address_mode == AM_UNKOWN)
    {
        sprintf(param->buffer_error,"Impossible to decode address mode for instruction '%s  %s' (line %d, file '%s')%s%s",
                current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,
                (strlen(buffer_error) > 0) ? " : ":"",buffer_error);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
    
    /** We will process all the Opcodes **/
    if(!my_stricmp(current_line->opcode_txt,"ADC") || !my_stricmp(current_line->opcode_txt,"ADCL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x69;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x69;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x6D;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x6F;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x65;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x72;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x67;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x7D;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x7F;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* $08,Y must also be interpreted as $ 0008, Y or DP, Y Does not exist */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x79;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x75;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x61;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x71;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x77;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x63;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x73;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"AND") || !my_stricmp(current_line->opcode_txt,"ANDL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x29;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x29;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x2D;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x2F;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x25;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x32;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x27;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x3D;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x3F;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* $08,Y must also be interpreted as $ 0008, Y or DP, Y Does not exist */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x39;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x35;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x21;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x31;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x37;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x23;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x33;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"ASL"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x0A;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x0E;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x06;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x1E;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x16;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BCC") || !my_stricmp(current_line->opcode_txt,"BLT"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x90;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BCS") || !my_stricmp(current_line->opcode_txt,"BGE"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB0;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BEQ"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF0;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BIT"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x89;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x89;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x2C;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x24;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x3C;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x34;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BMI"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x30;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BNE"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD0;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BPL"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x10;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BRA"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x80;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BRK"))
    {
        if(address_mode == AM_IMPLICIT)      /* The BRK exists with or without a signature byte */
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x00;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_IMMEDIATE_8 || address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x00;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BRL"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x82;
            current_line->address_mode = AM_PC_RELATIVE_LONG;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BVC"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x50;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"BVS"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x70;
            current_line->address_mode = AM_PC_RELATIVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CLC"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x18;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CLD"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xD8;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CLI"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x58;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CLV"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xB8;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CMP") || !my_stricmp(current_line->opcode_txt,"CMPL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC9;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xC9;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xCD;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0xCF;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC5;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD2;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC7;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xDD;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0xDF;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* $08,Y must also be interpreted as $ 0008, Y or DP, Y Does not exist */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xD9;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD5;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC1;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD1;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD7;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC3;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD3;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"COP"))
    {
        if(address_mode == AM_IMPLICIT)  /* COP exists with or without signature byte */
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x02;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_IMMEDIATE_8 || address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x02;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CPX"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE0;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xE0;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xEC;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE4;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"CPY"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC0;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xC0;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xCC;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC4;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"DEC"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x3A;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xCE;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC6;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xDE;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD6;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"DEX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xCA;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"DEY"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x88;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"EOR") || !my_stricmp(current_line->opcode_txt,"EORL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x49;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x49;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x4D;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x4F;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x45;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x52;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x47;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x5D;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x5F;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* $08,Y must also be interpreted as $ 0008, Y or DP, Y Does not exist */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x59;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x55;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x41;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x51;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x57;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x43;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x53;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"INC"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x1A;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xEE;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE6;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xFE;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF6;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"INX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xE8;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"INY"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xC8;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"JMP") || !my_stricmp(current_line->opcode_txt,"JML") || !my_stricmp(current_line->opcode_txt,"JMPL"))
    {
        if(address_mode == AM_ABSOLUTE || address_mode == AM_DIRECT_PAGE)       /* We pass the $ 00 for a $ 0000 */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x4C;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_INDIRECT || address_mode == AM_DIRECT_PAGE_INDIRECT)       /* We pass the ($ 00) for one ($ 0000) */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x6C;
            current_line->address_mode = AM_ABSOLUTE_INDIRECT;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x7C;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x5C;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDIRECT_LONG)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xDC;
            current_line->address_mode = AM_ABSOLUTE_INDIRECT_LONG;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"JSL"))
    {
        if(address_mode == AM_DIRECT_PAGE || address_mode == AM_ABSOLUTE || address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x22;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"JSR"))
    {
        if(address_mode == AM_ABSOLUTE || address_mode == AM_DIRECT_PAGE)       /* We pass the $ 00 for a $ 0000 */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x20;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X_INDIRECT || address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)       /* We pass the ($ 00, X) for one ($ 0000, X) */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xFC;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X_INDIRECT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"LDA") || !my_stricmp(current_line->opcode_txt,"LDAL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA9;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xA9;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xAD;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0xAF;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA5;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT || address_mode == AM_ABSOLUTE_INDIRECT)   /* the LDA ($ 0000) must be interpreted as an LDA ($ 00) */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB2;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG || address_mode == AM_ABSOLUTE_INDIRECT_LONG)  /* the LDA [$ 0000] must be interpreted as an LDA [$ 00] */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA7;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xBD;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0xBF;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)   /* we had an LDA $00,Y the LDA DP,Y Does not exist, so has to be LDA $0000,Y */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xB9;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB5;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA1;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB1;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB7;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA3;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB3;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"LDX"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA2;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xA2;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE || address_mode == AM_ABSOLUTE_LONG)   /* Long addressing in Abs addressing is cast */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xAE;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA6;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xBE;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB6;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"LDY"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA0;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xA0;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE || address_mode == AM_ABSOLUTE_LONG)   /* Long addressing in Abs addressing is cast */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xAC;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xA4;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xBC;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xB4;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"LSR"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x4A;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x4E;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x46;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x5E;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x56;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"MVN"))
    {
        if(address_mode == AM_BLOCK_MOVE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x54;
            current_line->address_mode = AM_BLOCK_MOVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"MVP"))
    {
        if(address_mode == AM_BLOCK_MOVE)
        { 
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x44;
            current_line->address_mode = AM_BLOCK_MOVE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"NOP"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xEA;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"ORA") || !my_stricmp(current_line->opcode_txt,"ORAL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x09;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x09;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x0D;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x0F;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x05;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x12;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x07;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x1D;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x1F;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* $08,Y must also be interpreted as $ 0008, Y or DP, Y Does not exist */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x19;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x15;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x01;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x11;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x17;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x03;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x13;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PEA"))
    {
        if(address_mode == AM_ABSOLUTE_LONG || address_mode == AM_ABSOLUTE || address_mode == AM_DIRECT_PAGE || address_mode == AM_IMMEDIATE_8 || address_mode == AM_IMMEDIATE_16)    /* One must be able to write PEA $1234 or PEA $0 or PEA #0 or PEA #$1234 */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xF4;
            current_line->address_mode = AM_ABSOLUTE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PEI"))
    {
        if(address_mode == AM_DIRECT_PAGE_INDIRECT || address_mode == AM_DIRECT_PAGE || address_mode == AM_ABSOLUTE)    /* We must be able to write PEI $12 or PEI ($12) or PEI Label */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xD4;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PER"))
    {
        if(address_mode == AM_ABSOLUTE || address_mode == AM_IMMEDIATE_16)    /* PER Label or PER #Label or PER $address => Relative Long as BRL */
        {    
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x62;
            current_line->address_mode = AM_PC_RELATIVE_LONG;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHA"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x48;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHB"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x8B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHD"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x0B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHK"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x4B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHP"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x08;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xDA;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PHY"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x5A;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PLA"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x68;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PLB"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xAB;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PLD"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x2B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PLP"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x28;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PLX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xFA;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"PLY"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x7A;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"REP"))
    {
        if(address_mode == AM_IMMEDIATE_8 || address_mode == AM_IMMEDIATE_16 || address_mode == AM_DIRECT_PAGE)   /* We can come up with REP $30 */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xC2;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"ROL"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x2A;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_ABSOLUTE || address_mode == AM_ABSOLUTE_LONG)   /* We cast the Long in Absolute */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x2E;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x26;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x3E;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x36;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"ROR"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x6A;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_ABSOLUTE || address_mode == AM_ABSOLUTE_LONG)   /* We cast the Long in Absolute */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x6E;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x66;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x7E;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x76;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"RTI"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x40;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"RTL"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x6B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"RTS"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x60;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"SBC") || !my_stricmp(current_line->opcode_txt,"SBCL"))
    {
        if(address_mode == AM_IMMEDIATE_8)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE9;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
        else if(address_mode == AM_IMMEDIATE_16)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xE9;
            current_line->address_mode = AM_IMMEDIATE_16;
        }
        else if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xED;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0xEF;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE5;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF2;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE7;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xFD;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0xFF;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* $08,Y must also be interpreted as $ 0008, Y or DP, Y Does not exist */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0xF9;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF5;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE1;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF1;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF7;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE3;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xF3;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"SEC"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x38;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"SED"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xF8;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"SEI"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x78;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"SEP"))
    {
        if(address_mode == AM_IMMEDIATE_8 || address_mode == AM_IMMEDIATE_16 || address_mode == AM_DIRECT_PAGE)         /* On peut tompber sur of SEP $20 */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0xE2;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"STA") || !my_stricmp(current_line->opcode_txt,"STAL"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x8D;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_ABSOLUTE_LONG)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x8F;
            current_line->address_mode = AM_ABSOLUTE_LONG;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x85;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT || address_mode == AM_ABSOLUTE_INDIRECT)             /* STA ($0000) can also be interpreted as STA ($00) */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x92;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG || address_mode == AM_ABSOLUTE_INDIRECT_LONG)   /* STA [$0000] can also be interpreted as STA [$00] */
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x87;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x9D;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_LONG_INDEXED_X)
        {
            current_line->nb_byte = 4;
            current_line->opcode_byte = 0x9F;
            current_line->address_mode = AM_ABSOLUTE_LONG_INDEXED_X;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDEXED_Y)    /* We pass the $00,Y for a $0000,Y */
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x99;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x95;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x81;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X_INDIRECT;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x91;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_INDEXED_Y;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x97;
            current_line->address_mode = AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y;
        }
        else if(address_mode == AM_STACK_RELATIVE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x83;
            current_line->address_mode = AM_STACK_RELATIVE;
        }
        else if(address_mode == AM_STACK_RELATIVE_INDIRECT_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x93;
            current_line->address_mode = AM_STACK_RELATIVE_INDIRECT_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"STP"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xDB;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"STX"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x8E;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x86;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_Y)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x96;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_Y;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"STY"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x8C;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x84;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x94;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"STZ"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x9C;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x64;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
        else if(address_mode == AM_ABSOLUTE_INDEXED_X)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x9E;
            current_line->address_mode = AM_ABSOLUTE_INDEXED_X;
        }
        else if(address_mode == AM_DIRECT_PAGE_INDEXED_X)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x74;
            current_line->address_mode = AM_DIRECT_PAGE_INDEXED_X;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TAX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xAA;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TAY"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xA8;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TCD"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x5B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TCS"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x1B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TDC"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x7B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TRB"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x1C;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x14;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TSB"))
    {
        if(address_mode == AM_ABSOLUTE)
        {
            current_line->nb_byte = 3;
            current_line->opcode_byte = 0x0C;
            current_line->address_mode = AM_ABSOLUTE;
        }
        else if(address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x04;
            current_line->address_mode = AM_DIRECT_PAGE;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TSC"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x3B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TSX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xBA;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TXA"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x8A;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TXS"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x9A;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TXY"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x9B;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TYA"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x98;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"TYX"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xBB;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"WAI"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xCB;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"WDM"))    /* Consumes 1 or 2 bytes (Signature) */
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0x42;
            current_line->address_mode = AM_IMPLICIT;
        }
        else if(address_mode == AM_IMMEDIATE_8 || address_mode == AM_DIRECT_PAGE)
        {
            current_line->nb_byte = 2;
            current_line->opcode_byte = 0x42;
            current_line->address_mode = AM_IMMEDIATE_8;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"XBA"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xEB;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    else if(!my_stricmp(current_line->opcode_txt,"XCE"))
    {
        if(address_mode == AM_IMPLICIT)
        {
            current_line->nb_byte = 1;
            current_line->opcode_byte = 0xFB;
            current_line->address_mode = AM_IMPLICIT;
        }
    }
    
    /* AM_UNKOWN = problem decoding address mode */
    if(current_line->nb_byte <= 0)
    {
        sprintf(param->buffer_error,"Impossible to decode address mode for instruction '%s  %s' (line %d, file '%s')%s%s",
                current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,
                (strlen(buffer_error) > 0) ? " : ":"",buffer_error);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
}


/**************************************************************************/
/*  BuildOneCodeLineOperand() :  Creation of the Operand for a Code Line. */
/**************************************************************************/
static void BuildOneCodeLineOperand(struct source_line *current_line, int *has_error_rtn, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    int64_t operand_value_64 = 0, operand_value_1 = 0, operand_value_2 = 0;
    int operand_size = 0, operand_value = 0, modif = 0, delta = 0, is_reloc_1 = 0, is_reloc_2 = 0, is_mvp_mvn = 0, is_multi_fixed = 0, line_address = 0;
    DWORD AddressLong_1 = 0, AddressLong_2 = 0;
    WORD OffsetPatch_1 = 0, OffsetPatch_2 = 0, OffsetReference_1 = 0, OffsetReference_2 = 0;
    BYTE ByteCnt_1 = 0, ByteCnt_2 = 0, BitShiftCnt_1 = 0, BitShiftCnt_2 = 0;
    struct relocate_address *current_address_1 = NULL;
    struct relocate_address *current_address_2 = NULL;
    char *next_sep = NULL;
    struct external *current_external_1 = NULL;
    struct external *current_external_2 = NULL;  
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
    
    /* Init */
    strcpy(buffer_error,"");
    is_multi_fixed = current_omfproject->is_multi_fixed;
    
    /** We have nothing to do for lines without Operand (ERR, ...) **/
    if(current_line->nb_byte == 0 || current_line->nb_byte == 1)
        return;
    
    /*** Clean the Operand of () [], X, Y, S ... ***/
    modif = 1;
    strcpy(param->buffer_operand,current_line->operand_txt);
    while(modif)
    {
        /* Init */
        modif = 0;
        
        /* Delete ,X */
        if(strlen(param->buffer_operand) > 2)
            if(param->buffer_operand[strlen(param->buffer_operand)-2] == ',' && toupper(param->buffer_operand[strlen(param->buffer_operand)-1]) == 'X')
            {
                param->buffer_operand[strlen(param->buffer_operand)-2] = '\0';
                modif = 1;
            }
        
        /* Delete ,Y */
        if(strlen(param->buffer_operand) > 2)
            if(param->buffer_operand[strlen(param->buffer_operand)-2] == ',' && toupper(param->buffer_operand[strlen(param->buffer_operand)-1]) == 'Y')
            {
                param->buffer_operand[strlen(param->buffer_operand)-2] = '\0';
                modif = 1;
            }
        
        /* Delete ,S */
        if(strlen(param->buffer_operand) > 2)
            if(param->buffer_operand[strlen(param->buffer_operand)-2] == ',' && toupper(param->buffer_operand[strlen(param->buffer_operand)-1]) == 'S')
            {
                param->buffer_operand[strlen(param->buffer_operand)-2] = '\0';
                modif = 1;
            }
        
        /* Delete () */
        if(strlen(param->buffer_operand) > 2)
            if(param->buffer_operand[0] == '(' && param->buffer_operand[strlen(param->buffer_operand)-1] == ')')
            {
                memmove(&param->buffer_operand[0],&param->buffer_operand[1],strlen(&param->buffer_operand[1])+1);
                param->buffer_operand[strlen(param->buffer_operand)-1] = '\0';
                modif = 1;
            }
        
        /* Delete [] */
        if(strlen(param->buffer_operand) > 2)
            if(param->buffer_operand[0] == '[' && param->buffer_operand[strlen(param->buffer_operand)-1] == ']')
            {
                memmove(&param->buffer_operand[0],&param->buffer_operand[1],strlen(&param->buffer_operand[1])+1);
                param->buffer_operand[strlen(param->buffer_operand)-1] = '\0';
                modif = 1;
            }
    }
    
    /* Size of the Operand */
    operand_size = current_line->nb_byte-1;
    
    /** Calculate the value of the Operand **/
    next_sep = strchr(param->buffer_operand,',');
    if((!my_stricmp(current_line->opcode_txt,"MVN") || !my_stricmp(current_line->opcode_txt,"MVP")) && next_sep != NULL)
    {
        /** NOTE: MVN / MVP have two expressions **/
        *next_sep = '\0';
        
        /* Expression 1 */
        operand_value_1 = EvalExpressionAsInteger(param->buffer_operand,buffer_error,current_line,1,&is_reloc_1,&ByteCnt_1,&BitShiftCnt_1,&OffsetReference_1,&AddressLong_1,&current_external_1,current_omfsegment);
        if(strlen(buffer_error) > 0)
        {
            sprintf(param->buffer_error,"Impossible to evaluate Operand '%s' for instruction '%s  %s' (line %d, file '%s') : %s",
                    param->buffer_operand,current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
            my_RaiseError(ERROR_RAISE,param->buffer_error);      
        }
        
        /* Expression 2 */      
        operand_value_2 = EvalExpressionAsInteger(next_sep+1,buffer_error,current_line,1,&is_reloc_2,&ByteCnt_2,&BitShiftCnt_2,&OffsetReference_2,&AddressLong_2,&current_external_2,current_omfsegment);
        if(strlen(buffer_error) > 0)
        {
            sprintf(param->buffer_error,"Impossible to evaluate Operand '%s' for instruction '%s  %s' (line %d, file '%s') : %s",
                    param->buffer_operand,current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
            my_RaiseError(ERROR_RAISE,param->buffer_error);      
        }
        
        /** We merge the 2 taking only the valid bytes **/
        operand_value_64 = (int64_t) BuildBestMVXWord((DWORD)operand_value_1,(DWORD)operand_value_2);
        
        /* Record that we managed an MVP / MVN */
        is_mvp_mvn = 1;
    }
    else
    {
        /** This Opcode has only one expression for the Operand **/
        operand_value_64 = EvalExpressionAsInteger(param->buffer_operand,buffer_error,current_line,operand_size,&is_reloc_1,&ByteCnt_1,&BitShiftCnt_1,&OffsetReference_1,&AddressLong_1,&current_external_1,current_omfsegment);
        if(strlen(buffer_error) > 0)
        {
            sprintf(param->buffer_error,"Impossible to evaluate Operand '%s' for instruction '%s  %s' (line %d, file '%s') : %s",
                    param->buffer_operand,current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
            my_RaiseError(ERROR_RAISE,param->buffer_error);      
        }
        
        /* No direct page addressing if you have a Bank != 0 */
        if((operand_value_64 & 0xFF0000) != 0 && current_line->no_direct_page == 0)
            current_line->no_direct_page = 1;
    }
    
    /** We will modify the value according to the addressing mode **/
    if(current_line->address_mode == AM_PC_RELATIVE)
    {
        /* Eliminate the upper part of the address 00 Bank */
        operand_value = (int) (0xFFFF & operand_value_64);
        
        /* We look if we jump up or down (+/- 8 bit) */
        if((current_line->address+2) > operand_value)
        {
            delta = (current_line->address+2) - operand_value;
            if(delta > 128)
            {
                /* First Error Encountered */
                if(*has_error_rtn == 0)
                {
                    sprintf(param->buffer_latest_error,"Bad Jump for instruction '%s  %s' (line %d, file '%s') : Too long",
                            current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
                    *has_error_rtn = 1;
                }
                /* Value in Error */
                operand_value = 0x00;
            }
            else
                operand_value = 256 - delta;
        }
        else
        {
            delta =  operand_value - (current_line->address+2);
            if(delta > 127)
            {
                /* First Error Encountered */
                if(*has_error_rtn == 0)
                {              
                    sprintf(param->buffer_latest_error,"Bad Jump for instruction '%s  %s' (line %d, file '%s') : Too long",
                            current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
                    *has_error_rtn = 1;
                }
                /* Value in Error */
                operand_value = 0x00;
            }
            else
                operand_value = delta;
        }
    }
    else if(current_line->address_mode == AM_PC_RELATIVE_LONG)
    {
        /* Eliminate the upper part of the address 00 Bank */
        operand_value = (int) (0xFFFF & operand_value_64);
        
        /* Look if we jump up or down (+/- 16 bit) */
        if((current_line->address+3) > operand_value)
        {
            delta = (current_line->address+3) - operand_value;
            if(delta > 32768)
            {
                /* First Error Encountered */
                if(*has_error_rtn == 0)
                {  
                    sprintf(param->buffer_latest_error,"Bad Jump for instruction '%s  %s' (line %d, file '%s') : Too long",
                            current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
                    *has_error_rtn = 1;
                }
                /* Value in Error */
                operand_value = 0x0000;
            }
            else
                operand_value = 65536 - delta;
        }
        else
        {
            delta =  operand_value - (current_line->address+3);
            if(delta > 32767)
            {
                /* First Error Encountered */
                if(*has_error_rtn == 0)
                {
                    sprintf(param->buffer_latest_error,"Bad Jump for instruction '%s  %s' (line %d, file '%s') : Too long",
                            current_line->opcode_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name);
                    *has_error_rtn = 1;
                }
                /* Value in Error */
                operand_value = 0x0000;
            }
            else
                operand_value = delta;
        }
    }
    else
        operand_value = (int) (0xFFFFFFFF & operand_value_64);    /* Keep 32 bits */
    
    /**  Place the bytes of the Operand  **/

    /* Keep the original value */
    current_line->operand_value = operand_value;
    /* Truncated based on expected byte number */
    current_line->operand_byte[0] = (unsigned char) (operand_value & 0x0000FF);
    if(operand_size > 1)
        current_line->operand_byte[1] = (unsigned char) ((operand_value >> 8) & 0x0000FF);
    if(operand_size > 2)
        current_line->operand_byte[2] = (unsigned char) ((operand_value >> 16) & 0x0000FF);
    
    /* Maintains 24 bit address for resolving DP addresses */
    current_line->operand_address_long = AddressLong_1;
    current_line->external_address = NULL;
    
    /**  Relocatable Address  **/
    if((is_reloc_1 == 1 || is_reloc_2 == 1)  && current_line->address_mode != AM_PC_RELATIVE && current_line->address_mode != AM_PC_RELATIVE_LONG)
    { 
        /* The address of the operand is relocatable */
        current_line->address_is_rel = 1;
        
        /* MV* can have up to 2 relocatable addresses */
        if(is_reloc_1 == 1)
        {
            /* Address of the line takes into account [ORG $ Addr ORG] */
            if(current_line->is_fix_address == 1 && current_line->address != current_line->global_address)
                line_address = current_line->global_address;
            else
                line_address = current_line->address;
            
            /* For MVN / MVP, the first address is +1 or +2 */
            OffsetPatch_1 = (WORD)(line_address + 1 + ((is_mvp_mvn==1) ? 1 : 0));
            current_address_1 = BuildRelocateAddress(ByteCnt_1,BitShiftCnt_1,OffsetPatch_1,OffsetReference_1,current_external_1,current_omfsegment);
            current_address_1->object_line = &current_line->operand_byte[0+((is_mvp_mvn==1)?1:0)];
            
            /* We will keep a pointer to the address (do not switch to direct page addressing for EXT) */
            if(current_external_1 != NULL)
                current_line->external_address = current_address_1;
        }
        if(is_reloc_2 == 1)
        {
            /* Address of the line takes into account [ORG $ Addr ORG] */
            if(current_line->is_fix_address == 1 && current_line->address != current_line->global_address)
                line_address = current_line->global_address;
            else
                line_address = current_line->address;
            
            /* A second address to relocate : MVN or MVP */
            OffsetPatch_2 = (WORD)(line_address + 1);
            current_address_2 = BuildRelocateAddress(ByteCnt_2,BitShiftCnt_2,OffsetPatch_2,OffsetReference_2,current_external_2,current_omfsegment);
            current_address_2->object_line = &current_line->operand_byte[0];
        }
        
        /** We are preparing what will be displayed in the File Output.txt **/
        if(is_reloc_1 == 1 && is_reloc_2 == 0)
        {
            sprintf(current_line->reloc,"%c %d",(current_external_1==NULL)?' ':'E',ByteCnt_1);
            if(BitShiftCnt_1 == 0xF0)
                strcat(current_line->reloc," >>16 ");
            else if(BitShiftCnt_1 == 0xF8)
                strcat(current_line->reloc," >> 8 ");
            else
                strcat(current_line->reloc,"      ");
        }
        else if(is_reloc_1 == 0 && is_reloc_2 == 1)
        {
            sprintf(current_line->reloc,"%c %d",(current_external_2==NULL)?' ':'E',ByteCnt_2);
            if(BitShiftCnt_2 == 0xF0)
                strcat(current_line->reloc," >>16 ");
            else if(BitShiftCnt_2 == 0xF8)
                strcat(current_line->reloc," >> 8 ");
            else
                strcat(current_line->reloc,"      ");
        }
        else  /* Both are relocatable => we take the first */
        {
            sprintf(current_line->reloc,"%c %d",(current_external_1==NULL)?' ':'E',ByteCnt_1);
            if(BitShiftCnt_1 == 0xF0)
                strcat(current_line->reloc," >>16 ");
            else if(BitShiftCnt_1 == 0xF8)
                strcat(current_line->reloc," >> 8 ");
            else
                strcat(current_line->reloc,"      ");
        }
    }
    else
        current_line->address_is_rel = 0;
}


/***********************************************************/
/*  DecodeAddressMode() :  Determines the addressing mode. */
/***********************************************************/
static int DecodeAddressMode(struct source_line *current_line, char *error_buffer_rtn, struct omf_segment *current_omfsegment)
{
    int opLen = 0, nb_byte = 0, nb_byte_left = 0, nb_byte_right = 0, is_address = 0;
    char *next_char = NULL;
    char operandBuf[1024];
    char* operand = &operandBuf[0];

    /* Copy the operand to work on it (on Delete {}) */
    for(int i = 0; i < (int)strlen(current_line->operand_txt); i++)
    {
        if(current_line->operand_txt[i] != '{' && current_line->operand_txt[i] != '}')
            operand[opLen++] = current_line->operand_txt[i];
    }
    operand[opLen] = '\0';

    /****************/
    /**  Implicit  **/
    /****************/
    if(opLen == 0 || !my_stricmp(operand,"A"))
        return(AM_IMPLICIT);
    
    /*****************/
    /**  Indexed X  **/
    /*****************/
    if( opLen > 2 && !my_stricmp(&operand[opLen-2], ",X") )
    {
        /* Delete ,X */
        operand[opLen-2] = '\0';

        /** $@,X $@@,X or $@@@,X **/
        nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
        if(nb_byte == 1 && current_line->no_direct_page == 0)
            return(AM_DIRECT_PAGE_INDEXED_X);
        else if(nb_byte == 2 || (nb_byte == 1 && current_line->no_direct_page == 1))
            return(AM_ABSOLUTE_INDEXED_X);
        else if(nb_byte == 3)
            return(AM_ABSOLUTE_LONG_INDEXED_X);

        /* Unknown */
        return(AM_UNKOWN);
    }
    
    /*****************/
    /**  Indexed Y  **/
    /*****************/
    if( opLen > 2 && !my_stricmp(&operand[opLen-2], ",Y") )
    {
        /* Delete ,Y */
        opLen -= 2;
        operand[opLen] = '\0';

        /** [$@],Y : DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y **/
        if(opLen > 2 && operand[0] == '[' && operand[opLen-1] == ']')
        {
            /* Delete [] */
            opLen -= 2;
            operand += 1;
            operand[opLen] = '\0';

            /* Check if this is a Page Direct */
            nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
            if(nb_byte == 1 || nb_byte == 2 || nb_byte == 3)
                return(AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y);    /* Anyway, there is only this one */

            /* Unknown */
            return(AM_UNKOWN);
        }

        /* NOTE: below, the ,y is no longer present! */

        /** (@,S),Y : STACK_RELATIVE_INDIRECT_INDEXED_Y **/
        if(opLen > 4 && operand[0] == '(' && operand[opLen-3] == ','&& toupper(operand[opLen-2]) == 'S' && operand[opLen-1] == ')')
        {
            /* Delete (,S) */
            opLen -= 4;
            operand += 1;
            operand[opLen] = '\0';

            /* Check if this is valid */
            nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
            if(nb_byte == 1)
                return(AM_STACK_RELATIVE_INDIRECT_INDEXED_Y);

            /* Unknown */
            return(AM_UNKOWN);
        }

        /** ($@),Y : DIRECT_PAGE_INDIRECT_INDEXED_Y **/
        if(opLen > 2 && operand[0] == '(' && operand[opLen-1] == ')')
        {
            /* Delete () */
            opLen -= 2;
            operand += 1;
            operand[opLen] = '\0';

            /* Check if this is a Page Direct (We can have a Label in Page Direct) */
            nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
            if(nb_byte == 1 || nb_byte == 2)
                return(AM_DIRECT_PAGE_INDIRECT_INDEXED_Y);

            /* Unknown */
            return(AM_UNKOWN);
        }

        /** We must choose between $@,Y and $@@,Y **/
        nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
        if(nb_byte == 1 && current_line->no_direct_page == 0)
            return(AM_DIRECT_PAGE_INDEXED_Y);
        else if(nb_byte == 2 || (nb_byte == 1 && current_line->no_direct_page == 1))
            return(AM_ABSOLUTE_INDEXED_Y);

        /* Unknown */
        return(AM_UNKOWN);
    }
    
    /**********************/
    /**  Stack Relative  **/
    /**********************/
    if( opLen > 2 && !my_stricmp(&operand[opLen-2], ",S") )
    {
        /* Delete ,S */
        opLen -= 2;
        operand[opLen] = '\0';

        /* @,S : Check if this is valid */
        nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
        if(nb_byte == 1)
            return(AM_STACK_RELATIVE);

        /* Unknown */
        return(AM_UNKOWN);
    }
    
    /****************/
    /**  Indirect  **/
    /****************/
    if(opLen > 2 && operand[0] == '(' && operand[opLen-1] == ')')
    {
        /* Delete () */
        opLen -= 2;
        operand += 1;
        operand[opLen] = '\0';

        /** X INDEXED **/
        if(opLen > 2 && operand[opLen-2] == ',' && toupper(operand[opLen-1]) == 'X')
        {
            /* Delete ,X */
            opLen -= 2;
            operand[opLen] = '\0';

            /** We must choose between ($@,X) and ($@@,X) **/
            nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
            if(nb_byte == 1 && current_line->no_direct_page == 0)
                return(AM_DIRECT_PAGE_INDEXED_X_INDIRECT);
            else if(nb_byte == 2 || (nb_byte == 1 && current_line->no_direct_page == 1))
                return(AM_ABSOLUTE_INDEXED_X_INDIRECT);

            /* Unknown */
            return(AM_UNKOWN);
        }

        /** We must choose between ($@) and ($@@) **/
        nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
        if(nb_byte == 1 && current_line->no_direct_page == 0)
            return(AM_DIRECT_PAGE_INDIRECT);
        else
            return(AM_ABSOLUTE_INDIRECT);

        /* Unknown */
        return(AM_UNKOWN);
    }
    
    /*********************/
    /**  Indirect Long  **/
    /*********************/
    if(opLen > 2 && operand[0] == '[' && operand[opLen-1] == ']')
    {
        /* Delete () */
        opLen -= 2;
        operand += 1;
        operand[opLen] = '\0';

        /** We must choose between [$@] and [$@@] **/
        nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment);
        if(nb_byte == 1 && current_line->no_direct_page == 0)
            return(AM_DIRECT_PAGE_INDIRECT_LONG);
        else
            return(AM_ABSOLUTE_INDIRECT_LONG);

        /* Unknown */
        return(AM_UNKOWN);
    }
    
    /********************/
    /**   Block Move   **/
    /********************/
    next_char = strchr(operand,',');
    if(next_char != NULL)
    {
        /* Remove the comma */
        *next_char = '\0';
        
        /* Check what is on each side of the , */
        nb_byte_left = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment); 
        nb_byte_right = GetOperandNbByte(next_char+1,current_line,&is_address,error_buffer_rtn,current_omfsegment); 
        
        /* We recognized a MVN|MVP src,dst */
        if((nb_byte_left == 1 || nb_byte_left == 2 || nb_byte_left == 3) && 
           (nb_byte_right == 1 || nb_byte_right == 2 || nb_byte_right == 3))
            return(AM_BLOCK_MOVE);
        
        /* Unknown */
        return(AM_UNKOWN);
    }
    
    /*******************************/
    /**  $Absolute or #Immediate  **/
    /*******************************/
    nb_byte = GetOperandNbByte(operand,current_line,&is_address,error_buffer_rtn,current_omfsegment); 
    if(nb_byte > 0)
    {
        /** $Absolute **/
        if(operand[0] != '#')
        {
            /** We must choose between $@, $@@ and $@@@ **/
            if(nb_byte == 1 && current_line->no_direct_page == 0)
                return(AM_DIRECT_PAGE);
            else if(nb_byte == 2 || (nb_byte == 1 && current_line->no_direct_page == 1))
                return(AM_ABSOLUTE);
            else if(nb_byte == 3)
                return(AM_ABSOLUTE_LONG);
        }
        else  /** #Immediate **/
        {
            /** We must choose between #@ and #@@ **/
            if(nb_byte == 1)
                return(AM_IMMEDIATE_8); 
            else if(nb_byte == 2)
                return(AM_IMMEDIATE_16); 
        }  
    }
    
    /***************/
    /**  Unknown  **/
    /***************/
    return(AM_UNKOWN);
}


/***************************************************************************************************************/
/*  GetOperandNbByte() :  Determine the number of bytes of a Operand and its Type (value or address to patch). */
/***************************************************************************************************************/
static int GetOperandNbByte(char *operand, struct source_line *current_line, int *is_address_rtn, char *buffer_error_rtn, struct omf_segment *current_omfsegment)
{
    int nb_element = 0, nb_max_byte = 0, nb_byte = 0, bit_mode = 0, value_format = 0, has_extra_hash = 0, has_long_addr = 0, is_block_copy = 0;
    int is_address = 1;
    char **tab_element = NULL;
    strcpy(buffer_error_rtn,"");
    
    /* Empty Chain: No operand */
    if(strlen(operand) == 0)
        return(0);
    
    /* Long Addressing? (LDAL, STAL...) */
    if(!my_stricmp(current_line->opcode_txt,"ADCL") || !my_stricmp(current_line->opcode_txt,"ANDL") || !my_stricmp(current_line->opcode_txt,"CMPL") ||
       !my_stricmp(current_line->opcode_txt,"EORL") || !my_stricmp(current_line->opcode_txt,"JML")  || !my_stricmp(current_line->opcode_txt,"JMPL") || 
       !my_stricmp(current_line->opcode_txt,"LDAL") || !my_stricmp(current_line->opcode_txt,"ORAL") || !my_stricmp(current_line->opcode_txt,"SBCL") || 
       !my_stricmp(current_line->opcode_txt,"STAL"))
        has_long_addr = 1;
    
    /* Copy of Block? (MVN, MVP) */
    if(!my_stricmp(current_line->opcode_txt,"MVN") || !my_stricmp(current_line->opcode_txt,"MVP"))
        is_block_copy = 1;
    
    /** Process the # < > ^ | **/
    int has_hash = (operand[0] == '#') ? 1 : 0;
    int has_less = (operand[has_hash] == '<') ? 1 : 0;
    int has_more = (operand[has_hash] == '>') ? 1 : 0;
    int has_exp = (operand[has_hash] == '^') ? 1 : 0;
    int has_pipe = (operand[has_hash] == '|' || operand[has_hash] == '!') ? 1 : 0;
    
    /** Cut the string of characters into several elements (skips the #> <^ | from the beginning) **/
    tab_element = DecodeOperandeAsElementTable(&operand[has_hash+has_less+has_more+has_exp+has_pipe],&nb_element,SEPARATOR_EVALUATE_EXPRESSION,current_line);
    if(tab_element == NULL)
    {
        sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",operand);
        return(0);
    }
    
    /** We will manage the - at the beginning => It does not change anything for the number of byte of the expression, we delete it **/
    if(!strcmp(tab_element[0],"-") && nb_element > 1)
    {
        /* We can remove the - */
        free(tab_element[0]);
        for(int i=1; i<nb_element; i++)
            tab_element[i-1] = tab_element[i];
        nb_element--;
    } 
    
    /** We must now only have: value [operator value [operator value] ...] **/
    if(nb_element % 2 == 0)
    {
        mem_free_table(nb_element,tab_element);
        sprintf(buffer_error_rtn,"The number of element in '%s' is even (should be value [operator value [operator value]...])",operand);
        return(0);
    }
    
    /*** Analyze the elements for table ***/
    for(int i=0; i<nb_element; i++)
    {
        /** Value **/
        if(i%2 == 0)
        {
            /* On Delete # which can be present at the beginning of the line */
            has_extra_hash = 0;
            while(tab_element[i][has_extra_hash] == '#')
                has_extra_hash++;
            
            /** Decimal **/
            if(IsDecimal(&tab_element[i][has_extra_hash],&nb_byte))
            {
                value_format = FORMAT_DECIMAL;
            }
            /** $HEX **/
            else if(IsHexaDecimal(&tab_element[i][has_extra_hash],&nb_byte))
            {
                value_format = FORMAT_HEXA;
            }
            /** %Binary **/
            else if(IsBinary(&tab_element[i][has_extra_hash],&nb_byte))
            {
                value_format = FORMAT_BINARY;
            }
            /** "Ascii **/
            else if(IsAscii(&tab_element[i][has_extra_hash],&nb_byte))
            {
                value_format = FORMAT_ASCII;
            }
            /**]Variable **/
            else if(IsVariable(&tab_element[i][has_extra_hash],&nb_byte,current_omfsegment))
            {
                value_format = FORMAT_VARIABLE;
            }
            /** * or Label **/
            else if(IsLabel(&tab_element[i][has_extra_hash],&nb_byte,current_omfsegment))
            {
                value_format = FORMAT_LABEL;
            }
            /** External **/
            else if(IsExternal(&tab_element[i][has_extra_hash],&nb_byte,current_omfsegment))
            {
                value_format = FORMAT_EXTERNAL;
            }
            else
            {
                /* Error : Impossible to determine the format */
                sprintf(buffer_error_rtn,"Can't find format for element '%s' in Operand '%s'",tab_element[i],operand);
                mem_free_table(nb_element,tab_element);
                return(0);
            }
            
            /** Max Bytes to Update for Expression **/
            nb_max_byte = (nb_max_byte < nb_byte) ? nb_byte : nb_max_byte;
        }
        /** Operator **/
        else
        {
            /* We'll just be satisfied with recognizing the Operator without trying to calculate anything */
            if(strlen(tab_element[i]) == 1 && IsSeparator(tab_element[i][0],SEPARATOR_EVALUATE_EXPRESSION) == 1)
                continue;
            
            /* Error : Impossible to recognize the Operator! */
            sprintf(buffer_error_rtn,"Can't recognize Operator '%s' in Operand '%s'",tab_element[i],operand);
            mem_free_table(nb_element,tab_element);
            return(0);
        }
    }
    
    /* Memory release */
    mem_free_table(nb_element,tab_element);
    
    /* Is the Operand an address? (#anything is a value, everything else is an address) */
    if(has_hash == 1)
        is_address = 0;
    
    /** 0 byte => Error **/
    if(nb_max_byte == 0)
    {
        sprintf(buffer_error_rtn,"Can't get length (in byte) for Operand '%s'",operand);
        *is_address_rtn = is_address;
        return(0);
    }
    
    /** We will adjust the size of the DATA according to MX **/
    if(is_address == 0)
    {
        bit_mode = GetBitMode(current_line);
        if(bit_mode == 8)
            nb_max_byte = 1;
        else if(bit_mode == 16)
            nb_max_byte = 2;
    }
    else
    {
        /* We adjust the address according to> and | */
        if(has_more == 1)
        {
            current_line->no_direct_page = 1;
            nb_max_byte = 3;
        }
        if(has_pipe == 1)
        {
            current_line->no_direct_page = 1;
            nb_max_byte = 2;
        }
        
        /* Long Addressing (LDAL, STAL...) */
        if(has_long_addr == 1)
        {
            current_line->no_direct_page = 1;
            nb_max_byte = 3;
        }
        if(is_block_copy == 0 && has_long_addr == 0 && has_more == 0 && nb_max_byte == 3)  /* Long Addressing is valid only if you have set Gold (or for MVP / MVN) */
            nb_max_byte = 2;                                 
        
        /* We asked for a compaction => We will put Direct Page instead of Absolute */
        if(nb_max_byte == 2 && current_line->use_direct_page == 1)
            nb_max_byte = 1;
    }
    
    /* Returns the number of bytes of the Operand (1 2 or 3) */
    *is_address_rtn = is_address;
    return(nb_max_byte);
}


/*******************************************************/
/*  GetBitMode():  Determine if we are in 8 or 16 bit. */
/*******************************************************/
static int GetBitMode(struct source_line *current_line)
{
    /** Register X or Y **/
    if(toupper(current_line->opcode_txt[strlen(current_line->opcode_txt)-1]) == 'X' ||
       toupper(current_line->opcode_txt[strlen(current_line->opcode_txt)-1]) == 'Y')
    {
        /* Emulation or Native */
        if(current_line->x[0] == '1')
            return(8);
        else if(current_line->x[0] == '0')
            return(16);
        else
            return(0);
    }
    else   /** Accumulator **/
    {
        /* Emulation or Native */
        if(current_line->m[0] == '1')
            return(8);
        else if(current_line->m[0] == '0')
            return(16);
        else
            return(0);
    }
}


/*******************************************************************************************/
/*  CompactDirectPageCode() : Pass all lines in to review to detect direct page addresses. */
/*******************************************************************************************/
int CompactDirectPageCode(struct omf_segment *current_omfsegment)
{
    int has_compact;
    struct source_file *first_file;
    struct source_line *current_line;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
    
    /* Init */
    has_compact = 0;
    
    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);
    
    /*** Process all lines to create the binary code of the Operand ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* This line is invalid */
        if(current_line->is_valid == 0 || current_line->is_dum == 1 || current_line->type != LINE_CODE)
            continue;
        if(current_line->no_direct_page == 1)
            continue;
        if(!IsPageDirectOpcode(current_line->opcode_txt))
            continue;
        if(current_line->address_is_rel == 1)    /* The address is relocatable (OMF) */
            continue;
        if(current_line->nb_byte != 3)   /* Operand Size = nb_byte-1 */
            continue;
        if(current_line->address_mode <= AM_IMPLICIT || current_line->address_mode >= AM_ABSOLUTE_LONG)
            continue;
        if(current_line->external_address != NULL)    /* This address is located in another Segment */
            continue;
        
        /** We will only consider fixed addresses $0000 / $00FF (or those in an OMF but coming via a DUM) **/
        if(current_line->operand_byte[1] == 0x00 && current_line->operand_byte[2] == 0x00 && current_line->use_direct_page == 0)
        {
            current_line->use_direct_page = 1;
            has_compact = 1;
        }
    }
    
    
    /* OK */
    return(has_compact);
}

/***********************************************************************/
