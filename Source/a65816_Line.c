/***********************************************************************/
/*                                                                     */
/*  a65816_Line.c : Module for line management.                        */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include "Dc_Library.h"

#include "a65816_OMF.h"		/* includes our header */

#include "a65816_File.h"
#include "a65816_Code.h"


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

/* Data operands */

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
    "CHK",                                           /* Remplace par 1 Byte of Checksum */
    NULL
};

char *directive_list[] = 
{
    "ANOP","ORG","PUT","PUTBIN",         /* PUTBIN Does not exist dans Merlin 16+ */
    "START","END",
    "DUM","DEND",
    "MX","XC","LONGA","LONGI",
    "USE","USING",
    "REL","DSK","LNK","SAV",
    "TYP",
    "IF","DO","ELSE","FIN",
    "LUP","--^",
    "ERR","DAT",
    "AST","CYC","EXP","LST","LSTDO","PAG","TTL","SKP","TR","KBD","PAU","SW","USR",   /* Nothing to do avec ces Directives */
    NULL
};

char *equivalence_list[] =    /* Equivalence or ]Variable */
{
    "EQU","=",
    NULL
};

/* Address Modes

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
static int ProcessSourceAsteriskLine(struct source_line *,struct source_file *);
static int ProcessMacroAsteriskLine(struct macro_line *,struct macro *);
static int ProcessSourceLineLocalLabel(struct source_line *,struct source_line *);
static int ProcessMacroLineLocalLabel(struct macro_line *,struct macro_line *);
static int ProcessSourceLineVariableLabel(struct source_line *);
static int ProcessMacroLineVariableLabel(struct macro_line *,struct macro *);
static void AddDateLine(struct source_line *);

/****************************************************/
/*  DecodeLineType() :  Determine the type of line. */
/****************************************************/
int DecodeLineType(struct source_line *first_line, struct macro *current_macro, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    int nb_error = 0, nb_label = 0, do_level = 0, do_status = 0, nb_global = 0, found = 0;
    int64_t value_wdc = 0;
    char *str_temp = NULL;
    char *new_label = NULL;
    char **tab_label = NULL;
    struct item *current_item = NULL;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct source_line *new_line = NULL;
    struct source_line *last_line = NULL;
    struct source_line *do_line = NULL;
    struct source_line *else_line = NULL;
    struct source_line *fin_line = NULL;
    char *new_opcode = NULL;
    struct global *current_global = NULL;
    char opcode[1024];
    char macro_name[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Source file */
    my_Memory(MEMORY_GET_FILE, &first_file, NULL, current_omfsegment);

    /*** Pass all lines in to review ***/
    for(current_line = first_line; current_line; current_line = current_line->next)
    {
        /* Already known the line */
        if(current_line->type != LINE_UNKNOWN)
            continue;

        /** Empty line **/
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

        /** ERR line Expression whose evaluation is done at the end **/
        if(!my_stricmp(current_line->opcode_txt,"ERR") && strlen(current_line->operand_txt))
        {
            current_line->type = LINE_CODE;
            continue;
        }

        /** DAT line to which Text has been added as Text **/
        if(!my_stricmp(current_line->opcode_txt,"DAT"))
        {
            current_line->type = LINE_DIRECTIVE;

            /* We must add a line of text with the date */
            if(strlen(current_line->operand_txt) > 0)
                AddDateLine(current_line);
            continue;
        }
        
        /** Line Defining a Global Entry Point for InterSegs **/
        if(strlen(current_line->label_txt) > 0 && !my_stricmp(current_line->opcode_txt,"ENT"))
        {
            current_line->type = LINE_GLOBAL;
            continue;
        }

        /** Line Defining One or More Global Entries for InterSegs **/
        if(strlen(current_line->label_txt) == 0 && !my_stricmp(current_line->opcode_txt,"ENT") && strlen(current_line->operand_txt) > 0)
        {
            /* Several EXT Labels in the form Label1, Label2, Label3 ... => Creation of new lines */
            tab_label = BuildUniqueListFromText(current_line->operand_txt,',',&nb_label);
            if(tab_label == NULL)
            {
                /* Error */
                printf("        => Error : Can't allocate memory to process line.\n");
                return(1);
            }

            /* We will keep these Labels to treat them after loading lines */
            for(int i=0; i<nb_label; i++)
                my_Memory(MEMORY_ADD_GLOBAL,tab_label[i],current_line,current_omfsegment);
            current_line->type = LINE_EMPTY;
            mem_free_list(nb_label,tab_label);
            continue;
        }

        /** Line defining an external label at this segment **/
        if(strlen(current_line->label_txt) > 0 && !my_stricmp(current_line->opcode_txt,"EXT"))
        {
            current_line->type = LINE_EXTERNAL;
            continue;
        }

        /** Line defining one | more External label à this segment **/
        if(strlen(current_line->label_txt) == 0 && !my_stricmp(current_line->opcode_txt,"EXT") && strlen(current_line->operand_txt) > 0)
        {
            /* We will reverse the Label and Operand: EXT Label => Label EXT */
            if(strchr(current_line->operand_txt,',') == NULL)
            {
                /* One EXT label */
                str_temp = current_line->label_txt;
                current_line->label_txt = current_line->operand_txt;
                current_line->operand_txt = str_temp;
                current_line->type = LINE_EXTERNAL;
                continue;
            }
            else
            {
                /* Several EXT Labels in the form Label1, Label2, Label3 ... => Creation of new lines */
                tab_label = BuildUniqueListFromText(current_line->operand_txt,',',&nb_label);
                if(tab_label == NULL)
                {
                    /* Error */
                    printf("        => Error : Can't allocate memory to process line.\n");
                    return(1);
                }

                /* Finally no label */
                if(nb_label == 0)
                {
                    mem_free_list(nb_label,tab_label);
                    current_line->type = LINE_EMPTY;
                    continue;
                }
                
                /** We will have to create lines for each of the Labels **/
                /* Current line */
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

                /* Following lines */
                for(int i=1; i<nb_label; i++)
                {
                    /* New line = Duplicate line */
                    new_line = DuplicateSourceLine(current_line);
                    if(new_line == NULL)
                    {
                        /* Error */
                        mem_free_list(nb_label,tab_label);
                        printf("        => Error : Can't allocate memory to process line.\n");
                        return(1);
                    }

                    /* New Label (from the bottom of the table) */
                    new_line->label_txt = strdup(tab_label[nb_label-i]);
                    if(new_line->label_txt == NULL)
                    {
                        /* Error */
                        mem_free_sourceline(new_line);
                        mem_free_list(nb_label,tab_label);
                        printf("        => Error : Can't allocate memory to process line.\n");
                        return(1);
                    }
                    
                    /* Attach the line */
                    new_line->next = current_line->next;
                    current_line->next = new_line;
                }

                /* Memory release */
                mem_free_list(nb_label,tab_label);
                continue;
            }
        }

        /** Identification of the type of line using the opcode **/
        if(strlen(current_line->opcode_txt) > 0)
        {
            /*** Macro (Macro detection is set before Opcodes detection because a macro WAIT could be interpreted as a 65c816 WAI) ***/
            /* Call via PMC or >>> */
            if((!my_stricmp(current_line->opcode_txt,"PMC") || !my_stricmp(current_line->opcode_txt,">>>")) && strlen(current_line->operand_txt) > 0)
            {
                /* We will isolate the name of the macro (because it can be pasted to the parameters) */
                strcpy(macro_name,current_line->operand_txt);
                for(int i=0; i<(int)strlen(macro_name); i++)
                    if(macro_name[i] == ',' || macro_name[i] == '.' || macro_name[i] == '/' || macro_name[i] == '-' || macro_name[i] == '(' || macro_name[i] == ' ')
                    {
                        macro_name[i] = '\0';
                        break;
                    }

                /* Search this Macro */
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
                /* Call with the name of the macro */
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

                /*** Opcode with a letter behind: LDA \ or LDA: (neither D nor L) ***/
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
                            current_line->no_direct_page = 1;   /* There is a character behind the opcode to prevent Direct Page */
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

                    /* We detect the REL (but we do not consider if we are on a multi-fixed project) */
                    if(!my_stricmp(current_line->opcode_txt,"REL") && current_omfproject->is_multi_fixed != 1)
                        current_omfsegment->is_relative = 1;

                    continue;
                }
            }

            /*** Equivalence or Variable ***/
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

        /* Error : Line still unknown :-( */
        if(current_line->type == LINE_UNKNOWN)
        {
            if(current_macro != NULL)
                printf("      => [Error] Unknown Macro line '%s' from Macro file '%s' (line %d), inserted in source file '%s' (line %d).\n",current_line->line_data,current_macro->file_name,current_macro->file_line_number,current_line->file->file_name,current_line->file_line_number);
            else
                printf("      => [Error] Unknown line '%s' in source file '%s' (line %d).\n",current_line->line_data,current_line->file->file_name,current_line->file_line_number);
            nb_error++;
        }
    }

    /*** We do not do the work of analyzing the ENTs in the Macros ***/
    if(current_macro == NULL)
    {
        /** Are there any ENTs that have been declared in ENT Label1, Label2... **/
        my_Memory(MEMORY_GET_GLOBAL_NB,&nb_global,NULL,current_omfsegment);
        for(int i=1; i<=nb_global; i++)
        {
            /* We have an ENT Label */
            my_Memory(MEMORY_GET_GLOBAL,&i,&current_global,current_omfsegment);

            /* Look for a line with this Label */
            for(found=0,current_line = first_file->first_line; current_line; current_line = current_line->next)
            {
                if(!strcmp(current_line->label_txt,current_global->name) && current_line->type == LINE_GLOBAL)
                {
                    /* We already have the ENT on the line of the label => nothing to do */
                    found = 1;
                    break;
                }
                else if(!strcmp(current_line->label_txt,current_global->name))
                {
                    /* We create a line ENT with the label */
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

                    /* Change values */
                    strcpy(current_line->operand_txt,"");    /* We empty the Operand of the ENT line */
                    free(current_line->opcode_txt);
                    current_line->opcode_txt = new_opcode;   /* The new Opcode is ENT */
                    current_line->type = LINE_GLOBAL;        /* This line is now a GLOBAL */
                    strcpy(new_line->label_txt,"");          /* We empty the label to avoid duplicates */

                    /* Attach the new line */
                    new_line->next = current_line->next;
                    current_line->next = new_line;

                    /* ENT label treated */
                    found = 1;
                    break;
                }
            }

            /* We did not find a line with this Label => we place it at the level of ENT Label1, Label2... */
            if(found == 0)
            {
                /* We create a line ENT with the label by placing it after the line ENT Label1, Label2...*/
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

                /* Change values */
                strcpy(new_line->operand_txt,"");      /* We empty the Operand of the ENT line */
                free(new_line->label_txt);
                new_line->label_txt = new_label;       /* Place the Label */
                new_line->type = LINE_GLOBAL;          /* This line is now a GLOBAL */

                /* Attach the new line below */
                new_line->next = current_global->source_line->next;
                current_global->source_line->next = new_line;
            }
        }
    }
    
    /*** We do not do the work of analysis of the DOs in Macro ***/
    if(current_macro == NULL)
    {
        /************************************************************************************/
        /*** 1st pass to mark the levels of the DO-ELSE-FIN and validate the nesting      ***/
        /************************************************************************************/
        do_level = 0;
        for(current_line=first_line; current_line; current_line=current_line->next)
        {
            /* We keep the last line of the file for the Error message */
            last_line = current_line;

            /* Line defining a new DO-FIN condition */
            if(current_line->type == LINE_DIRECTIVE && (!my_stricmp(current_line->opcode_txt,"DO") || !my_stricmp(current_line->opcode_txt,"IF")))
            {
                do_level++;
                current_line->do_level = do_level;
                continue;
            }

            /* Line defining the inversion of a condition DO-ELSE_FIN */
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

            /* Line defining the end of a DO-FIN */
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

            /* Normal line */
            current_line->do_level = do_level;
        }

        if(do_level > 0)
        {
            printf("      => [Error] Missing FIN directive in source file '%s' (due to previous usage of a DO or IF directive).\n",last_line->file->file_name);
            return(1);
        }

        /*************************************************************************************/
        /*** 2nd pass to evaluate the conditions DO and invalidate the lines of the source ***/
        /*************************************************************************************/
        for(do_line=first_line; do_line; do_line=do_line->next)
        {
            /* Line defining a first condition DO */
            if(do_line->type == LINE_DIRECTIVE && !my_stricmp(do_line->opcode_txt,"DO"))
            {
                /* Assessment of the condition */
                do_status = QuickConditionEvaluate(do_line,&value_wdc,current_omfsegment);

                /* Look for FIN line */
                for(fin_line=do_line; fin_line; fin_line=fin_line->next)
                    if(fin_line->type == LINE_DIRECTIVE && !my_stricmp(fin_line->opcode_txt,"FIN") && do_line->do_level == fin_line->do_level)
                        break;

                /* Look for a possible ELSE line */
                for(else_line=do_line; else_line!=fin_line; else_line=else_line->next)
                    if(else_line->type == LINE_DIRECTIVE && !my_stricmp(else_line->opcode_txt,"ELSE") && do_line->do_level == else_line->do_level)
                        break;
                if(else_line == fin_line)
                    else_line = NULL;
                
                /** On invalid DO - ELSE / FIN **/
                if(do_status == STATUS_DONT)
                {
                    for(current_line=do_line->next; current_line!=((else_line!=NULL)?else_line:fin_line); current_line=current_line->next)
                        current_line->is_valid = 0;
                }
                /** On invalid ELSE / FIN **/
                else if(do_status == STATUS_DO && else_line != NULL)
                {
                    for(current_line=else_line->next; current_line!=fin_line; current_line=current_line->next)
                        current_line->is_valid = 0;
                }
                
                /** We do not know: We're still part of FIN **/
                do_line = fin_line;
            }
        }
    }
    
    /* Returns the number of Error */
    return(nb_error);
}


/***********************************************************************/
/*  ProcessAllAsteriskLine() :  Replaces '*' in Code Lines and Macros. */
/***********************************************************************/
int ProcessAllAsteriskLine(struct omf_segment *current_omfsegment)
{
    int i, error, nb_macro;
    struct source_file *first_file;
    struct macro *current_macro;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Process the lines in the Source file **/
    error = ProcessSourceAsteriskLine(first_file->first_line,first_file);
    if(error)
        return(1);

    /** Process the Macros **/
    my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
    for(i=1; i<=nb_macro; i++)
    {
        my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);

        /** Process the lines of this Macro **/
        error = ProcessMacroAsteriskLine(current_macro->first_line,current_macro);
        if(error)
            return(1);
    }

    /* OK */
    return(0);
}


