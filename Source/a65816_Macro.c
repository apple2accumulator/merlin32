/***********************************************************************/
/*                                                                     */
/*  a65816_Macro.c : Module for Macros Management.                     */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include "Dc_Library.h"

#include "a65816_Macro.h"

#include "a65816_File.h"
#include "a65816_Line.h"

void LoadOneMacroFile(char *,char *,struct source_line *,struct omf_segment *);
static struct source_line *BuildMacroLine(struct source_line *,struct omf_segment *,struct omf_project *);
static struct source_line *BuildSourceMacroLine(struct source_line *,struct macro_line *,char **,struct omf_segment *);
static void BuildSubstituteValue(char *,char **,char *);
struct macro *mem_alloc_macro(char *,char *,int);
struct macro_line *mem_alloc_macroline(char *,char *,char *,char *);

/*******************************************************/
/*  LoadAllMacroFile() :  Loading of all Macros Files. */
/*******************************************************/
void LoadAllMacroFile(char *folder_path, struct omf_segment *current_omfsegment)
{
    int i, nb_file, is_error;
    char **tab_file_name;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /** Retrieve the Files of Directory **/
    tab_file_name = GetFolderFileList(folder_path,&nb_file,&is_error);
    if(tab_file_name == NULL && is_error == 0)
        return;
    if(tab_file_name == NULL && is_error == 1)
    {
        printf("    => Error, Can't get files list from Macro folder '%s'.\n",folder_path);
        return;
    }

    /* Prepare the name of folder */
    strcpy(param->buffer_folder_path,folder_path);
    if(strlen(param->buffer_folder_path) > 0)
        if(param->buffer_folder_path[strlen(param->buffer_folder_path)-1] != '\\' && param->buffer_folder_path[strlen(param->buffer_folder_path)-1] != '/')
            strcat(param->buffer_folder_path,FOLDER_SEPARATOR);

    /** We load all the Files .s present **/
    for(i=0; i<nb_file; i++)
    {
        /* We check the .s at the end */
        if(strlen(tab_file_name[i]) < 3)
            continue;
        if(my_stricmp(&(tab_file_name[i][strlen(tab_file_name[i])-2]),".s"))
            continue;
        
        /** Load the File **/
        LoadOneMacroFile(param->buffer_folder_path,tab_file_name[i],NULL,current_omfsegment);
    }
    
    /* Memory release */
    mem_free_list(nb_file,tab_file_name);
}


/*************************************************************/
/*  LoadSourceMacroFile() :  Loading Macros Files of Source. */
/*************************************************************/
void LoadSourceMacroFile(char *macro_folder_path, struct omf_segment *current_omfsegment)
{
    int i;
    struct source_file *first_file;
    struct source_line *current_line;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Récupère le Source */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /*** Pass all lines in to review ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* Already known the line */
        if(current_line->type != LINE_UNKNOWN)
            continue;

        /** On recherche les Lines USE **/
        if(!my_stricmp(current_line->opcode_txt,"USE") && strlen(current_line->operand_txt) > 0)
        {
            /* Ce File était can be un File contenant of code */
            if(!IsMacroFile(current_line->operand_txt,param->source_folder_path,macro_folder_path))
                continue;

            /* On va extraire le File name : 4/Locator.Macs => Locator.Macs.s */
            for(i=(int)strlen(current_line->operand_txt); i>=0; i--)
                if(current_line->operand_txt[i] == '/' || current_line->operand_txt[i] == ':')
                    break;
            strcpy(param->buffer_file_name,&current_line->operand_txt[i+1]);

            /* Ajoute le .s final */
            if(my_stricmp(&param->buffer_file_name[strlen(param->buffer_file_name)-2],".s"))
                strcat(param->buffer_file_name,".s");

            /** Load the File Macro **/
            printf("        - %s\n",param->buffer_file_name);
            LoadOneMacroFile(macro_folder_path,param->buffer_file_name,current_line,current_omfsegment);
        }
    }
}


