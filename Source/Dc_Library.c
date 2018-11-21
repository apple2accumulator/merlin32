/***********************************************************************/
/*                                                                     */
/*  Dc_Library.c : Module for the Generic Function Library.            */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>
#include <time.h>

/** Platform dependent code **/
/* MSVC only defines _WIN32 */
#if defined(_WIN32) || defined(WIN32) || defined(WIN64)
/* Windows */
#include <io.h>
#include <windows.h>                    /* GetFileAttributes() SetFileAttributes() FILE_ATTRIBUTE_HIDDEN */
#else
/* Linux + MacOS */
#include <inttypes.h>
#include <strings.h>                    /* strcasecmp() strncasecmp() */
#include <unistd.h>                     /* unlink() */
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "Dc_Library.h"
#include "a65816_Macro.h"
#include "a65816_File.h"
#include "a65816_Line.h"
#include "a65816_OMF.h"

int compare_item(const void *,const void *);
int compare_macro(const void *,const void *);
int compare_label(const void *,const void *);
int compare_equivalence(const void *,const void *);
int compare_variable(const void *,const void *);
int compare_external(const void *,const void *);

/*********************************************/
/*  my_RaiseError() :  Errors Management.    */
/*********************************************/
void my_RaiseError(int code, void *data)
{
    static int error_state = ERROR_NOT_READY_YET;
    static jmp_buf *context;
    static char *error_string;

    switch(code)
    {
        case ERROR_INIT :
            context = (jmp_buf *) data;
            error_string = NULL;
            error_state = ERROR_READY;
            break;

        case ERROR_END :
            error_state = ERROR_NOT_READY_YET;
            break;

        case ERROR_GET_STRING :
            if(error_state == ERROR_READY)
                *((char **) data) = error_string;
            break;

        default :  /* ERROR_RAISE */
            if(error_state == ERROR_READY)
            {
                /* Constructing the User Error Message */
                error_string = strdup((char *) data);

                /* Jump to the beginning of the code */
                error_state = ERROR_READY;
                longjmp(*context,1);
            }
            break;
    }
}


/**************************************************/
/*  my_File() :  Resource Management Files.       */
/**************************************************/
void my_File(int code, void *data)
{
    int i;
    static long hFile_tab[256];

    switch(code)
    {
        case FILE_INIT :
            for(i=0; i<256; i++)
                hFile_tab[i] = 0;
            break;

        case FILE_FREE :
            /* Closing the Directory */
            for(i=0; i<256; i++)
                if(hFile_tab[i] != 0)
                {
#if defined(WIN32) || defined(WIN64)        
                    _findclose(hFile_tab[i]);
#endif
                    hFile_tab[i] = 0;
                }
            break;

        case FILE_DECLARE_DIRECTORY :
            for(i=0; i<256; i++)
                if(hFile_tab[i] == 0)
                {
                    hFile_tab[i] = *((long *) data);
                    break;
                }
            break;

        case FILE_FREE_DIRECTORY :
            for(i=0; i<256; i++)
                if(hFile_tab[i] == *((long *) data))
                {
                    hFile_tab[i] = 0;
                    break;
                }
            break;

        default :
            break;
    }
}