/************************************************************************/
/*  ProcessSourceAsteriskLine() :  Replace the '*' in the line of Code. */
/************************************************************************/
static int ProcessSourceAsteriskLine(struct source_line *first_line, struct source_file *first_file)
{
    int use_address = 0;
    struct source_line *previous_line = NULL;
    struct source_line *current_line = NULL;
    struct source_line *new_line = NULL;
    char label_name[1024];
    char buffer_error[1024];

    /*** Pass all lines in to review ***/
    for(previous_line=NULL, current_line=first_line; current_line; previous_line=current_line, current_line=current_line->next)
    {
        /* We do not treat comments */
        if(current_line->type == LINE_EMPTY || current_line->type == LINE_COMMENT || current_line->type == LINE_GLOBAL || current_line->is_valid == 0)
            continue;

        /** Special case of ]Label = * or Label = * **/
        if(strlen(current_line->label_txt) > 0 && !strcmp(current_line->opcode_txt,"=") && !strcmp(current_line->operand_txt,"*"))
        {
            /* We convert this line to Empty line */
            strcpy(current_line->opcode_txt,"");
            strcpy(current_line->operand_txt,"");
            current_line->type = LINE_EMPTY;
            continue;
        }

        /** We search only in the Operand **/
        if(strchr(current_line->operand_txt,'*') != NULL)
        {
            /** Detected a * used as a value? **/
            use_address = UseCurrentAddress(current_line->operand_txt,&buffer_error[0],current_line);
            if(strlen(buffer_error) > 0)
            {
                printf("    Error : Impossible to analyze Operand '%s' in source file '%s' (line %d) : %s.\n",
                       current_line->operand_txt,current_line->file->file_name,current_line->file_line_number,buffer_error);
                return(1);
            }
            if(use_address == 0)
                continue;

            /** We will replace the * with a unique label **/
            /* Creation of a unique ANOP label */
            GetUNID(&label_name[0]);

            /** Creation of an Empty line ANOP with the label **/
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
            new_line->type_aux = current_line->type_aux;        /* We keep the inclusion information in the body of a Macro */
            new_line->is_inside_macro = current_line->is_inside_macro;
            if(new_line->label_txt == NULL || new_line->opcode_txt == NULL || new_line->operand_txt == NULL || new_line->comment_txt == NULL)
            {
                printf("    Error : Impossible to allocate memory to populate new Empty line.\n");
                mem_free_sourceline(new_line);
                return(1);
            }
            new_line->type = LINE_EMPTY;

            /* Attachment of the line above */
            new_line->next = current_line;
            if(previous_line == NULL)
                first_file->first_line = new_line;
            else
                previous_line->next = new_line;
            
            /** Replaces the * with a unique label **/
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


/**************************************************************/
/*  ProcessMacroAsteriskLine() :  Replace '*' in Macro lines. */
/**************************************************************/
static int ProcessMacroAsteriskLine(struct macro_line *first_line, struct macro *current_macro)
{
    int use_address = 0;
    struct macro_line *previous_line = NULL;
    struct macro_line *current_line = NULL;
    struct macro_line *new_line = NULL;
    char label_name[1024];
    char buffer_error[1024];

    /*** Pass all lines in to review ***/
    for(previous_line=NULL, current_line=first_line; current_line; previous_line=current_line, current_line=current_line->next)
    {
        /** Special case of ]Label = * or Label = * **/
        if(strlen(current_line->label) > 0 && !strcmp(current_line->opcode,"=") && !strcmp(current_line->operand,"*"))
        {
            /* We convert this line to Empty line */
            strcpy(current_line->opcode,"");
            strcpy(current_line->operand,"");
            continue;
        }

        /** We do not look when in the Operand **/
        if(strchr(current_line->operand,'*') != NULL)
        {
            /** Detected * used as a value? **/
            use_address = UseCurrentAddress(current_line->operand,&buffer_error[0],NULL);
            if(strlen(buffer_error) > 0)
            {
                printf("    Error : Impossible to analyze Operand '%s' in Macro '%s' : %s.\n",
                       current_line->operand,current_macro->name,buffer_error);
                return(1);
            }
            if(use_address == 0)
                continue;

            /** We will replace the * with a unique label **/
            /* Creation of a unique ANOP label */
            GetUNID(&label_name[0]);

            /** Creation of an Empty line ANOP with the label **/
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

            /* Attachment of the line above */
            new_line->next = current_line;
            if(previous_line == NULL)
                current_macro->first_line = new_line;
            else
                previous_line->next = new_line;
            
            /** Replaces the * with a unique label **/
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


/********************************************************/
/*  BuildLabelTable() :  Construction of Labels tables. */
/********************************************************/
int BuildLabelTable(struct omf_segment *current_omfsegment)
{
    struct label *current_label = NULL;
    struct source_line *current_line = NULL;
    struct source_file *first_file = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Pass all lines in to review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We only take the lines with label */
        if(strlen(current_line->label_txt) == 0)
            continue;

        /* We do not take local labels or variables */
        if(current_line->label_txt[0] == ':' || current_line->label_txt[0] == ']')
            continue;

        /* We do not take Equivalence */
        if(current_line->type == LINE_EQUIVALENCE)
            continue;

        /* We do not take the External */
        if(current_line->type == LINE_EXTERNAL)
            continue;

        /* We do not take Label in Macro */
        if(current_line->type == LINE_DIRECTIVE && current_line->type_aux == LINE_MACRO_DEF)
            continue;
        if(current_line->is_inside_macro == 1)
            continue;

        /* We do not take Labels in LUP */
        if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"LUP"))
            continue;
        if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"--^"))
            continue;

        /** Allocation of the Label structure **/
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

        /* A Global label is a label like any other */
        if(current_line->type == LINE_GLOBAL)
            current_label->is_global = 1;

        /* Create the structure */
        my_Memory(MEMORY_ADD_LABEL,current_label,NULL,current_omfsegment);
    }

    /* Sort Labels */
    my_Memory(MEMORY_SORT_LABEL,NULL,NULL,current_omfsegment);

    /* OK */
    return(0);
}


/*******************************************************************/
/*  BuildEquivalenceTable() :  Construction of Equivalence tables. */
/*******************************************************************/
int BuildEquivalenceTable(struct omf_segment *current_omfsegment)
{
    int nb_equivalence = 0, nb_element = 0, modified = 0, nb_modified = 0;
    struct equivalence *current_equivalence = NULL;
    struct equivalence *replace_equivalence = NULL;
    struct source_line *current_line = NULL;
    struct source_file *first_file = NULL;
    char *new_value = NULL;
    char **tab_element = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Pass all lines in to review (some equivalences from macro files are already registered) **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We only take Equivalence */
        if(current_line->type != LINE_EQUIVALENCE)
            continue;

        /* We only take the lines with label */
        if(strlen(current_line->label_txt) == 0)
            continue;

        /** Allocation of the structure for Equivalence **/
        current_equivalence = (struct equivalence *) calloc(1,sizeof(struct equivalence));
        if(current_equivalence == NULL)
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure equivalence");
        current_equivalence->name = strdup(current_line->label_txt);
        current_equivalence->valueStr = strdup(current_line->operand_txt);
        if(current_equivalence->name == NULL || current_equivalence->valueStr == NULL)
        {
            mem_free_equivalence(current_equivalence);
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure equivalence");
        }
        current_equivalence->source_line = current_line;   /* Cette equivalence vient d'un Source file */

        /* Create the structure */
        my_Memory(MEMORY_ADD_EQUIVALENCE,current_equivalence,NULL,current_omfsegment);
    }

    /* Sort the Equivalence */
    my_Memory(MEMORY_SORT_EQUIVALENCE,NULL,NULL,current_omfsegment);

    /** We will go back on equivalences to solve those depend on other equivalences **/
    modified = 1;
    nb_modified = 0;
    while(modified)
    {
        /* Init */
        modified= 0;
        my_Memory(MEMORY_GET_EQUIVALENCE_NB,&nb_equivalence,NULL,current_omfsegment);
        for(int i=1; i<=nb_equivalence; i++)
        {
            my_Memory(MEMORY_GET_EQUIVALENCE,&i,&current_equivalence,current_omfsegment);

            /** Cut out the expression **/
            tab_element = DecodeOperandeAsElementTable(current_equivalence->valueStr,&nb_element,SEPARATOR_REPLACE_LABEL,current_equivalence->source_line);
            if(tab_element == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'tab_element' table");

            /** Review the values **/
            param->buffer_operand[0]='\0';
            for(int j=0; j<nb_element; j++)
            {
                my_Memory(MEMORY_SEARCH_EQUIVALENCE,tab_element[j],&replace_equivalence,current_omfsegment);
                if(replace_equivalence != NULL)
                {
                    /* Adding {} certifies the order of evaluation */
                    strcat(param->buffer_operand,"{");
                    strcat(param->buffer_operand,replace_equivalence->valueStr);   /* Equivalence */
                    strcat(param->buffer_operand,"}");

                    /* The value has been changed */
                    modified = 1;
                }
                else
                    strcat(param->buffer_operand,tab_element[j]);
            }

            /* Memory release */
            mem_free_table(nb_element,tab_element);

            /** If the value has been changed, we replace it **/
            if(modified == 1)
            {
                new_value = strdup(param->buffer_operand);
                if(new_value == NULL)
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory to replace an Equivalence");

                /* Free the old */
                free(current_equivalence->valueStr);

                /* Position the new */
                current_equivalence->valueStr = new_value;
            }
        }

        /* Don't endlessly recurse */
        if(modified)
            nb_modified++;
        if(nb_modified > 10)
            my_RaiseError(ERROR_RAISE,"Recursivity detected in Equivalence replacement");
    }

    /* OK */
    return(0);
}


/*********************************************************/
/*  BuildExternalTable() :  External table construction. */
/*********************************************************/
int BuildExternalTable(struct omf_segment *current_omfsegment)
{
    struct external *current_external = NULL;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /* Init */
    my_Memory(MEMORY_FREE_EXTERNAL,NULL,NULL,current_omfsegment);

    /** Pass all lines in to review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We only take External */
        if(current_line->type != LINE_EXTERNAL)
            continue;

        /** Allocation of the structure External **/
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

        /* Create the structure */
        my_Memory(MEMORY_ADD_EXTERNAL,current_external,NULL,current_omfsegment);
    }

    /* Sort the External */
    my_Memory(MEMORY_SORT_EXTERNAL,NULL,NULL,current_omfsegment);

    /* OK */
    return(0);
}


