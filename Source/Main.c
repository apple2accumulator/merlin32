/***********************************************************************/
/*                                                                     */
/*  Main.c : Assembler module of a source code 65c816.                 */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>

#include "Dc_Library.h"
#include "a65816_Link.h"
#include "a65816_Line.h"
#include "a65816_File.h"
#include "a65816_Lup.h"
#include "a65816_Macro.h"
#include "a65816_Cond.h"
#include "a65816_Code.h"
#include "a65816_Data.h"
#include "a65816_OMF.h"

int Assemble65c816(char *,char *,int);

/****************************************************/
/*  help() :  Display the application help          */
/****************************************************/
void help( char* appName )
{
    printf("  Usage : %s [-V] <macro_folder_path> <source_file_path>.\n", appName);
}

/****************************************************/
/*  main() :  Main function of the application.     */
/****************************************************/
int main(int argc, char *argv[])
{
    jmp_buf context;
    int i, verbose_mode, context_value, error;
    char *error_string = NULL;
    struct omf_segment *current_omfsegment;
    struct parameter *param;
    char file_error_path[2048];
    char source_file_path[2048];
    char macro_folder_path[2048];

    /* Message Information */
    printf("%s v 1.1, (c) Brutal Deluxe 2011-2015\n",argv[0]);

    /* Verification of parameters */
    if(argc != 3 && argc != 4)
    {
        help(argv[0]);
        return(1);
    }
    if(argc == 3 && !my_stricmp(argv[1],"-V"))
    {
        help(argv[0]);
        return(1);
    }
    if(argc == 4 && my_stricmp(argv[1],"-V"))
    {
        help(argv[0]);
        return(1);
    }

    /* Parameter decoding */
    if(argc == 3)
    {
        verbose_mode = 0;
        strcpy(macro_folder_path,argv[1]);
        strcpy(source_file_path,argv[2]);
    }
    else
    {
        verbose_mode = 1;
        strcpy(macro_folder_path,argv[2]);
        strcpy(source_file_path,argv[3]);
    }

    /* Initialization */
    my_Memory(MEMORY_INIT,NULL,NULL,NULL);
    my_File(FILE_INIT,NULL);

    /* Initialization of Error Management */
    my_RaiseError(ERROR_INIT,NULL);
    context_value = setjmp(context);
    if(context_value)
    {
        /* Retrieve of the string containing the error message */
        my_RaiseError(ERROR_GET_STRING,&error_string);

        /* Error and end message */
        if(error_string)
        {
            printf("      => [Error] %s.\n",error_string);
            free(error_string);
        }

        /* We recover the OMF Current segment (if it exists) */
        my_Memory(MEMORY_GET_OMFSEGMENT,&current_omfsegment,NULL,NULL);

        /** We try to Dump something in the File Error_Output.txt **/
        if(current_omfsegment != NULL)
        {
            strcpy(file_error_path,"error_output.txt");
            my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);
            if(param != NULL)
                if(strlen(param->current_folder_path) > 0)
                    sprintf(file_error_path,"%serror_output.txt",param->current_folder_path);
            CreateOutputFile(file_error_path,current_omfsegment,NULL);
        }
        
        /* freeing of resources */
        my_Memory(MEMORY_FREE,NULL,NULL,NULL);

        /* Error */
        return(1);
    }
    my_RaiseError(ERROR_INIT,&context);

    /* Allocate memory of the param structure */
    param = mem_alloc_param();
    if(param == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure parameter");
    my_Memory(MEMORY_SET_PARAM,param,NULL,NULL);

    /** Preparation of Macro folder **/
    if(strlen(macro_folder_path) > 0)
        if(macro_folder_path[strlen(macro_folder_path)-1] != '\\' && macro_folder_path[strlen(macro_folder_path)-1] != '/')
            strcat(macro_folder_path,FOLDER_SEPARATOR);

    /** Prepares the Output file Path **/
    strcpy(param->output_file_path,source_file_path);
    for(i=(int)strlen(param->output_file_path); i>=0; i--)
        if(param->output_file_path[i] == '\\' || param->output_file_path[i] == '/')
        {
            param->output_file_path[i+1] = '\0';
            break;
        }
    if(i < 0)
        strcpy(param->output_file_path,"");
    strcpy(param->current_folder_path,param->output_file_path);

    /*** Assemble and Link all Files of Project ***/
    error = AssembleLink65c816(source_file_path,macro_folder_path,verbose_mode);

    /* free the resources */
    my_File(FILE_FREE,NULL);
    my_Memory(MEMORY_FREE,NULL,NULL,NULL);

    /* End of Errors Management */
    my_RaiseError(ERROR_END,NULL);

    /* OK */
    return(0);
}

/***********************************************************************/