/****************************************************/
/*  my_Memory() :  Memory Resource Management.      */
/****************************************************/
void my_Memory(int code, void *data, void *value, struct omf_segment *current_omfsegment)
{
    static struct parameter *param = NULL;         		/* Structure Parameter */
    static struct omf_segment *curr_omfsegment = NULL;
    struct source_file *current_file = NULL;              /* Source file */
    struct source_file *next_file = NULL;
    struct item *current_opcode = NULL;                   /* List of opcodes */
    struct item *next_opcode = NULL;
    struct item *new_opcode = NULL;
    struct item *current_data = NULL;                     /* List of data */
    struct item *next_data = NULL;
    struct item *new_data = NULL;
    struct item *current_directive = NULL;                /* List of directives */
    struct item *next_directive = NULL;
    struct item *new_directive = NULL;
    struct item *current_direqu = NULL;                   /* List of directive equivalences */
    struct item *next_direqu = NULL;
    struct item *new_direqu = NULL;
    struct item **found_item_ptr = NULL;
    struct macro *current_macro = NULL;                   /* List of macros */
    struct macro *next_macro = NULL;
    struct macro **found_macro_ptr = NULL;
    struct label *current_label = NULL;                   /* List of labels */
    struct label *next_label = NULL;
    struct label **found_label_ptr = NULL;
    struct equivalence *current_equivalence = NULL;       /* List of equivalences */
    struct equivalence *new_equivalence = NULL;
    struct equivalence *next_equivalence = NULL;
    struct equivalence **found_equivalence_ptr = NULL;
    struct variable *current_variable = NULL;             /* List of variables */
    struct variable *next_variable = NULL;
    struct variable **found_variable_ptr = NULL;
    struct external *current_external = NULL;             /* List of externals */
    struct external *next_external = NULL;
    struct external **found_external_ptr = NULL;
    struct global *current_global = NULL;                 /* List of globals */
    struct global *next_global = NULL;
    struct global *new_global = NULL;

    switch(code)
    {
        case MEMORY_INIT :
            param = NULL;
            current_omfsegment = NULL;
            break;

        case MEMORY_FREE :
            mem_free_param(param);
            my_Memory(MEMORY_INIT,NULL,NULL,NULL);
            break;

            /*************************/
            /*  Structure Parameter  */
            /*************************/
        case MEMORY_SET_PARAM :
            param = (struct parameter *) data;
            break;

        case MEMORY_GET_PARAM :
            *((struct parameter **) data) = param;
            break;

            /***************************/
            /*  Structure OMF Segment  */
            /***************************/
        case MEMORY_SET_OMFSEGMENT :
            curr_omfsegment = (struct omf_segment *) data;
            break;

        case MEMORY_GET_OMFSEGMENT :
            *((struct omf_segment **) data) = curr_omfsegment;
            break;

            /*************************************/
            /*  Declares Allocate memorys  */
            /*************************************/
        case MEMORY_DECLARE_ALLOC :
            for(int i=0; i<1024; i++)
                if(current_omfsegment->alloc_table[i] == NULL)
                {
                    current_omfsegment->alloc_table[i] = data;
                    break;
                }
            break;

        case MEMORY_FREE_ALLOC :
            for(int i=0; i<1024; i++)
                if(current_omfsegment->alloc_table[i] == data)
                {
                    current_omfsegment->alloc_table[i] = NULL;
                    break;
                }
            break;

            /********************/
            /*  Structure File  */
            /********************/
        case MEMORY_SET_FILE :
            current_omfsegment->first_file = (struct source_file *) data;
            break;

        case MEMORY_GET_FILE :
            *((struct source_file **) data) = current_omfsegment->first_file;
            break;

        case MEMORY_FREE_FILE :
            for(current_file = current_omfsegment->first_file; current_file; )
            {
                next_file = current_file->next;
                mem_free_sourcefile(current_file,(current_file==current_omfsegment->first_file)?1:0);
                current_file = next_file;
            }
            break;

            /*****************/
            /*  Opcode List  */
            /*****************/
        case MEMORY_ADD_OPCODE :
            /* Already here? */
            for(current_opcode = current_omfsegment->first_opcode; current_opcode; current_opcode = current_opcode->next)
                if(!my_stricmp(current_opcode->name,(char *) data))
                    return;

            /* Allocate memory */
            new_opcode = (struct item *) calloc(1,sizeof(struct item));
            if(new_opcode == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure item (opcode)");
            new_opcode->name = strdup((char *) data);
            if(new_opcode->name == NULL)
            {
                free(new_opcode);
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure item (opcode)");
            }

            /* Attaches the structure */
            if(current_omfsegment->first_opcode == NULL)
                current_omfsegment->first_opcode = new_opcode;
            else
                current_omfsegment->last_opcode->next = new_opcode;
            current_omfsegment->last_opcode = new_opcode;
            current_omfsegment->nb_opcode++;
            break;

        case MEMORY_GET_OPCODE_NB :
            *((int *) data) = current_omfsegment->nb_opcode;
            break;

        case MEMORY_GET_OPCODE :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_opcode)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_opcode)
            {
                *((char **)value) = current_omfsegment->tab_opcode[(*((int *)data))-1]->name;
                return;
            }

            /* Locate the structure */
            current_opcode = current_omfsegment->first_opcode;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_opcode = current_opcode->next;
            *((char **)value) = current_opcode->name;
            break;

        case MEMORY_SORT_OPCODE :
            if(current_omfsegment->nb_opcode == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_opcode)
                free(current_omfsegment->tab_opcode);
            current_omfsegment->tab_opcode = (struct item **) calloc(current_omfsegment->nb_opcode,sizeof(struct item *));
            if(current_omfsegment->tab_opcode == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_opcode table");

            /* Place the items */
            current_opcode = current_omfsegment->first_opcode;
            for(int i=0; current_opcode; current_opcode=current_opcode->next,i++)
                current_omfsegment->tab_opcode[i] = current_opcode;

            /* Sort items */
            qsort(current_omfsegment->tab_opcode,current_omfsegment->nb_opcode,sizeof(struct item *),compare_item);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_opcode; i++)
            {
                if(i == current_omfsegment->nb_opcode-1)
                    current_omfsegment->tab_opcode[i]->next = NULL;
                else
                    current_omfsegment->tab_opcode[i]->next = current_omfsegment->tab_opcode[i+1];
            }
            current_omfsegment->first_opcode = current_omfsegment->tab_opcode[0];
            current_omfsegment->last_opcode = current_omfsegment->tab_opcode[current_omfsegment->nb_opcode-1];
            break;

        case MEMORY_SEARCH_OPCODE :
            /* Init */
            *((struct item **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_opcode != NULL)
            {
                current_omfsegment->local_item.name = (char *)data;
                found_item_ptr = (struct item **) bsearch(&current_omfsegment->local_item_ptr,(void *)current_omfsegment->tab_opcode,current_omfsegment->nb_opcode,sizeof(struct item *),compare_item);
                if(found_item_ptr != NULL)
                    *((struct item **)value) = *found_item_ptr;
            }
            break;

        case MEMORY_FREE_OPCODE :
            for(current_opcode = current_omfsegment->first_opcode; current_opcode; )
            {
                next_opcode = current_opcode->next;
                mem_free_item(current_opcode);
                current_opcode = next_opcode;
            }
            current_omfsegment->nb_opcode = 0;
            current_omfsegment->first_opcode = NULL;
            current_omfsegment->last_opcode = NULL;
            if(current_omfsegment->tab_opcode)
                free(current_omfsegment->tab_opcode);
            current_omfsegment->tab_opcode = NULL;
            break;


            /***************/
            /*  Data List  */
            /***************/
        case MEMORY_ADD_DATA :
            /* Already here? */
            for(current_data = current_omfsegment->first_data; current_data; current_data = current_data->next)
                if(!my_stricmp(current_data->name,(char *) data))
                    return;

            /* Allocate memory */
            new_data = (struct item *) calloc(1,sizeof(struct item));
            if(new_data == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure item (data)");
            new_data->name = strdup((char *) data);
            if(new_data->name == NULL)
            {
                free(new_data);
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure item (data)");
            }

            /* Attaches the structure */
            if(current_omfsegment->first_data == NULL)
                current_omfsegment->first_data = new_data;
            else
                current_omfsegment->last_data->next = new_data;
            current_omfsegment->last_data = new_data;
            current_omfsegment->nb_data++;
            break;

        case MEMORY_GET_DATA_NB :
            *((int *) data) = current_omfsegment->nb_data;
            break;

        case MEMORY_GET_DATA :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_data)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_data)
            {
                *((char **)value) = current_omfsegment->tab_data[(*((int *)data))-1]->name;
                return;
            }

            /* Locate the structure */
            current_data = current_omfsegment->first_data;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_data = current_data->next;
            *((char **)value) = current_data->name;
            break;

        case MEMORY_SORT_DATA :
            if(current_omfsegment->nb_data == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_data)
                free(current_omfsegment->tab_data);
            current_omfsegment->tab_data = (struct item **) calloc(current_omfsegment->nb_data,sizeof(struct item *));
            if(current_omfsegment->tab_data == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_data table");

            /* Place the items */
            current_data = current_omfsegment->first_data;
            for(int i=0; current_data; current_data=current_data->next,i++)
                current_omfsegment->tab_data[i] = current_data;

            /* Sort items */
            qsort(current_omfsegment->tab_data,current_omfsegment->nb_data,sizeof(struct item *),compare_item);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_data; i++)
            {
                if(i == current_omfsegment->nb_data-1)
                    current_omfsegment->tab_data[i]->next = NULL;
                else
                    current_omfsegment->tab_data[i]->next = current_omfsegment->tab_data[i+1];
            }
            current_omfsegment->first_data = current_omfsegment->tab_data[0];
            current_omfsegment->last_data = current_omfsegment->tab_data[current_omfsegment->nb_data-1];
            break;

        case MEMORY_SEARCH_DATA :
            /* Init */
            *((struct item **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_data != NULL)
            {
                current_omfsegment->local_item.name = (char *) data;
                found_item_ptr = (struct item **) bsearch(&current_omfsegment->local_item_ptr,(void *)current_omfsegment->tab_data,current_omfsegment->nb_data,sizeof(struct item *),compare_item);
                if(found_item_ptr != NULL)
                    *((struct item **)value) = *found_item_ptr;
            }
            break;

        case MEMORY_FREE_DATA :
            for(current_data = current_omfsegment->first_data; current_data; )
            {
                next_data = current_data->next;
                mem_free_item(current_data);
                current_data = next_data;
            }
            current_omfsegment->nb_data = 0;
            current_omfsegment->first_data = NULL;
            current_omfsegment->last_data = NULL;
            if(current_omfsegment->tab_data)
                free(current_omfsegment->tab_data);
            current_omfsegment->tab_data = NULL;
            break;


            /********************/
            /*  Directive List  */
            /********************/
        case MEMORY_ADD_DIRECTIVE :
            /* Already here? */
            for(current_directive = current_omfsegment->first_directive; current_directive; current_directive = current_directive->next)
                if(!my_stricmp(current_directive->name,(char *) data))
                    return;

            /* Allocate memory */
            new_directive = (struct item *) calloc(1,sizeof(struct item));
            if(new_directive == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure item (directive)");
            new_directive->name = strdup((char *) data);
            if(new_directive->name == NULL)
            {
                free(new_directive);
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure item (directive)");
            }

            /* Attaches the structure */
            if(current_omfsegment->first_directive == NULL)
                current_omfsegment->first_directive = new_directive;
            else
                current_omfsegment->last_directive->next = new_directive;
            current_omfsegment->last_directive = new_directive;
            current_omfsegment->nb_directive++;
            break;

        case MEMORY_GET_DIRECTIVE_NB :
            *((int *) data) = current_omfsegment->nb_directive;
            break;

        case MEMORY_GET_DIRECTIVE :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_directive)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_directive)
            {
                *((char **)value) = current_omfsegment->tab_directive[(*((int *)data))-1]->name;
                return;
            }

            /* Locate the structure */
            current_directive = current_omfsegment->first_directive;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_directive = current_directive->next;
            *((char **)value) = current_directive->name;
            break;

        case MEMORY_SORT_DIRECTIVE :
            if(current_omfsegment->nb_directive == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_directive)
                free(current_omfsegment->tab_directive);
            current_omfsegment->tab_directive = (struct item **) calloc(current_omfsegment->nb_directive,sizeof(struct item *));
            if(current_omfsegment->tab_directive == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_directive table");

            /* Place the items */
            current_directive = current_omfsegment->first_directive;
            for(int i=0; current_directive; current_directive=current_directive->next,i++)
                current_omfsegment->tab_directive[i] = current_directive;

            /* Sort items */
            qsort(current_omfsegment->tab_directive,current_omfsegment->nb_directive,sizeof(struct item *),compare_item);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_directive; i++)
            {
                if(i == current_omfsegment->nb_directive-1)
                    current_omfsegment->tab_directive[i]->next = NULL;
                else
                    current_omfsegment->tab_directive[i]->next = current_omfsegment->tab_directive[i+1];
            }
            current_omfsegment->first_directive = current_omfsegment->tab_directive[0];
            current_omfsegment->last_directive = current_omfsegment->tab_directive[current_omfsegment->nb_directive-1];
            break;

        case MEMORY_SEARCH_DIRECTIVE :
            /* Init */
            *((struct item **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_directive != NULL)
            {
                current_omfsegment->local_item.name = (char *) data;
                found_item_ptr = (struct item **) bsearch(&current_omfsegment->local_item_ptr,(void *)current_omfsegment->tab_directive,current_omfsegment->nb_directive,sizeof(struct item *),compare_item);
                if(found_item_ptr != NULL)
                    *((struct item **)value) = *found_item_ptr;
            }
            break;

        case MEMORY_FREE_DIRECTIVE :
            for(current_directive = current_omfsegment->first_directive; current_directive; )
            {
                next_directive = current_directive->next;
                mem_free_item(current_directive);
                current_directive = next_directive;
            }
            current_omfsegment->nb_directive = 0;
            current_omfsegment->first_directive = NULL;
            current_omfsegment->last_directive = NULL;
            if(current_omfsegment->tab_directive)
                free(current_omfsegment->tab_directive);
            current_omfsegment->tab_directive = NULL;
            break;

            /********************************/
            /*  Directive Equivalence List  */
            /********************************/
        case MEMORY_ADD_DIREQU :
            /* Already here? */
            for(current_direqu = current_omfsegment->first_direqu; current_direqu; current_direqu = current_direqu->next)
                if(!my_stricmp(current_direqu->name,(char *) data))
                    return;

            /* Allocate memory */
            new_direqu = (struct item *) calloc(1,sizeof(struct item));
            if(new_direqu == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure item (equivalence)");
            new_direqu->name = strdup((char *) data);
            if(new_direqu->name == NULL)
            {
                free(new_direqu);
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure item (equivalence)");
            }

            /* Attaches the structure */
            if(current_omfsegment->first_direqu == NULL)
                current_omfsegment->first_direqu = new_direqu;
            else
                current_omfsegment->last_direqu->next = new_direqu;
            current_omfsegment->last_direqu = new_direqu;
            current_omfsegment->nb_direqu++;
            break;

        case MEMORY_GET_DIREQU_NB :
            *((int *) data) = current_omfsegment->nb_direqu;
            break;

        case MEMORY_GET_DIREQU :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_direqu)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_direqu)
            {
                *((char **)value) = current_omfsegment->tab_direqu[(*((int *)data))-1]->name;
                return;
            }

            /* Locate the structure */
            current_direqu = current_omfsegment->first_direqu;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_direqu = current_direqu->next;
            *((char **)value) = current_direqu->name;
            break;

        case MEMORY_SORT_DIREQU :
            if(current_omfsegment->nb_direqu == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_direqu)
                free(current_omfsegment->tab_direqu);
            current_omfsegment->tab_direqu = (struct item **) calloc(current_omfsegment->nb_direqu,sizeof(struct item *));
            if(current_omfsegment->tab_direqu == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_direqu table");

            /* Place the items */
            current_direqu = current_omfsegment->first_direqu;
            for(int i=0; current_direqu; current_direqu=current_direqu->next,i++)
                current_omfsegment->tab_direqu[i] = current_direqu;

            /* Sort items */
            qsort(current_omfsegment->tab_direqu,current_omfsegment->nb_direqu,sizeof(struct item *),compare_item);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_direqu; i++)
            {
                if(i == current_omfsegment->nb_direqu-1)
                    current_omfsegment->tab_direqu[i]->next = NULL;
                else
                    current_omfsegment->tab_direqu[i]->next = current_omfsegment->tab_direqu[i+1];
            }
            current_omfsegment->first_direqu = current_omfsegment->tab_direqu[0];
            current_omfsegment->last_direqu = current_omfsegment->tab_direqu[current_omfsegment->nb_direqu-1];
            break;

        case MEMORY_SEARCH_DIREQU :
            /* Init */
            *((struct item **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_direqu != NULL)
            {
                current_omfsegment->local_item.name = (char *) data;
                found_item_ptr = (struct item **) bsearch(&current_omfsegment->local_item_ptr,(void *)current_omfsegment->tab_direqu,current_omfsegment->nb_direqu,sizeof(struct item *),compare_item);
                if(found_item_ptr != NULL)
                    *((struct item **)value) = *found_item_ptr;
            }
            break;

        case MEMORY_FREE_DIREQU :
            for(current_direqu = current_omfsegment->first_direqu; current_direqu; )
            {
                next_direqu = current_direqu->next;
                mem_free_item(current_direqu);
                current_direqu = next_direqu;
            }
            current_omfsegment->nb_direqu = 0;
            current_omfsegment->first_direqu = NULL;
            current_omfsegment->last_direqu = NULL;
            if(current_omfsegment->tab_direqu)
                free(current_omfsegment->tab_direqu);
            current_omfsegment->tab_direqu = NULL;
            break;


            /******************/
            /*  Opcode Macro  */
            /******************/
        case MEMORY_ADD_MACRO :
            /* Attaches the structure */
            if(current_omfsegment->first_macro == NULL)
                current_omfsegment->first_macro = (struct macro *) data;
            else
                current_omfsegment->last_macro->next = (struct macro *) data;
            current_omfsegment->last_macro = (struct macro *) data;
            current_omfsegment->nb_macro++;
            break;

        case MEMORY_GET_MACRO_NB :
            *((int *) data) = current_omfsegment->nb_macro;
            break;

        case MEMORY_GET_MACRO :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_macro)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_macro)
            {
                *((struct macro **)value) = current_omfsegment->tab_macro[(*((int *)data))-1];
                return;
            }

            /* Locate the structure */
            current_macro = current_omfsegment->first_macro;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_macro = current_macro->next;
            *((struct macro **)value) = current_macro;
            break;

        case MEMORY_SORT_MACRO :
            if(current_omfsegment->nb_macro == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_macro)
                free(current_omfsegment->tab_macro);
            current_omfsegment->tab_macro = (struct macro **) calloc(current_omfsegment->nb_macro,sizeof(struct macro *));
            if(current_omfsegment->tab_macro == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_macro table");

            /* Place the items */
            current_macro = current_omfsegment->first_macro;
            for(int i=0; current_macro; current_macro=current_macro->next,i++)
                current_omfsegment->tab_macro[i] = current_macro;

            /* Sort items */
            qsort(current_omfsegment->tab_macro,current_omfsegment->nb_macro,sizeof(struct macro *),compare_macro);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_macro; i++)
            {
                if(i == current_omfsegment->nb_macro-1)
                    current_omfsegment->tab_macro[i]->next = NULL;
                else
                    current_omfsegment->tab_macro[i]->next = current_omfsegment->tab_macro[i+1];
            }
            current_omfsegment->first_macro = current_omfsegment->tab_macro[0];
            current_omfsegment->last_macro = current_omfsegment->tab_macro[current_omfsegment->nb_macro-1];
            break;

        case MEMORY_SEARCH_MACRO :
            /* Init */
            *((struct macro **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_macro != NULL)
            {
                current_omfsegment->local_macro.name = (char *) data;
                found_macro_ptr = (struct macro **) bsearch(&current_omfsegment->local_macro_ptr,(void *)current_omfsegment->tab_macro,current_omfsegment->nb_macro,sizeof(struct macro *),compare_macro);
                if(found_macro_ptr != NULL)
                    *((struct macro **)value) = *found_macro_ptr;
            }
            break;

        case MEMORY_FREE_MACRO :
            for(current_macro = current_omfsegment->first_macro; current_macro; )
            {
                next_macro = current_macro->next;
                mem_free_macro(current_macro);
                current_macro = next_macro;
            }
            current_omfsegment->nb_macro = 0;
            current_omfsegment->first_macro = NULL;
            current_omfsegment->last_macro = NULL;
            if(current_omfsegment->tab_macro)
                free(current_omfsegment->tab_macro);
            current_omfsegment->tab_macro = NULL;
            break;


            /******************/
            /*  Source Label  */
            /******************/
        case MEMORY_ADD_LABEL :
            /* Attaches the structure */
            if(current_omfsegment->first_label == NULL)
                current_omfsegment->first_label = (struct label *) data;
            else
                current_omfsegment->last_label->next = (struct label *) data;
            current_omfsegment->last_label = (struct label *) data;
            current_omfsegment->nb_label++;
            break;

        case MEMORY_GET_LABEL_NB :
            *((int *) data) = current_omfsegment->nb_label;
            break;

        case MEMORY_GET_LABEL :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_label)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_label)
            {
                *((struct label **)value) = current_omfsegment->tab_label[(*((int *)data))-1];
                return;
            }

            /* Locate the structure */
            current_label = current_omfsegment->first_label;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_label = current_label->next;
            *((struct label **)value) = current_label;
            break;

        case MEMORY_SORT_LABEL :
            if(current_omfsegment->nb_label == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_label)
                free(current_omfsegment->tab_label);
            current_omfsegment->tab_label = (struct label **) calloc(current_omfsegment->nb_label,sizeof(struct label *));
            if(current_omfsegment->tab_label == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_label table");

            /* Place the items */
            current_label = current_omfsegment->first_label;
            for(int i=0; current_label; current_label=current_label->next,i++)
                current_omfsegment->tab_label[i] = current_label;

            /* Sort items */
            qsort(current_omfsegment->tab_label,current_omfsegment->nb_label,sizeof(struct label *),compare_label);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_label; i++)
            {
                if(i == current_omfsegment->nb_label-1)
                    current_omfsegment->tab_label[i]->next = NULL;
                else
                    current_omfsegment->tab_label[i]->next = current_omfsegment->tab_label[i+1];
            }
            current_omfsegment->first_label = current_omfsegment->tab_label[0];
            current_omfsegment->last_label = current_omfsegment->tab_label[current_omfsegment->nb_label-1];
            break;

        case MEMORY_SEARCH_LABEL :
            /* Init */
            *((struct label **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_label != NULL)
            {
                current_omfsegment->local_label.name = (char *) data;
                found_label_ptr = (struct label **) bsearch(&current_omfsegment->local_label_ptr,(void *)current_omfsegment->tab_label,current_omfsegment->nb_label,sizeof(struct label *),compare_label);
                if(found_label_ptr != NULL)
                    *((struct label **)value) = *found_label_ptr;
            }
            break;

        case MEMORY_FREE_LABEL :
            for(current_label = current_omfsegment->first_label; current_label; )
            {
                next_label = current_label->next;
                mem_free_label(current_label);
                current_label = next_label;
            }
            current_omfsegment->nb_label = 0;
            current_omfsegment->first_label = NULL;
            current_omfsegment->last_label = NULL;
            if(current_omfsegment->tab_label)
                free(current_omfsegment->tab_label);
            current_omfsegment->tab_label = NULL;
            break;


            /************************/
            /*  Source Equivalence  */
            /************************/
        case MEMORY_ADD_EQUIVALENCE :
            /* Check the uniqueness */
            new_equivalence = (struct equivalence *) data;
            for(current_equivalence = current_omfsegment->first_equivalence; current_equivalence; current_equivalence=current_equivalence->next)
            {
                if(!strcmp(current_equivalence->name,new_equivalence->name) &&
                   current_equivalence->source_line == new_equivalence->source_line)
                {
                    /* Already Exists */
                    mem_free_equivalence(new_equivalence);
                    return;
                }
            }
            /* Attaches the structure */
            if(current_omfsegment->first_equivalence == NULL)
                current_omfsegment->first_equivalence = (struct equivalence *) data;
            else
                current_omfsegment->last_equivalence->next = (struct equivalence *) data;
            current_omfsegment->last_equivalence = (struct equivalence *) data;
            current_omfsegment->nb_equivalence++;
            break;

        case MEMORY_GET_EQUIVALENCE_NB :
            *((int *) data) = current_omfsegment->nb_equivalence;
            break;

        case MEMORY_GET_EQUIVALENCE :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_equivalence)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_equivalence)
            {
                *((struct equivalence **)value) = current_omfsegment->tab_equivalence[(*((int *)data))-1];
                return;
            }

            /* Locate the structure */
            current_equivalence = current_omfsegment->first_equivalence;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_equivalence = current_equivalence->next;
            *((struct equivalence **)value) = current_equivalence;
            break;

        case MEMORY_SORT_EQUIVALENCE :
            if(current_omfsegment->nb_equivalence == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_equivalence)
                free(current_omfsegment->tab_equivalence);
            current_omfsegment->tab_equivalence = (struct equivalence **) calloc(current_omfsegment->nb_equivalence,sizeof(struct equivalence *));
            if(current_omfsegment->tab_equivalence == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_equivalence table");

            /* Place the items */
            current_equivalence = current_omfsegment->first_equivalence;
            for(int i=0; current_equivalence; current_equivalence=current_equivalence->next,i++)
                current_omfsegment->tab_equivalence[i] = current_equivalence;

            /* Sort items */
            qsort(current_omfsegment->tab_equivalence,current_omfsegment->nb_equivalence,sizeof(struct equivalence *),compare_equivalence);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_equivalence; i++)
            {
                if(i == current_omfsegment->nb_equivalence-1)
                    current_omfsegment->tab_equivalence[i]->next = NULL;
                else
                    current_omfsegment->tab_equivalence[i]->next = current_omfsegment->tab_equivalence[i+1];
            }
            current_omfsegment->first_equivalence = current_omfsegment->tab_equivalence[0];
            current_omfsegment->last_equivalence = current_omfsegment->tab_equivalence[current_omfsegment->nb_equivalence-1];
            break;

        case MEMORY_SEARCH_EQUIVALENCE :
            /* Init */
            *((struct equivalence **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_equivalence != NULL)
            {
                current_omfsegment->local_equivalence.name = (char *) data;
                found_equivalence_ptr = (struct equivalence **) bsearch(&current_omfsegment->local_equivalence_ptr,(void *)current_omfsegment->tab_equivalence,current_omfsegment->nb_equivalence,sizeof(struct equivalence *),compare_equivalence);
                if(found_equivalence_ptr != NULL)
                    *((struct equivalence **)value) = *found_equivalence_ptr;
            }
            else
            {
                /* Search via the chain list */
                for(current_equivalence=current_omfsegment->first_equivalence; current_equivalence; current_equivalence=current_equivalence->next)
                    if(!strcmp(current_equivalence->name,(char *)data))
                    {
                        *((struct equivalence **)value) = current_equivalence;
                        break;
                    }
            }
            break;

        case MEMORY_FREE_EQUIVALENCE :
            for(current_equivalence = current_omfsegment->first_equivalence; current_equivalence; )
            {
                next_equivalence = current_equivalence->next;
                mem_free_equivalence(current_equivalence);
                current_equivalence = next_equivalence;
            }
            current_omfsegment->nb_equivalence = 0;
            current_omfsegment->first_equivalence = NULL;
            current_omfsegment->last_equivalence = NULL;
            if(current_omfsegment->tab_equivalence)
                free(current_omfsegment->tab_equivalence);
            current_omfsegment->tab_equivalence = NULL;
            break;


            /*********************/
            /*  Source Variable  */
            /*********************/
        case MEMORY_ADD_VARIABLE :
            /* Attaches the structure */
            if(current_omfsegment->first_variable == NULL)
                current_omfsegment->first_variable = (struct variable *) data;
            else
                current_omfsegment->last_variable->next = (struct variable *) data;
            current_omfsegment->last_variable = (struct variable *) data;
            current_omfsegment->nb_variable++;
            break;

        case MEMORY_GET_VARIABLE_NB :
            *((int *) data) = current_omfsegment->nb_variable;
            break;

        case MEMORY_GET_VARIABLE :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_variable)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_variable)
            {
                *((struct variable **)value) = current_omfsegment->tab_variable[(*((int *)data))-1];
                return;
            }

            /* Locate the structure */
            current_variable = current_omfsegment->first_variable;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_variable = current_variable->next;
            *((struct variable **)value) = current_variable;
            break;

        case MEMORY_SORT_VARIABLE :
            if(current_omfsegment->nb_variable == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_variable)
                free(current_omfsegment->tab_variable);
            current_omfsegment->tab_variable = (struct variable **) calloc(current_omfsegment->nb_variable,sizeof(struct variable *));
            if(current_omfsegment->tab_variable == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_variable table");

            /* Place the items */
            current_variable = current_omfsegment->first_variable;
            for(int i=0; current_variable; current_variable=current_variable->next,i++)
                current_omfsegment->tab_variable[i] = current_variable;

            /* Sort items */
            qsort(current_omfsegment->tab_variable,current_omfsegment->nb_variable,sizeof(struct variable *),compare_variable);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_variable; i++)
            {
                if(i == current_omfsegment->nb_variable-1)
                    current_omfsegment->tab_variable[i]->next = NULL;
                else
                    current_omfsegment->tab_variable[i]->next = current_omfsegment->tab_variable[i+1];
            }
            current_omfsegment->first_variable = current_omfsegment->tab_variable[0];
            current_omfsegment->last_variable = current_omfsegment->tab_variable[current_omfsegment->nb_variable-1];
            break;

        case MEMORY_SEARCH_VARIABLE :
            /* Init */
            *((struct variable **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_variable != NULL)
            {
                current_omfsegment->local_variable.name = (char *) data;
                found_variable_ptr = (struct variable **) bsearch(&current_omfsegment->local_variable_ptr,(void *)current_omfsegment->tab_variable,current_omfsegment->nb_variable,sizeof(struct variable *),compare_variable);
                if(found_variable_ptr != NULL)
                    *((struct variable **)value) = *found_variable_ptr;
            }
            else
            {
                /* Search via the chain list */
                for(current_variable=current_omfsegment->first_variable; current_variable; current_variable=current_variable->next)
                    if(!strcmp(current_variable->name,(char *)data))
                    {
                        *((struct variable **)value) = current_variable;
                        break;
                    }
            }
            break;

        case MEMORY_FREE_VARIABLE :
            for(current_variable = current_omfsegment->first_variable; current_variable; )
            {
                next_variable = current_variable->next;
                mem_free_variable(current_variable);
                current_variable = next_variable;
            }
            current_omfsegment->nb_variable = 0;
            current_omfsegment->first_variable = NULL;
            current_omfsegment->last_variable = NULL;
            if(current_omfsegment->tab_variable)
                free(current_omfsegment->tab_variable);
            current_omfsegment->tab_variable = NULL;
            break;

            /*********************/
            /*  Source External  */
            /*********************/
        case MEMORY_ADD_EXTERNAL :
            /* Attaches the structure */
            if(current_omfsegment->first_external == NULL)
                current_omfsegment->first_external = (struct external *) data;
            else
                current_omfsegment->last_external->next = (struct external *) data;
            current_omfsegment->last_external = (struct external *) data;
            current_omfsegment->nb_external++;
            break;

        case MEMORY_GET_EXTERNAL_NB :
            *((int *) data) = current_omfsegment->nb_external;
            break;

        case MEMORY_GET_EXTERNAL :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_external)
                return;

            /* Direct access through the table */
            if(current_omfsegment->tab_external)
            {
                *((struct external **)value) = current_omfsegment->tab_external[(*((int *)data))-1];
                return;
            }

            /* Locate the structure */
            current_external = current_omfsegment->first_external;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_external = current_external->next;
            *((struct external **)value) = current_external;
            break;

        case MEMORY_SORT_EXTERNAL :
            if(current_omfsegment->nb_external == 0)
                return;

            /* Allocate memory */
            if(current_omfsegment->tab_external)
                free(current_omfsegment->tab_external);
            current_omfsegment->tab_external = (struct external **) calloc(current_omfsegment->nb_external,sizeof(struct external *));
            if(current_omfsegment->tab_external == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for tab_external table");

            /* Place the items */
            current_external = current_omfsegment->first_external;
            for(int i=0; current_external; current_external=current_external->next,i++)
                current_omfsegment->tab_external[i] = current_external;

            /* Sort items */
            qsort(current_omfsegment->tab_external,current_omfsegment->nb_external,sizeof(struct external *),compare_external);

            /* Replace the links */
            for(int i=0; i<current_omfsegment->nb_external; i++)
            {
                if(i == current_omfsegment->nb_external-1)
                    current_omfsegment->tab_external[i]->next = NULL;
                else
                    current_omfsegment->tab_external[i]->next = current_omfsegment->tab_external[i+1];
            }
            current_omfsegment->first_external = current_omfsegment->tab_external[0];
            current_omfsegment->last_external = current_omfsegment->tab_external[current_omfsegment->nb_external-1];
            break;

        case MEMORY_SEARCH_EXTERNAL :
            /* Init */
            *((struct external **)value) = NULL;

            /** Search by the table **/
            if(current_omfsegment->tab_external != NULL)
            {
                current_omfsegment->local_external.name = (char *) data;
                found_external_ptr = (struct external **) bsearch(&current_omfsegment->local_external_ptr,(void *)current_omfsegment->tab_external,current_omfsegment->nb_external,sizeof(struct external *),compare_external);
                if(found_external_ptr != NULL)
                    *((struct external **)value) = *found_external_ptr;
            }
            else
            {
                /* Search via the chain list */
                for(current_external=current_omfsegment->first_external; current_external; current_external=current_external->next)
                    if(!strcmp(current_external->name,(char *)data))
                    {
                        *((struct external **)value) = current_external;
                        break;
                    }
            }
            break;

        case MEMORY_FREE_EXTERNAL :
            for(current_external = current_omfsegment->first_external; current_external; )
            {
                next_external = current_external->next;
                mem_free_external(current_external);
                current_external = next_external;
            }
            current_omfsegment->nb_external = 0;
            current_omfsegment->first_external = NULL;
            current_omfsegment->last_external = NULL;
            if(current_omfsegment->tab_external)
                free(current_omfsegment->tab_external);
            current_omfsegment->tab_external = NULL;
            break;

            /*****************************************************************************/
            /*  Label ENT which must be rewired because defined as ENT Labal1, Label2... */
            /*****************************************************************************/
        case MEMORY_ADD_GLOBAL :
            /* We do not want the same label twice */
            for(current_global = current_omfsegment->first_global; current_global; current_global = current_global->next)
                if(!strcmp(current_global->name,(char *)data))
                    return;

            /* Allocate memory */
            new_global = (struct global *) calloc(1,sizeof(struct global));
            if(new_global == NULL)
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for global structure");
            new_global->source_line = (struct source_line *) value;
            new_global->name = strdup((char *)data);
            if(new_global->name == NULL)
            {
                free(new_global);
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for global structure");
            }

            /* Attaches the structure */
            if(current_omfsegment->first_global == NULL)
                current_omfsegment->first_global = new_global;
            else
                current_omfsegment->last_global->next = new_global;
            current_omfsegment->last_global = new_global;
            current_omfsegment->nb_global++;
            break;

        case MEMORY_GET_GLOBAL_NB :
            *((int *) data) = current_omfsegment->nb_global;
            break;

        case MEMORY_GET_GLOBAL :
            /* Init */
            *((char **)value) = NULL;

            /* Verify expected value */
            if(*((int *)data) <= 0 || *((int *)data) > current_omfsegment->nb_global)
                return;

            /* Locate the structure */
            current_global = current_omfsegment->first_global;
            for(int i=0; i<(*((int *)data))-1; i++)
                current_global = current_global->next;
            *((struct global **)value) = current_global;
            break;

        case MEMORY_FREE_GLOBAL :
            for(current_global = current_omfsegment->first_global; current_global; )
            {
                next_global = current_global->next;
                mem_free_global(current_global);
                current_global = next_global;
            }
            current_omfsegment->nb_global = 0;
            current_omfsegment->first_global = NULL;
            current_omfsegment->last_global = NULL;
            break;

        default :
            break;
    }
}


/****************************************************/
/*  my_stricmp() : Case insensitive string compare. */
/****************************************************/
int my_stricmp(char *string1, char *string2)
{
#if defined(WIN32) || defined(WIN64) 
    return(stricmp(string1,string2));
#else
    return(strcasecmp(string1,string2));
#endif
}

/**************************************************************/
/*  my_strnicmp() :  Case insensitive string compare w/lenth. */
/**************************************************************/
int my_strnicmp(char *string1, char *string2, size_t length)
{
#if defined(WIN32) || defined(WIN64) 
    return(strnicmp(string1,string2,length));
#else
    return(strncasecmp(string1,string2,length));
#endif
}