/**********************************************************/
/*  CheckForDuplicatedLabel() :  Search Duplicate Labels. */
/**********************************************************/
int CheckForDuplicatedLabel(struct omf_segment *current_omfsegment)
{
    int nb_label = 0, nb_equivalence = 0, nb_error = 0;
    struct label *previous_label = NULL;
    struct label *current_label = NULL;
    struct equivalence *previous_equivalence = NULL;
    struct equivalence *current_equivalence = NULL;

    /** Search duplicates in Labels **/
    previous_label = NULL;
    my_Memory(MEMORY_GET_LABEL_NB,&nb_label,NULL,current_omfsegment);
    for(int i=1; i<=nb_label; i++)
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

    /** Find duplicates in Equivalence (but not in variables) **/
    previous_equivalence = NULL;
    my_Memory(MEMORY_GET_EQUIVALENCE_NB,&nb_equivalence,NULL,current_omfsegment);
    for(int i=1; i<=nb_equivalence; i++)
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

    /** Search for duplicates between Label and Equivalence **/
    for(int i=1; i<=nb_equivalence; i++)
    {
        my_Memory(MEMORY_GET_EQUIVALENCE,&i,&current_equivalence,current_omfsegment);

        /* Search for a Label with the same name */
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


/****************************************************************/
/*  ProcessAllLocalLabel() :   replace local labels with unid_. */
/****************************************************************/
int ProcessAllLocalLabel(struct omf_segment *current_omfsegment)
{
    int error = 0, nb_macro = 0;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct source_line *last_line = NULL;
    struct macro *current_macro = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Process the lines in the Source file **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
        if(current_line->next == NULL)
            last_line = current_line;
    error = ProcessSourceLineLocalLabel(first_file->first_line,last_line);

    /** Process the Macros **/
    my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
    for(int i=1; i<=nb_macro; i++)
    {
        my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);

        /* Process the lines of this Macro */
        error = ProcessMacroLineLocalLabel(current_macro->first_line,current_macro->last_line);
    }

    /* OK */
    return(0);
}


/*******************************************************************************************************/
/*  ProcessSourceLineLocalLabel() : We replace the local labels with unid_ of the lines of the Source. */
/*******************************************************************************************************/
static int ProcessSourceLineLocalLabel(struct source_line *first_line, struct source_line *last_line)
{
    struct source_line *current_line = NULL;
    struct source_line *other_line = NULL;
    struct source_line *begin_global_line = NULL;
    struct source_line *end_global_line = NULL;
    struct source_line *replace_line = NULL;
    int found = 0, nb_local = 0;
    char *new_label = NULL;
    char *new_operand = NULL;
    char previous_label[256];
    char unique_label[256];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /** If a Label: Local exists in only 1 copy, we will globalize it _Local **/
    for(current_line=first_line; current_line; current_line=current_line->next)
    {
        /* We ignore the lines comments / those without labels / Label other than :Label */
        if(current_line->type == LINE_COMMENT || current_line->is_valid == 0)
            continue;
        if(strlen(current_line->label_txt) == 0)
            continue;
        if(current_line->label_txt[0] != ':')
            continue;

        /** Is this label unique? **/
        for(found=0,other_line=first_line; other_line; other_line=other_line->next)
        {
            /* We ignore the lines comments / those without labels / Label other than :Label */
            if(other_line->type == LINE_COMMENT || other_line->is_valid == 0)
                continue;
            if(strlen(other_line->label_txt) == 0)
                continue;
            if(other_line->label_txt[0] != ':')
                continue;
            if(other_line == current_line)
                continue;

            /* Compare the Label */
            if(!strcmp(current_line->label_txt,other_line->label_txt))
            {
                found = 1;
                break;
            }
        }

        /** It's unique, we'll try to find a name other than oz_unid... **/
        if(found == 0)
        {
            /* We replace the : with _ */
            strcpy(unique_label,current_line->label_txt);
            unique_label[0] = '_';

            /* Is this _Label unique? */
            for(found=0,other_line=first_line; other_line; other_line=other_line->next)
            {
                /* We ignore the lines comments / those without labels / Label other than :Label */
                if(other_line->type == LINE_COMMENT || other_line->is_valid == 0)
                    continue;
                if(strlen(other_line->label_txt) == 0)
                    continue;

                /* Compare the Label */
                if(!strcmp(unique_label,other_line->label_txt))
                {
                    found = 1;
                    break;
                }
            }

            /* It's unique, so use to replace */
            if(found == 0)
            {
                /* Replace :Label with _Label throughout the file */
                strcpy(previous_label,current_line->label_txt);
                for(other_line=first_line; other_line; other_line=other_line->next)
                {
                    /* We replaced the label */
                    if(!strcmp(other_line->label_txt,previous_label))
                    {
                        other_line->label_txt[0] = '_';
                        other_line->was_local_label = 1;     /* This label is a local label */
                    }

                    /** Replaces the Label in the Operand **/
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

    /** We are looking for the first Global Label **/
    for(begin_global_line=first_line; begin_global_line; begin_global_line=begin_global_line->next)
    {
        /* Comment / non-label lines are ignored / those with variable labels ] */
        if(begin_global_line->type == LINE_COMMENT || begin_global_line->is_valid == 0)
            continue;
        if(strlen(begin_global_line->label_txt) == 0)
            continue;
        if(begin_global_line->label_txt[0] == ']')
            continue;
        if(begin_global_line->was_local_label == 1)
            continue;

        /* Error : You can not start your source with a Local Label */
        if(begin_global_line->label_txt[0] == ':')
        {
            printf("      => [Error] Local Label : '%s' can not be first label in file '%s' (line %d).\n",begin_global_line->data,begin_global_line->file->file_name,begin_global_line->file_line_number);
            return(1);
        }
        
        /* We are on the 1st Global Label */
        break;
    }
    
    /* No global Label in the source => We take the beginning of the file as reference */
    if(begin_global_line == NULL)
        begin_global_line = first_line;

    /** Process local labels located between 2 global labels **/
    while(begin_global_line)
    {
        /* Search the following global label */
        for(nb_local=0,end_global_line = begin_global_line->next; end_global_line; end_global_line=end_global_line->next)
        {
            /* We jump */
            if(end_global_line->type == LINE_COMMENT || end_global_line->is_valid == 0)
                continue;
            if(strlen(end_global_line->label_txt) == 0)
                continue;
            if(end_global_line->label_txt[0] == ']')
                continue;
            if(end_global_line->was_local_label == 1)
                continue;
            
            /* Count matches */
            if(end_global_line->label_txt[0] == ':')
            {
                nb_local++;
                continue;
            }
            
            /* New global label */
            break;
        }

        /** No Global Label => We take the last line of the Source **/
        if(end_global_line == NULL)
            end_global_line = last_line;

        /** We're done **/
        if(nb_local == 0 && end_global_line == last_line)
            return(0);

        /** No local label in the meantime, we go to the next **/
        if(nb_local == 0 && end_global_line != NULL)
        {
            begin_global_line = end_global_line;
            continue;
        }

        /** there is local label in the meantime, process it **/
        if(nb_local > 0)
        {
            /** We will review all the lines of the interval to correct all local labels **/
            for(current_line=begin_global_line; current_line; current_line=current_line->next)
            {
                /* Skips invalid rows */
                if(current_line->is_valid == 0)
                    continue;

                /** We are looking for a local Label **/
                if(current_line->label_txt[0] == ':')
                {
                    /* Creation of a unique label */
                    GetUNID(unique_label);

                    /** Replacement is made throughout the interval **/
                    for(replace_line=begin_global_line; replace_line; replace_line=replace_line->next)
                    {
                        /** Replaces the Label in the Operand **/
                        if(replace_line->type == LINE_CODE || replace_line->type == LINE_DATA || replace_line->type == LINE_MACRO)
                        {
                            new_operand = ReplaceInOperand(replace_line->operand_txt,current_line->label_txt,unique_label,SEPARATOR_REPLACE_LABEL,replace_line);
                            if(new_operand != replace_line->operand_txt)
                            {
                                free(replace_line->operand_txt);
                                replace_line->operand_txt = new_operand;
                            }
                        }

                        /* End of zone */
                        if(replace_line == end_global_line)
                            break;
                    }
                    
                    /** Replaces the label of the line **/
                    new_label = strdup(unique_label);
                    if(new_label == NULL)
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new local label");
                    free(current_line->label_txt);
                    current_line->label_txt = new_label;
                    current_line->was_local_label = 1;     /* This label is a local label */
                }
                
                /* Zone ends */
                if(current_line == end_global_line)
                    break;
            }
        }
    }

    /* OK */
    return(0);
}


/****************************************************************************************/
/*  ProcessMacroLineLocalLabel() :  We replace local labels with unid_ lines of Macros. */
/****************************************************************************************/
static int ProcessMacroLineLocalLabel(struct macro_line *first_line, struct macro_line *last_line)
{
    struct macro_line *current_line = NULL;
    struct macro_line *begin_global_line = NULL;
    struct macro_line *end_global_line = NULL;
    struct macro_line *replace_line = NULL;
    int nb_local = 0;
    char *new_label = NULL;
    char *new_operand = NULL;
    char unique_label[256];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /** We are looking for the first Global Label **/
    for(begin_global_line=first_line; begin_global_line; begin_global_line=begin_global_line->next)
    {
        /* Comment / non-label lines are ignored / those with variable labels ] */
        if(strlen(begin_global_line->label) == 0)
            continue;
        if(begin_global_line->label[0] == ']')
            continue;
        
        /* We are on the 1st Global Label */
        break;
    }
    
    /* No global label in the source => We take the 1st line as reference */
    if(begin_global_line == NULL)
        begin_global_line = first_line;
    
    /** Process local labels located between 2 global labels **/
    while(begin_global_line)
    {
        /* Search the following global label */
        for(nb_local=0,end_global_line = begin_global_line->next; end_global_line; end_global_line=end_global_line->next)
        {
            /* We jump */
            if(strlen(end_global_line->label) == 0)
                continue;
            if(end_global_line->label[0] == ']')
                continue;
            
            /* Count matches */
            if(end_global_line->label[0] == ':')
            {
                nb_local++;
                continue;
            }
            
            /* New global label */
            break;
        }

        /* We reached the end without meeting a Global Label */
        if(end_global_line == NULL)
            end_global_line = last_line;

        /** We're done **/
        if(nb_local == 0 && end_global_line == last_line)
            return(0);

        /** No local label in the meantime, we go to the next **/
        if(nb_local == 0 && end_global_line != NULL)
        {
            begin_global_line = end_global_line;
            continue;
        }

        /** there is local label in the meantime, process it **/
        if(nb_local > 0)
        {
            /** We will review all the lines of the interval to correct all local labels **/
            for(current_line=begin_global_line; current_line; current_line=current_line->next)
            {
                /** We are looking for a local Label **/
                if(current_line->label[0] == ':')
                {
                    /* Creation of a unique label */
                    GetUNID(unique_label);

                    /** Replacement is made throughout the interval **/
                    for(replace_line=begin_global_line; replace_line; replace_line=replace_line->next)
                    {
                        /** Replaces the Label in the Operand **/
                        new_operand = ReplaceInOperand(replace_line->operand,current_line->label,unique_label,SEPARATOR_REPLACE_LABEL,NULL);
                        if(new_operand != replace_line->operand)
                        {
                            free(replace_line->operand);
                            replace_line->operand = new_operand;
                        }

                        /* End of zone */
                        if(replace_line == end_global_line)
                            break;
                    }
                    
                    /** Replaces the label of the line **/
                    new_label = strdup(unique_label);
                    if(new_label == NULL)
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new local label in Macro");
                    free(current_line->label);
                    current_line->label = new_label;
                }
                
                /* Zone ends */
                if(current_line == end_global_line)
                    break;
            }
        }
    }

    /* OK */
    return(0);
}


/************************************************************************/
/*  ProcessAllVariableLabel() :  Variable labels are replaced by unid_. */
/************************************************************************/
int ProcessAllVariableLabel(struct omf_segment *current_omfsegment)
{
    int error = 0, nb_macro = 0;
    struct source_file *first_file = NULL;
    struct macro *current_macro = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE, &first_file, NULL, current_omfsegment);

    /** Process the lines in the Source file **/
    error = ProcessSourceLineVariableLabel(first_file->first_line);

    /** Process the Macros **/
    my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
    for(int i=1; i<=nb_macro; i++)
    {
        my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);

        /** Process the lines of this Macro **/
        error = ProcessMacroLineVariableLabel(current_macro->first_line,current_macro);
    }

    /* OK */
    return(0);
}


/*************************************************************************************************/
/*  ProcessSourceLineVariableLabel() :  We replace the variable labels of the source with unid_. */
/*************************************************************************************************/
int ProcessSourceLineVariableLabel(struct source_line *first_line)
{
    struct source_line *replace_line = NULL;
    struct source_line *begin_variable_line = NULL;
    struct source_line *end_variable_line = NULL;
    int use_address = 0;
    char *new_label = NULL;
    char *new_operand = NULL;
    char unique_label[256];
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /*** Process all Variable Labels ]label ***/
    for(begin_variable_line=first_line; begin_variable_line; begin_variable_line=begin_variable_line->next)
    {
        /* We ignore comment lines / those without labels */
        if(begin_variable_line->type == LINE_COMMENT || strlen(begin_variable_line->label_txt) == 0 || begin_variable_line->is_valid == 0)
            continue;
        if(begin_variable_line->type == LINE_VARIABLE)
        {
            /* It is necessary to differentiate a true Equivalence of a label ]LP = * */
            if(strchr(begin_variable_line->operand_txt,'*') == NULL)
                continue;

            /** Detected * used as a value? **/
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

        /** Start of the search area **/
        if(begin_variable_line->label_txt[0] == ']')
        {
            /** Search to the end of the search range for = Other Variable Label with the same name **/
            for(end_variable_line=begin_variable_line->next; end_variable_line; end_variable_line=end_variable_line->next)
            {
                /* We ignore comment lines / those without labels */
                if(end_variable_line->type == LINE_COMMENT || strlen(end_variable_line->label_txt) == 0 || end_variable_line->is_valid == 0)
                    continue;

                /* We are looking for the same Label (Case sensitive) */
                if(!strcmp(begin_variable_line->label_txt,end_variable_line->label_txt))
                    break;
            }

            /* Creation of a unique label */
            GetUNID(unique_label);

            /** We replace on the zone **/
            for(replace_line=begin_variable_line; replace_line != end_variable_line; replace_line=replace_line->next)
            {
                /* We do not process invalid lines */
                if(replace_line->is_valid == 0)
                    continue;

                /** Replaces the Label in the Operand **/
                new_operand = ReplaceInOperand(replace_line->operand_txt,begin_variable_line->label_txt,unique_label,SEPARATOR_REPLACE_VARIABLE,replace_line);
                if(new_operand != replace_line->operand_txt)
                {
                    free(replace_line->operand_txt);
                    replace_line->operand_txt = new_operand;
                }
            }

            /** Replaces the label of the line **/
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


/*******************************************************************************************/
/*  ProcessMacroLineVariableLabel() :  We replace the variable labels of a Macro by unid_. */
/*******************************************************************************************/
int ProcessMacroLineVariableLabel(struct macro_line *first_line, struct macro *current_macro)
{
    struct macro_line *replace_line = NULL;
    struct macro_line *begin_variable_line = NULL;
    struct macro_line *end_variable_line = NULL;
    int use_address = 0;
    char *new_label = NULL;
    char *new_operand = NULL;
    char buffer_error[1024];
    char unique_label[256];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /*** Process all Variable Labels ]label ***/
    for(begin_variable_line=first_line; begin_variable_line; begin_variable_line=begin_variable_line->next)
    {
        /* We ignore comment lines / those without labels */
        if(strlen(begin_variable_line->label) == 0)
            continue;
        if(!strcmp(begin_variable_line->opcode,"="))
        {
            /* It is necessary to differentiate a true Equivalence of a label ]LP = * */
            if(strchr(begin_variable_line->operand,'*') == NULL)
                continue;

            /** Detected * used as a value? **/
            use_address = UseCurrentAddress(begin_variable_line->operand,&buffer_error[0],NULL);
            if(strlen(buffer_error) > 0)
            {
                printf("    Error : Impossible to analyze Operand '%s' in Macro %s : %s.\n",begin_variable_line->operand,current_macro->name,buffer_error);
                return(1);
            }
            if(use_address == 0)
                continue;
        }

        /** Start of the search area **/
        if(begin_variable_line->label[0] == ']')
        {
            /** Search to the end of the search range for = Other Variable Label with the same name **/
            for(end_variable_line=begin_variable_line->next; end_variable_line; end_variable_line=end_variable_line->next)
            {
                /* We ignore comment lines / those without labels */
                if(strlen(end_variable_line->label) == 0)
                    continue;

                /* We are looking for the same Label (Case sensitive) */
                if(!strcmp(begin_variable_line->label,end_variable_line->label))
                    break;
            }

            /* Creation of a unique label */
            GetUNID(unique_label);

            /** We replace on the zone **/
            for(replace_line=begin_variable_line; replace_line != end_variable_line; replace_line=replace_line->next)
            {
                /** Replaces the Label in the Operand **/
                new_operand = ReplaceInOperand(replace_line->operand,begin_variable_line->label,unique_label,SEPARATOR_REPLACE_VARIABLE,NULL);
                if(new_operand != replace_line->operand)
                {
                    free(replace_line->operand);
                    replace_line->operand = new_operand;
                }
            }

            /** Replaces the label of the line **/
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


/***************************************************/
/*  ProcessEquivalence() :  Replaces Equivalences. */
/***************************************************/
int ProcessEquivalence(struct omf_segment *current_omfsegment)
{
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    int nb_modified = 0;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /*** Process all Operands that may contain Equivalence ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We ignore comment lines / those without labels */
        if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY || current_line->type == LINE_GLOBAL || strlen(current_line->operand_txt) == 0)
            continue;

        /* No replacement on Data HEX lines */
        if(current_line->type == LINE_DATA && !my_stricmp(current_line->opcode_txt,"HEX"))
            continue;

        /* Replaces Equivalences on the Line */
        nb_modified += ProcessLineEquivalence(current_line,current_omfsegment);
    }

    /* OK */
    return(0);
}


/******************************************************************/
/*  ProcessLineEquivalence() :  Replaces equivalences for 1 line. */
/******************************************************************/
int ProcessLineEquivalence(struct source_line *current_line, struct omf_segment *current_omfsegment)
{
    int modified = 0, nb_element = 0, is_variable = 0, is_label = 0, is_hexa = 0, nb_byte = 0;
    char **tab_element = NULL;
    char *new_operand = NULL;
    char variable_name[1024];
    struct equivalence *current_equivalence = NULL;
    struct variable *current_variable = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /** We will cut the operand into several unitary elements **/
    tab_element = DecodeOperandeAsElementTable(current_line->operand_txt,&nb_element,SEPARATOR_REPLACE_LABEL,current_line);
    if(tab_element == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to decode Operand as element table");
    
    /** We rebuild the chain by replacing the values (case sensitive) **/
    param->buffer_operand[0]='\0';
    for(int i=0; i<nb_element; i++)
    {
        my_Memory(MEMORY_SEARCH_EQUIVALENCE,tab_element[i],&current_equivalence,current_omfsegment);
        if(current_equivalence != NULL)
        {
            /* Particular case of a variable ]var or a label ]var and an equivalence var */
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
                        /* We are looking to see if there is no local label with this name */
                        if(IsLocalLabel(variable_name,current_omfsegment))
                            is_label = 1;
                    }
                }
                else if(!strcmp(tab_element[i-1],"$"))
                {
                    /* Can we interpret $ variable as a Hexa number? */
                    sprintf(variable_name,"$%s",tab_element[i]);
                    if(IsHexaDecimal(variable_name,&nb_byte))
                        is_hexa = 1;
                }
            }

            /* We will not replace if it's finally a Variable / Label / Hex form */
            if(is_variable || is_label || is_hexa)
                strcat(param->buffer_operand,tab_element[i]);               /* Variable */
            else
            {
                strcat(param->buffer_operand,"{");
                strcat(param->buffer_operand,current_equivalence->valueStr);   /* Equivalence */
                strcat(param->buffer_operand,"}");
            }
        }
        else
            strcat(param->buffer_operand,tab_element[i]);
    }

    /* Memory release of the table of values */
    mem_free_table(nb_element,tab_element);

    /** Replaces the Operand (if something has been changed) **/
    if(strcmp(param->buffer_operand,current_line->operand_txt))
    {
        /* New operand */
        new_operand = strdup(param->buffer_operand);
        if(new_operand == NULL)
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory to replace an Equivalence");

        /* Free the old */
        free(current_line->operand_txt);

        /* store the new */
        current_line->operand_txt = new_operand;

        /* Note that we modified the line */
        modified = 1;
    }

    /* Return if we changed the line */
    return(modified);
}


/*******************************************/
/*  IsLocalLabel() :  Is it a local label? */
/*******************************************/
static int IsLocalLabel(char *label_name, struct omf_segment *current_omfsegment)
{
    struct source_line *current_line = NULL;
    struct source_file *first_file = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Pass all lines in to review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We do not process Variables in Macro */
        if(current_line->type != LINE_CODE && current_line->type_aux != LINE_DATA)
            continue;

        /* Search for local ]label */
        if(strcmp(current_line->label_txt,label_name))
            return(1);
    }

    /* Not found */
    return(0);
}


/***********************************************************/
/*  BuildVariableTable() :  Construct the Variable Tables. */
/***********************************************************/
int BuildVariableTable(struct omf_segment *current_omfsegment)
{
    struct variable *current_variable = NULL;
    struct source_line *current_line = NULL;
    struct source_file *first_file = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /* Init */
    my_Memory(MEMORY_SORT_VARIABLE,NULL,NULL,current_omfsegment);

    /** Pass all lines in to review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We do not process Variables in Macro */
        if(current_line->type == LINE_DIRECTIVE && current_line->type_aux == LINE_MACRO_DEF)
            continue;

        /* We only take Lines with a label */
        if(strlen(current_line->label_txt) == 0)
            continue;

        /* We only take Variables ]XX = */
        if(current_line->type != LINE_VARIABLE)
            continue;

        /** Search for a variable of the same name already exist **/
        my_Memory(MEMORY_SEARCH_VARIABLE,current_line->label_txt,&current_variable,current_omfsegment);

        /** Allocate a new variable structure **/
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

            /* Create the structure */
            my_Memory(MEMORY_ADD_VARIABLE,current_variable,NULL,current_omfsegment);
        }

        /* This line is pointed at the variable */
        current_line->variable = current_variable;
    }

    /* Sort the Variables */
    my_Memory(MEMORY_SORT_VARIABLE,NULL,NULL,current_omfsegment);

    /* OK */
    return(0);
}


/***************************************************************/
/*  BuildReferenceTable() :  Construction of reference tables. */
/***************************************************************/
void BuildReferenceTable(struct omf_segment *current_omfsegment)
{
    /** Opcode **/
    for(int i=0; opcode_list[i]!=NULL; i++)
        my_Memory(MEMORY_ADD_OPCODE,opcode_list[i],NULL,current_omfsegment);
    my_Memory(MEMORY_SORT_OPCODE,NULL,NULL,current_omfsegment);

    /** Data **/
    for(int i=0; data_list[i]!=NULL; i++)
        my_Memory(MEMORY_ADD_DATA,data_list[i],NULL,current_omfsegment);
    my_Memory(MEMORY_SORT_DATA,NULL,NULL,current_omfsegment);

    /** Directive **/
    for(int i=0; directive_list[i]!=NULL; i++)
        my_Memory(MEMORY_ADD_DIRECTIVE,directive_list[i],NULL,current_omfsegment);
    my_Memory(MEMORY_SORT_DIRECTIVE,NULL,NULL,current_omfsegment);

    /** DirectiveEqu **/
    for(int i=0; equivalence_list[i]!=NULL; i++)
        my_Memory(MEMORY_ADD_DIREQU,equivalence_list[i],NULL,current_omfsegment);
    my_Memory(MEMORY_SORT_DIREQU,NULL,NULL,current_omfsegment);
}


/*******************************************************************/
/*  ProcessMXDirective() :  We will recognize the MX of each line. */
/*******************************************************************/
int ProcessMXDirective(struct omf_segment *current_omfsegment)
{
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    int is_reloc = 0;
    int64_t value = 0;
    char m  = '1', x = '1';
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct external *current_external = NULL;
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);

    /*** Process all lines ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /** New value of M and X **/
        if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"MX"))
        {
            /* Recover the value */
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

            /* Decode M and X */
            m = ((value & 0x02) == 0) ? '0' : '1';
            x = ((value & 0x01) == 0) ? '0' : '1';

            /** Put the MX values on the line **/
            current_line->m[0] = m;
            current_line->x[0] = x;
        }
        else if(current_line->type == LINE_CODE && (!my_stricmp(current_line->opcode_txt,"REP") || !my_stricmp(current_line->opcode_txt,"SEP")))
        {
            /* Recover the value */
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

            /* Decode M and X */
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

            /** Put the MX values on the line **/
            current_line->m[0] = m;
            current_line->x[0] = x;
        }
        else if(current_line->type == LINE_CODE && !my_stricmp(current_line->opcode_txt,"SEC") && current_line->next != NULL)
        {
            /** Put the MX values on the line **/
            current_line->m[0] = m;
            current_line->x[0] = x;

            /* We have an XCE that follows => 8 bit */
            if(current_line->next->is_valid == 1 && current_line->next->type == LINE_CODE && !my_stricmp(current_line->next->opcode_txt,"XCE"))
            {
                m = '1';
                x = '1';
            }
        }
        else
        {
            /** Put the MX values on the line **/
            current_line->m[0] = m;
            current_line->x[0] = x;
        }
    }

    /* OK */
    return(0);
}


