/***********************************************************************/
/*                                                                     */
/*  a65816_Cond.c : Conditional Management Module.                     */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
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
/* ProcessConditionalDirective() : Processes sources lines with conditions. */
/****************************************************************************/
int ProcessConditionalDirective(struct omf_segment *current_omfsegment)
{
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    int64_t value_64 = 0;
    int value = 0, level = 0, is_reloc = 0;
    struct source_file *first_file = NULL;
    struct source_line *begin_line = NULL;
    struct source_line *end_line = NULL;
    struct source_line *current_line = NULL;
    struct source_line *next_line = NULL;
    struct external *current_external = NULL;
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL,NULL);

    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);

    /*** Review all rows to calculate IF level / ELSE / END DO / ELSE / FIN ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We will not ignore the invalid lines and the Macros defined in the Source file */
        if(current_line->is_valid == 0 || current_line->is_inside_macro == 1)
            continue;

        /** We will have to manage the variables outside of Wolf to calculate their value **/
        if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"DO") || !my_stricmp(current_line->opcode_txt,"IF")))
        {
            /* Increases the level */
            current_line->cond_level = level;
            level++;
        }
        else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"ELSE"))
        {
            /* Error */
            if(level == 0)
            {
                sprintf(param->buffer_error,"Error : Conditional ELSE without IF or DO before (line %d from file '%s')",current_line->file_line_number,current_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* We are in a condition */
            current_line->cond_level = level-1;
        }
        else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"FIN"))
        {
            /* Error */
            if(level == 0)
                printf("        Warning : Conditional FIN without IF or DO before (line %d from file '%s')\n",current_line->file_line_number,current_line->file->file_name);
            else
            {
                /* Lower the level */
                level--;
                current_line->cond_level = level;
            }
        }
        else
            current_line->cond_level = level;
    }

    /* Level must be zero here (ie, found matching FIN statement) */
    if(level != 0)
    {
        sprintf(param->buffer_error,"Error : Missing %d FIN conditional in source code",level);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

    /** We will evaluate the conditions **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We will not ignore the invalid lines */
        if(current_line->is_valid == 0 || current_line->is_inside_macro == 1)
            continue;

        /*** An IF / DO ELSE FIN block is processed to determine the valid part ***/
        if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"DO") || !my_stricmp(current_line->opcode_txt,"IF")))
        {
            /* Evaluation of DO */
            if(!my_stricmp(current_line->opcode_txt,"DO"))
            {
                /* We evaluate the condition to 0 or 1 */
                value_64 = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
                if(strlen(buffer_error) > 0)
                {
                    sprintf(param->buffer_error,"Impossible to evaluate DO conditional part '%s' (line %d from file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

                /* Conversion to 0 / 1 */
                value = (value_64 == 0) ? 0 : 1;
            }
            else if(!my_stricmp(current_line->opcode_txt,"IF"))
            {
                /* Evaluate MX */
                if(!my_strnicmp(current_line->operand_txt,"MX",2))
                {
                    /* Evaluate the expression */
                    value_64 = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
                    if(strlen(buffer_error) > 0)
                    {
                        sprintf(param->buffer_error,"Impossible to evaluate IF conditional part '%s' (line %d from file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                        my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }
                    /* Conversion to 0 / 1 */
                    value = (value_64 == 0) ? 0 : 1;
                }
                else  /* First character */
                {
                    /* Evaluate the expression */
                    value = 0;
                    if(strlen(current_line->operand_txt) >= 3)
                        if(current_line->operand_txt[0] == current_line->operand_txt[2])
                            value = 1;
                }
            }

            /** We will mark the following lines **/
            current_line->is_valid = 1;                  /* The IF / DO line is valid */
            for(next_line=current_line->next; next_line; next_line=next_line->next)
            {
                if(next_line->type == LINE_DIRECTIVE && !my_stricmp(next_line->opcode_txt,"ELSE") && next_line->cond_level == current_line->cond_level)
                {
                    /* Inverse */
                    value = (value == 0) ? 1 : 0;
                    next_line->is_valid = 1;             /* The ELSE line of the IF / DO is valid */
                }
                else if(next_line->type == LINE_DIRECTIVE && !my_stricmp(next_line->opcode_txt,"FIN") && next_line->cond_level == current_line->cond_level)
                {
                    /* End of block */
                    next_line->is_valid = 1;             /* The FIN line of the IF / DO is valid */
                    break;
                }
                else
                    next_line->is_valid = value;         /* The validity of the lines inside depends on the evaluation of the condition */
            }

            /* We will proceed to the following condition (we will evaluate the FI present in the valid part of this FI) */
        }
        else
            current_line->is_valid = 1;                  /* By default the lines are valid */
    }

    /** We will mark the Lup zones as invalid **/
    for(begin_line=first_file->first_line; begin_line; begin_line=begin_line->next)
    {
        /* Invalid lines are ignored */
        if(begin_line->is_valid == 0 || begin_line->is_inside_macro == 1)
            continue;

        if(begin_line->type == LINE_DIRECTIVE && !my_stricmp(begin_line->opcode_txt,"LUP"))
        {
            /** Search the end of Lup **/
            for(end_line=begin_line->next; end_line; end_line=end_line->next)
                if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"--^"))
                    break;
                else if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"LUP"))
                {
                    /* Error: We start a news while the previous one is not finished */
                    sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

            /* Nothing found ? */
            if(end_line == NULL)
            {
                sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /** Marks Lup lines as invalid (except those in Code Macros) **/
            for(current_line=begin_line; current_line != end_line->next; current_line=current_line->next)
                current_line->is_valid = 0;

            /* We continue at the end of the zone */
            begin_line = end_line;
        }
    }

    /* OK */
    return(0);
}

/***********************************************************************/
