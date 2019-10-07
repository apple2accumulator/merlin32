/***********************************************************************/
/*                                                                     */
/*  a65816_File.c : Module for the management of the Files.            */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Dc_Library.h"
#include "a65816_Line.h"
#include "a65816_Macro.h"
#include "a65816_OMF.h"
#include "a65816_File.h"


/********************************************************/
/*  LoadAllSourceFile() :  Loading of all Source Files. */
/********************************************************/
int LoadAllSourceFile(char *first_file_path, char *macro_folder_path, struct omf_segment *current_omfsegment)
{
    int i, file_number, line_number, nb_error;
    struct source_file *first_file;
    struct source_file *last_file;
    struct source_file *new_file;
    struct source_line *current_line;
    struct source_line *next_line;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    file_number = 1;
    nb_error = 0;

    /* Extract the File name */
    for(i=(int)strlen(first_file_path); i>=0; i--)
        if(first_file_path[i] == '/' || first_file_path[i] == '\\')
            break;
    strcpy(param->buffer_file_name,&first_file_path[i+1]);

    /** Loading the first Source file **/
    printf("        - %s\n",param->buffer_file_name);
    first_file = LoadOneSourceFile(first_file_path,param->buffer_file_name,file_number);
    if(first_file == NULL)
    {
        sprintf(param->buffer_error,"Impossible to load Source file '%s'",first_file_path);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
    last_file = first_file;

    /* Stores the first Source file */
    my_Memory(MEMORY_SET_FILE,first_file,NULL,current_omfsegment);

    /** Loading of all Files Source: We only look in the 1st File **/
    for(current_line = first_file->first_line; current_line; )
    {
        if((!my_stricmp(current_line->opcode_txt,"PUT") || !my_stricmp(current_line->opcode_txt,"PUTBIN") || !my_stricmp(current_line->opcode_txt,"USE")) && strlen(current_line->operand_txt) > 0)
        {
            /* If the inclusion is done by a Use on check if we are dealing with a File of Macro */
            if(!my_stricmp(current_line->opcode_txt,"USE") && IsMacroFile(current_line->operand_txt,param->source_folder_path,macro_folder_path))
            {
                current_line = current_line->next;
                continue;
            }

            /* New File */
            file_number++;

            /* We insert the new File here */
            next_line = current_line->next;

            /** Source file or Macro **/
            if(!my_stricmp(current_line->opcode_txt,"PUT") || !my_stricmp(current_line->opcode_txt,"USE"))
            {
                /* Built the File name */
                strcpy(param->buffer_file_name,current_line->operand_txt);

                /* Build the full path of the File */
                sprintf(param->buffer_file_path,"%s%s",param->source_folder_path,param->buffer_file_name);

                /* Should you add a .s? */
                if(my_IsFileExist(param->buffer_file_path) == 0)
                {
                    /* The File Does not exist */
                    if(strlen(param->buffer_file_name) > 2)
                        if(my_stricmp(&param->buffer_file_name[strlen(param->buffer_file_name)-2],".s"))
                        {
                            /* Add the .s to the name and to the full path */
                            strcat(param->buffer_file_name,".s");
                            strcat(param->buffer_file_path,".s");
                        }
                }

                /* Load the Text File */
                printf("        - %s\n",param->buffer_file_name);
                new_file = LoadOneSourceFile(param->buffer_file_path,param->buffer_file_name,file_number);
                if(new_file == NULL)
                {
                    sprintf(param->buffer_error,"Impossible to open Source file '%s'",param->buffer_file_path);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
            }
            else   /* PUTBIN: Binary File */
            {
                /* Construct the name */
                strcpy(param->buffer_file_name,current_line->operand_txt);

                /* Built the path */
                sprintf(param->buffer_file_path,"%s%s",param->source_folder_path,param->buffer_file_name);

                /* Load the Binary File */
                printf("        - %s\n",param->buffer_file_name);
                new_file = LoadOneBinaryFile(param->buffer_file_path,param->buffer_file_name,file_number);
                if(new_file == NULL)
                {
                    sprintf(param->buffer_error,"Impossible to open Binary file '%s'",param->buffer_file_path);
                    my_RaiseError(ERROR_RAISE,param->buffer_error);
                }
            }

            /* File attachment to the previous ones */
            if(new_file != NULL)
            {
                last_file->next = new_file;
                last_file = new_file;
            }

            /* Insert the Lines of this File to the previous Lines */
            if(new_file != NULL)
            {
                new_file->last_line->next = current_line->next;
                current_line->next = new_file->first_line;
            }

            /* Next line */
            current_line = next_line;
        }
        else
            current_line = current_line->next;
    }

    /** Global numbering of Lines **/
    for(line_number=1,current_line = first_file->first_line; current_line; current_line = current_line->next,line_number++)
        current_line->line_number = line_number;

    /* OK */
    return(nb_error);
}


/**************************************************/
/*  LoadOneSourceFile() :  Loading a Source file. */
/**************************************************/
struct source_file *LoadOneSourceFile(char *file_path, char *file_name, int file_number)
{
    struct source_file *current_file = NULL;
    struct source_line *current_line = NULL;
    int nb_line = 1;
    size_t file_size = 0;
    unsigned char *file_data = NULL;
    char *begin_line = NULL;
    char *end_line = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Loading the File */
    file_data = LoadTextFileData(file_path,&file_size);
    if(file_data == NULL)
        return(NULL);

    /* Allocate memory */
    current_file = (struct source_file *) calloc(1,sizeof(struct source_file));
    if(current_file == NULL)
    {
        free(file_data);
        return(NULL);
    }

    /* Data */
    current_file->data = file_data;

    /* File Path */
    current_file->file_path = strdup(file_path);

    /* Build file name */
    current_file->file_name = strdup(file_name);
    
    /* File number */
    current_file->file_number = file_number;

    /* Count the number of rows */
    for(int i = 0; i < (int)file_size; i++)
    {
        if(current_file->data[i] == '\n')
            nb_line++;
    }

    /* Allocate memory for table of ligne */
    current_file->tab_line = (char **) calloc(nb_line,sizeof(char *));

    /* Verification of memory allocations */
    if(current_file->file_path == NULL || current_file->file_name == NULL || current_file->tab_line == NULL)
    {
        mem_free_sourcefile(current_file,0);
        return(NULL);
    }

    /** Determine the beginning of each line **/
    begin_line = (char *) current_file->data;
    int line = 0;
    for(; begin_line; line++)
    {
        /* Keep a pointer to the beginning of line */
        current_file->tab_line[line] = begin_line;

        /* End of line */
        end_line = strchr(begin_line,'\n');
        if(end_line != NULL)
            *end_line = '\0';

        /* Next line */
        begin_line = (end_line == NULL) ? NULL : end_line+1;
    }
    current_file->nb_line = line;

    /** Create the lines **/
    for(int i = 0; i<current_file->nb_line; i++)
    {
        /* Decoding of the line */
        current_line = BuildSourceLine(current_file,i);
        if(current_line == NULL)
        {
            mem_free_sourcefile(current_file,0);
            sprintf(param->buffer_error,"Impossible to build source line %d",i);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

        /* Si on tomber sur un END, on s'arrête là */
        if(!my_stricmp(current_line->opcode_txt,"END"))
        {
            current_file->nb_line = i+1;
            break;
        }

        /* On va repérer les Lines utilisant déjà un label ozunid_ */
        if(!my_strnicmp(current_line->label_txt,"ozunid_",strlen("ozunid_")))
            ProcessOZUNIDLine(current_line->label_txt);

        /* Attachment to the list */
        if(current_file->first_line == NULL)
            current_file->first_line = current_line;
        else
            current_file->last_line->next = current_line;
        current_file->last_line = current_line;
    }

    /* Return the structure */
    return(current_file);
}


/**************************************************/
/*  LoadOneBinaryFile() :  Loading a Source file. */
/**************************************************/
struct source_file *LoadOneBinaryFile(char *file_path, char *file_name, int file_number)
{
    struct source_file *current_file = NULL;
    struct source_line *current_line = NULL;
    int offset = 0, nb_line = 0;
    size_t file_bin_size = 0, file_size = 0;
    char *file_data = NULL;
    unsigned char *file_bin_data = NULL;
    char *begin_line = NULL;
    char *end_line = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Loading the binary File */
    file_bin_data = LoadBinaryFileData(file_path,&file_bin_size);
    if(file_bin_data == NULL)
        return(NULL);

    /** Text conversion with HEX byte, byte... **/
    nb_line = (int)file_bin_size / 16;
    if(nb_line*16 != (int)file_bin_size)
        nb_line++;                      /* The Last line will not have 16 bytes of Data */
    file_size = nb_line*((int)strlen(" HEX  \n")) + file_bin_size*3 + 1;    /* 0A, */

    /* Allocate memory */
    file_data = (char *) calloc(file_size,sizeof(char));
    if(file_data == NULL)
    {
        free(file_bin_data);
        return(NULL);
    }

    /* Construction of buffer Text */
    for(int i = 0; i < (int)file_bin_size; i++)
    {
        /* Beginning of line */
        if(i%16 == 0)
        {
            /* End of the previous line */
            if(offset > 0)
            {
                strcpy(&file_data[offset],"\n");
                offset++;
            }
            /* Beginning of line */
            strcpy(&file_data[offset]," HEX  ");
            offset += (int) strlen(" HEX  ");
        }

        /* We place a value */
        sprintf(&file_data[offset],"%02X",file_bin_data[i]);
        offset += 2;

        /* We add the , */
        if(i%16 != 15 && i != (int)(file_bin_size-1))
        {
            strcpy(&file_data[offset],",");
            offset++;
        }
    }
    /* End of the Last line */
    strcat(file_data,"\n");
    file_size = (int) strlen(file_data);

    /* Memory release of binary File */
    free(file_bin_data);

    /* Allocate memory */
    current_file = (struct source_file *) calloc(1,sizeof(struct source_file));
    if(current_file == NULL)
    {
        free(file_data);
        return(NULL);
    }

    /* Data */
    current_file->data = (unsigned char *) file_data;

    /* File Path */
    current_file->file_path = strdup(file_path);

    /* Build file name */
    current_file->file_name = strdup(file_name);
    
    /* File number */
    current_file->file_number = file_number;

    /* Count the number of rows */
    nb_line = 1;
    for(int i = 0; i < (int)file_size; i++)
    {
        if(current_file->data[i] == '\n')
            nb_line++;
    }

    /* Allocate memory for table of ligne */
    current_file->tab_line = (char **) calloc(nb_line,sizeof(char *));

    /* Verification of memory allocations */
    if(current_file->file_path == NULL || current_file->file_name == NULL || current_file->tab_line == NULL)
    {
        mem_free_sourcefile(current_file,0);
        return(NULL);
    }

    /** Determine the beginning of each line **/
    begin_line = (char *) current_file->data;
    int line = 0;
    for(; begin_line; line++)
    {
        /* Keep a pointer to the beginning of line */
        current_file->tab_line[line] = begin_line;

        /* End of line */
        end_line = strchr(begin_line,'\n');
        if(end_line != NULL)
            *end_line = '\0';

        /* Next line */
        begin_line = (end_line == NULL) ? NULL : end_line+1;
    }
    current_file->nb_line = line;

    /** Create the lines **/
    for(int i = 0; i < current_file->nb_line; i++)
    {
        /* Decoding of the line */
        current_line = BuildSourceLine(current_file,i);
        if(current_line == NULL)
        {
            mem_free_sourcefile(current_file,0);
            sprintf(param->buffer_error,"Impossible to build source line %d",i);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

        /* Attachment to the list */
        if(current_file->first_line == NULL)
            current_file->first_line = current_line;
        else
            current_file->last_line->next = current_line;
        current_file->last_line = current_line;
    }

    /* Return the structure */
    return(current_file);
}


/************************************************/
/*  BuildObjectCode() :  Create the File Objet. */
/************************************************/
int BuildObjectCode(struct omf_segment *current_omfsegment)
{
    BYTE checksum_byte = 0;
    int i = 0, object_length = 0;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /** Calculate the max size of the object **/
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    for(object_length=0,current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We do not take invalid lines */
        if(current_line->is_valid == 0 || current_line->is_dum == 1)
            continue;

        /* For a Macro, we only take the call line */
        if(current_line->type == LINE_MACRO && current_line->is_in_source == 0)
            continue;
        if(current_line->type == LINE_DIRECTIVE && current_line->is_in_source == 0)
            continue;

        /* Count matches the size of the object code */
        object_length += current_line->nb_byte;
    }

    /* Allocate memory */
    current_omfsegment->object_code = (unsigned char *) calloc(object_length+1,sizeof(unsigned char));
    if(current_omfsegment->object_code == NULL)
        return(1);

    /** Fill out structure of buffer objet **/
    for(object_length=0,current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We do not take invalid lines */
        if(current_line->is_valid == 0 || current_line->is_dum == 1)
            continue;

        /* For a Macro, we only take the call line */
        if(current_line->type == LINE_MACRO && current_line->is_in_source == 0)
            continue;
        if(current_line->type == LINE_DIRECTIVE && current_line->is_in_source == 0)
            continue;
        if(current_line->nb_byte == 0)         /* ERR */
            continue;

        /** Place the object code **/
        if(current_line->type == LINE_CODE)
        {
            /* Opcode Byte + Operand Byte(s) */
            current_omfsegment->object_code[object_length++] = current_line->opcode_byte;
            memcpy(&current_omfsegment->object_code[object_length],current_line->operand_byte,current_line->nb_byte-1);
            object_length += current_line->nb_byte-1;
        }
        else if(current_line->type == LINE_DATA && !my_stricmp(current_line->opcode_txt,"CHK"))
        {
            /* Calculates the checksum from the beginning */
            for(i=0; i<object_length; i++)
                checksum_byte = (i == 0) ? current_omfsegment->object_code[i] : (checksum_byte ^ current_omfsegment->object_code[i]);

            /* Checksum Byte */
            current_omfsegment->object_code[object_length] = checksum_byte;
            object_length++;
        }
        else if(current_line->type == LINE_DATA)
        {
            /* Data Byte(s) */
            memcpy(&current_omfsegment->object_code[object_length],current_line->data,current_line->nb_byte);
            object_length += current_line->nb_byte;
        }
    }

    /* Object code size */
    current_omfsegment->object_length = object_length;

    /* OK */
    return(0);
}


/*********************************************************/
/*  BuildObjectFile() :  Create the File Object on Disk. */
/*********************************************************/
int BuildObjectFile(char *output_folder_path, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    FILE *fd = NULL;
    int nb_write = 0;
    char file_path[1024];
    struct source_file *first_file = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Retrieves the first Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /** Do we have a name yet? **/
    if(strlen(current_omfsegment->object_name) == 0)
    {
        /* On utilise le Source file name */
        strcpy(current_omfsegment->object_name,first_file->file_name);
        for(int i = (int)strlen(current_omfsegment->object_name); i >= 0; i--)
        {
            if(current_omfsegment->object_name[i] == '.')
            {
                current_omfsegment->object_name[i] = '\0';
                break;
            }
        }
    }

    /** Have you a name for the File Output.txt ? **/
    if(current_omfproject->nb_segment == 1)
    {
        if(strlen(param->output_file_path) == 0 || !my_stricmp(param->output_file_path,param->current_folder_path))
            sprintf(param->output_file_path,"%s%s_Output.txt",param->current_folder_path,current_omfsegment->object_name);
    }
    else
    {
        if(strlen(param->output_file_path) == 0 || !my_stricmp(param->output_file_path,param->current_folder_path))
            sprintf(param->output_file_path,"%s%s_S%02X_Output.txt",param->current_folder_path,current_omfsegment->object_name,current_omfsegment->segment_number);  /* Multi Segments */
    }

    /*** Create the file on disk ***/
    sprintf(file_path,"%s%s",output_folder_path,current_omfsegment->object_name);

    /* Information */
    printf("     => Creating Object file '%s'\n",file_path);

    /* Create the File */
#if defined(WIN32) || defined(WIN64)  
    fd = fopen(file_path,"wb+");
#else
    fd = fopen(file_path,"w+");
#endif
    if(fd == NULL)
    {
        printf("    Error : Can't create Object file '%s'.\n",file_path);
        return(1);
    }

    /* Write the object code to the File	 */
    nb_write = (int) fwrite(current_omfsegment->object_code,1,current_omfsegment->object_length,fd);
    if(nb_write != current_omfsegment->object_length)
        printf("    Error : Can't write Object file '%s' data (%d bytes / %d bytes).\n",file_path,nb_write,current_omfsegment->object_length);
    
    /* Close the File */
    fclose(fd);

    /*** Updating the File FileInformation ***/
    UpdateFileInformation(output_folder_path,current_omfsegment->object_name,current_omfproject);

    /* OK */
    return(0);
}


/*******************************************************************************************/
/*  BuildSingleObjectFile() :  Create the File Single Binary Multi-Segment Object on Disk. */
/*******************************************************************************************/
int BuildSingleObjectFile(char *output_folder_path, int file_number, struct omf_project *current_omfproject)
{
    FILE *fd;
    int nb_write;
    char file_path[1024];
    struct omf_segment *current_omfsegment;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /** Have you a name for the File Output.txt ? **/
    if(strlen(param->output_file_path) == 0 || !my_stricmp(param->output_file_path,param->current_folder_path))
        sprintf(param->output_file_path,"%s%s_Output.txt",param->current_folder_path,current_omfproject->dsk_name_tab[file_number]);

    /*** Create the file on disk ***/
    sprintf(file_path,"%s%s",output_folder_path,current_omfproject->dsk_name_tab[file_number]);

    /* Information */
    printf("     => Creating Object file '%s'\n",file_path);

    /* Create the File */
#if defined(WIN32) || defined(WIN64) 
    fd = fopen(file_path,"wb+");
#else
    fd = fopen(file_path,"w+");
#endif
    if(fd == NULL)
    {
        printf("    Error : Can't create Object file '%s'.\n",file_path);
        return(1);
    }

    /** Writing the bodys in the movie one behind the other **/
    for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
        /* 1 Segment (only if it belongs to the File) */
        if(current_omfsegment->file_number == (file_number+1))
        {
            nb_write = (int) fwrite(current_omfsegment->object_code,1,current_omfsegment->object_length,fd);
            if(nb_write != current_omfsegment->object_length)
            {
                printf("    Error : Can't write Object file '%s' data (%d bytes / %d bytes).\n",file_path,nb_write,current_omfsegment->object_length);
                break;
            }
        }
    }
    
    /* Close the File */
    fclose(fd);

    /*** Updating the File _FileInformation ***/
    UpdateFileInformation(output_folder_path,current_omfproject->dsk_name_tab[file_number],current_omfproject);

    /* OK */
    return(0);
}


/**********************************************************/
/*  CreateOutputFile() :  Create the File of Text Output. */
/**********************************************************/
int CreateOutputFile(char *file_path, struct omf_segment *current_omfsegment, struct omf_project *current_omfproject)
{
    FILE *fd;
    int i, j, nb_byte_left, is_multi_fixed, nb_byte;
    int file_length;
    int label_length;
    int opcode_length;
    int operand_length;
    char buffer_format[256];
    struct source_file *first_file;
    struct source_file *current_file;
    struct source_line *current_line;
    char *line_type_tab[] = {"Unknown   ", "Comment    ", "Directive  ", "Equivalence", "Variable   ", "Code       ", "Data       ", "Macro      ", "Empty      ", "Global     ", "External   "};
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    is_multi_fixed = 0;

    /* Is it a projet multi-segment fixed */
    if(current_omfproject != NULL)
        if(current_omfproject->is_multi_fixed == 1)
            is_multi_fixed = 1;

    /* Size of names of Files */
    file_length = 0;
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    if(first_file == NULL)
        return(1);
    for(current_file=first_file; current_file; current_file=current_file->next)
    {
        if(file_length < (int) strlen(current_file->file_name))
            file_length = (int) strlen(current_file->file_name);
    }
    if(file_length < 4)
        file_length = 4;

    /* Information */
    printf("     => Creating Output file '%s'\n",file_path);

    /* Create the output file */
    fd = fopen(file_path,"w+");
    if(fd == NULL)
        return(1);

    /** Taille des marges **/
    label_length = 10;
    opcode_length = 4;
    operand_length = 15;
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We do not take invalid lines */
        if(current_line->is_valid == 0)
            continue;

        /* For a Macro, we only take the call line */
        if(current_line->type == LINE_MACRO && current_line->is_in_source == 0)
            continue;
        if(current_line->type == LINE_DIRECTIVE && current_line->is_in_source == 0)
            continue;

        /* Align the column of label on the widest */
        if((int) strlen(current_line->label_txt) > label_length)
            label_length = (int) strlen(current_line->label_txt);

        /* Look at the opcode having an operand */
        if((int) strlen(current_line->opcode_txt) > opcode_length && (int) strlen(current_line->operand_txt) > 0)
            opcode_length = (int) strlen(current_line->opcode_txt);

        /* We will watch the operand for a comment */
        if((int) strlen(current_line->operand_txt) > operand_length && (int) strlen(current_line->comment_txt) > 0)
            operand_length = (int) strlen(current_line->operand_txt);
        if(operand_length > 21)
            operand_length = 21;
    }

    /** Entity **/
    /* Feature */
    strcpy(param->buffer_line,"------+----");
    for(i=0; i<file_length+2; i++)
        strcat(param->buffer_line,"-");
    strcat(param->buffer_line,"------+-------------+----+---------+------+-----------------------+-------------------------------------------------------------------\n");
    fwrite(param->buffer_line,1,strlen(param->buffer_line),fd);
    /* WORDING */
    strcpy(param->buffer_line," Line | # File");
    for(i=0; i<file_length-2; i++)
        strcat(param->buffer_line," ");
    strcat(param->buffer_line,"  Line | Line Type   | MX |  Reloc  | Size | Address   Object Code |  Source Code                                                      \n");
    fwrite(param->buffer_line,1,strlen(param->buffer_line),fd);
    /* Feature */
    strcpy(param->buffer_line,"------+----");
    for(i=0; i<file_length+2; i++)
        strcat(param->buffer_line,"-");
    strcat(param->buffer_line,"------+-------------+----+---------+------+-----------------------+-------------------------------------------------------------------\n");
    fwrite(param->buffer_line,1,strlen(param->buffer_line),fd);

    /*** Lines treatment ***/
    for(current_line=first_file->first_line; current_line; current_line=current_line->next)
    {
        /* We do not take invalid lines */
        if(current_line->is_valid == 0)
            continue;

        /* For a Macro, we only take the call line */
        if(current_line->type == LINE_MACRO && current_line->is_in_source == 0)
            continue;
        /* We do not take the Directive unless they have a Label used */
        if(current_line->type == LINE_DIRECTIVE && current_line->is_in_source == 0)
            continue;

        /** Creation of the line **/
        /* In case of Dump in Error, we can have Lines with a number of bytes of FFFFF */
        nb_byte = current_line->nb_byte;
        if(nb_byte == 0xFFFFF)
            nb_byte = 0;

        /* Number of the line + File name + Number of the line of File */
        sprintf(buffer_format,"%%5d | %%2d %%%ds  %%5d",file_length);
        sprintf(param->buffer_line,buffer_format,current_line->line_number,current_line->file->file_number,current_line->file->file_name,current_line->file_line_number);

        /* Type | MX |  Reloc  | Num Bytes | Address */
        sprintf(&param->buffer_line[strlen(param->buffer_line)]," | %s | %s%s |%s| %4d | %02X/%04X",
                (current_line->is_dum==1)?"Dum        ":line_type_tab[current_line->type],
                current_line->m,current_line->x,current_line->reloc,nb_byte,current_line->bank,(WORD)current_line->address);

        /* Comment */
        if(current_line->type == LINE_COMMENT)
        {
            strcat(param->buffer_line,"               | ");
            strcat(param->buffer_line,current_line->line_data);
        }
        else
        {
            /** Code objet **/
            if(current_line->type == LINE_CODE)
            {
                /* Opcode Bye + Operand Byte */
                sprintf(param->buffer_value," : %02X ",current_line->opcode_byte);
                for(i=0; i<nb_byte-1; i++)
                    sprintf(&param->buffer_value[strlen(param->buffer_value)],"%02X ",current_line->operand_byte[i]);
                strcat(param->buffer_value,"               ");
            }
            else if(current_line->type == LINE_DATA)
            {
                /* Operand Byte */
                strcpy(param->buffer_value," : ");
                for(i=0; i<MIN(4,nb_byte); i++)
                    sprintf(&param->buffer_value[strlen(param->buffer_value)],"%02X ",current_line->data[i]);
                strcat(param->buffer_value,"               ");
            }
            else
                strcpy(param->buffer_value,"               ");
            param->buffer_value[15] = '|';
            param->buffer_value[16] = '\0';
            strcat(param->buffer_line,param->buffer_value);

            /* Label */
            sprintf(buffer_format," %%-%ds",label_length);
            sprintf(param->buffer_value,buffer_format,current_line->label_txt);
            strcat(param->buffer_line,param->buffer_value);

            /* Opcode */
            sprintf(buffer_format,"  %%-%ds",opcode_length);
            sprintf(param->buffer_value,buffer_format,(current_line->type == LINE_EMPTY)?"":current_line->opcode_txt);
            strcat(param->buffer_line,param->buffer_value);

            /* Operand */
            sprintf(buffer_format,"  %%-%ds",operand_length);
            sprintf(param->buffer_value,buffer_format,(current_line->type == LINE_EMPTY)?"":current_line->operand_txt);
            strcat(param->buffer_line,param->buffer_value);

            /* Comment */
            strcat(param->buffer_line,current_line->comment_txt);
        }

        /* End of line */
        strcat(param->buffer_line,"\n");

        /* Write the line in the File */
        fwrite(param->buffer_line,1,strlen(param->buffer_line),fd);

        /** Finalize the Data > 4 bytes **/
        if(current_line->type == LINE_DATA && nb_byte > 4)
        {
            for(i=4; i<nb_byte; i+=4)
            {
                /* Number of bytes still available */
                nb_byte_left = (nb_byte-i) >= 4 ? 4 : nb_byte-i;

                /* Empty line */
                strcpy(param->buffer_line,"      |    ");
                for(j=0; j<file_length+2; j++)
                    strcat(param->buffer_line," ");
                strcat(param->buffer_line,"      |             |    |         |      |           ");
                for(j=0; j<4; j++)
                {
                    if(j < nb_byte_left)
                        sprintf(&param->buffer_line[strlen(param->buffer_line)],"%02X ",current_line->data[i+j]);
                    else
                        strcat(param->buffer_line,"   ");
                }
                strcat(param->buffer_line,"|\n");
                fwrite(param->buffer_line,1,strlen(param->buffer_line),fd);
            }
        }
    }

    /* Feature */
    strcpy(param->buffer_line,"------+----");
    for(i=0; i<file_length+2; i++)
        strcat(param->buffer_line,"-");
    strcat(param->buffer_line,"------+-------------+----+---------+------+-----------------------+-------------------------------------------------------------------\n");
    fwrite(param->buffer_line,1,strlen(param->buffer_line),fd);

    /* Close the File */
    fclose(fd);

    /* OK */
    return(0);
}


/**************************************************************************/
/*  mem_free_sourcefile() :  Memory release of the source_file structure. */
/**************************************************************************/
void mem_free_sourcefile(struct source_file *current_sourcefile, int free_line)
{
    struct source_line *current_line;
    struct source_line *next_line;

    if(current_sourcefile)
    {
        if(current_sourcefile->file_path)
            free(current_sourcefile->file_path);

        if(current_sourcefile->file_name)
            free(current_sourcefile->file_name);

        if(current_sourcefile->data)
            free(current_sourcefile->data);

        if(current_sourcefile->tab_line)
            free(current_sourcefile->tab_line);

        if(free_line == 1)
        {
            for(current_line=current_sourcefile->first_line; current_line; )
            {
                next_line = current_line->next;
                mem_free_sourceline(current_line);
                current_line = next_line;
            }
        }

        free(current_sourcefile);
    }
}

/***********************************************************************/