/**********************************************************************/
/*  EvaluateVariableLine() :  Evaluation of the variable on its line. */
/**********************************************************************/
int EvaluateVariableLine(struct source_line *current_line, struct omf_segment *current_omfsegment)
{
    int64_t value = 0;
    int is_reloc = 0;
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    char buffer_error[1024] = "";
    struct variable *current_variable = NULL;
    struct external *current_external = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /** Look for the variable **/
    my_Memory(MEMORY_SEARCH_VARIABLE,current_line->label_txt,&current_variable,current_omfsegment);
    if(current_variable == NULL)
        return(0);

    /** Evaluate the variable **/
    value = EvalExpressionAsInteger(current_line->operand_txt,&buffer_error[0],current_line,4,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
    if(strlen(buffer_error) > 0)
    {
        sprintf(param->buffer_error,"Impossible to evaluate Variable '%s' value '%s' (line %d, file '%s') : %s",
                current_variable->name,current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

    /* Stores the value */
    current_variable->value = value;

    /* OK */
    return(0);
}


/***********************************************************************/
/*  ComputeLineAddress() : Determine the addresses of the valid lines. */
/***********************************************************************/
int ComputeLineAddress(struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    int line_number = 0, current_address = 0, global_address = 0, new_address = 0, dum_address = 0, nb_byte = 0, has_previous_label = 0, is_reloc = 0, is_first_org = 0, is_fix_address = 0;
    int current_bank = 0, global_bank = 0, new_bank = 0, dum_bank = 0;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct external *current_external = NULL;
    char *next_sep = NULL;
    char operand[1024];
    char buffer_error[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Init */
    if(current_omfproject->is_omf == 1)
    {
        /* The OMF code is relocatable */
        current_bank = 0;
        current_address = 0x0000;
        current_omfsegment->is_omf = 1;
        is_fix_address = 0;
    }
    else if(current_omfproject->is_single_binary == 1)
    {
        /* The ORG address is passed from Segment to Segment (which follow one another) */
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

    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(0);

    /** Number all lines **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* Set Line Number */
        current_line->line_number = line_number++;
    }

    /** Search an REL if we do not know yet what is the type of project **/
    if(current_omfproject->is_omf == 0 && current_omfproject->is_single_binary == 0 && current_omfproject->is_multi_fixed == 0)
    {
        for(current_line=first_file->first_line; current_line; current_line=current_line->next)
        {
            /* Invalid lines are ignored */
            if(current_line->is_valid == 0)
                continue;

            /** We are looking for the first REL **/
            if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"REL"))
            {
                /* We must not have anything before */
                if(has_previous_label == 1)
                {
                    sprintf(param->buffer_error,"Error : The REL directive should be located at the top of the file (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

                /* The assembly will start in $0000 */
                current_bank = 0;
                current_address = 0x0000;
                global_bank = 0;
                global_address = 0x0000;

                /* The file can be relocated in OMF format */
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
    
    /*** Process all lines ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /** Directive amending the address: ORG + DUM **/
        if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"ORG"))
        {
            /** Org $ Addr (for OMFs and SingleBinary, we have an area [ORG $ Addr - ORG] or fixed address) **/
            if(strlen(current_line->operand_txt) > 0)
            {
                /* Retrieve the new address */
                int64_t new_address_64 = EvalExpressionAsInteger(current_line->operand_txt, &buffer_error[0], current_line, 2, &is_reloc, &byte_count, &bit_shift, &offset_reference, &address_long, &current_external, current_omfsegment);
                if(strlen(buffer_error) > 0)
                {
                    sprintf(param->buffer_error,"Error : Impossible to evaluate ORG Address : '%s' (line %d, file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
                /* We only keep 32 bits */
                new_address = (int) (0xFFFFFFFF & new_address_64);

                /* We stay in the 64 KB */
                new_bank = new_address >> 16;
                new_address = new_address & 0xFFFF;

                /* new addresse */
                if( current_line->is_dum )
                {
                    /* in DUM section */
                    dum_bank = new_bank;
                    dum_address = new_address;
                }
                else
                {
                    /* in Code section */
                    current_line->bank = current_bank;
                    current_line->address = current_address;
                    current_line->is_fix_address = is_fix_address;
                    current_line->global_bank = global_bank;          /* Address without consideration of [ORG $ Addr ORG] */
                    current_line->global_address = global_address;
                    current_bank = new_bank;
                    current_address = new_address;
                }

                /* The first ORG is used to define the global address (for fixed address binaries) */
                if(is_first_org == 1 && current_omfproject->is_omf == 0 && current_omfproject->is_single_binary == 0)
                {
                    global_bank = new_bank;
                    global_address = new_address;
                    is_first_org = 0;
                }
                
                /* From now on all lines are in Fixed addresses => not relocatable */
                is_fix_address = 1;
                continue;
            }
            else    /* ORG */
            {
                /** The address of the global file is reestablished **/
                current_bank = global_bank;
                current_address = global_address;

                /* From now on all OMF / SingleBinary lines are no longer in Fixed addresses */
                if(current_omfproject->is_omf == 1 || current_omfproject->is_single_binary == 1)
                    is_fix_address = 0;
            }
        }
        else if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"DUM"))
        {
            /* Retrieve the new address */
            int64_t dum_address_64 = EvalExpressionAsInteger(current_line->operand_txt, &buffer_error[0], current_line, 2, &is_reloc, &byte_count, &bit_shift, &offset_reference, &address_long, &current_external, current_omfsegment);
            if(strlen(buffer_error) > 0)
            {
                sprintf(param->buffer_error,"Error : Impossible to evaluate DUM Address : '%s' (line %d, file '%s') : %s",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
            /* We only keep 32 bits */
            dum_address = (int) (0xFFFFFFFF & dum_address_64);

            /* We stay in the 64 KB */
            dum_bank = dum_address >> 16;
            dum_address = 0xFFFF & dum_address;
        }

        /* Put the running address on the line */
        if(current_line->is_dum == 1)
        {
            current_line->bank = dum_bank;
            current_line->address = dum_address;
            current_line->is_fix_address = is_fix_address;
            current_line->global_bank = global_bank;          /* Address without consideration of [ORG $ Addr ORG] */
            current_line->global_address = global_address;

            /* Define the following address */
            if(current_line->nb_byte == 0xFFFF)
            {
                /* Special case of DS lines: Alignment on the next $100 */
                nb_byte = 0x100 - (dum_address & 0x0000FF);
                current_line->nb_byte = nb_byte;
                dum_address += nb_byte;
            }
            else if(current_line->nb_byte == 0xFFFFF)
            {
                /** Special case of DS lines with Labels in: We try to reevaluate **/
                /* Isolate the expression indicating the length */
                strcpy(operand,current_line->operand_txt);
                next_sep = strchr(operand,',');
                if(next_sep)
                    *next_sep = '\0';

                /* Calculate the expression */
                nb_byte = (int) EvalExpressionAsInteger(operand,&buffer_error[0],current_line,3,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
                if(strlen(buffer_error) > 0)
                {
                    sprintf(param->buffer_error,"Error : Impossible to evaluate DS data size : '%s' (line %d, file '%s') : %s",operand, current_line->file_line_number, current_line->file->file_name, buffer_error);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
                if(nb_byte < 0)
                {
                    sprintf(param->buffer_error,"Error : Evaluation of DS data size ends up as negative value (%d) : '%s' (line %d, file '%s')",nb_byte, operand, current_line->file_line_number, current_line->file->file_name);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
                
                /* We finally have the size occupied by the line */
                current_line->nb_byte = nb_byte;
                dum_address += nb_byte;
            }
            else
            {
                /* We jump from the size of the instruction */
                dum_address += current_line->nb_byte;
            }

            /* Error: exceeds 64 KB */
            if(dum_address > 0xFFFF)
            {
                sprintf(param->buffer_error,"Error : DUM Object code size > 64 KB (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
        else
        {
            /* Address of the line */
            current_line->bank = current_bank;
            current_line->address = current_address;
            current_line->is_fix_address = is_fix_address;
            current_line->global_bank = global_bank;          /* Address without consideration of [ORG $ Addr ORG] */
            current_line->global_address = global_address;

            /* Define the following address */
            if(current_line->nb_byte == 0xFFFF)
            {
                /* Special case of DS lines: Alignment on the next $100 */
                nb_byte = 0x100 - (current_address & 0x0000FF);
                current_line->nb_byte = nb_byte;
                current_address += nb_byte;
                global_address += nb_byte;
            }
            else if(current_line->nb_byte == 0xFFFFF)
            {
                /** Special case of DS lines with Labels in: We try to reevaluate **/
                /* Isolate the expression indicating the length */
                strcpy(operand,current_line->operand_txt);
                next_sep = strchr(operand,',');
                if(next_sep)
                    *next_sep = '\0';

                /* Calculate the expression */
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
                
                /* We finally have the size occupied by the line */
                current_line->nb_byte = nb_byte;
                current_address += nb_byte;
                global_address += nb_byte;
            }
            else if(current_line->nb_byte > 0)
            {
                /* We jump from the size of the instruction */
                current_address += current_line->nb_byte;
                global_address += current_line->nb_byte;
            }

            /* Error: exceeds 64 KB */
            if(current_address > 0x10000)     /* @TODO: why is this labelled a bug? 0xFFFF */
            {
                sprintf(param->buffer_error,"Error : Object code size > 64 KB (line %d, file '%s')",current_line->file_line_number,current_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }

    /* OK */
    return(0);
}


/*********************************************************************/
/*  BuildRelocateAddress() :  An address is marked as to be patched. */
/*********************************************************************/
struct relocate_address *BuildRelocateAddress(BYTE ByteCnt, BYTE BitShiftCnt, WORD OffsetPatch, WORD OffsetReference, struct external *current_external, struct omf_segment *current_omfsegment)
{
    struct relocate_address *current_address = NULL;
    struct relocate_address *next_address = NULL;

    /* Allocate memory */
    current_address = (struct relocate_address *) calloc(1,sizeof(struct relocate_address));
    if(current_address == NULL)
        my_RaiseError(ERROR_RAISE,"Error : Can't allocate memory for relocate_address structure.");

    /* Fill out structure */
    current_address->ByteCnt = ByteCnt;
    current_address->BitShiftCnt = BitShiftCnt;
    current_address->OffsetPatch = OffsetPatch;
    current_address->OffsetReference = OffsetReference;

    /* Refer to a label external to the Segment */
    current_address->external = current_external;

    /* Attaches by sorting OffsetPath addresses */
    if(current_omfsegment->first_address == NULL)
    {
        current_omfsegment->first_address = current_address;
        current_omfsegment->last_address = current_address;
    }
    else
    {
        /* Add in 1st position */
        if(current_address->OffsetPatch < current_omfsegment->first_address->OffsetPatch)
        {
            current_address->next = current_omfsegment->first_address;
            current_omfsegment->first_address = current_address;
        }
        else if(current_address->OffsetPatch >= current_omfsegment->last_address->OffsetPatch)
        {
            /* Tie in last position */
            current_omfsegment->last_address->next = current_address;
            current_omfsegment->last_address = current_address;
        }
        else
        {
            /* Attach in the middle */
            for(next_address=current_omfsegment->first_address; ; next_address=next_address->next)
                if(next_address->next->OffsetPatch >= current_address->OffsetPatch)
                {
                    current_address->next = next_address->next;
                    next_address->next = current_address;
                    break;
                }
        }
    }

    /* go to next address */
    current_omfsegment->nb_address++;

    return(current_address);
}


/************************************************************/
/*  CheckForUnknownLine() :  Search all unidentified lines. */
/************************************************************/
int CheckForUnknownLine(struct omf_segment *current_omfsegment)
{
    int nb_error = 0;
    struct source_line *current_line = NULL;
    struct source_file *first_file = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Pass all lines in to review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We do not take Label in Macro */
        if(current_line->type == LINE_UNKNOWN)
        {
            printf("      => [Error] Unkown line : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
            nb_error++;
        }
    }

    /* Returns the number of Error detected */
    return(nb_error);
}


/***************************************************/
/*  CheckForDumLine() :  Check all DUM-DEND lines. */
/***************************************************/
int CheckForDumLine(struct omf_segment *current_omfsegment)
{
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct source_line *dend_line = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Pass all lines in to review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /* We will search the DUM-DEND areas */
        if(current_line->type == LINE_DIRECTIVE && !my_stricmp(current_line->opcode_txt,"DUM"))
        {
            /* Check for the presence of an Operand */
            if(strlen(current_line->operand_txt) == 0)
            {
                printf("      => [Error] Empty DUM line : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
                return(1);
            }

            /* We will search the DEND and we mark all the lines between */
            for(dend_line=current_line; dend_line; dend_line=dend_line->next)
            {
                /* Invalid lines are ignored */
                if(dend_line->is_valid == 0)
                    continue;

                /* Mark the line */
                dend_line->is_dum = 1;
                if(dend_line != current_line)
                    dend_line->dum_line = current_line;

                if(dend_line->type == LINE_DIRECTIVE && !my_stricmp(dend_line->opcode_txt,"DEND"))
                    break;

                /* We should not fall back on a DUM */
                if(current_line != dend_line && dend_line->type == LINE_DIRECTIVE && !my_stricmp(dend_line->opcode_txt,"DUM"))
                {
                    printf("      => [Error] DUM line with DUM found before DEND : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
                    return(1);
                }
            }

            /* Past the DEND? */
            if(dend_line == NULL)
            {
                printf("      => [Error] DUM line without DEND : '%s  %s  %s' in file '%s' (line %d).\n",current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->file->file_name,current_line->file_line_number);
                return(1);
            }

            /* We continue after the DEND line */
            current_line = dend_line;
        }
    }

    /* OK */
    return(0);
}


/**************************************************/
/*  CheckForErrLine() :  Evaluates all ERR lines. */
/**************************************************/
int CheckForErrLine(struct omf_segment *current_omfsegment)
{
    int64_t value = 0;
    int is_reloc = 0;
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    char buffer_error[1024] = "";
    struct external *current_external = NULL;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Pass all the ERR lines in review **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /** Search for ERR lines **/
        if(current_line->type == LINE_CODE && !my_stricmp(current_line->opcode_txt,"ERR"))
        {
            /* We pass the line in Directive for the Output file */
            current_line->type = LINE_DIRECTIVE;

            /* Check for the presence of an Operand, otherwise we consider that there is no Error */
            if(strlen(current_line->operand_txt) == 0)
                continue;

            /** Evaluater the operand **/
            value = EvalExpressionAsInteger(current_line->operand_txt, &buffer_error[0], current_line, 4, &is_reloc, &byte_count, &bit_shift, &offset_reference, &address_long, &current_external, current_omfsegment);
            if(strlen(buffer_error) > 0)
            {
                /* Error in the evaluation */
                printf("      => [Error] Impossible to evaluate ERR expression '%s' (line %d, file '%s') : %s\n",current_line->operand_txt,current_line->file_line_number,current_line->file->file_name,buffer_error);
                return(1);
            }

            /** If it's different from Zero, it's an Error **/
            if((int) value != 0)
            {
                /* Force Error */
                printf("      => [Error] The evaluation of ERR expression '%s' is '0x%X' (line %d, file '%s')\n",current_line->operand_txt,(int)value,current_line->file_line_number,current_line->file->file_name);
                return(1);
            }
        }
    }

    /* OK */
    return(0);
}


/*************************************************************/
/*  CheckForDirectPageLine() :  Check all Direct Page lines. */
/*************************************************************/
int CheckForDirectPageLine(struct omf_segment *current_omfsegment)
{
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Review all Direct Page lines **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /** We will search the lines of code Page Direct **/
        if(current_line->type == LINE_CODE && IsPageDirectAddressMode(current_line->address_mode))
        {
            /* Check for the presence of a valid Operand */
            if(current_line->operand_value == 0xFFFFFFFF)
                continue;

            /** If the operand is not an address Page Redirect => Error **/
            if((current_line->operand_value & 0xFFFFFF00) != 0x00000000)
            {
                /* Force Error */
                printf("      => [Bad Address Mode] Operand address '%s' (=0x%X) is located outside of the Direct Page (line %d, file '%s')\n",current_line->operand_txt,current_line->operand_value,current_line->file_line_number,current_line->file->file_name);
                return(1);
            }
        }
    }

    /* OK */
    return(0);
}


/**************************************************************************************************/
/*  ProcessDirectiveWithLabelLine() :  Conversion of Directive Lines with Label into Empty Lines. */
/**************************************************************************************************/
int ProcessDirectiveWithLabelLine(struct omf_segment *current_omfsegment)
{
    int found = 0;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct source_line *other_line = NULL;

    /* Recover the Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Review all Direct Page lines **/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Invalid lines are ignored */
        if(current_line->is_valid == 0)
            continue;

        /** Search lines for DIRECTIVE **/
        if(current_line->type == LINE_DIRECTIVE && current_line->is_inside_macro == 0 && strlen(current_line->label_txt) > 0)
        {
            /* We check that the label is used = pointed by another line */
            for(other_line=first_file->first_line,found=0; other_line; other_line=other_line->next)
            {
                /* Invalid lines are ignored */
                if(other_line->is_valid == 0)
                    continue;
                
                /* This line points to the DIRECTIVE line */
                if(other_line->operand_address_long == (DWORD)current_line->address)
                {
                    found = 1;
                    break;
                }
            }

            /** We pass the online line EMPTY for it to be displayed in the output **/
            if(found == 1)
                current_line->type = LINE_EMPTY;
        }
    }

    /* OK */
    return(0);
}

/*************************************************/
/*  BuildSourceLine() :  Decoding a Source Line. */
/*************************************************/
struct source_line *BuildSourceLine(struct source_file *current_file, int line_number)
{
    struct source_line *current_line = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Allocate memory */
    current_line = (struct source_line *) calloc(1,sizeof(struct source_line));
    if(current_line == NULL)
        return(NULL);
    current_line->type = LINE_UNKNOWN;

    /* Fill out structure */
    current_line->file_line_number = line_number+1;
    current_line->line_data = current_file->tab_line[line_number];
    current_line->file = current_file;

    /* This line was in the Source file (unlike lines coming from Macro or Lup) */
    current_line->is_in_source = 1;

    /* By default, any line is valid */
    current_line->is_valid = 1;

    /* Values of E, C, M and X */
    strcpy(current_line->m,"?");
    strcpy(current_line->x,"?");

    /* Reloc */
    strcpy(current_line->reloc,"         ");

    /* Address */
    current_line->address = -1;    /* Not yet determined */

    /* Object code size */
    current_line->nb_byte = -1;    /* Not yet determined */

    /* Does the line forbid  Direct Page? */
    current_line->no_direct_page = 0;

    /* Value of the Operand */
    current_line->operand_value = 0xFFFFFFFF;

    /* Long address of the label pointed by the operand  */
    current_line->operand_address_long = 0xFFFFFFFF;

    /** Comment lines **/
    strcpy(param->buffer_line,current_line->line_data);
    CleanBuffer(param->buffer_line);
    if(strlen(param->buffer_line) == 0 || param->buffer_line[0] == ';' || param->buffer_line[0] == '*')
    {
        /* Empty the fields */
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

    /*** Cutting the line in 4 blocks: Label / Opcode / Operand / Comment ***/
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

    /* Return line */
    return(current_line);
}


/*******************************************************/
/*  DuplicateSourceLine() :  Duplicate a line of Code. */
/*******************************************************/
struct source_line *DuplicateSourceLine(struct source_line *current_line)
{
    /* Allocate memory */
    struct source_line *new_line = (struct source_line *) calloc(1,sizeof(struct source_line));
    if(new_line == NULL)
        return(NULL);

    /* Copy the values */
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

    /* Duplicate some values */
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

    /* Returns the line */
    return(new_line);
}


/*********************************************************************/
/*  BuildEmptyLabelLine() :  Create an empty Code line with a Label. */
/*********************************************************************/
struct source_line *BuildEmptyLabelLine(char *label, struct source_line *current_line)
{
    /* Allocate memory */
    struct source_line *new_line = (struct source_line *) calloc(1,sizeof(struct source_line));
    if(new_line == NULL)
        return(NULL);

    /* Copy the values */
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

    /* Duplicate some values */
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

    /* Returns the line */
    return(new_line);
}


/*****************************************************************/
/*  DecodeLine() :  Decodes a line by separating the 4 elements. */
/*****************************************************************/
void DecodeLine(char *line_data, char *label_rtn, char *opcode_rtn, char *operand_rtn, char *comment_rtn)
{
    int has_data = 0, nb_separator = 0;
    struct item *all_item = NULL;
    struct item *current_item = NULL;
    struct item *opcode_item = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Init */
    strcpy(label_rtn,"");
    strcpy(opcode_rtn,"");
    strcpy(operand_rtn,"");
    strcpy(comment_rtn,"");

    /** Cut the line into separate elements by spaces or tabs (taking into account the comments and the chains '' or '') **/
    all_item = ExtractAllIem(line_data);
    if(all_item == NULL)
        return;                 /* Empty line */

    /** Special cases **/
    for(has_data=0, current_item = all_item; current_item; current_item = current_item->next)
    {
        if(current_item->type == TYPE_DATA)
        {
            has_data = 1;
            break;
        }
    }

    // if we didn't find a TYPE_DATA then we are done
    if(has_data == 0)
    {
        mem_free_item_list(all_item);
        return;                 /* Empty line */
    }

    /** Special case: Comment line **/
    for(current_item = all_item; current_item; current_item = current_item->next)
    {
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
    }

    /** Comment: Value that starts with one ; **/
    for(current_item = all_item; current_item->next; current_item = current_item->next)
    {
        if(current_item->next->type == TYPE_DATA && current_item->next->name[0] == ';')
        {
            /* Keep the comment, free more */
            strcpy(comment_rtn,current_item->next->name);
            mem_free_item_list(current_item->next);
            current_item->next = NULL;
            break;
        }
    }

    /** Label: All that is glued to the left **/
    if(all_item->type == TYPE_DATA)
        strcpy(label_rtn,all_item->name);

    /** Opcode: DATA after the first separator **/
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

    /** Operand: What's left (the comment section has already been deleted) **/
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

    /* Memory release */
    mem_free_item_list(all_item);

    /** We remove spaces and tabs surrounding values **/
    CleanBuffer(label_rtn);
    CleanBuffer(opcode_rtn);
    CleanBuffer(operand_rtn);
    CleanBuffer(comment_rtn);
}


/***************************************************************/
/*  AddDateLine() :  Adding a Date line for the DAT directive. */
/***************************************************************/
static void AddDateLine(struct source_line *current_line)
{
    struct source_line *new_line = NULL;
    char buffer[256];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* We will decode the type of Date requested */
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
    
    /** Creation of a DATA Line **/
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

    /* New values */
    new_line->type = LINE_DATA;
    new_line->opcode_txt = strdup("ASC");
    new_line->operand_txt = strdup(buffer);
    if(new_line->opcode_txt == NULL || new_line->operand_txt == NULL)
    {
        mem_free_sourceline(new_line);
        printf("      => [Error] Impossible to allocate memory to insert DAT value (line %d, file '%s')\n",current_line->file_line_number,current_line->file->file_path);
        return;
    }

    /* Insert the line after the DAT line */
    new_line->next = current_line->next;
    current_line->next = new_line;
}


/**************************************************************************/
/*  mem_free_sourceline() :  Memory release of the source_line structure. */
/**************************************************************************/
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
/*  mem_free_sourceline_list() :  Free the source line list. */
/*******************************************************************/
void mem_free_sourceline_list(struct source_line *first_sourceline)
{
    /** Release the structured chain list **/
    for(struct source_line *current_sourceline = first_sourceline; current_sourceline; )
    {
        struct source_line *next_sourceline = current_sourceline->next;
        mem_free_sourceline(current_sourceline);
        current_sourceline = next_sourceline;
    }
}


/***************************************************************/
/*  mem_free_label() :  Memory release of the label structure. */
/***************************************************************/
void mem_free_label(struct label *current_label)
{
    if(current_label)
    {
        if(current_label->name)
            free(current_label->name);

        free(current_label);
    }
}


/*********************************************************************/
/*  mem_free_variable() :  Memory release of the variable structure. */
/*********************************************************************/
void mem_free_variable(struct variable *current_variable)
{
    if(current_variable)
    {
        if(current_variable->name)
            free(current_variable->name);

        free(current_variable);
    }
}


/***************************************************************************/
/*  mem_free_equivalence() :  Memory release of the equivalence structure. */
/***************************************************************************/
void mem_free_equivalence(struct equivalence *current_equivalence)
{
    if(current_equivalence)
    {
        if(current_equivalence->name)
            free(current_equivalence->name);

        if(current_equivalence->valueStr)
            free(current_equivalence->valueStr);

        free(current_equivalence);
    }
}


/*********************************************************************/
/*  mem_free_external() :  Memory release of the structure external. */
/*********************************************************************/
void mem_free_external(struct external *current_external)
{
    if(current_external)
    {
        if(current_external->name)
            free(current_external->name);

        free(current_external);
    }
}


/******************************************************************/
/*  mem_free_global() :  Memory release of the overall structure. */
/******************************************************************/
void mem_free_global(struct global *current_global)
{
    if(current_global)
    {
        if(current_global->name)
            free(current_global->name);

        free(current_global);
    }
}