/***********************************************/
/*  my_printf64() :  Integer 64 into a string. */
/***********************************************/
void my_printf64(int64_t value_64, char *buffer)
{
#if defined(WIN32) || defined(WIN64) 
    sprintf(buffer,"%I64d",value_64);
#else
    sprintf(buffer,"%"PRId64, value_64);
#endif
}


/*************************************************/
/*  my_atoi64() :  String into a Integer 64 bit. */
/*************************************************/
int64_t my_atoi64(char *expression)
{
#if defined(WIN32) || defined(WIN64) 
    return(_atoi64(expression));
#else
    return(atoll(expression));
#endif
}


/**************************************************/
/*  bo_memcpy() : Copy respecting the Byte order. */
/**************************************************/
void bo_memcpy(void *dst, void *src, size_t nb_byte)
{
    unsigned char data_src[10];
    unsigned char data_dst[10];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /** Classic memcpy for Intel processor (Little Endian) **/
    if(param->byte_order == BYTE_ORDER_INTEL || nb_byte == 1)
        memcpy(dst,src,nb_byte);
    else
    {
        /* Change byte order for Big Endian processor */
        memcpy(&data_src[0],src,nb_byte);

        /* 2 Byte */
        if(nb_byte == 2)
        {
            data_dst[0] = data_src[1];
            data_dst[1] = data_src[0];
        }
        /* 4 Byte */
        else if(nb_byte == 4)
        {
            data_dst[0] = data_src[3];
            data_dst[1] = data_src[2];
            data_dst[2] = data_src[1];
            data_dst[3] = data_src[0];
        }

        /* Copy bytes */
        memcpy(dst,&data_dst[0],nb_byte);
    }
}


/*******************************************************************/
/*  GetFileProperCasePath() :  Case insensitive Search for a file. */
/*******************************************************************/
char *GetFileProperCasePath(char *file_path_arg)
{
    int i, is_error, nb_found;
    char folder_current[1024];
    char folder_path[1024];
    char file_path[1024];
    char file_name[1024];
    static char file_path_case[1024];
    int nb_file_name;
    char **tab_file_name;

    /* Init */
    strcpy(file_path_case,"");

    /** We need an absolute path **/
#if defined(WIN32) || defined(WIN64)
    if(file_path_arg[1] != ':')
    {
        /* Current directory */
        GetCurrentDirectory(1024,folder_current);
        if(strlen(folder_current) > 0)
            if(folder_current[strlen(folder_current)-1] != '\\')
                strcat(folder_current,"\\");
        sprintf(file_path,"%s%s",folder_current,file_path_arg);
    }
    else
        strcpy(file_path,file_path_arg);
#else
    if(file_path_arg[0] != '/')
    {
        /* Current directory */
        getcwd(folder_current,1024);
        if(strlen(folder_current) > 0)
            if(folder_current[strlen(folder_current)-1] != '/')
                strcat(folder_current,"/");
        sprintf(file_path,"%s%s",folder_current,file_path_arg);
    }
    else
        strcpy(file_path,file_path_arg);
#endif

    /** Retrieve the File + File name **/
    strcpy(folder_path,file_path);
    strcpy(file_name,file_path);
    for(i=(int)strlen(folder_path); i>=0; i--)
        if(folder_path[i] == '\\' || folder_path[i] == '/')
        {
            folder_path[i] = '\0';
            strcpy(file_name,&folder_path[i+1]);
            break;
        }

    /** List of Directory Lines **/
    tab_file_name = GetFolderFileList(folder_path,&nb_file_name,&is_error);
    if(tab_file_name == NULL)
        return(NULL);

    /** Search the Filename **/
    for(i=0,nb_found=0; i<nb_file_name; i++)
    {
        if(!my_stricmp(file_name,tab_file_name[i]))
        {
            sprintf(file_path_case,"%s%s%s",folder_path,FOLDER_SEPARATOR,tab_file_name[i]);
            nb_found++;
        }
    }

    /* Memory release */
    mem_free_list(nb_file_name,tab_file_name);

    /* Nothing found */
    if(nb_found == 0)
        return(NULL);

    /* More than one File => ambiguous */
    if(nb_found > 1)
        return(NULL);

    /* Only one => OK */
    return(&file_path_case[0]);
}


/************************************************************************/
/*  GetFolderFileList() : Retrieves the list of Files from a directory. */
/************************************************************************/
char **GetFolderFileList(char *folder_path, int *nb_file_rtn, int *is_error)
{
#if defined(WIN32) || defined(WIN64)  
    int rc;
    intptr_t hFile;
    int first_time;
    struct _finddata_t c_file;
#else
    DIR *dp;
    struct dirent *dir_entry;
#endif
    int nb_item;
    struct item *first_item;
    struct item *last_item;
    struct item *current_item;
    struct item *next_item;
    int nb_file;
    char **tab_file_name;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM,&param,NULL,NULL);

    /* Init */
    *is_error = 0;
    nb_file = 0;
    tab_file_name = NULL;
    nb_item = 0;
    first_item = NULL;
    last_item = NULL;
    
    /* Prepare the file */
    strcpy(param->buffer_folder_path,folder_path);
    if(strlen(param->buffer_folder_path) > 0)
        if(param->buffer_folder_path[strlen(param->buffer_folder_path)-1] != '\\' && param->buffer_folder_path[strlen(param->buffer_folder_path)-1] != '/')
            strcat(param->buffer_folder_path,FOLDER_SEPARATOR);

#if defined(WIN32) || defined(WIN64)  
    /** We loop on all the Files present **/
    sprintf(param->buffer_file_path,"%s*.*",param->buffer_folder_path);
    first_time = 1;
    while(1)
    {
        if(first_time == 1)
        {
            hFile = _findfirst(param->buffer_file_path,&c_file);
            rc = (int) hFile;
            my_File(FILE_DECLARE_DIRECTORY,&hFile);
        }
        else
            rc = _findnext(hFile,&c_file);

        /* We analyze the result */
        if(rc == -1)
            break;    /* no more files */

        /** This entry is treated **/
        first_time++;

        /* Some entry is unknown */
        if((c_file.attrib & _A_SUBDIR) == _A_SUBDIR)
            continue;

        /** Keep the File **/
        /* Allocate memory */
        current_item = (struct item *) calloc(1,sizeof(struct item));
        if(current_item == NULL)
        {
            my_File(FILE_FREE_DIRECTORY,&hFile);
            _findclose(hFile);
            /* Memory release */
            for(current_item = first_item; current_item; current_item = next_item)
            {
                next_item = current_item->next;
                if(current_item->name)
                    free(current_item->name);
                free(current_item);
            }
            *is_error = 1;
            return(NULL);
        }
        current_item->name = strdup(c_file.name);
        if(current_item->name == NULL)
        {
            my_File(FILE_FREE_DIRECTORY,&hFile);
            _findclose(hFile);
            /* Memory release */
            for(current_item = first_item; current_item; current_item = next_item)
            {
                next_item = current_item->next;
                if(current_item->name)
                    free(current_item->name);
                free(current_item);
            }
            *is_error = 1;
            return(NULL);
        }
        /* Addition to the list */
        nb_item++;
        if(first_item == NULL)
            first_item = current_item;
        else
            last_item->next = current_item;
        last_item = current_item;
    }

    /* We are closing */
    my_File(FILE_FREE_DIRECTORY,&hFile);
    _findclose(hFile);
#else  
    /* Opening the directory */
    dp = opendir(folder_path);
    if(dp == NULL)
    {
        *is_error = 1;
        return(NULL);
    }

    /** Loop on all Files **/
    for(;;)
    {
        /* Retrieving an entry */
        dir_entry = readdir(dp);
        if(dir_entry == NULL)
            break;

        /* Ignore Invisible Files */
        if(dir_entry->d_name[0] == '.')
            continue;
        /* Ignore Directories */
        if(dir_entry->d_type == DT_DIR)
            continue;

        /** Keep the File **/
        /* Allocate memory */
        current_item = (struct item *) calloc(1,sizeof(struct item));
        if(current_item == NULL)
        {
            closedir(dp);
            /* Memory release */
            for(current_item = first_item; current_item; current_item = next_item)
            {
                next_item = current_item->next;
                if(current_item->name)
                    free(current_item->name);
                free(current_item);
            }
            *is_error = 1;
            return(NULL);
        }
        current_item->name = strdup(dir_entry->d_name);
        if(current_item->name == NULL)
        {
            closedir(dp);
            /* Memory release */
            for(current_item = first_item; current_item; current_item = next_item)
            {
                next_item = current_item->next;
                if(current_item->name)
                    free(current_item->name);
                free(current_item);
            }
            *is_error = 1;
            return(NULL);
        }
        /* Addition to the list */
        nb_item++;
        if(first_item == NULL)
            first_item = current_item;
        else
            last_item->next = current_item;
        last_item = current_item;
    }

    /* Closing the directory */
    closedir(dp);
#endif

    /* Nothing found ? */
    if(nb_item == 0)
    {
        *nb_file_rtn = 0;
        return(NULL);
    }

    /** Conversion List -> Table **/
    tab_file_name = (char **) calloc(nb_item,sizeof(char *));
    if(tab_file_name == NULL)
    {
        /* Memory release */
        for(current_item = first_item; current_item; current_item = next_item)
        {
            next_item = current_item->next;
            if(current_item->name)
                free(current_item->name);
            free(current_item);
        }
        *is_error = 1;
        return(NULL);
    }

    /* Adding names */
    for(current_item = first_item; current_item; current_item = current_item->next)
        tab_file_name[nb_file++] = current_item->name;

    /* Release list (but not name) */
    for(current_item = first_item; current_item; current_item = next_item)
    {
        next_item = current_item->next;
        free(current_item);
    }

    /* Return the list */
    *nb_file_rtn = nb_file;
    return(tab_file_name);
}


/*************************************************************/
/*  my_IsFileExist() : Determine if the File exists on disk. */
/*************************************************************/
int my_IsFileExist(char *file_path)
{
    FILE *fd;
    int i;
    char *file_case_path = NULL;
    char file_path_OS[1024] = "";

    /** Names are not case-sensitive on Windows **/
#if defined(WIN32) || defined(WIN64) 
    /* Windows path */
    strcpy(file_path_OS,file_path);
    for(i=0; i<(int)strlen(file_path_OS); i++)
        if(file_path_OS[i] == '/')
            file_path_OS[i] = '\\';

    /* Opening the File */
    fd = fopen(file_path_OS,"r");
    if(fd == NULL)
        return(0);     /* Does not exist */

    /* Closing */
    fclose(fd);

    /* Exists */
    return(1);
#else
    /** The names are case-sensitive under Unix **/
    /* Unix Way */
    strcpy(file_path_OS,file_path);
    for(i=0; i<(int)strlen(file_path_OS); i++)
        if(file_path_OS[i] == '\\')
            file_path_OS[i] = '/';

    /* Opening the File */
    fd = fopen(file_path_OS,"r");
    if(fd != NULL)
    {
        /* Exists */
        fclose(fd);
        return(1);
    }

    /** Find the exact name of the File in the directory **/
    file_case_path = GetFileProperCasePath(file_path);
    if(file_case_path != NULL)
        return(1);    /* found */

    /* Not found */
    return(0);
#endif
}


/**********************************************************/
/*  LoadTextFileData() :  Retrieve data from a text File. */
/**********************************************************/
unsigned char *LoadTextFileData(char *file_path, size_t *data_length_rtn)
{
    FILE *fd;
    size_t nb_read, file_size;
    unsigned char *data;
    char *file_case_path = NULL;
    char file_path_OS[1024] = "";

    /** Opening the File **/
    fd = fopen(file_path,"r");
    if(fd == NULL)
    {
#if defined(WIN32) || defined(WIN64) 
        return(NULL);
#else
        /** The names are case-sensitive under Unix **/
        /* Quick Win => replaces the .s with .S */
        if(strlen(file_path) > 2)
            if(file_path[strlen(file_path)-2] == '.' && toupper(file_path[strlen(file_path)-1]) == 'S')
            {
                /* Change Case of the .s */
                strcpy(file_path_OS,file_path);
                if(file_path_OS[strlen(file_path_OS)-1] == 'S')
                    file_path_OS[strlen(file_path_OS)-1] = 's';
                else
                    file_path_OS[strlen(file_path_OS)-1] = 'S';

                /* Re-open file */
                fd = fopen(file_path_OS,"r");
            }

        /* Find the exact name of the File in the directory */
        if(fd == NULL)
        {
            file_case_path = GetFileProperCasePath(file_path);
            if(file_case_path != NULL)
                fd = fopen(file_case_path,"r");
        }

        /* Nothing found */
        if(fd == NULL)
            return(NULL);
#endif
    }

    /* File Size */
    fseek(fd,0L,SEEK_END);
    file_size = ftell(fd);
    fseek(fd,0L,SEEK_SET);

    /* Allocate memory */
    data = (unsigned char *) calloc(1,file_size+1);
    if(data == NULL)
    {
        fclose(fd);
        return(NULL);
    }

    /* Reading data */
    nb_read = fread(data,1,file_size,fd);
    if(nb_read < 0)
    {
        free(data);
        fclose(fd);
        return(NULL);
    }
    data[nb_read] = '\0';

    /* Close the File */
    fclose(fd);

    /* Return data and size */
    *data_length_rtn = nb_read;
    return(data);
}


/**************************************************************/
/*  LoadBinaryFileData() :  Retrieve data from a binary File. */
/**************************************************************/
unsigned char *LoadBinaryFileData(char *file_path, size_t *data_length_rtn)
{
    FILE *fd;
    size_t nb_read, file_size;
    unsigned char *data;
    char *file_case_path = NULL;

    /* Opening the File */
#if defined(WIN32) || defined(WIN64) 
    fd = fopen(file_path,"rb");
#else
    /** The names are case-sensitive under Unix **/
    fd = fopen(file_path,"r");
    if(fd == NULL)
    {
        /* Find the exact name of the File in the directory */
        file_case_path = GetFileProperCasePath(file_path);
        if(file_case_path != NULL)
            fd = fopen(file_case_path,"r");
    }
#endif
    if(fd == NULL)
        return(NULL);

    /* File Size */
    fseek(fd,0L,SEEK_END);
    file_size = ftell(fd);
    fseek(fd,0L,SEEK_SET);

    /* Allocate memory */
    data = (unsigned char *) calloc(1,file_size+1);
    if(data == NULL)
    {
        fclose(fd);
        return(NULL);
    }

    /* Reading data */
    nb_read = (int) fread(data,1,file_size,fd);
    if(nb_read < 0)
    {
        free(data);
        fclose(fd);
        return(NULL);
    }

    /* Close the File */
    fclose(fd);

    /* Return data and size */
    *data_length_rtn = nb_read;
    return(data);
}


/***************************************************************/
/*  GetLabelFromLine() :  Isolates lablel component of a line. */
/***************************************************************/
int GetLabelFromLine(char *data, int offset, char *value_rtn)
{
    int i, length;

    /* Init */
    strcpy(value_rtn,"");

    /* Do we have something? */
    if(data[offset] == ' ' || data[offset] == '\n')
    {
        /* Empty, so we'll look for the next character */
        for(length=0; length<(int)strlen(data); length++)
            if(data[offset+length] != ' ' && data[offset+length] != '\t')
                break;
    }
    else
    {
        /* Copy the value */
        for(i=0; data[i+offset] != ' ' && data[i+offset] != '\t' && data[i+offset] != '\0'; i++)
            value_rtn[i] = data[i+offset];
        value_rtn[i] = '\0';

        /* Eliminate the empty area that follows */
        for(length=i; length<(int)strlen(data); length++)
            if(data[offset+length] != ' ' && data[offset+length] != '\t')
                break;
    }

    /* Returns the new offset */
    return(offset+length);
}


/******************************************************************/
/*  GetOpcodeFromLine() :  Isolates opcode component of the line. */
/******************************************************************/
int GetOpcodeFromLine(char *data, int offset, char *value_rtn)
{
    int i;

    /* Init */
    strcpy(value_rtn,"");

    /* nothing */
    if(strlen(&data[offset]) == 0)
        return(offset);

    /* Copy the value */
    for(i=0; data[i+offset] != ' ' && data[i+offset] != '\t' && data[i+offset] != '\0'; i++)
        value_rtn[i] = data[i+offset];
    value_rtn[i] = '\0';

    /* Returns the new offset */
    return(offset+i);
}