/*********************************************************/
/*  LoadOneMacroFile() :  Chargement d'un File Macro. */
/*********************************************************/
void LoadOneMacroFile(char *folder_path, char *file_name, struct source_line *macro_line, struct omf_segment *current_omfsegment)
{
    size_t data_size = 0;
    int macro_level = 0, line_number = 0;
    char *data = NULL;
    char *begin_line = NULL;
    char *end_line = NULL;
    char *next_line = NULL;
    struct macro *first_macro = NULL;
    struct macro *last_macro = NULL;
    struct macro *current_macro = NULL;
    struct macro_line *current_line = NULL;
    struct equivalence *current_equivalence = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* File Path */
    sprintf((char *)param->buffer,"%s%s",folder_path,file_name);

    /* Loading the file in memory (search in the Library folder) */
    data = (char *) LoadTextFileData((char *)param->buffer,&data_size);
    if(data == NULL)
    {
        /* We will look in the sources folder */
        sprintf((char *)param->buffer,"%s%s",param->source_folder_path,file_name);
        data = (char *) LoadTextFileData((char *)param->buffer,&data_size);
        if(data == NULL)
        {
            if(macro_line != NULL)
                sprintf(param->buffer_error,"Macro file '%s' not found (Source file '%s', line %d : use %s)",file_name,macro_line->file->file_name,macro_line->file_line_number,macro_line->operand_txt);
            else
                sprintf(param->buffer_error,"Impossible to load Macro file '%s'",param->buffer);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }
    }
    my_Memory(MEMORY_DECLARE_ALLOC,data,NULL,current_omfsegment);

    /** Process all Lines of File **/
    line_number = 0;
    begin_line = data;
    while(begin_line)
    {
        /* End of the line */
        end_line = strchr(begin_line,'\n');
        if(end_line)
            *end_line = '\0';
        next_line = (end_line == NULL) ? NULL : end_line + 1;

        /* Empty line */
        if(strlen(begin_line) == 0 || begin_line[0] == ';' || begin_line[0] == '*')
        {
            begin_line = next_line;
            line_number++;
            continue;
        }

        /** We are looking for MAC Lines **/
        DecodeLine(begin_line,param->buffer_label,param->buffer_opcode,param->buffer_operand,param->buffer_comment);
        if(!my_stricmp(param->buffer_opcode,"MAC"))
        {
            /* Macro Level */
            macro_level++;

            /* new Macro */
            current_macro = mem_alloc_macro(file_name,param->buffer_label,line_number);
            if(current_macro == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to register macro '%s' from file '%s'",param->buffer_label,file_name);
                mem_free_macro_list(first_macro);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Attach the macro */
            if(first_macro == NULL || last_macro == NULL)
                first_macro = current_macro;
            else
                last_macro->next = current_macro;
            last_macro = current_macro;
        }
        else if((!my_stricmp(param->buffer_opcode,"<<<") || !my_stricmp(param->buffer_opcode,"EOM")) && macro_level > 0)
        {
            /** If this line contains a Label, it is integrated in the macro as Empty line **/
            if(strlen(param->buffer_label) > 0)
            {
                for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
                {
                    /* Creation of the line */
                    current_line = mem_alloc_macroline(param->buffer_label,"","",param->buffer_comment);
                    if(current_line == NULL)
                    {
                        sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,param->buffer_label,param->buffer_opcode,param->buffer_operand);
                        mem_free_macro_list(first_macro);
                        my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }

                    /* Attachement */
                    if(current_macro->first_line == NULL)
                        current_macro->first_line = current_line;
                    else
                        current_macro->last_line->next = current_line;
                    current_macro->last_line = current_line;
                }
            }
            
            /** We will finish all macros in progress **/
            for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
                my_Memory(MEMORY_ADD_MACRO,current_macro,NULL,current_omfsegment);

            /* Init */
            first_macro = NULL;

            /* Macro Level */
            macro_level = 0;
        }
        else if(macro_level > 0)
        {
            /** Add this macro_line to the saved macros **/
            for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
            {
                /* Creation of the line */
                current_line = mem_alloc_macroline(param->buffer_label,param->buffer_opcode,param->buffer_operand,param->buffer_comment);
                if(current_line == NULL)
                {
                    sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,param->buffer_label,param->buffer_opcode,param->buffer_operand);
                    mem_free_macro_list(first_macro);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

                /* Attachement */
                if(current_macro->first_line == NULL)
                    current_macro->first_line = current_line;
                else
                    current_macro->last_line->next = current_line;
                current_macro->last_line = current_line;
            }
        }
        else
        {
            /** We are outside the definition of a Macro, there may be some EQU (Util.Macs.s) **/
            if((!my_stricmp(param->buffer_opcode,"=") || !my_stricmp(param->buffer_opcode,"EQU")) && strlen(param->buffer_label) > 0)
            {
                /** Allocation of the structure Equivalence **/
                current_equivalence = (struct equivalence *) calloc(1,sizeof(struct equivalence));
                if(current_equivalence == NULL)
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure equivalence");
                current_equivalence->name = strdup(param->buffer_label);
                current_equivalence->value = strdup(param->buffer_operand);
                if(current_equivalence->name == NULL || current_equivalence->value == NULL)
                {
                    mem_free_equivalence(current_equivalence);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure equivalence");
                }

                /* Create the structure */
                my_Memory(MEMORY_ADD_EQUIVALENCE,current_equivalence,NULL,current_omfsegment);
            }
        }

        /* Next line */
        begin_line = next_line;
        line_number++;
    }

    /* Memory release */
    my_Memory(MEMORY_FREE_ALLOC,data,NULL,current_omfsegment);
}


