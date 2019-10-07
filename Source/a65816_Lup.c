/***********************************************************************/
/*                                                                     */
/*  a65816_Lup.c : Module for managing Lup.                            */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
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
/*  ReplaceLupWithCode() :  Replaces Lups with their code. */
/************************************************************/
int ReplaceLupWithCode(struct omf_segment *current_omfsegment)
{
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    int is_reloc = 0;
    int64_t value = 0;
    struct source_file *first_file = NULL;
    struct source_line *begin_line = NULL;
    struct source_line *end_line = NULL;
    struct source_line *first_lup_line = NULL;
    struct source_line *last_lup_line = NULL;
    struct variable *current_variable = NULL;
    struct external *current_external = NULL;
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);

    /*** Process all lines ***/
    for(begin_line=first_file->first_line; begin_line; begin_line=begin_line->next)
    {
        /* Invalid lines are ignored */
        if(begin_line->is_valid == 0)
            continue;

        /** We will have to manage the variables outside of Wolf to calculate their value **/
        if(begin_line->type == LINE_VARIABLE)
        {
            /* Retrieve the variable */
            my_Memory(MEMORY_SEARCH_VARIABLE,begin_line->label_txt,&current_variable,current_omfsegment);
            if(current_variable == NULL)
            {
                /* Error : We start a news while the previous one is not finished */
                sprintf(param->buffer_error,"Impossible to locate Variable '%s' (line %d from file '%s')",begin_line->label_txt,begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Calculate its new value */
            value = EvalExpressionAsInteger(begin_line->operand_txt,buffer_error,begin_line,begin_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
            if(strlen(buffer_error) > 0)
            {
                /* Error : We start a news while the previous one is not finished */
                sprintf(param->buffer_error,"Impossible to evaluate Variable '%s' value '%s' (line %d from file '%s')",begin_line->label_txt,begin_line->operand_txt,begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Stores the value */
            current_variable->value = value;
        }

        /** Searches for Lup's calls (in the code, not in the body of the Macros in the code) **/
        if(begin_line->type == LINE_DIRECTIVE && begin_line->is_inside_macro == 0 && !my_stricmp(begin_line->opcode_txt,"LUP"))
        {
            /** Search the end of Lup **/
            for(end_line=begin_line->next; end_line; end_line=end_line->next)
            {
                /* Invalid lines are ignored */
                if(end_line->is_valid == 0)
                    continue;

                if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"--^"))
                    break;
                else if(end_line->type == LINE_DIRECTIVE && !my_stricmp(end_line->opcode_txt,"LUP"))
                {
                    /* Error : We start a news while the previous one is not finished */
                    sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
            }

            /* Nothing found ? */
            if(end_line == NULL)
            {
                sprintf(param->buffer_error,"Impossible to locate end of Lup '--^', line %d from file '%s'",begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /** Construct the lines of codes of this Lup (we replace the labels by new ones) **/
            first_lup_line = BuildLupLine(begin_line,end_line,current_omfsegment);
            if(first_lup_line == NULL)
            {
                /* Nothing to insert: Next line */
                begin_line = end_line;
            }
            else
            {
                /* Last line of the Lup */
                for(last_lup_line = first_lup_line; last_lup_line->next != NULL; last_lup_line=last_lup_line->next)
                    ;

                /** Insert the lines of code behind the end of the Lup **/
                last_lup_line->next = end_line->next;
                end_line->next = first_lup_line;

                /* Next line */
                begin_line = last_lup_line;
            }
        }
    }

    /* OK */
    return(0);
}


/******************************************************/
/*  BuildLupLine() :  Construction of Lup Code Lines. */
/******************************************************/
static struct source_line *BuildLupLine(struct source_line *begin_line, struct source_line *end_line, struct omf_segment *current_omfsegment)
{
    int64_t nb_iter_64 = 0;
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    char *new_label = NULL;
    int i = 0, nb_iter = 0, is_reloc = 0;
    struct source_line *first_line = NULL;
    struct source_line *last_line = NULL;
    struct source_line *first_new_line = NULL;
    struct source_line *last_new_line = NULL;
    struct source_line *label_line = NULL;
    struct source_line *current_line = NULL;
    struct external *current_external = NULL;
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /** Decoding the number of loops **/
    if(strlen(begin_line->operand_txt) == 0)
        nb_iter_64 = 0;	/* We have a PB because we have a LUP but without Nb, we will therefore consider that we LOOP 0 times (happens in the case of Macro with variable parameters) */
    else
    {
        nb_iter_64 = EvalExpressionAsInteger(begin_line->operand_txt,buffer_error,begin_line,begin_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
        if(strlen(buffer_error) > 0)
        {
            sprintf(param->buffer_error,"Impossible to get Lup value '%s' (line %d from file '%s') : %s",begin_line->operand_txt,begin_line->file_line_number,begin_line->file->file_name,buffer_error);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }
    }

    /* There will be no iteration */
    if(nb_iter_64 <= 0 || nb_iter_64 > 0x8000)
    {
        /* No label found, abort */
        if(strlen(begin_line->label_txt) == 0 && strlen(end_line->label_txt) == 0)
            return(NULL);

        /* We will have to create empty lines to accommodate the Labels */
        if(strlen(begin_line->label_txt) > 0)
        {
            /* We will have to create an Empty line to receive the label of the beginning */
            label_line = BuildEmptyLabelLine(begin_line->label_txt,begin_line);
            if(label_line == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Add the line */
            first_line = label_line;
            last_line = label_line;
        }
        if(strlen(end_line->label_txt) > 0)
        {
            /* We will have to create an Empty line to hold the end label */
            label_line = BuildEmptyLabelLine(end_line->label_txt,end_line);
            if(label_line == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",end_line->file_line_number,end_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Put the line containing the end label in second position */
            if(first_line == NULL)
                first_line = label_line;
            else
                last_line->next = label_line;
            last_line = label_line;
        }

        /* Return Label Lines */
        return(first_line);
    }
    nb_iter = (int) nb_iter_64;

    /*** We will produce 'nb_iter' code iterations ***/
    for(i=0; i<nb_iter; i++)
    {
        /** Produce lines of an iteration + Change labels **/
        first_new_line = BuildSourceLupOneIterationLine(begin_line,end_line,i+1,current_omfsegment);
        if(first_new_line == NULL)
            break;
        /* Last line */
        for(current_line=first_new_line; current_line; current_line=current_line->next)
            if(current_line->next == NULL)
                last_new_line = current_line;

        /* Attach the lines to the previous ones */
        if(first_line == NULL)
            first_line = first_new_line;
        else
            last_line->next = first_new_line;
        last_line = last_new_line;
    }

    /** There is a label on the LUP line **/
    if(strlen(begin_line->label_txt) > 0)
    {
        /* We try to place it on the 1st line of substitution lines */
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
            /* We will have to create an Empty line to receive the label */
            label_line = BuildEmptyLabelLine(begin_line->label_txt,first_line);
            if(label_line == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Put the line before all the others */
            label_line->next = first_line;
            first_line = label_line;
        }
    }

    /** There is a label on the line of --^ **/
    if(strlen(end_line->label_txt) > 0)
    {
        /* We try to place it on the Last line of substitution lines */
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
            /* We will have to create an Empty line to receive the label */
            label_line = BuildEmptyLabelLine(end_line->label_txt,last_line);
            if(label_line == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to process line (line %d from file '%s')",end_line->file_line_number,end_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Put the line after all the others */
            last_line->next = label_line;
            last_line = label_line;
        }
    }

    /* Returns the list of generated statements */
    return(first_line);
}


/*****************************************************************************************/
/*  BuildSourceLupOneIterationLine() :  Construction of Source lines from a loop of Lup. */
/*****************************************************************************************/
static struct source_line *BuildSourceLupOneIterationLine(struct source_line *begin_line, struct source_line *end_line, int iter, struct omf_segment *current_omfsegment)
{
    int is_reloc = 0;
    int64_t value = 0;
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    char *new_operand = NULL;
    struct source_line *current_line = NULL;
    struct source_line *new_line = NULL;
    struct source_line *first_line = NULL;
    struct source_line *last_line = NULL;
    struct source_line *label_line = NULL;
    struct variable *current_variable = NULL;
    struct external *current_external = NULL;
    char label_unique[512];
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /*** The first line is the LUP, the last is the --^ ***/
    for(current_line = begin_line->next; current_line != end_line; current_line = current_line->next)
    {
        /** Line of code / data / directive (we exclude Global) **/
        if(current_line->type == LINE_CODE || current_line->type == LINE_EMPTY || current_line->type == LINE_DATA ||
           current_line->type == LINE_DIRECTIVE || current_line->type == LINE_MACRO)
        {
            /** We will duplicate the line by adapting the Label and the Operand **/
            new_line = BuildSourceLupLine(current_line,iter,current_omfsegment);                   /* iter : 1 -> nb_iter */

            /* Add this line to the list */
            if(first_line == NULL)
                first_line = new_line;
            else
                last_line->next = new_line;
            last_line = new_line;
        }
        /** Variable **/
        else if(current_line->type == LINE_VARIABLE)
        {
            /** Update the value of the variable **/
            /* Retrieve the variable */
            my_Memory(MEMORY_SEARCH_VARIABLE,current_line->label_txt,&current_variable,current_omfsegment);
            if(current_variable == NULL)
            {
                sprintf(param->buffer_error,"Impossible to find Variable '%s' declaration (line %d from file '%s') : %s",current_line->label_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Calculate its new value */
            value = EvalExpressionAsInteger(current_line->operand_txt,buffer_error,current_line,current_line->nb_byte-1,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
            if(strlen(buffer_error) > 0)
            {
                sprintf(param->buffer_error,"Impossible to evaluate Variable '%s' value '%s' (line %d from file '%s') : %s",current_line->label_txt,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Stores the value */
            current_variable->value = value;
        }
    }

    /*** We will modify the Global Labels to get something unique ***/
    for(current_line=first_line; current_line; current_line=current_line->next)
    {
        /* We will only treat Global Labels */
        if(strlen(current_line->label_txt) > 0 && current_line->label_txt[0] != ':' && current_line->label_txt[0] != ']')
        {
            /* Creation of a unique label */
            GetUNID(&label_unique[0]);

            /** Pass all lines in to review **/
            for(label_line=first_line; label_line; label_line=label_line->next)
            {
                /** Replaces the Label in the Operand **/
                new_operand = ReplaceInOperand(label_line->operand_txt,current_line->label_txt,label_unique,SEPARATOR_REPLACE_VARIABLE,label_line);
                if(new_operand != label_line->operand_txt)
                {
                    free(label_line->operand_txt);
                    label_line->operand_txt = new_operand;
                }
            }

            /* It is replaced in the line that has defined it */
            free(current_line->label_txt);
            current_line->label_txt = strdup(label_unique);
            if(current_line->label_txt == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to replace Lup at line %d from file '%s' [Update Label]",begin_line->file_line_number,begin_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }

    /* Returns this set of rows */
    return(first_line);
}


/***************************************************************/
/*  BuildSourceLupLine() :  Building a Source Line from a Lup. */
/***************************************************************/
static struct source_line *BuildSourceLupLine(struct source_line *current_source_line, int iter, struct omf_segment *current_omfsegment)
{
    int i, j, k, l;
    struct source_line *new_source_line = NULL;
    char new_operand_txt[1024];
    char value_txt[256];
    char variable_name[1024];
    struct variable *current_variable = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Allocation of the new line */
    new_source_line = (struct source_line *) calloc(1,sizeof(struct source_line));
    if(new_source_line == NULL)
        return(NULL);
    
    /** Transfer the characteristics of the line Source (number of the line ...) **/
    new_source_line->file_line_number = current_source_line->file_line_number;
    new_source_line->file = current_source_line->file;
    new_source_line->type = current_source_line->type;
    new_source_line->no_direct_page = current_source_line->no_direct_page;
    new_source_line->address = -1;
    new_source_line->nb_byte = -1;
    new_source_line->is_valid = 1;           /* the line is valid */
    strcpy(new_source_line->m,"?");
    strcpy(new_source_line->x,"?");
    strcpy(new_source_line->reloc,"         ");
    new_source_line->operand_value = 0xFFFFFFFF;
    new_source_line->operand_address_long = 0xFFFFFFFF;
    new_source_line->macro = current_source_line->macro;

    /** The first elements are duplicated **/
    new_source_line->label_txt = strdup(current_source_line->label_txt);
    new_source_line->opcode_txt = strdup(current_source_line->opcode_txt);
    new_source_line->comment_txt = strdup(current_source_line->comment_txt);
    if(new_source_line->label_txt == NULL || new_source_line->opcode_txt == NULL || new_source_line->comment_txt == NULL)
    {
        mem_free_sourceline(new_source_line);
        sprintf(param->buffer_error,"Impossible to allocate memory to process Lup at line %d from file '%s'",current_source_line->file_line_number,current_source_line->file->file_name);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

    /** Modify the Label: @ -> ASC iter **/
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
                new_source_line->label_txt[i] = (char)('A' + iter - 1);    /* A-Z */
        }

    /** Variable replacements are performed in the Operand **/
    for(i=0,l=0; i<(int)strlen(current_source_line->operand_txt); i++)
    {
        if(current_source_line->operand_txt[i] == ']')
        {
            /* Isolate the name of the variable */
            variable_name[0] = ']';
            for(j=i+1,k=1; j<(int)strlen(current_source_line->operand_txt); j++)
                if(IsSeparator(current_source_line->operand_txt[j],0))
                    break;
                else
                    variable_name[k++] = current_source_line->operand_txt[j];
            variable_name[k] = '\0';

            /* Search the variable */
            my_Memory(MEMORY_SEARCH_VARIABLE,variable_name,&current_variable,current_omfsegment);
            if(current_variable == NULL)
            {
                mem_free_sourceline(new_source_line);
                sprintf(param->buffer_error,"Impossible to find Lup Variable '%s' declaration (line %d from file '%s')",current_source_line->label_txt,current_source_line->file_line_number,current_source_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Set the value in the correct form: Variable values are limited to 32 bits */
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

            /* Continue to explore the operand */
            i += (int) (strlen(variable_name) - 1);   /* i++ */
        }
        else
            new_operand_txt[l++] = current_source_line->operand_txt[i];
    }
    new_operand_txt[l] = '\0';

    /* Allocate memory */
    new_source_line->operand_txt = strdup(new_operand_txt);
    if(new_source_line->operand_txt == NULL)
    {
        mem_free_sourceline(new_source_line);
        sprintf(param->buffer_error,"Impossible to allocate memory to process Lup at line %d from file '%s'",current_source_line->file_line_number,current_source_line->file->file_name);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

    /* Return line */
    return(new_source_line);
}

/***********************************************************************/