/*****************************************************/
/*  CleanBuffer() : We clean up a buffer like @Trim  */
/*****************************************************/
void CleanBuffer(char *buffer)
{
    /* trim end of string */
    int length = (int)strlen(buffer);
    for(int i = length-1; i >= 0; i--)
    {
        if(buffer[i] == '\0' || buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t')
            buffer[i] = '\0';
        else
            break;
    }

    /* trim start of string */
    length = (int)strlen(buffer);
    int j = 0;
    for(int i = 0; i < length; i++)
    {
        if(buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t')
            j++;
        else
            break;
    }

    /* move chars in string if needed */
    if(j > 0)
        memmove(&buffer[0],&buffer[j],(length-j)+1);
}


/*****************************************************/
/*  CleanUpName() : Remove the 'or' around the name  */
/*****************************************************/
void CleanUpName(char *buffer)
{
    size_t length = strlen(buffer);
    if(length < 2)
        return;

    /* Are there 'or' around the name? */
    if((buffer[0] == '\'' && buffer[length-1] == '\'') || (buffer[0] == '"' && buffer[length-1] == '"'))
    {
        memmove(&buffer[0],&buffer[1],length-2);
        buffer[length-2] = '\0';
    }
}


/**************************************************************/
/*  GetFolderFromPath() :  Extract the directory from a path. */
/**************************************************************/
void GetFolderFromPath(char *file_path, char *folder_path_rtn)
{
    int i;

    /* Init */
    strcpy(folder_path_rtn,"");

    /* extract the path */
    for(i=(int)strlen(file_path); i>=0; i--)
        if(file_path[i] == '\\' || file_path[i] == '/')
        {
            memcpy(folder_path_rtn,file_path,i+1);
            folder_path_rtn[i+1] = '\0';
            break;
        }
}


/******************************************************************/
/*  ExtractAllIem() :  Separate the different elements of a line. */
/******************************************************************/
struct item *ExtractAllIem(char *data)
{
    int end = 0, length = 0, offset = 0;
    struct item *current_item = NULL;
    struct item *first_item = NULL;
    struct item *last_item = NULL;
    int value_type = 0;
    char value[1024];

    /* Particular case : Empty line */
    length = (int) strlen(data);
    if(length == 0)
        return(NULL);

    /* Particular case : Line comment */
    if(data[0] == '*' || data[0] == ';')
    {
        current_item = mem_alloc_item(data,TYPE_DATA);
        return(current_item);
    }

    /** Walk the line **/
    value[0] = '\0';
    for(int i=0; i<length; i++)
    {
        /** Handle the separators **/
        if(data[i] == '\t' || data[i] == ' ')
        {
            if(offset == 0)                          /* 1st value */
            {
                value[offset++] = data[i];
                value_type = TYPE_SEPARATOR;
            }
            else if(value_type == TYPE_SEPARATOR)   /* Continue with separators */
                value[offset++] = data[i];
            else
            {
                /* End of the previous value DATA */
                value[offset] = '\0';
                current_item = mem_alloc_item(value,value_type);
                if(first_item == NULL)
                    first_item = current_item;
                else
                    last_item->next = current_item;
                last_item = current_item;

                /* Start of a new SEPARATOR value */
                offset = 0;
                value[offset++] = data[i];
                value_type = TYPE_SEPARATOR;
            }
        }
        else
        {
            if(offset == 0)                          /* 1st value */
            {
                /* Particular case : a DATA value starting with one; is a comment that goes to the end of the line */
                if(data[i] == ';')
                {
                    current_item = mem_alloc_item(&data[i],TYPE_DATA);
                    if(first_item == NULL)
                        first_item = current_item;
                    else
                        last_item->next = current_item;
                    last_item = current_item;
                    return(first_item);
                }

                /* Particular case for "and ': We go until' the end */
                if(data[i] == '"' || data[i] == '\'')
                {
                    end = 0;
                    /* Is there an end of chain? */
                    for(int j=i+1; j<length; j++)
                        if(data[j] == data[i])
                        {
                            end = j;
                            break;
                        }

                    /* We take all these data together */
                    if(end)
                    {
                        memcpy(&value[offset],&data[i],end-i+1);
                        offset += end-i+1;
                        i += (end-i+1)-1;    /* -1 or i++ */
                        continue;
                    }
                }

                /* 1st value */
                value[offset++] = data[i];
                value_type = TYPE_DATA;
            }
            else if(value_type == TYPE_DATA)   /* Continue in the DATA */
            {
                /* Particular case for "and ': We go until' the end */
                if(data[i] == '"' || data[i] == '\'')
                {
                    end = 0;
                    /* Is there an end of chain? */
                    for(int j=i+1; j<length; j++)
                        if(data[j] == data[i])
                        {
                            end = j;
                            break;
                        }

                    /* We take all these data together */
                    if(end)
                    {
                        memcpy(&value[offset],&data[i],end-i+1);
                        offset += end-i+1;
                        i += (end-i+1)-1;    /* -1 or i++ */
                        continue;
                    }
                }

                /* One more DATA */
                value[offset++] = data[i];
            }
            else
            {
                /* End of previous value SEPARATOR */
                value[offset] = '\0';
                current_item = mem_alloc_item(value,value_type);
                if(first_item == NULL)
                    first_item = current_item;
                else
                    last_item->next = current_item;
                last_item = current_item;

                /* new DATA */
                offset = 0;
                value_type = TYPE_DATA;

                /** Beginning of a new value DATA **/
                /* Particular case : a DATA value starting with one; is a comment that goes to the end of the line */
                if(data[i] == ';')
                {
                    current_item = mem_alloc_item(&data[i],TYPE_DATA);
                    if(first_item == NULL)
                        first_item = current_item;
                    else
                        last_item->next = current_item;
                    last_item = current_item;
                    return(first_item);
                }

                /* Particular case for "and ': We go until' the end */
                if(data[i] == '"' || data[i] == '\'')
                {
                    end = 0;
                    /* Is there an end of chain? */
                    for(int j=i+1; j<length; j++)
                        if(data[j] == data[i])
                        {
                            end = j;
                            break;
                        }

                    /* We take all these data together */
                    if(end)
                    {
                        memcpy(&value[offset],&data[i],end-i+1);
                        offset += end-i+1;
                        i += (end-i+1)-1;    /* -1 or i++ */
                        continue;
                    }
                }

                /* new DATA */
                value[offset++] = data[i];
            }
        }
    }

    /** Handle the remainder of the value **/
    if(offset > 0)
    {
        value[offset] = '\0';
        current_item = mem_alloc_item(value,value_type);
        if(first_item == NULL)
            first_item = current_item;
        else
            last_item->next = current_item;
        last_item = current_item;
    }

    /* Returns the chain list */
    return(first_item);
}


/******************************************/
/*  IsDecimal() :  Is it a decimal value? */
/******************************************/
int IsDecimal(char *value, int *nb_byte_rtn)
{
    int decimal = 0, is_negative = 0;

    /* Init */
    *nb_byte_rtn = 0;
    is_negative = 0;

    /* Empty value */
    if(strlen(value) == 0)
        return(0);

    /* We can have a -3 */
    if(value[0] == '-')
        is_negative = 1;

    /* Test the range of character 0-9 */
    for(int i=is_negative; i<(int)strlen(value); i++)
        if(value[i] < '0' || value[i] > '9')
            return(0);

    /* Determine the number of bytes */
    decimal = atoi(value);
    if(decimal <= 0xFF)
        *nb_byte_rtn = 1;
    else if(decimal <= 0xFFFF)
        *nb_byte_rtn = 2;
    else if(decimal <= 0xFFFFFF)
        *nb_byte_rtn = 3;
    else
        return(0);     /* Value too big */

    /* OK */
    return(1);
}


/**************************************************/
/*  IsHexaDecimal() :  Is it a hexadecimal value? */
/**************************************************/
int IsHexaDecimal(char *value, int *nb_byte_rtn)
{
    int is_negative = 0, length = 0;
    char hexa_value[16];

    /* Init */
    *nb_byte_rtn = 0;
    is_negative = 0;

    /* The first character must be a $ */
    if(value[0] != '$')
        return(0);

    /* Empty value */
    if(strlen(value) == 1)
        return(0);

    /* We can have a $-3, but here 3 is in decimal! */
    if(value[1] == '-')
        is_negative = 1;

    /* Test the range of character 0-F */
    for(int i=1+is_negative; i<(int)strlen(value); i++)
        if(toupper(value[i]) < '0' || toupper(value[i]) > 'F')
            return(0);

    /* Determine the number of bytes (4 bytes max for the LONG) */
    if(is_negative == 1)
    {
        /* Convert to HEX */
        sprintf(hexa_value,"%X",atoi(&value[2]));
        length = (int) strlen(hexa_value);
        if((length%2) == 1)
            length++;
        if((length/2) > 4)
            return(0);                      /* Value too big */
        *nb_byte_rtn = length/2;
    }
    else
    {
        /* Value coded in HEX */
        length = (int) strlen(&value[1]);
        if((length%2) == 1)
            length++;
        if((length / 2) > 4)
            return(0);                      /* Value too big */
        *nb_byte_rtn = length/2;
    }

    /* OK */
    return(1);
}


/********************************************/
/*  IsBinary() : Is it a binary expression? */
/********************************************/
int IsBinary(char *value, int *nb_byte_rtn)
{
    int nb_bit = 0;

    /* Init */
    *nb_byte_rtn = 0;

    /* The first character is a % */
    if(value[0] != '%')
        return(0);

    /* Check the value range */
    for(int i=1; i<(int)strlen(value); i++)
    {
        if(value[i] == '0' || value[i] == '1')
            nb_bit++;
        else if(value[i] == '_')
            ;                /* Seperator */
        else
            return(0);       /* Prohibited value */
    }

    /* Empty value */
    if(nb_bit == 0)
        return(0);

    /* Determine the number of bytes */
    if(nb_bit <= 8)
        *nb_byte_rtn = 1;
    else if(nb_bit <= 16)
        *nb_byte_rtn = 2;
    else if(nb_bit <= 24)
        *nb_byte_rtn = 3;
    else
        return(0);       /* Value too large */

    /* OK */
    return(1);
}


/***************************************************/
/*  IsAscii() : Is it an Ascii expression "" or "? */
/***************************************************/
int IsAscii(char *value, int *nb_byte_rtn)
{
    int nb_char = 0;

    /* Init */
    *nb_byte_rtn = 0;

    /* Empty */
    if(strlen(value) < 2)
        return(0);

    /* The first character is a " or a ' */
    if(value[0] != '"' && value[0] != '\'')
        return(0);
    if(value[0] != value[strlen(value)-1])
        return(0);

    /* Count the number of characters between "" */
    for(int i=1; i<(int)strlen(value)-1; i++)
        nb_char++;

    /* Empty value */
    if(nb_char == 0)
        return(0);

    /* Determine the number of bytes */
    if(nb_char == 1)
        *nb_byte_rtn = 1;
    else if(nb_char == 2)
        *nb_byte_rtn = 2;
    else
        return(0);       /* Value too large */

    /* OK */
    return(1);
}


/*****************************************************/
/*  IsVariable() :  Is this an expression ]Variable. */
/*****************************************************/
int IsVariable(char *value, int *nb_byte_rtn, struct omf_segment *current_omfsegment)
{
    struct variable *current_variable;

    /* Init */
    *nb_byte_rtn = 0;

    /* Empty */
    if(strlen(value) < 2)
        return(0);

    /* The first character is a ] */
    if(value[0] != ']')
        return(0);

    /* Retrieve the variable */
    my_Memory(MEMORY_SEARCH_VARIABLE,value,&current_variable,current_omfsegment);
    if(current_variable == NULL)
        return(0);

    /* We try to evaluate the value of the Variable */

    /* Number of bytes of value */
    if(current_variable->value < 256)
        *nb_byte_rtn = 1;
    else if(current_variable->value < 65536)
        *nb_byte_rtn = 2;
    else
        *nb_byte_rtn = 3;

    /* OK */
    return(1);
}


/********************************/
/*  IsLabel() :  Is it a Label? */
/********************************/
int IsLabel(char *name, int *nb_byte_rtn, struct omf_segment *current_omfsegment)
{
    struct label *current_label;

    /* Init */
    *nb_byte_rtn = 0;

    /** Search for a Label with this name **/
    my_Memory(MEMORY_SEARCH_LABEL,name,&current_label,current_omfsegment);
    if(current_label != NULL)
    {
        *nb_byte_rtn = 2;
        return(1);
    }

    /* Not found */
    return(0);
}


/***************************************/
/*  IsExternal() :  Is it an External? */
/***************************************/
int IsExternal(char *name, int *nb_byte_rtn, struct omf_segment *current_omfsegment)
{
    struct external *current_external;

    /* Init */
    *nb_byte_rtn = 0;

    /** Search for an External with this name **/
    my_Memory(MEMORY_SEARCH_EXTERNAL,name,&current_external,current_omfsegment);
    if(current_external != NULL)
    {
        *nb_byte_rtn = 3;
        return(1);
    }

    /* Not found */
    return(0);
}


/*********************************************/
/*  GetUNID() :  Creation of a unique label. */
/*********************************************/
void GetUNID(char *unique_rtn)
{
    int new_index;
    static int index = 1;

    /* Init ? */
    if(!my_strnicmp(unique_rtn,"INIT=",strlen("INIT=")))
    {
        new_index = atoi(&unique_rtn[strlen("INIT=")]);
        if(new_index > index)
            index = new_index;
    }
    else
        sprintf(unique_rtn,"ozunid_%d",index++);
}


/******************************************************************/
/*  ProcessOZUNIDLine() :  the source must already has a ozunid_. */
/******************************************************************/
void ProcessOZUNIDLine(char *label)
{
    int i, index;
    char buffer[256];

    /* label have enough chars? */
    if(strlen(label) <= strlen("ozunid_"))
        return;

    /* Make sure the label is a value past the ozunid_ prefix */
    for(i=(int)strlen("ozunid_"); i<(int)strlen(label); i++)
        if(label[i] < '0' || label[i] > '9')
            return;

    /* We have a number, so we go to the next */
    index = atoi(label) + 1;

    /* Initializes the counter */
    sprintf(buffer,"INIT=%d",index);
    GetUNID(buffer);
}


/*********************************************************************/
/*  ReplaceInOperand() :  Replaces a string of chars in the Operand. */
/*********************************************************************/
char *ReplaceInOperand(char *string, char *search_string, char *replace_string, int separator_mode, struct source_line *current_line)
{
    int i, nb_found;
    int nb_element;
    char **tab_element;
    char *new_string = NULL;

    /* Particular cases : current_line can be NULL if we replace in a Macro */
    if(strlen(string) == 0 || strlen(search_string) == 0 || strlen(string) < strlen(search_string))
        return(string);

    /*** Find the presence of the search_string string in string ***/
    /** Rapid search (case sensitive) **/
    for(i=0,nb_found=0; i<(int)(strlen(string) - strlen(search_string) + 1); i++)
    {
        if(!strncmp(&string[i],search_string,strlen(search_string)))
        {
            nb_found++;
            i += (int) strlen(search_string)-1;
        }
    }

    /* Nothing found */
    if(nb_found == 0)
        return(string);
    
    /** Allocate memory (it is expected to be wider) **/
    new_string = (char *) calloc(strlen(string)+nb_found*strlen(replace_string)+1,sizeof(char));
    if(new_string == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'new_string'");

    /** We will cut the operand into several unitary elements **/
    tab_element = DecodeOperandeAsElementTable(string,&nb_element,separator_mode,current_line);
    if(tab_element == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to decode operand as element table");
    
    /** We rebuild the chain by replacing the values (case sensitive) **/
    for(i=0; i<nb_element; i++)
    {
        if(!strcmp(tab_element[i],search_string))
            strcat(new_string,replace_string);
        else
            strcat(new_string,tab_element[i]);
    }

    /* Memory release of the table of values */
    mem_free_table(nb_element,tab_element);

    /* Return the modified string */
    return(new_string);
}


/********************************************************************************/
/*  DecodeOperandeAsElementTable() :  Breaks the operand into several elements. */
/********************************************************************************/
char **DecodeOperandeAsElementTable(char *string, int *nb_element_rtn, int separator_mode, struct source_line *current_line)
{
    int bufIdx = 0;
    int opLen = (int)strlen(string);
    char buffer[1024];
    int nb_element = 1;
    char **tab_element = NULL;

    /** Determine the number of items (expects wide) **/
    for(int i = 0; i < opLen; i++)
    {
        if(string[i] == '\'' || string[i] == '"' || string[i] == '[' || string[i] == ']' ||
           string[i] == '+' || string[i] == '-' || string[i] == '/' || string[i] == '*' ||
           string[i] == '%' || string[i] == '$' || string[i] == '#' || string[i] == '&' ||
           string[i] == ',' || string[i] == ';' || string[i] == ':' || string[i] == ' ' ||
           string[i] == '(' || string[i] == ')' || string[i] == '.' || string[i] == '^' ||
           string[i] == '<' || string[i] == '>' || string[i] == '\\' || string[i] == '!' ||
           string[i] == '|' || string[i] == '@' || string[i] == '{' || string[i] == '}' ||
           string[i] == '=' || string[i] == '\t' || string[i] == '\n')
            nb_element += 2;
    }
    
    /* Allocate memory for table */
    tab_element = (char **)calloc(nb_element,sizeof(char *));
    if(tab_element == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for a table");

    /*** Place the elements in the table ***/
    nb_element = 0;
    for(int i = 0; i < opLen; i++)
    {
        /** Find character strings **/
        if(string[i] == '"' || string[i] == '\'')
        {
            /* Finish the previous one */
            if(IsSeparator(string[i],separator_mode))
            {
                buffer[bufIdx] = '\0';
                tab_element[nb_element] = strdup(buffer);
                if(tab_element[nb_element] == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                }
                nb_element++;
                bufIdx = 0;
            }

            /* Store the string surrounded by its delimiters */
            buffer[bufIdx++] = string[i];
            int found = 0;
            int k;
            for(k = i + 1; k < opLen; k++)
            {
                buffer[bufIdx++] = string[k];
                if(string[k] == string[i])
                {
                    found = 1;
                    break;
                }
            }

            /* Invalid string: No end */
            if(found == 0)
            {
                if(current_line == NULL)
                    printf("    Error : Wrong format for '%s' : Missing String delimiter (Macro).\n",string);
                else
                    printf("    Error : Wrong format for '%s' : Missing String delimiter (File '%s', Line %d).\n",string,current_line->file->file_name,current_line->file_line_number);
                mem_free_table(nb_element,tab_element);
                return(NULL);
            }

            /* Store the string surrounded by its delimiters */
            if(IsSeparator(string[i],separator_mode))
            {
                buffer[bufIdx] = '\0';
                tab_element[nb_element] = strdup(buffer);
                if(tab_element[nb_element] == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                }
                nb_element++;
                bufIdx = 0;
            }

            /* Continue after the string */
            i += (k-i+1) - 1;
            continue;
        }

        /** Numerical value, Label or Operator **/
        if(separator_mode == SEPARATOR_EVALUATE_EXPRESSION)
        {
            /*** Because of the ambiguous Operators, we must separate the expression & evaluate separately. ***/

            /** Simple Case 1: Neither Operator nor Separator **/
            if(!IsSeparator(string[i],SEPARATOR_EVALUATE_EXPRESSION))
            {
                /* Add a character to the buffer */
                buffer[bufIdx++] = string[i];
                continue;
            }

            /** Simple Case 2: Unambiguous Operators or Seperators (* may be the current address, but we will isolate as we would an operator) **/
            if(string[i] == '+' || string[i] == '*' || string[i] == '/' || string[i] == '&' || string[i] == '.' || string[i] == '!' || string[i] == '{' || string[i] == '}' || string[i] == ' ' || string[i] == '\t' || string[i] == '\n')
            {
                /* Finish the previous one */
                buffer[bufIdx] = '\0';
                if(bufIdx > 0)
                {
                    /* Adds the element */
                    tab_element[nb_element] = strdup(buffer);
                    if(tab_element[nb_element] == NULL)
                    {
                        mem_free_table(nb_element,tab_element);
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                    }
                    nb_element++;

                    /* Reset index */
                    bufIdx = 0;
                }

                /* Stores the separator alone (unless it is a neutral separator: space, \ t, \ n) */
                if(string[i] != ' ' && string[i] != '\n' && string[i] != '\t')
                {
                    /* Adds the element */
                    buffer[0] = string[i];
                    buffer[1] = '\0';
                    tab_element[nb_element] = strdup(buffer);
                    if(tab_element[nb_element] == NULL)
                    {
                        mem_free_table(nb_element,tab_element);
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                    }
                    nb_element++;
                }

                /* Continue */
                continue;
            }

            /*** You have to manage <> # and - as Particular cases since they are not necessarily separators ***/

            /** Particular case : '-' as unary encapsulated operator (# -1 or $ -3 or # $ - 2) **/
            if(string[i] == '-' && bufIdx > 0 && (buffer[bufIdx-1] == '#' || buffer[bufIdx-1] == '$'))
            {
                /* We handle the decimal and the hex */
                if((string[i+1] >= '0' && string[i+1] <= '9') || (toupper(string[i+1]) >= 'A' && toupper(string[i+1]) <= 'F'))
                {
                    /* We include - as the sign indicator */
                    buffer[bufIdx++] = string[i];
                    continue;
                }
            }
            /** Particular case : <> and # used as the addressing mode **/
            if(string[i] == '<' || string[i] == '>' || string[i] == '#')
            {
                /* Case 1: At the very beginning of the expression */
                if(bufIdx == 0 && nb_element == 0)
                {
                    /* We include the <> # as the first letter of the value to come */
                    buffer[bufIdx++] = string[i];
                    continue;
                }

                /* Case 2: <and> have a # just before (#> LABEL or # <LABEL) */
                if((string[i] == '<' || string[i] == '>') && bufIdx == 1 && buffer[0] == '#')
                {
                    /* We include the <> as the 2nd letter of the current value */
                    buffer[bufIdx++] = string[i];
                    continue;
                }

                /* Case 3: An Operator Has A Separator The Precedes */
                if(bufIdx == 0 && nb_element > 0)
                {
                    if(strlen(tab_element[nb_element-1]) == 1 && IsSeparator(tab_element[nb_element-1][0],SEPARATOR_EVALUATE_EXPRESSION))
                    {
                        /* We include the <> # as the first letter of the value to come */
                        buffer[bufIdx++] = string[i];
                        continue;
                    }
                }
            }

            /** Finally, this is Operators => Ends the previous value + Stores the operator alone **/
            /* Finish the previous one */
            buffer[bufIdx] = '\0';
            if(bufIdx > 0)
            {
                /* Adds the element */
                tab_element[nb_element] = strdup(buffer);
                if(tab_element[nb_element] == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                }
                nb_element++;

                /* Reset index */
                bufIdx = 0;
            }

            /* Stores the separator alone (unless it is a neutral separator: space, \ t, \ n) */
            if(string[i] != ' ' && string[i] != '\n' && string[i] != '\t')
            {
                /* Adds the element */
                buffer[0] = string[i];
                buffer[1] = '\0';
                tab_element[nb_element] = strdup(buffer);
                if(tab_element[nb_element] == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                }
                nb_element++;
            }
        }
        else
        {
            /** We are looking for separators **/
            if(IsSeparator(string[i],separator_mode))
            {
                /* Finish the previous one */
                buffer[bufIdx] = '\0';
                if(bufIdx > 0)
                {
                    tab_element[nb_element] = strdup(buffer);
                    if(tab_element[nb_element] == NULL)
                    {
                        mem_free_table(nb_element,tab_element);
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                    }
                    nb_element++;
                }

                /* Stores the separator alone (unless it is a neutral separator: space, \ t, \ n) */
                if(string[i] != ' ' && string[i] != '\n' && string[i] != '\t')
                {
                    buffer[0] = string[i];
                    buffer[1] = '\0';
                    tab_element[nb_element] = strdup(buffer);
                    if(tab_element[nb_element] == NULL)
                    {
                        mem_free_table(nb_element,tab_element);
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
                    }
                    nb_element++;
                }

                /* Reset index */
                bufIdx = 0;

                /* Continue */
                continue;
            }

            /* Add a character to the buffer */
            buffer[bufIdx++] = string[i];
        }
    }

    /**Last element remaining **/
    buffer[bufIdx] = '\0';
    if(bufIdx > 0)
    {
        tab_element[nb_element] = strdup(buffer);
        if(tab_element[nb_element] == NULL)
        {
            mem_free_table(nb_element,tab_element);
            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for an item into a table");
        }
        nb_element++;
    }

    /* Return the table */
    *nb_element_rtn = nb_element;
    return(tab_element);
}


/****************************************************************/
/*  IsSeparator() :  Determine if the character is a separator. */
/****************************************************************/
int IsSeparator(char c, int separator_mode)
{
    /** If we analyze to replace the labels, the $ is not considered as a separator ($ BB with BB as EQU) **/
    if(separator_mode == SEPARATOR_REPLACE_LABEL)
    {
        /* List of separators */
        if(c == ' ' || c == '+' || c == '-' || c == '.' || c == '&' || c == '!' || c == '*' || c == '/' ||
           c == '#' || c == ',' || c == ';' || c == '<' || c == '>' || c == '^' || c == '|' || c == '[' ||
           c == ']' || c == '(' || c == ')' || c == '{' || c == '}' || c == '%' || c == '=')
            return(1);
    }
    /** If we analyze to replace the Variable Label, the] and the $ are not considered as a separator **/
    else if(separator_mode == SEPARATOR_REPLACE_VARIABLE)
    {
        /* List of separators */
        if(c == ' ' || c == '+' || c == '-' || c == '.' || c == '&' || c == '!' || c == '*' || c == '/' ||
           c == '#' || c == ',' || c == ';' || c == '<' || c == '>' || c == '^' || c == '|' || c == '[' ||
           c == '(' || c == ')' || c == '{' || c == '}' || c == '%' || c == '=')
            return(1);
    }
    /** We will have to evaluate an expression, the cutting is done according to the operators **/
    else if(separator_mode == SEPARATOR_EVALUATE_EXPRESSION)
    {
        /* List of separators */
        if(c == '<' || c == '=' || c == '>' || c == '#' ||              /* < egal > different */
           c == '+' || c == '-' || c == '*' || c == '/' ||              /* + - * / */
           c == '&' || c == '.' || c == '!' ||                          /* AND / OR / EXCLUSIVE */
           c == '{' || c == '}' || c == ' ' || c == '\t' || c == '\n')  /* Operators / Separators Priority */
            return(1);
    }
    /** The data are separated by, **/
    else if(separator_mode == SEPARATOR_DATA_VALUES)
    {
        /* List of separators */
        if(c == ',')
            return(1);
    }
    
    /* Not a separator */
    return(0);
}


/**********************************************************************/
/*  GetByteValue() :  Decodes a BYTE value encoded as Hex or Decimal. */
/**********************************************************************/
BYTE GetByteValue(char *value_txt)
{
    BYTE value_byte;
    int i, j;
    unsigned int value_int = 0;
    int offset = 0;
    int is_hexa = 0;
    int is_binary = 0;

    /* Do you have a # first? */
    if(value_txt[offset] == '#')
        offset++;

    /* Is a $ following? */
    if(value_txt[offset] == '$')
    {
        offset++;
        is_hexa = 1;
    }
    else if(value_txt[offset] == '%')
    {
        offset++;
        is_binary = 1;
    }

    /** Decode the value **/
    if(is_binary)
    {
        for(i=0,j=0; i<(int)strlen(&value_txt[offset]); i++)
        {
            if(value_txt[offset+strlen(&value_txt[offset])-1-i] == '1')
            {
                value_int += (1 << j);
                j++;
            }
            else if(value_txt[offset+strlen(&value_txt[offset])-1-i] == '0')
                j++;
        }
    }
    else if(is_hexa)
        sscanf(&value_txt[offset],"%X",&value_int);
    else
        sscanf(&value_txt[offset],"%d",&value_int);

    /* Conversion to BYTE */
    value_byte = (BYTE) value_int;

    /* Returns the value */
    return(value_byte);
}


/*******************************************************************/
/*  GetWordValue() :  Decodes a WORD value that is Hex or Decimal. */
/*******************************************************************/
WORD GetWordValue(char *value_txt)
{
    WORD value_wd;
    int i, j;
    unsigned int value_int = 0;
    int offset = 0;
    int is_hexa = 0;
    int is_binary = 0;

    /* Do you have a # first? */
    if(value_txt[offset] == '#')
        offset++;

    /* Is a $ following? */
    if(value_txt[offset] == '$')
    {
        offset++;
        is_hexa = 1;
    }
    else if(value_txt[offset] == '%')
    {
        offset++;
        is_binary = 1;
    }

    /** Decode the value **/
    if(is_binary)
    {
        for(i=0,j=0; i<(int)strlen(&value_txt[offset]); i++)
        {
            if(value_txt[offset+strlen(&value_txt[offset])-1-i] == '1')
            {
                value_int += (1 << j);
                j++;
            }
            else if(value_txt[offset+strlen(&value_txt[offset])-1-i] == '0')
                j++;
        }
    }
    else if(is_hexa)
        sscanf(&value_txt[offset],"%X",&value_int);
    else
        sscanf(&value_txt[offset],"%d",&value_int);

    /* Conversion to WORD */
    value_wd = (WORD) value_int;

    /* Returns the value */
    return(value_wd);
}


/*********************************************************************/
/*  GetDwordValue() :  Decodes a DWORD value that is Hex or Decimal. */
/*********************************************************************/
DWORD GetDwordValue(char *value_txt)
{
    DWORD value_dwd;
    int i, j;
    unsigned int value_int = 0;
    int offset = 0;
    int is_hexa = 0;
    int is_binary = 0;

    /* Do you have a # first? */
    if(value_txt[offset] == '#')
        offset++;

    /* Is a $ following? */
    if(value_txt[offset] == '$')
    {
        offset++;
        is_hexa = 1;
    }
    else if(value_txt[offset] == '%')
    {
        offset++;
        is_binary = 1;
    }

    /** Decode the value **/
    if(is_binary)
    {
        for(i=0,j=0; i<(int)strlen(&value_txt[offset]); i++)
        {
            if(value_txt[offset+strlen(&value_txt[offset])-1-i] == '1')
            {
                value_int += (1 << j);
                j++;
            }
            else if(value_txt[offset+strlen(&value_txt[offset])-1-i] == '0')
                j++;
        }
    }
    else if(is_hexa)
        sscanf(&value_txt[offset],"%X",&value_int);
    else
        sscanf(&value_txt[offset],"%d",&value_int);

    /* Conversion to DWORD */
    value_dwd = (DWORD) value_int;

    /* Returns the value */
    return(value_dwd);
}


/************************************************************/
/*  GetVariableValue() :  Retrieve the value of a variable. */
/************************************************************/
int64_t GetVariableValue(char *variable_name, int *is_error_rtn, struct omf_segment *current_omfsegment)
{
    struct variable *current_variable;

    /* Init */
    *is_error_rtn = 0;

    /** Look for the variable **/
    my_Memory(MEMORY_SEARCH_VARIABLE,variable_name,&current_variable,current_omfsegment);
    if(current_variable == NULL)
    {
        *is_error_rtn = 1;
        return(0);
    }

    /* Returns the value */
    return(current_variable->value);
}


/********************************************************************/
/*  GetBinaryValue() :  Retrieves the value of a binary expression. */
/********************************************************************/
int64_t GetBinaryValue(char *expression)
{
    int bitIdx = 0;
    int64_t v = 1, value = 0;

    /** We only want 0,1 and _ **/
    for(int i=(int)strlen(expression)-1; i>=0; i--)
    {
        if(expression[i] == '0')
        {
            bitIdx++;
        }
        else if(expression[i] == '1')
        {
            value += (v << bitIdx);
            bitIdx++;
        }
        else if(expression[i] == '_')
            continue;
        else
            return(-1);    /* Invalid character for binary */
    }
    /* Returns the value */
    return(value);
}


/*********************************************************************/
/*  GetDecimalValue() :  Retrieve the value of a decimal expression. */
/*********************************************************************/
int64_t GetDecimalValue(char *expression, int *is_error_rtn)
{
    int is_negative = 0;
    int64_t value = 0;

    /* Init */
    *is_error_rtn = 0;

    /* Is there a - sign in front? */
    if(expression[0] == '-')
        is_negative = 1;

    /** We only want 0-9 **/
    for(int i = is_negative; i < (int)strlen(expression); i++)
    {
        if(expression[i] < '0' || expression[i] > '9')
        {
            *is_error_rtn = 1;
            return(0);    /* Invalid character for decimal */
        }
    }

    /* Decode the expression */
    value = my_atoi64(expression);

    /* Returns the value */
    return(value);
}


/***********************************************************************/
/*  GetHexaValue() :  Retrieves the value of a hexadecimal expression. */
/***********************************************************************/
int64_t GetHexaValue(char *expression)
{
    int is_negative = 0;
    int64_t value = 0;

    /* Signe au début, attention in $ -3, 3 is in decimal! */
    if(expression[0] == '-')
        is_negative = 1;

    /** We only want 0-9 A-F **/
    for(int i = is_negative; i < (int)strlen(expression); i++)
    {
        if(!((expression[i] >= '0' && expression[i] <= '9') || (toupper(expression[i]) >= 'A' && toupper(expression[i]) <= 'F')))
            return(-1);    /* Invalid character for hexadecimal */
    }
    /** Decode the HEX expression **/
    if(is_negative == 1)
    {
        value = atoi(&expression[1]);      /* in $ -3, 3 is in decimal! */
    }
    else
    {
        int bitShift = 0;
        for(int i = (int)strlen(expression)-1; i >= is_negative; i--, bitShift+=4)
        {
            int64_t v;

            /* Decode the character 0-F */
            if(expression[i] >= '0' && expression[i] <= '9')
                v = (expression[i] - '0');
            else
                v = ((toupper(expression[i]) - 'A') + 10);

            /* Add to value */
            value += (v << bitShift);
        }
    }

    /* Returns the value */
    if(is_negative == 1)
        return(value*(-1));
    else
        return(value);
}


/****************************************************************/
/*  GetAsciiValue() :  Retrieving value of an ASCII Expression. */
/****************************************************************/
int64_t GetAsciiValue(char *expression)
{
    int64_t value = 0;
    char *next_char = NULL;

    /** We want "c" or "cc" **/
    if(strlen(expression) != 3 && strlen(expression) != 4)
        return(-1);
    if(expression[0] != expression[strlen(expression)-1])
        return(-1);
    if(expression[0] != '"' && expression[0] != '\'')
        return(-1);

    /* We check that the characters are in the range ASCII */
    if(strlen(expression) == 3)
        next_char = strchr(ASCII_TABLE,expression[1]);
    else
    {
        next_char = strchr(ASCII_TABLE,expression[1]);
        if(next_char != NULL)
            next_char = strchr(ASCII_TABLE,expression[2]);
    }
    if(next_char == NULL)
        return(-1);

    /** Decode the expression **/
    if(strlen(expression) == 3)
    {
        if(expression[0] == '"')
            value = (unsigned char) (expression[1] | 0x80);
        if(expression[0] == '\'')
            value = (unsigned char) (expression[1] & 0x7F);
    }
    else
    {
        if(expression[0] == '"')
            value = (((WORD)(expression[2] | 0x80)) << 8) | (WORD) (expression[1] | 0x80);
        if(expression[0] == '\'')
            value = (((WORD)(expression[2] & 0x7F)) << 8) | (WORD) (expression[1] & 0x7F);
    }
    
    /* Returns the value */
    return(value);
}


/***********************************************************/
/*  GetAddressValue() :  Retrieve the value of an address. */
/***********************************************************/
int64_t GetAddressValue(char *expression, int current_address, struct external **current_external_rtn, int *is_dum_label_rtn, int *is_fix_label_rtn, struct omf_segment *current_omfsegment)
{
    int64_t address = 0;
    struct label *current_label = NULL;
    struct external *current_external = NULL;

    /* Init */
    *current_external_rtn = NULL;
    *is_dum_label_rtn = 0;               /* By default, the label is not DUM */
    *is_fix_label_rtn = 0;               /* By default, the label is not ORG */

    /* Particular case of * */
    if(!strcmp(expression,"*"))
        return(current_address);

    /* Is it an External Label */
    my_Memory(MEMORY_SEARCH_EXTERNAL,expression,&current_external,current_omfsegment);
    if(current_external != NULL)
    {
        *current_external_rtn = current_external;
        address = 0;
        return(address);
    }

    /* We are looking for a Label */
    my_Memory(MEMORY_SEARCH_LABEL,expression,&current_label,current_omfsegment);
    if(current_label == NULL)
        return(-1);

    /* This label is in a DUM section */
    if(current_label->line->is_dum == 1)
        *is_dum_label_rtn = 1;

    /* This label is in a section [ORG $Addr ORG] */
    if(current_label->line->is_fix_address == 1)
        *is_fix_label_rtn = 1;

    /* The label is valid but the address is currently unknown */
    if(current_label->line->address == -1)
        return(-2);

    /* Calculate the address */
    address = (current_label->line->bank << 16) | (0xFFFF & current_label->line->address);

    /* Returns the value */
    return(address);
}


/********************************************************************************/
/*  QuickConditionEvaluate() :  Retrieve the value of a conditional expression. */
/********************************************************************************/
int QuickConditionEvaluate(struct source_line *cond_line, int64_t *value_expression_rtn, struct omf_segment *current_omfsegment)
{
    int is_algebric = 0, first_value_is_negative = 0, nb_element = 0, is_error = 0, nb_open = 0, has_priority = 0, is_operator = 0, nb_item = 0;
    int64_t value = 0, value_expression = 0, value_variable = 0, value_binary = 0, value_decimal = 0, value_hexa = 0, value_ascii = 0, value_address = 0;
    int j = 0, has_dash = 0, has_less = 0, has_more = 0, has_exp = 0, has_pipe = 0, has_extra_dash = 0;
    char operator_c = 0;
    char *new_value_txt = NULL;
    char **tab_element = NULL;
    char expression[1024];
    char buffer[1024];
    char buffer_error[1024];

    /** Three possibilities: 0 (STATUS_DONT), 1 (STATUS_DO) or do not know (STATUS_UNKNWON) **/
    /* Quick Win */
    if(!my_stricmp(cond_line->operand_txt,"0"))
    {
        *value_expression_rtn = 0;
        return(STATUS_DONT);
    }
    if(!my_stricmp(cond_line->operand_txt,"1"))
    {
        *value_expression_rtn = 1;
        return(STATUS_DO);
    }

    /* Init */

    /** We will treat the # <> ^ | from the very beginning **/
    has_dash = (cond_line->operand_txt[0] == '#') ? 1 : 0;
    has_less = (cond_line->operand_txt[has_dash] == '<') ? 1 : 0;
    has_more = (cond_line->operand_txt[has_dash] == '>') ? 1 : 0;
    has_exp = (cond_line->operand_txt[has_dash] == '^') ? 1 : 0;
    has_pipe = (cond_line->operand_txt[has_dash] == '|' || cond_line->operand_txt[has_dash] == '!') ? 1 : 0;

    /** If there is no operator (<=> # + - / * &. ^), Delete the {} **/
    for(int i=(has_dash+has_less+has_more+has_exp+has_pipe); i<(int)strlen(cond_line->operand_txt); i++)
    {
        if(cond_line->operand_txt[i] == '=' || /* cond_line->operand_txt[i] == '<' || cond_line->operand_txt[i] == '>' || cond_line->operand_txt[i] == '#' ||    A REFAIRE */
           cond_line->operand_txt[i] == '+' || cond_line->operand_txt[i] == '-' || cond_line->operand_txt[i] == '/' || cond_line->operand_txt[i] == '*' ||
           cond_line->operand_txt[i] == '.' || cond_line->operand_txt[i] == '&' || cond_line->operand_txt[i] == '^')
        {
            is_algebric = 1;
            break;
        }
    }
    if(is_algebric == 0)
    {
        for(int i=0; i<(int)strlen(cond_line->operand_txt); i++)
        {
            if(cond_line->operand_txt[i] != '{' && cond_line->operand_txt[i] != '}')
                expression[j++] = cond_line->operand_txt[i];
        }
        expression[j] = '\0';
    }
    else
        strcpy(expression,cond_line->operand_txt);

    /** We will treat the # <> ^ | **/
    has_dash = (expression[0] == '#') ? 1 : 0;
    has_less = (expression[has_dash] == '<') ? 1 : 0;
    has_more = (expression[has_dash] == '>') ? 1 : 0;
    has_exp = (expression[has_dash] == '^') ? 1 : 0;
    has_pipe = (expression[has_dash] == '|' || expression[has_dash] == '!') ? 1 : 0;

    /** Cut the string of characters into several elements (skips the #> <^ | from the beginning) **/
    tab_element = DecodeOperandeAsElementTable(&expression[has_dash+has_less+has_more+has_exp+has_pipe],&nb_element,SEPARATOR_EVALUATE_EXPRESSION,cond_line);
    if(tab_element == NULL)
    {
        sprintf(buffer_error,"Impossible to decode Operand '%s' as element table",expression);
        return(STATUS_UNKNWON);
    }

    /* Expression Empty */
    if(nb_element == 0 || (nb_element == 1 && strlen(tab_element[0]) == 0))
    {
        /* It's an Error */
        strcpy(buffer_error,"Empty expression");
        mem_free_table(nb_element,tab_element);
        return(STATUS_UNKNWON);
    }

    /** Converting non-separator elements to numeric values **/
    for(int i=0; i<nb_element; i++)
    {
        if(!(strlen(tab_element[i]) == 1 && IsSeparator(tab_element[i][0],SEPARATOR_EVALUATE_EXPRESSION)))     /* The * = current address is going to be considered as a separator */
        {
            /* If one or more # are at the beginning of the value, it is removed */
            has_extra_dash = 0;
            while(tab_element[i][has_extra_dash] == '#')
                has_extra_dash++;

            /*** Evaluate each value ***/
            if(tab_element[i][has_extra_dash] == '\'' || tab_element[i][has_extra_dash] == '"')
            {
                /** We will replace the "Ascii" ("A" or "AA") **/
                value_ascii = GetAsciiValue(&tab_element[i][has_extra_dash]);
                if(value_ascii == -1)
                {
                    sprintf(buffer_error,"'%s' can't be translated as an ascii expression",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(STATUS_UNKNWON);
                }

                /* We can replace the expression */
                my_printf64(value_ascii,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    sprintf(buffer_error,"Impossible to allocate memory for new 'value_txt'");
                    return(STATUS_UNKNWON);
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else if(tab_element[i][has_extra_dash] == '%')
            {
                /** We will replace the % Binary **/
                value_binary = GetBinaryValue(&tab_element[i][1+has_extra_dash]);
                if(value_binary == -1)
                {
                    sprintf(buffer_error,"'%s' can't be translated as a binary expression",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(STATUS_UNKNWON);
                }

                /* We can replace the expression */
                my_printf64(value_binary,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    sprintf(buffer_error,"Impossible to allocate memory for new 'value_txt'");
                    return(STATUS_UNKNWON);
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else if(tab_element[i][has_extra_dash] == '$')
            {
                /** We will replace the $ Hex **/
                value_hexa = GetHexaValue(&tab_element[i][1+has_extra_dash]);
                if(value_hexa == -1)
                {
                    sprintf(buffer_error,"'%s' can't be translated as a hexadecimal expression",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(STATUS_UNKNWON);
                }

                /* We can replace the expression */
                my_printf64(value_hexa,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    sprintf(buffer_error,"Impossible to allocate memory for new 'value_txt'");
                    return(STATUS_UNKNWON);
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else if(tab_element[i][0] == ']')
            {
                /** We will retrieve the value of the ]Variable or the address of the ]Label **/
                value_variable = GetQuickVariable(tab_element[i],cond_line,&is_error,current_omfsegment);
                if(is_error == 1)
                {
                    sprintf(buffer_error,"'%s' can't be translated as a Variable",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(STATUS_UNKNWON);
                }

                /* We can replace */
                my_printf64(value_variable,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    sprintf(buffer_error,"Impossible to allocate memory for new 'value_txt'");
                    return(STATUS_UNKNWON);
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else
            {
                /** We think to detect a Label or an EQU **/
                value_address = GetQuickValue(&tab_element[i][has_extra_dash],cond_line,&is_error,current_omfsegment);
                if(is_error == 0)
                {
                    /* We can replace the expression */
                    my_printf64(value_address,buffer);
                    new_value_txt = strdup(buffer);
                    if(new_value_txt == NULL)
                    {
                        mem_free_table(nb_element,tab_element);
                        sprintf(buffer_error,"Impossible to allocate memory for new 'value_txt'");
                        return(STATUS_UNKNWON);
                    }
                    free(tab_element[i]);
                    tab_element[i] = new_value_txt;
                }
                else
                {
                    /** We could be a numerical constant **/
                    value_decimal = GetDecimalValue(&tab_element[i][has_extra_dash],&is_error);
                    if(is_error == 0)
                    {
                        /* We can replace the expression */
                        my_printf64(value_decimal,buffer);
                        new_value_txt = strdup(buffer);
                        if(new_value_txt == NULL)
                        {
                            mem_free_table(nb_element,tab_element);
                            sprintf(buffer_error,"Impossible to allocate memory for new 'value_txt'");
                            return(STATUS_UNKNWON);
                        }
                        free(tab_element[i]);
                        tab_element[i] = new_value_txt;
                    }
                    else
                    {
                        /** We do not know how to interpret this value **/
                        sprintf(buffer_error,"'%s' can't be translated as a numeric expression",tab_element[i]);
                        mem_free_table(nb_element,tab_element);
                        return(STATUS_UNKNWON);
                    }
                }
            }
        }
    }

    /** We will manage the - at the beginning => Passage of the first value in negative **/
    if(!strcmp(tab_element[0],"-") && nb_element > 1)
    {
        /* The first value is negative */
        first_value_is_negative = 1;

        /* We can remove the - */
        free(tab_element[0]);
        for(j=1; j<nb_element; j++)
            tab_element[j-1] = tab_element[j];
        nb_element--;
    }

    /** We must now only have: value [operator value [operator value ...]] **/
    if(nb_element % 2 == 0)
    {
        strcpy(buffer_error,"The number of element is even (should be value [operator value [operator value]...])");
        mem_free_table(nb_element,tab_element);
        return(STATUS_UNKNWON);
    }

    /** Are there any {} and are they properly nested? **/
    has_priority = 0;
    nb_open = 0;
    for(int i=0; i<nb_element; i++)
        if(!strcmp(tab_element[i],"{"))
        {
            nb_open++;
            has_priority = 1;
        }
        else if(!strcmp(tab_element[i],"}"))
        {
            nb_open--;
            if(nb_open < 0)
            {
                /* Error */
                strcpy(buffer_error,"Wrong '}' in expression");
                mem_free_table(nb_element,tab_element);
                return(STATUS_UNKNWON);
            }
        }
    if(nb_open != 0)
    {
        /* Error */
        strcpy(buffer_error,"Missing '}' in expression");
        mem_free_table(nb_element,tab_element);
        return(STATUS_UNKNWON);
    }

    /** We will evaluate the expression **/
    is_operator = 0;
    for(int i=0; i<nb_element; i++)
    {
        /** We will try to decode all types of data **/
        if(is_operator == 0)
        {
            /** We must come across a value **/
            if(!strcmp(tab_element[i],"{"))
                value = EvaluateAlgebricExpression(tab_element,i,nb_element,cond_line->address,&nb_item);
            else
            {
                /** The digital conversion has already been done **/
                value = my_atoi64(tab_element[i]);
            }

            /** Add this value to the global expression **/
            if(i == 0)
                value_expression = value*((first_value_is_negative == 1) ? -1 : 1);
            else
            {
                /** Applies the operator to both values **/
                if(operator_c == '>')
                    value_expression = (value_expression > value) ? 1 : 0;
                else if(operator_c == '=')
                    value_expression = (value_expression == value) ? 1 : 0;
                else if(operator_c == '<')
                    value_expression = (value_expression < value) ? 1 : 0;
                else if(operator_c == '#')
                    value_expression = (value_expression != value) ? 1 : 0;
                else if(operator_c == '+')
                    value_expression = value_expression + value;
                else if (operator_c == '-')
                    value_expression = value_expression - value;
                else if (operator_c == '*')
                    value_expression = value_expression * value;
                else if (operator_c == '/')
                {
                    /* Beware of divisions by Zero */
                    if(value == 0)
                    {
                        strcpy(buffer_error,"Division by Zero");
                        mem_free_table(nb_element,tab_element);
                        return(STATUS_UNKNWON);
                    }

                    value_expression = value_expression / value;
                }
                else if (operator_c == '!')
                    value_expression = value_expression ^ value;
                else if (operator_c == '.')
                    value_expression = value_expression | value;
                else if (operator_c == '&')
                    value_expression = value_expression & value;
            }

            /* Next will be an operator */
            is_operator = 1;

            /* If we had one { we skip all the expression */
            if(!strcmp(tab_element[i],"{"))
                i += nb_item;
        }
        else
        {
            /** We must recognize the operator **/
            if(strcmp(tab_element[i],"<") && strcmp(tab_element[i],"=") && strcmp(tab_element[i],">") && strcmp(tab_element[i],"#") &&
               strcmp(tab_element[i],"+") && strcmp(tab_element[i],"-") && strcmp(tab_element[i],"*") && strcmp(tab_element[i],"/") &&
               strcmp(tab_element[i],"!") && strcmp(tab_element[i],".") && strcmp(tab_element[i],"&"))
            {
                sprintf(buffer_error,"The '%s' is not a valid operator",tab_element[i]);
                mem_free_table(nb_element,tab_element);
                return(STATUS_UNKNWON);
            }

            /* Keep the operator for the future */
            operator_c = tab_element[i][0];

            /* Next will be a value */
            is_operator = 0;
        }
    }

    /* Memory release */
    mem_free_table(nb_element,tab_element);

    /* Do we know the expression */
    *value_expression_rtn = value_expression;
    if(value_expression == 0)
        return(STATUS_DONT);
    else
        return(STATUS_DO);
}


/*************************************************************/
/*  GetQuickValue() :  Recover the value of a named element. */
/*************************************************************/
int64_t GetQuickValue(char *name, struct source_line *cond_line, int *is_error_rtn, struct omf_segment *current_omfsegment)
{
    int result = 0, nb_valid_line = 0;
    int64_t value_expression = 0;
    struct source_line **tab_line;

    /** Create quick access table to the lines **/
    /* Number of valid lines */
    for(struct source_line *current_line = current_omfsegment->first_file->first_line; current_line; current_line=current_line->next)
    {
        if(current_line == cond_line)
            break;
        if(current_line->is_valid == 0)
            continue;
        nb_valid_line++;
    }
    /* Allocate memory */
    tab_line = (struct source_line **) calloc(nb_valid_line+1,sizeof(struct source_line *));
    if(tab_line == NULL)
    {
        *is_error_rtn = 1;
        return(-1);
    }
    /* Fill out structure */
    int lineIdx = 0;
    for(struct source_line *current_line = current_omfsegment->first_file->first_line; current_line; current_line=current_line->next)
    {
        if(current_line == cond_line)
            break;
        if(current_line->is_valid == 0)
            continue;
        tab_line[lineIdx++] = current_line;
    }

    /** We go from bottom to top for Labels and Equals **/
    for(int i = nb_valid_line-1; i >= 0; i--)
    {
        if(!strcmp(name,tab_line[i]->label_txt))
        {
            /* Equ */
            if(!my_stricmp(tab_line[i]->opcode_txt,"EQU") || !my_stricmp(tab_line[i]->opcode_txt,"="))
            {
                /** We will have to evaluate the Operand **/
                result = QuickConditionEvaluate(tab_line[i],&value_expression,current_omfsegment);
                if(result == STATUS_UNKNWON)
                {
                    /* Impossible to Evaluate */
                    free(tab_line);
                    *is_error_rtn = 1;
                    return(-1);
                }

                /* Memory release */
                free(tab_line);

                /* Return the value */
                *is_error_rtn = 0;
                return(value_expression);
            }
            else   /* Simple label */
            {
                /* We return the approximate address of the Label line */
                free(tab_line);
                *is_error_rtn = 0;
                return(2*i);
            }
        }
    }

    /* Memory release */
    free(tab_line);

    /* On a Not found */
    *is_error_rtn = 1;
    return(-1);
}


/***********************************************************/
/*  GetQuickVariable() :  Recover the value of a variable. */
/***********************************************************/
int64_t GetQuickVariable(char *variable_name, struct source_line *cond_line, int *is_error_rtn, struct omf_segment *current_omfsegment)
{
    int result, is_variable;
    int64_t value_expression;
    struct source_line *current_line;

    /** It is necessary to differentiate a ]Variable and ]Local Label **/
    is_variable = 1;
    for(current_line = current_omfsegment->first_file->first_line; current_line; current_line=current_line->next)
    {
        if(current_line == cond_line)
            break;
        if(current_line->is_valid == 0)
            continue;
        
        /* ]Label or ]Variable */
        if(!strcmp(current_line->label_txt,variable_name))
        {
            if(!my_stricmp(current_line->opcode_txt,"="))
                is_variable = 1;
            else
                is_variable = 0;
            break;
        }
    }

    /** It's a ]Local Label **/
    if(is_variable == 0)
    {
        /* We use the function of Evaluating a Label */
        value_expression = GetQuickValue(variable_name,cond_line,is_error_rtn,current_omfsegment);
        return(value_expression);
    }
    
    /** We go from bottom to top to find the value of the ]Variable **/
    for(current_line = current_omfsegment->first_file->first_line; current_line; current_line=current_line->next)
    {
        if(current_line == cond_line)
            break;
        if(current_line->is_valid == 0)
            continue;

        /* Look for the first definition of the Variable */
        if(!strcmp(variable_name,current_line->label_txt) && !my_stricmp(current_line->opcode_txt,"="))
        {
            /** We will have to evaluate the Operand **/
            result = QuickConditionEvaluate(current_line,&value_expression,current_omfsegment);
            if(result == STATUS_UNKNWON)
            {
                /* Impossible to Evaluate */
                *is_error_rtn = 1;
                return(-1);
            }

            /* Return the value */
            *is_error_rtn = 0;
            return(value_expression);
        }
    }

    /* On a Not found */
    *is_error_rtn = 1;
    return(-1);
}


/**************************************************************/
/*  EvalExpressionAsInteger() :  Evaluation of an expression. */
/**************************************************************/
int64_t EvalExpressionAsInteger(char *expression_param, char *buffer_error_rtn, struct source_line *current_line, int operand_size, int *is_reloc_rtn, BYTE *byte_count_rtn, BYTE *bit_shift_rtn, WORD *offset_reference_rtn, DWORD *expression_address_rtn, struct external **external_rtn, struct omf_segment *current_omfsegment)
{
    int has_priority = 0, nb_open = 0, is_algebric = 0;
    int64_t value = 0, value_expression = 0, value_variable = 0, value_binary = 0, value_decimal = 0, value_hexa = 0, value_ascii = 0, value_address = 0;
    char *new_value_txt = NULL;
    int nb_element = 0, is_operator = 0, first_value_is_negative = 0, nb_address = 0, has_extra_dash = 0, nb_item = 0, is_pea_opcode = 0, is_mvn_opcode = 0, is_error = 0, is_dum_label = 0, is_fix_label = 0;
    int has_dash = 0, has_less = 0, has_more = 0, has_exp = 0, has_pipe = 0;
    char expression[1024];
    struct external *current_external = NULL;
    struct external *has_external = NULL;
    char **tab_element = NULL;
    char operator_c = 0;
    char buffer[1024];

    /* Init */
    strcpy(buffer_error_rtn,"");
    *byte_count_rtn = (BYTE)(current_line->nb_byte - 1);   /* Size of the Operand */
    *expression_address_rtn = 0xFFFFFFFF;        /* This is not a long address */
    is_pea_opcode = !my_stricmp(current_line->opcode_txt,"PEA");
    is_mvn_opcode = (!my_stricmp(current_line->opcode_txt,"MVN") || !my_stricmp(current_line->opcode_txt,"MVP"));

    /** We will treat the # <> ^ | from the very beginning **/
    has_dash = (expression_param[0] == '#') ? 1 : 0;
    has_less = (expression_param[has_dash] == '<') ? 1 : 0;
    has_more = (expression_param[has_dash] == '>') ? 1 : 0;
    has_exp = (expression_param[has_dash] == '^') ? 1 : 0;
    has_pipe = (expression_param[has_dash] == '|' || expression_param[has_dash] == '!') ? 1 : 0;

    /** If there is no operator (<=> # + - / * &. ^), Delete the {} **/
    for(int i=(has_dash+has_less+has_more+has_exp+has_pipe); i<(int)strlen(expression_param); i++)
        if(expression_param[i] == '=' || /* expression_param[i] == '<' || expression_param[i] == '>' || expression_param[i] == '#' ||    A REFAIRE */
           expression_param[i] == '+' || expression_param[i] == '-' || expression_param[i] == '/' || expression_param[i] == '*' ||
           expression_param[i] == '.' || expression_param[i] == '&' || expression_param[i] == '^')
        {
            is_algebric = 1;
            break;
        }
    if(is_algebric == 0)
    {
        int j = 0;
        for(int i=0; i<(int)strlen(expression_param); i++)
            if(expression_param[i] != '{' && expression_param[i] != '}')
                expression[j++] = expression_param[i];
        expression[j] = '\0';
    }
    else
        strcpy(expression,expression_param);

    /** Process the # <> ^ | **/
    has_dash = (expression[0] == '#') ? 1 : 0;
    has_less = (expression[has_dash] == '<') ? 1 : 0;
    has_more = (expression[has_dash] == '>') ? 1 : 0;
    has_exp = (expression[has_dash] == '^') ? 1 : 0;
    has_pipe = (expression[has_dash] == '|' || expression[has_dash] == '!') ? 1 : 0;

    /** Cut the string of characters into several elements (skips the #> <^ | from the beginning) **/
    tab_element = DecodeOperandeAsElementTable(&expression[has_dash+has_less+has_more+has_exp+has_pipe],&nb_element,SEPARATOR_EVALUATE_EXPRESSION,current_line);
    if(tab_element == NULL)
    {
        sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",expression);
        return(0);
    }

    /* Expression Empty */
    if(nb_element == 0 || (nb_element == 1 && strlen(tab_element[0]) == 0))
    {
        /* Particular case of BRK/COP/WDM sans signature */
        if(!my_stricmp(current_line->opcode_txt,"BRK") || !my_stricmp(current_line->opcode_txt,"COP") || !my_stricmp(current_line->opcode_txt,"WDM"))
            return(0);
        
        /* It's an Error */
        strcpy(buffer_error_rtn,"Empty expression");
        mem_free_table(nb_element,tab_element);
        return(0);
    }

    /** We will replace the MX **/
    if(strcmp(current_line->m,"?") && strcmp(current_line->x,"?"))
    {
        for(int i=0; i<nb_element; i++)
        {
            /* Is a letter? */
            if(!my_stricmp(tab_element[i],"MX"))
            {
                /* We can replace the expression */
                sprintf(buffer,"%d",(atoi(current_line->m) << 1)|atoi(current_line->x));
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
        }
    }

    /** We will evaluate / replace ]Variable **/
    for(int i=0; i<nb_element; i++)
    {
        if(tab_element[i][0] == ']')
        {
            /* Recover the value */
            value_variable = GetVariableValue(tab_element[i],&is_error,current_omfsegment);
            if(is_error == 1)
            {
                sprintf(buffer_error_rtn,"'%s' can't be translated as a Variable",tab_element[i]);
                mem_free_table(nb_element,tab_element);
                return(0);
            }

            /* We can replace */
            my_printf64(value_variable,buffer);
            new_value_txt = strdup(buffer);
            if(new_value_txt == NULL)
            {
                mem_free_table(nb_element,tab_element);
                my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
            }
            free(tab_element[i]);
            tab_element[i] = new_value_txt;
        }
    }

    /** We will evaluate the members coded in Ascii, Binary, Hex, Label **/
    for(int i=0; i<nb_element; i++)
    {
        if(!(strlen(tab_element[i]) == 1 && IsSeparator(tab_element[i][0],SEPARATOR_EVALUATE_EXPRESSION)))     /* The * = current address is going to be considered as a separator */
        {
            /* If one or more # are at the beginning of the value, it is removed */
            has_extra_dash = 0;
            while(tab_element[i][has_extra_dash] == '#')
                has_extra_dash++;

            /*** Evaluate each value ***/
            if(tab_element[i][has_extra_dash] == '\'' || tab_element[i][has_extra_dash] == '"')
            {
                /** We will replace the "Ascii" ("A" or "AA") **/
                value_ascii = GetAsciiValue(&tab_element[i][has_extra_dash]);
                if(value_ascii == -1)
                {
                    sprintf(buffer_error_rtn,"'%s' can't be translated as an ascii expression",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(0);
                }

                /* We can replace the expression */
                my_printf64(value_ascii,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else if(tab_element[i][has_extra_dash] == '%')
            {
                /** We will replace the % Binary **/
                value_binary = GetBinaryValue(&tab_element[i][1+has_extra_dash]);
                if(value_binary == -1)
                {
                    sprintf(buffer_error_rtn,"'%s' can't be translated as a binary expression",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(0);
                }

                /* We can replace the expression */
                my_printf64(value_binary,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else if(tab_element[i][has_extra_dash] == '$')
            {
                /** We will replace the $ Hex **/
                value_hexa = GetHexaValue(&tab_element[i][1+has_extra_dash]);
                if(value_hexa == -1)
                {
                    sprintf(buffer_error_rtn,"'%s' can't be translated as a hexadecimal expression",tab_element[i]);
                    mem_free_table(nb_element,tab_element);
                    return(0);
                }

                /* We can replace the expression */
                my_printf64(value_hexa,buffer);
                new_value_txt = strdup(buffer);
                if(new_value_txt == NULL)
                {
                    mem_free_table(nb_element,tab_element);
                    my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
                }
                free(tab_element[i]);
                tab_element[i] = new_value_txt;
            }
            else
            {
                /** Do we detect a Label (External or Internal to the Segment) **/
                value_address = GetAddressValue(&tab_element[i][has_extra_dash],current_line->address,&has_external,&is_dum_label,&is_fix_label,current_omfsegment);
                if(value_address == -2)
                {
                    /* The address is not ready */
                    sprintf(buffer_error_rtn,"Address of label '%s' is unknown at this time",&tab_element[i][has_extra_dash]);
                    mem_free_table(nb_element,tab_element);
                    return(0xFFFF);
                }
                else if(value_address != -1)
                {
                    /* We can replace the expression */
                    my_printf64(value_address,buffer);
                    new_value_txt = strdup(buffer);
                    if(new_value_txt == NULL)
                    {
                        mem_free_table(nb_element,tab_element);
                        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
                    }
                    free(tab_element[i]);
                    tab_element[i] = new_value_txt;

                    /* We have detected a relocatable address (the DUM and FIX are not relocatable) */
                    if(is_dum_label == 0 && is_fix_label == 0)
                        nb_address++;

                    /** The label is in an external Segment **/
                    if(has_external != NULL)
                    {
                        if(current_external == NULL)
                            current_external = has_external;
                        else
                        {
                            /* Error: Using External Label imposes some constraints in the expressions */
                            sprintf(buffer_error_rtn,"You can't use 2 External labels (%s and %s) in the same expression",current_external->name,has_external->name);
                            mem_free_table(nb_element,tab_element);
                            return(0);
                        }

                        /* The expression already calls for at least one Internal Label: we do not mix Internal and External */
                        if(nb_address > 1)
                        {
                            /* Error: Using External Label imposes some constraints in the expressions */
                            sprintf(buffer_error_rtn,"You can't mix an External label (%s) with an Internal label (%s) the same expression",has_external->name,&tab_element[i][has_extra_dash]);
                            mem_free_table(nb_element,tab_element);
                            return(0);
                        }
                    }
                    else
                    {
                        /** We do not mix External and Internal **/
                        if(current_external != NULL)
                        {
                            /* Error: Using External Label imposes some constraints in the expressions */
                            sprintf(buffer_error_rtn,"You can't mix an Internal label (%s) with an External label (%s) the same expression",&tab_element[i][has_extra_dash],current_external->name);
                            mem_free_table(nb_element,tab_element);
                            return(0);
                        }
                    }
                }
                else
                {
                    /** We could be a numerical constant **/
                    value_decimal = GetDecimalValue(&tab_element[i][has_extra_dash],&is_error);
                    if(is_error == 0)
                    {
                        /* We can replace the expression */
                        my_printf64(value_decimal,buffer);
                        new_value_txt = strdup(buffer);
                        if(new_value_txt == NULL)
                        {
                            mem_free_table(nb_element,tab_element);
                            my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for new 'value_txt'");
                        }
                        free(tab_element[i]);
                        tab_element[i] = new_value_txt;
                    }
                    else
                    {
                        /** We do not know how to interpret this value **/
                        sprintf(buffer_error_rtn,"'%s' can't be translated as a numeric expression",tab_element[i]);
                        mem_free_table(nb_element,tab_element);
                        return(0);
                    }
                }
            }
        }
    }

    /** We will manage the - at the beginning => Passage of the first value in negative **/
    if(!strcmp(tab_element[0],"-") && nb_element > 1)
    {
        /* The first value is negative */
        first_value_is_negative = 1;

        /* We can remove the - */
        free(tab_element[0]);
        for(int j=1; j<nb_element; j++)
            tab_element[j-1] = tab_element[j];
        nb_element--;
    }

    /** We must now only have: value [operator value [operator value ...]] **/
    if(nb_element % 2 == 0)
    {
        strcpy(buffer_error_rtn,"The number of element is even (should be value [operator value [operator value]...])");
        mem_free_table(nb_element,tab_element);
        return(0);
    }

    /** Are there any {} and are they properly nested? **/
    has_priority = 0;
    nb_open = 0;
    for(int i=0; i<nb_element; i++)
        if(!strcmp(tab_element[i],"{"))
        {
            nb_open++;
            has_priority = 1;
        }
        else if(!strcmp(tab_element[i],"}"))
        {
            nb_open--;
            if(nb_open < 0)
            {
                /* Error */
                strcpy(buffer_error_rtn,"Wrong '}' in expression");
                mem_free_table(nb_element,tab_element);
                return(0);
            }
        }
    if(nb_open != 0)
    {
        /* Error */
        strcpy(buffer_error_rtn,"Missing '}' in expression");
        mem_free_table(nb_element,tab_element);
        return(0);
    }

    /** We will evaluate the expression **/
    is_operator = 0;
    for(int i=0; i<nb_element; i++)
    {
        /** We will try to decode all types of data **/
        if(is_operator == 0)
        {
            /** We must come across a value **/
            if(!strcmp(tab_element[i],"{"))
                value = EvaluateAlgebricExpression(tab_element,i,nb_element,current_line->address,&nb_item);
            else
            {
                /** The digital conversion has already been done **/
                value = my_atoi64(tab_element[i]);
            }

            /** Add this value to the global expression **/
            if(i == 0)
                value_expression = value*((first_value_is_negative == 1) ? -1 : 1);
            else
            {
                /** Applies the operator to both values **/
                if(operator_c == '>')
                    value_expression = (value_expression > value) ? 1 : 0;
                else if(operator_c == '=')
                    value_expression = (value_expression == value) ? 1 : 0;
                else if(operator_c == '<')
                    value_expression = (value_expression < value) ? 1 : 0;
                else if(operator_c == '#')
                    value_expression = (value_expression != value) ? 1 : 0;
                else if(operator_c == '+')
                    value_expression = value_expression + value;
                else if (operator_c == '-')
                    value_expression = value_expression - value;
                else if (operator_c == '*')
                    value_expression = value_expression * value;
                else if (operator_c == '/')
                {
                    /* Beware of divisions by Zero */
                    if(value == 0)
                    {
                        strcpy(buffer_error_rtn,"Division by Zero");
                        mem_free_table(nb_element,tab_element);
                        return(0);
                    }

                    value_expression = value_expression / value;
                }
                else if (operator_c == '!')
                    value_expression = value_expression ^ value;
                else if (operator_c == '.')
                    value_expression = value_expression | value;
                else if (operator_c == '&')
                    value_expression = value_expression & value;
            }

            /* Next will be an operator */
            is_operator = 1;

            /* If we had one { we skip all the expression */
            if(!strcmp(tab_element[i],"{"))
                i += nb_item;
        }
        else
        {
            /** We must recognize the operator **/
            if(strcmp(tab_element[i],"<") && strcmp(tab_element[i],"=") && strcmp(tab_element[i],">") && strcmp(tab_element[i],"#") &&
               strcmp(tab_element[i],"+") && strcmp(tab_element[i],"-") && strcmp(tab_element[i],"*") && strcmp(tab_element[i],"/") &&
               strcmp(tab_element[i],"!") && strcmp(tab_element[i],".") && strcmp(tab_element[i],"&"))
            {
                sprintf(buffer_error_rtn,"The '%s' is not a valid operator",tab_element[i]);
                mem_free_table(nb_element,tab_element);
                return(0);
            }

            /* Keep the operator for the future */
            operator_c = tab_element[i][0];

            /* Next will be a value */
            is_operator = 0;
        }
    }

    /* Memory release */
    mem_free_table(nb_element,tab_element);

    /** We give reloceability information via Prefix #><^| **/
    if((nb_address%2) == 1)
    {
        /* We return a relocatable address */
        *is_reloc_rtn = 1;
        *expression_address_rtn = (0x00FFFFFF & value_expression);   /* Adresse Longue 24 bit : Bank/HighLow */

        /* Code Line */
        if(current_line->type == LINE_CODE)
        {
            /* Number of Bytes to Relocate */
            if((has_dash == 1 || is_pea_opcode == 1) && has_exp == 1)          /* # ^ = 1 or 2 Byte relocate */
            {
                /* For an External Label, relocate 2 bytes */
                if(*external_rtn != NULL)
                {
                    if(operand_size == 1)
                        *byte_count_rtn = 1;
                    else
                        *byte_count_rtn = 2;
                }
                else
                    *byte_count_rtn = 1;      /* For an internal label, relocate 1 byte */
            }
            else
                *byte_count_rtn = (BYTE)operand_size;

            /* Bit Shift Count */
            if((has_dash == 1 || is_pea_opcode == 1) && has_more == 1)
                *bit_shift_rtn = 0xF8;                   /* >> 8 */
            else if(((has_dash == 1 || is_pea_opcode == 1) && has_exp == 1) || is_mvn_opcode == 1)
                *bit_shift_rtn = 0xF0;                   /* >> 16 */
            else
                *bit_shift_rtn = 0x00;

            /* We point to the label External */
            if(current_external != NULL)
                *external_rtn = current_external;
        }
        else if(current_line->type == LINE_DATA)
        {
            /* Number of Bytes to Relocate */
            if(operand_size == 3 || operand_size == 4)
                *byte_count_rtn = 3;
            else if(has_exp == 1)          /* ^ = 1 Byte à reloger */
                *byte_count_rtn = 1;
            else
                *byte_count_rtn = (BYTE)operand_size;

            /* Bit Shift Count */
            if(has_more == 1)
                *bit_shift_rtn = 0xF8;                   /* >> 8 */
            else if(has_exp == 1)
                *bit_shift_rtn = 0xF0;                   /* >> 16 */
            else
                *bit_shift_rtn = 0x00;
            
            /* We point to the label External */
            if(current_external != NULL)
                *external_rtn = current_external;
        }

        /* Offset where the value is stored */
        *offset_reference_rtn = (WORD) value_expression;
    }
    else
        *expression_address_rtn = (0x00FFFFFF & value_expression);   /* Adresse Longue 24 bit : Bank/HighLow */
    
    /** We modify the value returned according to the Prefix #><^| **/
    if((has_dash == 1 || is_pea_opcode == 1 || current_line->type == LINE_DATA) && has_more == 1)
        value_expression = value_expression >> 8;
    else if((has_dash == 1 || is_pea_opcode == 1 || current_line->type == LINE_DATA) && has_exp == 1)
        value_expression = value_expression >> 16;

    /* Returns the expression */
    return(value_expression);
}


/*********************************************************************************/
/*  EvaluateAlgebricExpression() :  Evaluate of an algebraic expression with {}. */
/*********************************************************************************/
int64_t EvaluateAlgebricExpression(char **tab_element, int current_element, int nb_element, int address, int *nb_item_rtn)
{
    int last_element = 0, is_value = 0, stack_pointer = 0, output_pointer = 0, value_index = 0, nb_open=0;
    int64_t value = 0;
    char address_line[256];
    int64_t value_tab[256];
    char **stack = NULL;
    char **output = NULL;

    /* Prepare the address */
    sprintf(address_line,"%d",address);

    /** We spot the {} **/
    for(int i=current_element; i<nb_element; i++)
    {
        if(!my_stricmp(tab_element[i],"{"))
            nb_open++;
        else if(!my_stricmp(tab_element[i],"}"))
        {
            nb_open--;
            if(nb_open == 0)
            {
                last_element = i;
                break;
            }
        }
    }

    /* Allocate memory */
    stack = (char **) calloc(nb_element,sizeof(char *));
    if(stack == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'stack' table");
    output = (char **) calloc(nb_element,sizeof(char *));
    if(output == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'output' table");

    /** Switch to reverse Polish notation respecting {} and precedence of the operators **/
    is_value = 1;
    stack_pointer = 0;
    output_pointer = 0;
    for(int i=current_element; i<=last_element; i++)
    {
        /* Stack the { */
        if(!my_stricmp(tab_element[i],"{"))
        {
            stack[stack_pointer++] = tab_element[i];
        }
        else if(!my_stricmp(tab_element[i],"}"))
        {
            /* Operators are removed until the { */
            for(int j=stack_pointer-1; j>=0; j--)
            {
                if(!my_stricmp(stack[j],"{"))
                    break;
                output[output_pointer++] = stack[j];
                stack_pointer--;
            }
            stack_pointer--;
        }
        else if(is_value == 1)
        {
            /* Add this value to the output */
            if(!my_stricmp(tab_element[i],"*"))
                output[output_pointer++] = &address_line[0];      /* * is the current address */
            else
                output[output_pointer++] = tab_element[i];

            /* The next element will be an operator */
            is_value = 0;
        }
        else
        {
            /* Add this Operator */
            if(stack_pointer == 0)
                stack[stack_pointer++] = tab_element[i];
            else if(!my_stricmp(stack[stack_pointer-1],"{"))
                stack[stack_pointer++] = tab_element[i];
            else if(HasPriority(tab_element[i],stack[stack_pointer-1]))
                stack[stack_pointer++] = tab_element[i];
            else
            {
                output[output_pointer++] = stack[stack_pointer-1];
                stack[stack_pointer-1] = tab_element[i];
            }

            /* The next element will be a value */
            is_value = 1;
        }
    }
    /* If there are any things left on the pile, we move them */
    for(int i=stack_pointer-1; i>=0; i--)
        output[output_pointer++] = stack[i];

    /** Move and we evaluate the rest **/
    value_index = 0;
    for(int i=0; i<output_pointer; i++)
    {
        /* Operator */
        if(!my_stricmp(output[i],"<") || !my_stricmp(output[i],"=") || !my_stricmp(output[i],">") || !my_stricmp(output[i],"#") ||
           !my_stricmp(output[i],"+") || !my_stricmp(output[i],"-") || !my_stricmp(output[i],"*") || !my_stricmp(output[i],"/") ||
           !my_stricmp(output[i],"&") || !my_stricmp(output[i],".") || !my_stricmp(output[i],"!"))
        {
            if(!my_stricmp(output[i],"<"))
                value_tab[value_index-2] = (value_tab[value_index-2] < value_tab[value_index-1]) ? 1 : 0;
            else if(!my_stricmp(output[i],"="))
                value_tab[value_index-2] = (value_tab[value_index-2] == value_tab[value_index-1]) ? 1 : 0;
            else if(!my_stricmp(output[i],">"))
                value_tab[value_index-2] = (value_tab[value_index-2] > value_tab[value_index-1]) ? 1 : 0;
            else if(!my_stricmp(output[i],"#"))
                value_tab[value_index-2] = (value_tab[value_index-2] != value_tab[value_index-1]) ? 1 : 0;
            else if(!my_stricmp(output[i],"+"))
                value_tab[value_index-2] = value_tab[value_index-2] + value_tab[value_index-1];
            else if(!my_stricmp(output[i],"-"))
                value_tab[value_index-2] = value_tab[value_index-2] - value_tab[value_index-1];
            else if(!my_stricmp(output[i],"*"))
                value_tab[value_index-2] = value_tab[value_index-2] * value_tab[value_index-1];
            else if(!my_stricmp(output[i],"/"))
                value_tab[value_index-2] = value_tab[value_index-2] / value_tab[value_index-1];
            else if(!my_stricmp(output[i],"&"))
                value_tab[value_index-2] = value_tab[value_index-2] & value_tab[value_index-1];
            else if(!my_stricmp(output[i],"."))
                value_tab[value_index-2] = value_tab[value_index-2] | value_tab[value_index-1];
            else if(!my_stricmp(output[i],"!"))
                value_tab[value_index-2] = value_tab[value_index-2] ^ value_tab[value_index-1];
            value_index--;
        }
        else
            value_tab[value_index++] = my_atoi64(output[i]);
    }
    value = value_tab[0];

    /* Memory release */
    free(stack);
    free(output);

    /* Number of items */
    *nb_item_rtn = (last_element - current_element);

    /* Return the value */
    return(value);
}


/************************************************************/
/*  HasPriority() :  Establishes the priority of Operators. */
/************************************************************/
int HasPriority(char *op1, char *op2)
{
    int p1 = 0, p2 = 0;

    /* Operator's priority 1 */
    if(!my_stricmp(op1,"<") || !my_stricmp(op1,"=") || !my_stricmp(op1,">") || !my_stricmp(op1,"#"))
        p1 = 0;
    else if(!my_stricmp(op1,"+") || !my_stricmp(op1,"-"))
        p1 = 1;
    else if(!my_stricmp(op1,"*") || !my_stricmp(op1,"/"))
        p1 = 2;
    else if(!my_stricmp(op1,"&") || !my_stricmp(op1,".") || !my_stricmp(op1,"!"))
        p1 = 3;

    /* Operator's priority 2 */
    if(!my_stricmp(op2,"<") || !my_stricmp(op2,"=") || !my_stricmp(op2,">") || !my_stricmp(op2,"#"))
        p2 = 0;
    else if(!my_stricmp(op2,"+") || !my_stricmp(op2,"-"))
        p2 = 1;
    else if(!my_stricmp(op2,"*") || !my_stricmp(op2,"/"))
        p2 = 2;
    else if(!my_stricmp(op2,"&") || !my_stricmp(op2,".") || !my_stricmp(op2,"!"))
        p2 = 3;

    /* Comparaison */
    return((p1>p2)?1:0);
}


/*****************************************************************/
/*  BuildBestMVXWord() :  Returns the operand bytes for MVN/MVP. */
/*****************************************************************/
int BuildBestMVXWord(DWORD value_1, DWORD value_2)
{
    BYTE byte_high, byte_low;
    BYTE value_byte_1[4];
    BYTE value_byte_2[4];

    /* Transform values into bytes (by Byte Order) */
    bo_memcpy(&value_byte_1[0],&value_1,sizeof(DWORD));
    bo_memcpy(&value_byte_2[0],&value_2,sizeof(DWORD));

    /* We take the first Byte non-zero (privileging the strong +) */
    byte_high = (value_byte_1[2] != 0x00 || value_byte_1[1] != 0x00) ? value_byte_1[2] : value_byte_1[0];
    byte_low =  (value_byte_2[2] != 0x00 || value_byte_2[1] != 0x00) ? value_byte_2[2] : value_byte_2[0];

    /* Values of the Banks */
    return((int)(((WORD)byte_high) << 8 | byte_low));
}


/**********************************************************************/
/*  IsPageDirectOpcode() :  Is it an opcode that accepts Direct Page. */
/**********************************************************************/
int IsPageDirectOpcode(char *opcode)
{
    char *opcode_list[] = {"ADC","AND","ASL","BIT","CMP","CPX","CPY","DEC","EOR","INC","LDA","LDX","LDY","LSR","ORA","ROL","ROR","SBC","STA","STX","STY","STZ","TRB","TSB",NULL};

    /* Find an Opcode accepting the Direct Page */
    for(int i = 0; opcode_list[i]; i++)
        if(!my_stricmp(opcode_list[i],opcode))
            return(1);

    /* Not found */
    return(0);
}


/***********************************************************************/
/*  IsPageDirectAddressMode() :  Is it an Addressing Page Direct Page. */
/***********************************************************************/
int IsPageDirectAddressMode(int address_mode)
{
    /* Search a Direct Page Addressing Mode */
    if(address_mode == AM_DIRECT_PAGE || address_mode == AM_DIRECT_PAGE_INDEXED_X || address_mode == AM_DIRECT_PAGE_INDEXED_Y ||
       address_mode == AM_DIRECT_PAGE_INDIRECT || address_mode == AM_DIRECT_PAGE_INDIRECT_LONG || address_mode == AM_DIRECT_PAGE_INDEXED_X_INDIRECT ||
       address_mode == AM_DIRECT_PAGE_INDIRECT_INDEXED_Y || address_mode == AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y)
        return(1);

    /* Not found */
    return(0);
}


/*********************************************************/
/*  IsDirectPageLabel() :  returns 1 if label is for DP. */
/*********************************************************/
int isLabelForDirectPage(struct label *current_label, struct omf_segment *current_omfsegment)
{
    int64_t dum_address = 0;
    int is_reloc = 0;
    BYTE byte_count = 0, bit_shift = 0;
    WORD offset_reference = 0;
    DWORD address_long = 0;
    struct external *current_external = NULL;
    char buffer_error[1024] = "";

    if(current_label == NULL)
        return(0);

    /* Is this a label located in a DUM */
    if(current_label->line->is_dum == 1)
    {
        /* We try to evaluate the address of the DUM */
        dum_address = EvalExpressionAsInteger(current_label->line->dum_line->operand_txt,&buffer_error[0],current_label->line->dum_line,2,&is_reloc,&byte_count,&bit_shift,&offset_reference,&address_long,&current_external,current_omfsegment);
        if(strlen(buffer_error) == 0 && dum_address < 0x100)
            return(1);     /* can be */
        else
            return(0);     /* no */
    }
    else if(current_omfsegment->is_relative == 1)
        return(0);

    /* Is the nearest ORG < 0x100 */

    /* can be */
    return(1);
}

/***********************************************************/
/*  IsDirectPageLabel() :  Can this label be placed in DP. */
/***********************************************************/
int IsDirectPageLabel(char *label_name, struct omf_segment *current_omfsegment)
{
    struct label *current_label;
    struct source_file *first_file;

    /* Source file */
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);

    /* Search for the Label */
    my_Memory(MEMORY_SEARCH_LABEL,label_name,&current_label,current_omfsegment);

    return isLabelForDirectPage( current_label, current_omfsegment );
}


/******************************************************************/
/*  UseCurrentAddress() :  Is there a * placed in value position. */
/******************************************************************/
int UseCurrentAddress(char *operand, char *buffer_error_rtn, struct source_line *current_line)
{
    int nb_element = 0;
    char **tab_element = NULL;

    /* Init */
    strcpy(buffer_error_rtn,"");

    /* Rapid search */
    if(strchr(operand,'*') == NULL)
        return(0);

    /** Cut the string of characters into several elements (skips the #> <^ | from the beginning) **/
    tab_element = DecodeOperandeAsElementTable(operand,&nb_element,SEPARATOR_EVALUATE_EXPRESSION,current_line);
    if(tab_element == NULL)
    {
        sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",operand);
        return(0);
    }

    /** Review all the items **/
    int is_value = 1;
    for(int i = 0; i < nb_element; i++)
    {
        /* The first value can be a negative sign (-1*3) or a ! */
        if(i == 0 && !my_stricmp(tab_element[i],"-"))
            continue;
        if(i == 0 && !my_stricmp(tab_element[i],"!"))
            continue;
        if(i == 0 && !my_stricmp(tab_element[i],"#"))
            continue;

        /* We will ignore {} algebraic expressions */
        if(!my_stricmp(tab_element[i],"{") || !my_stricmp(tab_element[i],"}"))
            continue;

        /** On reconnait immédiatement les Operators no ambigus + / - & . ! = **/
        if(!my_stricmp(tab_element[i],"+") || !my_stricmp(tab_element[i],"-") || !my_stricmp(tab_element[i],"/") ||
           !my_stricmp(tab_element[i],"&") || !my_stricmp(tab_element[i],".") || !my_stricmp(tab_element[i],"!") ||
           !my_stricmp(tab_element[i],"="))
        {
            is_value = 1;
            continue;
        }

        /** We only look at the values **/
        if(is_value == 1)
        {
            if(!my_stricmp(tab_element[i],"*"))
            {
                mem_free_table(nb_element,tab_element);
                return(1);    /* found */
            }
            is_value = 0;
        }
        else
            is_value = 1;
    }

    /* Memory release */
    mem_free_table(nb_element,tab_element);

    /* Not found */
    return(0);
}


/*********************************************************************************/
/*  ReplaceCurrentAddressInOperand() :  Remplace le * par le label unique dédié. */
/*********************************************************************************/
void ReplaceCurrentAddressInOperand(char **operand_adr, char *label_name, char *buffer_error_rtn, struct source_line *current_line)
{
    int i, nb_element, is_value;
    char *operand;
    char *new_operand;
    char **tab_element;
    char buffer[1024];

    /* Init */
    operand = *operand_adr;
    strcpy(buffer_error_rtn,"");
    strcpy(buffer,"");

    /** Cut the string of characters into several elements (skips the #> <^ | from the beginning) **/
    tab_element = DecodeOperandeAsElementTable(operand,&nb_element,SEPARATOR_EVALUATE_EXPRESSION,current_line);
    if(tab_element == NULL)
    {
        sprintf(buffer_error_rtn,"Impossible to decode Operand '%s' as element table",operand);
        return;
    }

    /** Review all the items **/
    is_value = 1;
    for(i=0; i<nb_element; i++)
    {
        /* The first value can be a negative sign (-1*3) */
        if(i == 0 && (!my_stricmp(tab_element[i],"-") || !my_stricmp(tab_element[i],"!") || !my_stricmp(tab_element[i],"#")))
        {
            strcat(buffer,tab_element[i]);
            continue;
        }

        /* We will ignore {} algebraic expressions */
        if(!my_stricmp(tab_element[i],"{") || !my_stricmp(tab_element[i],"}"))
        {
            strcat(buffer,tab_element[i]);
            continue;
        }

        /** We recognize the Operators + / - **/
        if(!my_stricmp(tab_element[i],"+") || !my_stricmp(tab_element[i],"-") || !my_stricmp(tab_element[i],"/"))
        {
            strcat(buffer,tab_element[i]);
            is_value = 1;
            continue;
        }

        /** We only look at the values **/
        if(is_value == 1)
        {
            if(!my_stricmp(tab_element[i],"*"))
                strcat(buffer,label_name);
            else
                strcat(buffer,tab_element[i]);
            is_value = 0;
        }
        else
        {
            strcat(buffer,tab_element[i]);
            is_value = 1;
        }
    }

    /* Memory release */
    mem_free_table(nb_element,tab_element);

    /* Allocate memory */
    new_operand = strdup(buffer);
    if(new_operand == NULL)
    {
        sprintf(buffer_error_rtn,"Impossible to allocate memory to replace * with current address");
        return;
    }
    free(operand);
    *operand_adr = new_operand;
}


/**************************************************************/
/*  my_SetFileAttribute() :  Change the visibility of a File. */
/**************************************************************/
void my_SetFileAttribute(char *file_path, int flag)
{
#if defined(WIN32) || defined(WIN64)
    DWORD file_attributes;

    /* Attributs of File */
    file_attributes = GetFileAttributes(file_path);

    /* Change the visibility */
    if(flag == SET_FILE_VISIBLE)
    {
        /* Show the File */
        if((file_attributes | FILE_ATTRIBUTE_HIDDEN) == file_attributes)
            SetFileAttributes(file_path,file_attributes - FILE_ATTRIBUTE_HIDDEN);
    }
    else if(flag == SET_FILE_HIDDEN)
    {
        /* Hide the File */
        if((file_attributes | FILE_ATTRIBUTE_HIDDEN) != file_attributes)
            SetFileAttributes(file_path,file_attributes | FILE_ATTRIBUTE_HIDDEN);
    }
#else
    (void)file_path;
    (void)flag;
    /* On Unix the visibility of the File is linked to the File name */
#endif
}


/***************************************************/
/*  CreateBinaryFile() :  Creating a File on Disk. */
/***************************************************/
int CreateBinaryFile(char *file_path, unsigned char *data, int length)
{
    int nb_write;
    FILE *fd;

    /* Deleting the File */
    my_DeleteFile(file_path);

    /* Create the File */
#if defined(WIN32) || defined(WIN64)    
    fd = fopen(file_path,"wb");
#else
    fd = fopen(file_path,"w");
#endif
    if(fd == NULL)
        return(1);

    /* Writing the data */
    nb_write = (int) fwrite(data,1,length,fd);
    if(nb_write != length)
    {
        fclose(fd);
        return(2);
    }

    /* Close the File */
    fclose(fd);

    /* OK */
    return(0);
}


/*****************************************************/
/*  my_DeleteFile() :  Deletes a File from the disk. */
/*****************************************************/
void my_DeleteFile(char *file_path)
{
    /* Make the file visible */
    my_SetFileAttribute(file_path,SET_FILE_VISIBLE);

    /* Deletes the File */
    unlink(file_path);
}


/**************************************************************************/
/*  BuildUniqueListFromFile() :  Retrieves the list of values for a File. */
/**************************************************************************/
char **BuildUniqueListFromFile(char *file_path, int *nb_value)
{
    FILE *fd;
    int i, found, nb_line, line_length;
    char **tab;
    char buffer_line[1024];

    /* Opening the File */
    fd = fopen(file_path,"r");
    if(fd == NULL)
        return(NULL);

    /* Count the number of rows */
    nb_line = 0;
    fseek(fd,0L,SEEK_SET);
    while(fgets(buffer_line,1024-1,fd))
        nb_line++;

    /* Allocation for table */
    tab = (char **) calloc(nb_line,sizeof(char *));
    if(tab == NULL)
    {
        fclose(fd);
        return(NULL);
    }

    /** Reading the File **/
    nb_line = 0;
    fseek(fd,0L,SEEK_SET);
    while(fgets(buffer_line,1024-1,fd))
    {
        /** Preliminary cleaning treatment **/
        line_length = (int) strlen(buffer_line);
        if(line_length < 2)              /* Empty line */
            continue;
        if(buffer_line[line_length-1] == '\n')
            buffer_line[line_length-1] = '\0';  /* make the final \n into EOL (zero) */

        /** Stores the value **/
        for(i=0,found=0; i<nb_line; i++)
        {
            if(!my_stricmp(tab[i],buffer_line))
            {
                found = 1;
                break;
            }
        }

        if(found == 0)
        {
            tab[nb_line] = strdup(buffer_line);
            if(tab[nb_line] == NULL)
            {
                for(i=0;i<nb_line;i++)
                {
                    free(tab[i]);
                }
                free(tab);
                fclose(fd);
                return(NULL);
            }
            nb_line++;
        }
    }

    /* Close the File */
    fclose(fd);

    /* Return the table */
    *nb_value = nb_line;
    return(tab);
}

/***************************************************************************************/
/*  BuildUniqueListFromText() :  Retrieves the unique list of values in a Text buffer. */
/***************************************************************************************/
char **BuildUniqueListFromText(char *text, char separator, int *nb_value_rtn)
{
    int i, j, nb_found, nb_value, value_length;
    char **tab;
    char buffer_value[1024];

    /* Count the number of separators */
    nb_value = 1;
    for(i=0; text[i] != '\0'; i++)
        if(text[i] == separator)
            nb_value++;

    /* Allocation for table */
    tab = (char **) calloc(nb_value,sizeof(char *));
    if(tab == NULL)
        return(NULL);

    /** Reading values **/
    nb_found = 0;
    value_length = 0;
    for(i=0; text[i] != '\0'; i++)
    {
        if(text[i] == separator)
        {
            if(value_length > 0)
            {
                /* Complete the value */
                buffer_value[value_length] = '\0';

                /* Check that it does not already exist */
                for(j=0; j<nb_found; j++)
                {
                    if(!strcmp(tab[j],buffer_value))
                    {
                        value_length = 0;
                        break;
                    }
                }

                /* Stores the value */
                if(value_length > 0)
                {
                    tab[nb_found] = strdup(buffer_value);
                    if(tab[nb_found] == NULL)
                    {
                        for(j=0; j<nb_found; j++)
                        {
                            if(tab[j] != NULL)
                                free(tab[j]);
                        }
                        free(tab);
                        return(NULL);
                    }
                    nb_found++;
                    value_length = 0;
                }
            }
        }
        else
        {
            buffer_value[value_length++] = text[i];
        }
    }

    /** Last value **/
    if(value_length > 0)
    {
        /* Complete the value */
        buffer_value[value_length] = '\0';

        /* Check that it does not already exist */
        for(j=0; j<nb_found; j++)
        {
            if(!strcmp(tab[j],buffer_value))
            {
                value_length = 0;
                break;
            }
        }

        /* Stores the value */
        if(value_length > 0)
        {
            tab[nb_found] = strdup(buffer_value);
            if(tab[nb_found] == NULL)
            {
                for(j=0; j<nb_found; j++)
                {
                    if(tab[j] != NULL)
                        free(tab[j]);
                }
                free(tab);
                return(NULL);
            }
            nb_found++;
            value_length = 0;
        }
    }

    /* Return the table */
    *nb_value_rtn = nb_found;
    return(tab);
}


/***********************************************************/
/*  IsProdosName() :  is the name a valid ProDOS filename? */
/***********************************************************/
int IsProdosName(char *file_name)
{
    int i;

    /* 15 char max */
    if(strlen(file_name) == 0 || strlen(file_name) > 15)
        return(0);

    /* Letters, numbers or . */
    for(i=0; i<(int)strlen(file_name); i++)
        if(!((file_name[0] >= 'a' && file_name[0] <= 'z') || (file_name[0] >= 'A' && file_name[0] <= 'Z') ||
             (file_name[0] >= '0' && file_name[0] <= '9') ||
             file_name[0] == '.'))
            return(0);

    /* A letter at the beginning */
    if(!((file_name[0] >= 'a' && file_name[0] <= 'z') || (file_name[0] >= 'A' && file_name[0] <= 'Z')))
        return(0);

    /* OK */
    return(1);
}


/***********************************************************/
/*  BuildAbsolutePath() :  Building an absolute file path. */
/***********************************************************/
void BuildAbsolutePath(char *file_name, char *folder_path, char *file_path_rtn)
{
    /* Init */
    strcpy(file_path_rtn,file_name);

    /* Is file_name absolute? */
    if(file_name[1] == ':' || file_name[0] == '/')
        return;

    /* Folder Path + File Name */
    strcpy(file_path_rtn,folder_path);
    if(file_name[0] == '/' || file_name[0] == '\\')
        strcat(file_path_rtn,&file_name[1]);
    else
        strcat(file_path_rtn,file_name);
}


/**************************************************/
/*  mem_free_list() :  Memory release of a table. */
/**************************************************/
void mem_free_list(int nb_element, char **element_tab)
{
    if(element_tab)
    {
        for(int i = 0; i < nb_element; i++)
        {
            if(element_tab[i])
                free(element_tab[i]);
        }
        free(element_tab);
    }
}


/*********************************************************/
/*  compare_item() : Comparison function for Quick Sort. */
/*********************************************************/
int compare_item(const void *data_1, const void *data_2)
{
    /* Parameter retrieval */
    struct item *item_1 = *((struct item **) data_1);
    struct item *item_2 = *((struct item **) data_2);

    /* Comparison of item names (Opcode, directive, Equivalence...) */
    return(my_stricmp(item_1->name,item_2->name));
}


/**********************************************************/
/*  compare_macro() : Comparison function for Quick Sort. */
/**********************************************************/
int compare_macro(const void *data_1, const void *data_2)
{
    /* Parameter retrieval */
    struct macro *macro_1 = *((struct macro **) data_1);
    struct macro *macro_2 = *((struct macro **) data_2);

    /* Comparison of macro names (case sensitive) */
    return(strcmp(macro_1->name,macro_2->name));
}


/**********************************************************/
/*  compare_label() : Comparison function for Quick Sort. */
/**********************************************************/
int compare_label(const void *data_1, const void *data_2)
{
    /* Parameter retrieval */
    struct label *label_1 = *((struct label **) data_1);
    struct label *label_2 = *((struct label **) data_2);

    /* Comparison of the keys: The labels are case-sensitive */
    return(strcmp(label_1->name,label_2->name));
}


/****************************************************************/
/*  compare_equivalence() : Comparison function for Quick Sort. */
/****************************************************************/
int compare_equivalence(const void *data_1, const void *data_2)
{
    struct equivalence *equivalence_1;
    struct equivalence *equivalence_2;

    /* Parameter retrieval (case sensitive) */
    equivalence_1 = *((struct equivalence **) data_1);
    equivalence_2 = *((struct equivalence **) data_2);

    /* Key comparison */
    return(strcmp(equivalence_1->name,equivalence_2->name));
}


/*************************************************************/
/*  compare_variable() : Comparison function for Quick Sort. */
/*************************************************************/
int compare_variable(const void *data_1, const void *data_2)
{
    /* Parameter retrieval */
    struct variable *variable_1 = *((struct variable **) data_1);
    struct variable *variable_2 = *((struct variable **) data_2);

    /* Key comparison (case sensitive) */
    return(strcmp(variable_1->name,variable_2->name));
}


/*************************************************************/
/*  compare_external() : Comparison function for Quick Sort. */
/*************************************************************/
int compare_external(const void *data_1, const void *data_2)
{
    /* Parameter retrieval (case sensitive) */
    struct external *external_1 = *((struct external **) data_1);
    struct external *external_2 = *((struct external **) data_2);

    /* Key comparison */
    return(strcmp(external_1->name,external_2->name));
}


/***************************************************************/
/*  mem_alloc_item() :  Allocate memory of the item structure. */
/***************************************************************/
struct item *mem_alloc_item(char *name, int type)
{
    /* Allocation */
    struct item *current_item = (struct item *) calloc(1,sizeof(struct item));
    if(current_item == NULL)
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for structure item");
    
    /* Fill out structure */
    current_item->name = strdup(name);
    if(current_item->name == NULL)
    {
        free(current_item);
        my_RaiseError(ERROR_RAISE,"Impossible to allocate memory for 'name' from structure item");
    }
    current_item->type = type;
    
    /* Returns the structure */
    return(current_item);
}


/*************************************************************/
/*  mem_free_item() :  Memory release of the item structure. */
/*************************************************************/
void mem_free_item(struct item *current_item)
{
    if(current_item)
    {
        if(current_item->name)
            free(current_item->name);

        free(current_item);
    }
}


/***************************************************************/
/*  mem_free_item_list() :  Memory release of item structures. */
/***************************************************************/
void mem_free_item_list(struct item *all_item)
{
    for(struct item *current_item = all_item; current_item; )
    {
        struct item *next_item = current_item->next;
        mem_free_item(current_item);
        current_item = next_item;
    }
}


/*****************************************************************/
/*  mem_alloc_param() :  Allocate memory of the Param structure. */
/*****************************************************************/
struct parameter *mem_alloc_param(void)
{
    WORD one_word;
    unsigned char one_word_bin[16];
    time_t clock;
    struct tm *p;
    int year, hour;
    char *month[] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
    struct parameter *param;

    /* Allocate memory */
    param = (struct parameter *) calloc(1,sizeof(struct parameter));
    if(param == NULL)
        return(NULL);

    /* Buffers (64 KB) */
    param->buffer_line = (char *) calloc(1,65536);
    param->buffer_value = (char *) calloc(1,65536);
    param->buffer_folder_path = (char *) calloc(1,65536);
    param->buffer_file_path = (char *) calloc(1,65536);
    param->buffer_file_name = (char *) calloc(1,65536);
    param->buffer_label = (char *) calloc(1,65536);
    param->buffer_opcode = (char *) calloc(1,65536);
    param->buffer_operand = (char *) calloc(1,65536);
    param->buffer_comment = (char *) calloc(1,65536);
    param->buffer_error = (char *) calloc(1,65536);
    param->buffer = (unsigned char *) calloc(1,65536);

    /* Memory check */
    if(param->buffer_line == NULL || param->buffer_value == NULL ||
       param->buffer_folder_path == NULL || param->buffer_file_path == NULL ||
       param->buffer_label == NULL || param->buffer_opcode == NULL ||
       param->buffer_operand == NULL || param->buffer_comment == NULL ||
       param->buffer == NULL || param->buffer_file_name == NULL ||
       param->buffer_error == NULL)
    {
        mem_free_param(param);
        return(NULL);
    }

    /* Build dates */
    time(&clock);
    p = localtime(&clock);
    year = (p->tm_year+1900) - 2000;
    hour = (p->tm_hour > 12) ? (p->tm_hour - 12) : p->tm_hour;
    sprintf(param->date_1,"%02d-%s-%02d",p->tm_mday,month[p->tm_mon],year);
    sprintf(param->date_2,"%02d/%02d/%02d",p->tm_mon+1,p->tm_mday,year);
    sprintf(param->date_3,"%02d-%s-%02d  %2d:%02d:%02d %s",p->tm_mday,month[p->tm_mon],year,hour,p->tm_min,p->tm_sec,(p->tm_hour>12)?"PM":"AM");
    sprintf(param->date_4,"%02d/%02d/%02d  %2d:%02d:%02d %s",p->tm_mon+1,p->tm_mday,year,hour,p->tm_min,p->tm_sec,(p->tm_hour>12)?"PM":"AM");

    /** Byte Order : Little Endian / Big Endian **/
    one_word = 1;
    memcpy(&one_word_bin[0],&one_word,sizeof(WORD));
    if(one_word_bin[0] == 0x01)
        param->byte_order = BYTE_ORDER_INTEL;     /* Little Indian */
    else
        param->byte_order = BYTE_ORDER_MOTOROLA;  /* Big Endian */

    /* Returns the structure */
    return(param);
}


/***************************************************************/
/*  mem_free_param() :  Memory release of the Param structure. */
/***************************************************************/
void mem_free_param(struct parameter *param)
{
    if(param)
    {
        if(param->buffer_line)
            free(param->buffer_line);

        if(param->buffer_value)
            free(param->buffer_value);

        if(param->buffer_folder_path)
            free(param->buffer_folder_path);

        if(param->buffer_file_path)
            free(param->buffer_file_path);

        if(param->buffer_file_name)
            free(param->buffer_file_name);

        if(param->buffer_label)
            free(param->buffer_label);

        if(param->buffer_opcode)
            free(param->buffer_opcode);

        if(param->buffer_operand)
            free(param->buffer_operand);

        if(param->buffer_comment)
            free(param->buffer_comment);

        if(param->buffer_error)
            free(param->buffer_error);

        if(param->buffer)
            free(param->buffer);

        free(param);
    }
}


/**************************************************/
/*  mem_free_table() :  Releasing a string table. */
/**************************************************/
void mem_free_table(int nb_item, char **table)
{
    int i;

    if(table == NULL)
        return;

    for(i=0; i<nb_item; i++)
        if(table[i] != NULL)
            free(table[i]);

    free(table);
}

/***********************************************************************/