/***********************************************************/
/*  GetMacroFromSource() :  Retrieves Source Files macros. */
/***********************************************************/
void GetMacroFromSource(struct omf_segment *current_omfsegment)
{
    struct macro *first_macro = NULL;
    struct macro *last_macro = NULL;
    struct macro *current_macro = NULL;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct macro_line *current_macroline = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Recover the 1st source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return;

    /*** Process all lines ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        if(!my_stricmp(current_line->opcode_txt,"MAC"))
        {
            /* new Macro */
            current_macro = mem_alloc_macro(current_line->file->file_name,current_line->label_txt,current_line->file_line_number);
            if(current_macro == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to register macro '%s' from file '%s'",current_line->label_txt,current_line->file->file_name);
                mem_free_macro_list(first_macro);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }

            /* Attach the macro */
            if(first_macro == NULL || last_macro == NULL)
                first_macro = current_macro;
            else
                last_macro->next = current_macro;
            last_macro = current_macro;

            /* This line is of type definition of macro */
            current_line->type = LINE_DIRECTIVE;
            current_line->type_aux = LINE_MACRO_DEF;

            /* This is a Macro declared in the Source */
            current_line->is_inside_macro = 1;
        }
        else if(!my_stricmp(current_line->opcode_txt,"<<<") || !my_stricmp(current_line->opcode_txt,"EOM"))
        {
            /** If this line contains a Label, it is integrated in the macro as Empty line **/
            if(strlen(current_line->label_txt) > 0)
            {
                for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
                {
                    /* Creation of the line */
                    current_macroline = mem_alloc_macroline(current_line->label_txt,"","",current_line->comment_txt);
                    if(current_macroline == NULL)
                    {
                        sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,current_line->label_txt,current_line->opcode_txt,current_line->operand_txt);
                        mem_free_macro_list(first_macro);
                        my_RaiseError(ERROR_RAISE,param->buffer_error);
                    }

                    /* Attachement */
                    if(current_macro->first_line == NULL)
                        current_macro->first_line = current_macroline;
                    else
                        current_macro->last_line->next = current_macroline;
                    current_macro->last_line = current_macroline;
                }
            }

            /** We will finish all macros in progress **/
            for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
                my_Memory(MEMORY_ADD_MACRO,current_macro,NULL,current_omfsegment);

            /* Init */
            first_macro = NULL;

            /* This line is of type definition of macro */
            current_line->type = LINE_DIRECTIVE;
            current_line->type_aux = LINE_MACRO_DEF;

            /* This is a Macro declared in the Source */
            current_line->is_inside_macro = 1;
        }
        else if(first_macro != NULL)
        {
            /** Add this macro_line to the saved macros **/
            for(current_macro=first_macro; current_macro; current_macro=current_macro->next)
            {
                /* Creation of the line */
                current_macroline = mem_alloc_macroline(current_line->label_txt,current_line->opcode_txt,current_line->operand_txt,current_line->comment_txt);
                if(current_line == NULL)
                {
                    sprintf(param->buffer_error,"Impossible to allocate memory to process macro '%s' line '%s  %s  %s'",current_macro->name,current_line->label_txt,current_line->opcode_txt,current_line->operand_txt);
                    mem_free_macro_list(first_macro);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }

                /* Attachement */
                if(current_macro->first_line == NULL)
                    current_macro->first_line = current_macroline;
                else
                    current_macro->last_line->next = current_macroline;
                current_macro->last_line = current_macroline;
            }

            /* This line is of type definition of macro */
            current_line->type = LINE_DIRECTIVE;
            current_line->type_aux = LINE_MACRO_DEF;

            /* This is a Macro declared in the Source */
            current_line->is_inside_macro = 1;
        }
    }
}


/***********************************************************************/
/*  CheckForDuplicatedMacro() :  Search for Macro with the same name. */
/***********************************************************************/
void CheckForDuplicatedMacro(struct omf_segment *current_omfsegment)
{
    int i, nb_macro;
    struct macro *previous_macro = NULL;
    struct macro *current_macro;

    /** Passe en revue toutes les Macro **/
    my_Memory(MEMORY_GET_MACRO_NB,&nb_macro,NULL,current_omfsegment);
    for(i=1; i<=nb_macro; i++)
    {
        my_Memory(MEMORY_GET_MACRO,&i,&current_macro,current_omfsegment);
        if(current_macro != NULL && previous_macro != NULL)
            if(!strcmp(current_macro->name,previous_macro->name))
                printf("    [Warning] Macro : '%s' found in '%s' and '%s'\n",current_macro->name,previous_macro->file_name,current_macro->file_name);

        previous_macro = current_macro;
    }
}


/******************************************************************/
/*  ReplaceMacroWithContent() :  Replaces Macros with their code. */
/******************************************************************/
int ReplaceMacroWithContent(struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    struct source_file *first_file;
    struct source_line *current_line;
    struct source_line *first_macro_line;
    struct source_line *last_macro_line;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

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

        /** Search the calls of Macro **/
        if(current_line->type == LINE_MACRO)
        {
            /* Construct the code lines of this Macro by inserting the parameters and replacing the Labels */
            first_macro_line = BuildMacroLine(current_line,current_omfsegment,current_omfproject);
            if(first_macro_line == NULL)
            {
                /* Error */
                return(1);
            }
            /* Last line of la Macro */
            for(last_macro_line = first_macro_line; last_macro_line->next != NULL; last_macro_line=last_macro_line->next)
                ;

            /** Insert the lines lines behind the call of the Macro **/
            last_macro_line->next = current_line->next;
            current_line->next = first_macro_line;

            /* Next line */
            current_line = last_macro_line;
        }
    }

    /* OK */
    return(0);
}


/**************************************************************/
/*  BuildMacroLine() :  	Creating code lines from a Macro. */
/**************************************************************/
static struct source_line *BuildMacroLine(struct source_line *current_source_line, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    struct macro *current_macro;
    struct source_line *new_source_line = NULL;
    struct source_line *label_source_line = NULL;
    struct source_line *first_source_line = NULL;
    struct source_line *last_source_line = NULL;
    struct source_line *first_source_macro_line;
    struct source_line *last_source_macro_line;
    struct macro_line *current_macro_line = NULL;
    char *next_sep;
    char *new_operand;
    char *var_list = NULL;
    int i, j, nb_error, nb_var = 0;
    char **var_tab;
    char label_unique[256];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    current_macro = current_source_line->macro;

    /****************************************/
    /**  Extract the variables of the call  **/
    /****************************************/
    /* Init */
    var_tab = (char **) calloc(9,sizeof(char *));
    if(var_tab == NULL)
    {
        sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Build Line]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
    for(i=0; i<9; i++)
    {
        var_tab[i] = (char *) calloc(256,sizeof(char));
        if(var_tab[i] == NULL)
        {
            for(j=0; j<i; j++)
                free(var_tab[j]);
            free(var_tab);
            sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Build Line]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }
    }

    /** We position ourselves at the beginning of the parameters **/
    if(!my_stricmp(current_source_line->opcode_txt,"PMC") || !my_stricmp(current_source_line->opcode_txt,">>>"))
    {
        /* The name of the Macro and the parameters are pasted into the Operand */
        for(i=0; i<(int)strlen(current_source_line->operand_txt); i++)
            if(current_source_line->operand_txt[i] == '.' || current_source_line->operand_txt[i] == '/' || current_source_line->operand_txt[i] == ',' ||
               current_source_line->operand_txt[i] == '-' || current_source_line->operand_txt[i] == '(' || current_source_line->operand_txt[i] == ' ')
            {
                var_list = &current_source_line->operand_txt[i+1];
                break;
            }
    }
    else
        var_list = strlen(current_source_line->operand_txt) == 0 ? NULL : current_source_line->operand_txt;

    /** There are only 8 maximum variables separated by ; **/
    while(var_list)
    {
        next_sep = strchr(var_list,';');

        /* Latest entries */
        if(next_sep == NULL)
        {
            strcpy(var_tab[nb_var+1],var_list);
            nb_var++;
            break;
        }
        
        /* Copy the entry */
        memcpy(var_tab[nb_var+1],var_list,next_sep-var_list);
        var_tab[nb_var+1][next_sep-var_list] = '\0';
        nb_var++;

        /* advance to next list */
        var_list += (next_sep-var_list+1);
    }
    sprintf(var_tab[0],"%d",nb_var);

    /*** Duplication of the Lines of the Macro ***/
    for(current_macro_line=current_macro->first_line; current_macro_line; current_macro_line=current_macro_line->next)
    {
        /* Creation of the line Source */
        new_source_line = BuildSourceMacroLine(current_source_line,current_macro_line,var_tab,current_omfsegment);
        if(new_source_line == NULL)
        {
            for(i=0; i<9; i++)
                free(var_tab[i]);
            free(var_tab);
            sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Build Line]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

        /* Attachment of the Source line */
        if(first_source_line == NULL)
            first_source_line = new_source_line;
        else
            last_source_line->next = new_source_line;
        last_source_line = new_source_line;
    }

    /* Memory release */
    for(i=0; i<9; i++)
        free(var_tab[i]);
    free(var_tab);

    /*** We will modify the Global Labels to get something unique ***/
    for(new_source_line=first_source_line; new_source_line; new_source_line=new_source_line->next)
    {
        /* We will only treat Global Labels */
        if(strlen(new_source_line->label_txt) > 0 && new_source_line->label_txt[0] != ':' && new_source_line->label_txt[0] != ']')
        {
            /* Creation of a unique label */
            GetUNID(&label_unique[0]);

            /** Pass all lines in to review **/
            for(label_source_line=first_source_line; label_source_line; label_source_line=label_source_line->next)
            {
                /** Replaces the Label in the Operand **/
                new_operand = ReplaceInOperand(label_source_line->operand_txt,new_source_line->label_txt,label_unique,SEPARATOR_REPLACE_VARIABLE,label_source_line);
                if(new_operand != label_source_line->operand_txt)
                {
                    free(label_source_line->operand_txt);
                    label_source_line->operand_txt = new_operand;
                }
            }

            /* It is replaced in the line that has defined it */
            free(new_source_line->label_txt);
            new_source_line->label_txt = strdup(label_unique);
            if(new_source_line->label_txt == NULL)
            {
                sprintf(param->buffer_error,"Impossible to allocate memory to replace macro '%s' at line %d from file '%s' [Update Label]",current_macro->name,current_source_line->file_line_number,current_source_line->file->file_name);
                my_RaiseError(ERROR_RAISE,param->buffer_error);
            }
        }
    }

    /** We will analyze the new Lines to determine their types **/
    nb_error = DecodeLineType(first_source_line,current_macro,current_omfsegment,current_omfproject);
    if(nb_error > 0)
    {
        /* Error: We erase everything and we exit in Error */
        mem_free_sourceline_list(first_source_line);
        return(NULL);
    }

    /** We have detected a macro => We must substitute this call with the code lines (recursivity) **/
    for(new_source_line=first_source_line; new_source_line; new_source_line=new_source_line->next)
    {
        if(new_source_line->type == LINE_MACRO)
        {
            /** Create the substituted lines **/
            first_source_macro_line = BuildMacroLine(new_source_line,current_omfsegment,current_omfproject);
            if(first_source_macro_line == NULL)
            {
                /* Error: We erase everything and we exit in Error */
                mem_free_sourceline_list(first_source_line);
                return(NULL);
            }
            /* We position ourselves at the end */
            for(last_source_macro_line=first_source_macro_line; last_source_macro_line->next != NULL; last_source_macro_line = last_source_macro_line->next)
                ;

            /** Insert substituted Lines in their place **/
            last_source_macro_line->next = new_source_line->next;
            new_source_line->next = first_source_macro_line;

            /* Next line */
            new_source_line = last_source_macro_line;
        }
    }
    
    /* Return the Lines */
    return(first_source_line);
}


/**********************************************************************/
/*  BuildSourceMacroLine() :  Building a line of Source from a Macro. */
/**********************************************************************/
static struct source_line *BuildSourceMacroLine(struct source_line *current_source_line, struct macro_line *current_macro_line, char **var_tab, struct omf_segment *current_omfsegment)
{
    int is_modified;
    struct source_line *new_source_line = NULL;
    char buffer[1024];

    /* Allocation of the new line */
    new_source_line = (struct source_line *) calloc(1,sizeof(struct source_line));
    if(new_source_line == NULL)
        return(NULL);
    
    /** Transfer the characteristics of the line Source (number of the line ...) **/
    new_source_line->file_line_number = current_source_line->file_line_number;
    new_source_line->file = current_source_line->file;
    new_source_line->type = LINE_UNKNOWN;
    new_source_line->address = -1;
    new_source_line->nb_byte = -1;
    new_source_line->is_valid = 1;           /* the line is valid */
    new_source_line->operand_value = 0xFFFFFFFF;
    new_source_line->operand_address_long = 0xFFFFFFFF;
    strcpy(new_source_line->m,"?");
    strcpy(new_source_line->x,"?");
    strcpy(new_source_line->reloc,"         ");

    /** Transfer the elements of the Macro line **/
    BuildSubstituteValue(current_macro_line->label,var_tab,buffer);
    new_source_line->label_txt = strdup(buffer);
    BuildSubstituteValue(current_macro_line->opcode,var_tab,buffer);
    new_source_line->opcode_txt = strdup(buffer);
    BuildSubstituteValue(current_macro_line->operand,var_tab,buffer);
    new_source_line->operand_txt = strdup(buffer);
    BuildSubstituteValue(current_macro_line->comment,var_tab,buffer);
    new_source_line->comment_txt = strdup(buffer);
    if(new_source_line->label_txt == NULL || new_source_line->opcode_txt == NULL || new_source_line->operand_txt == NULL || new_source_line->comment_txt == NULL)
    {
        mem_free_sourceline(new_source_line);
        return(NULL);
    }

    /* Replaces Equivalences on the Line */
    is_modified = ProcessLineEquivalence(new_source_line,current_omfsegment);

    /* Return line */
    return(new_source_line);
}


/************************************************************************************/
/*  BuildSubstituteValue() :  We replace the x's by the values passed in parameter. */
/************************************************************************************/
static void BuildSubstituteValue(char *src_string, char **var_tab, char *dst_string_rtn)
{
    int j = 0;

    /* We are looking for a ] */
    for(int i = 0; i<(int)strlen(src_string); i++)
    {
        if(src_string[i] == ']' && (src_string[i+1] >= '0' && src_string[i+1] <= '8'))
        {
            memcpy(&dst_string_rtn[j],var_tab[src_string[i+1]-'0'],strlen(var_tab[src_string[i+1]-'0']));
            j += (int) strlen(var_tab[src_string[i+1]-'0']);
            i++;
        }
        else
            dst_string_rtn[j++] = src_string[i];
    }
    /* end of string */
    dst_string_rtn[j] = '\0';
}


/*************************************************************/
/*  IsMacroFile() :  Determine if this File contains Macros. */
/*************************************************************/
int IsMacroFile(char *file_name, char *source_folder_path, char *macro_folder_path)
{
    int i, found;
    struct source_file *macro_file;
    struct source_line *current_line;
    char file_path[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    found = 0;

    /* We go quickly, we look at the name */
    if(strlen(file_name) > strlen(".Macs.s"))
        if(!my_stricmp(&file_name[strlen(file_name)-strlen(".Macs.s")],".Macs.s"))
            return(1);
    if(strlen(file_name) > strlen(".Macs"))
        if(!my_stricmp(&file_name[strlen(file_name)-strlen(".Macs")],".Macs"))
            return(1);

    /* We try to open the File with his name */
    sprintf(file_path,"%s%s",source_folder_path,file_name);
    macro_file = LoadOneSourceFile(file_path,file_name,0);
    if(macro_file == NULL)
    {
        /** We add a .S at the end **/
        strcat(file_path,".s");
        macro_file = LoadOneSourceFile(file_path,file_name,0);

        /** We will clean the File name **/
        if(macro_file == NULL)
        {
            /* We will extract the File name: 4 / Locator.Macs => Locator.Macs.s */
            for(i=(int)strlen(file_name); i>=0; i--)
                if(file_name[i] == '/' || file_name[i] == ':')
                    break;
            strcpy(param->buffer_file_name,&file_name[i+1]);

            /* Add the final .s */
            if(my_stricmp(&param->buffer_file_name[strlen(param->buffer_file_name)-2],".s"))
                strcat(param->buffer_file_name,".s");

            /* We try to open the File with his name */
            sprintf(file_path,"%s%s",macro_folder_path,param->buffer_file_name);
            macro_file = LoadOneSourceFile(file_path,param->buffer_file_name,0);
        }
    }

    /* We have failed to open the File, we declare it as a File Macro and we leave the following code to declare it not available. */
    if(macro_file == NULL)
        return(1);

    /** We will analyze the Lines in order to find MAC >>> **/
    /* File Empty ? */
    if(macro_file->first_line == NULL)
    {
        mem_free_sourcefile(macro_file,1);
        return(1);
    }

    /** Pass all lines in to review **/
    for(current_line = macro_file->first_line; current_line; current_line = current_line->next)
    {
        /* Comment / Empty */
        if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY)
            continue;

        /* Recognize Macro Opcode */
        if(!my_stricmp(current_line->opcode_txt,"MAC"))
        {
            found = 1;
            break;
        }
    }

    /* Memory release */
    mem_free_sourcefile(macro_file,1);

    /* Indicates if we found Macro */
    return(found);
}


/*******************************************************************/
/*  mem_alloc_macro() :  Allocate memory of the macro structure.   */
/*******************************************************************/
struct macro *mem_alloc_macro(char *file_name, char *name, int line_number)
{
    struct macro *current_macro;

    /* Allocate memory */
    current_macro = (struct macro *) calloc(1,sizeof(struct macro));
    if(current_macro == NULL)
        return(NULL);

    /* Fill out structure */
    current_macro->file_line_number = line_number;
    current_macro->file_name = strdup(file_name);
    current_macro->name = strdup(name);
    if(current_macro->file_name == NULL || current_macro->name == NULL)
    {
        mem_free_macro(current_macro);
        return(NULL);
    }

    /* Return the structure */
    return(current_macro);
}


/****************************************************************************/
/*  mem_alloc_macroline() :  Allocate memory of the macro_line structure.   */
/****************************************************************************/
struct macro_line *mem_alloc_macroline(char *label, char *opcode, char *operand, char *comment)
{
    struct macro_line *current_line;

    /* Allocate memory */
    current_line = (struct macro_line *) calloc(1,sizeof(struct macro_line));
    if(current_line == NULL)
        return(NULL);

    /* Fill out structure */
    current_line->label = strdup(label);
    current_line->opcode = strdup(opcode);
    current_line->operand = strdup(operand);
    current_line->comment = strdup(comment);
    if(current_line->label == NULL || current_line->opcode == NULL || current_line->operand == NULL || current_line->comment == NULL)
    {
        mem_free_macroline(current_line);
        return(NULL);
    }

    /* Return the structure */
    return(current_line);
}


/************************************************************************/
/*  mem_free_macroline() :  Memory release of the macro_line structure. */
/************************************************************************/
void mem_free_macroline(struct macro_line *current_line)
{
    if(current_line)
    {
        if(current_line->label)
            free(current_line->label);

        if(current_line->opcode)
            free(current_line->opcode);

        if(current_line->operand)
            free(current_line->operand);

        if(current_line->comment)
            free(current_line->comment);

        free(current_line);
    }
}


/****************************************************************/
/*  mem_free_macro() :  Memory release of the marcro structure. */
/****************************************************************/
void mem_free_macro(struct macro *current_macro)
{
    struct macro_line *current_line;
    struct macro_line *next_line;

    if(current_macro)
    {
        if(current_macro->name)
            free(current_macro->name);

        if(current_macro->file_name)
            free(current_macro->file_name);

        for(current_line=current_macro->first_line; current_line; )
        {
            next_line = current_line->next;
            mem_free_macroline(current_line);
            current_line = next_line;
        }

        free(current_macro);
    }
}


/*****************************************************************/
/*  mem_free_macro_list() :  Memory release of macro structures. */
/*****************************************************************/
void mem_free_macro_list(struct macro *all_macro)
{
    struct macro *current_macro;
    struct macro *next_macro;

    for(current_macro=all_macro; current_macro; )
    {
        next_macro = current_macro->next;
        mem_free_macro(current_macro);
        current_macro = next_macro;
    }
}

/***********************************************************************/
