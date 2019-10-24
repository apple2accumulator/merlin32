/***********************************************************************/
/*                                                                     */
/*  a65816_Link.c : Assembly / Linkage module of a 65c816 source.      */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

/** Platform dependent code **/
/* MSVC only defines _WIN32 */
#if defined(_WIN32) || defined(WIN32) || defined(WIN64)
/* Windows */
#else
/* Linux + MacOS */
#include <unistd.h>                     /* unlink() */
#endif

#include "Dc_Library.h"

#include "a65816_Link.h"

#include "a65816_File.h"
#include "a65816_Lup.h"
#include "a65816_Cond.h"
#include "a65816_Code.h"
#include "a65816_Data.h"
#include "a65816_OMF.h"

static int Assemble65c816Segment(struct omf_project *,struct omf_segment *,char *);
static int Link65c816Segment(struct omf_project *, struct omf_segment *);
static int IsLinkFile(struct source_file *);
static struct omf_project *BuildSingleSegment(char *);
static struct omf_project *BuildLinkFile(struct source_file *);

/*********************************************************************/
/*  AssembleLink65c816() :  Assembles and Links source files 65c816. */
/*********************************************************************/
int AssembleLink65c816(char *master_file_path, char *macro_folder_path, int verbose_mode, int symbol_mode)
{
    int error = 0, is_link_file = 0, org_offset = 0;
    char file_name[1024] = "";
    char file_error_path[1024+16];
    struct source_file *master_file = NULL;
    struct omf_project *current_omfproject = NULL;
    struct omf_segment *current_omfsegment = NULL;
    struct omf_segment *tmp_omfsegment = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* File name */
    strcpy(file_name,master_file_path);
    for(int i=(int)strlen(master_file_path); i>=0; i--)
        if(master_file_path[i] == '\\' || master_file_path[i] == '/')
        {
            strcpy(file_name,&master_file_path[i+1]);
            break;
        }

    /* File Error_Output.txt */
    snprintf(file_error_path, 1024+16,"%serror_output.txt",param->current_folder_path);
    unlink(file_error_path);

    /* Allocation of an OMF Temporary Segment */
    tmp_omfsegment = mem_alloc_omfsegment();
    if(tmp_omfsegment == NULL)
    {
        strcpy(param->buffer_error,"Impossible to allocate memory to build temporary segment structure");
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }
    my_Memory(MEMORY_SET_OMFSEGMENT,tmp_omfsegment,NULL,NULL);

    /** Load in memory the main file **/
    master_file = LoadOneSourceFile(master_file_path,file_name,0);
    if(master_file == NULL)
    {
        sprintf(param->buffer_error,"Impossible to load Master Source file '%s'",master_file_path);
        my_RaiseError(ERROR_RAISE,param->buffer_error);
    }

    /** Is it a File Link or a Source File **/
    is_link_file = IsLinkFile(master_file);
    if(is_link_file == 0)
    {
        /* Memory release of File Link */
        mem_free_sourcefile(master_file,1);
        mem_free_omfsegment(tmp_omfsegment);
        my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);

        /* Init File */
        my_File(FILE_INIT,NULL);

        /** Creation of 1 omf_header + 1 omf_segment **/
        current_omfproject = BuildSingleSegment(master_file_path);
        if(current_omfproject == NULL)
        {
            sprintf(param->buffer_error,"Impossible to allocate memory to process Master Source file '%s'",master_file_path);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

        /* Declares the current OMF Segment */
        my_Memory(MEMORY_SET_OMFSEGMENT,current_omfproject->first_segment,NULL,NULL);

        /*** Mono Segment: Assembles Source Files + Creation of File Output.txt for 1 Segment ***/
        printf("  + Assemble project files...\n");
        error = Assemble65c816Segment(current_omfproject,current_omfproject->first_segment,macro_folder_path);
        if(error)
        {
            /* Creation of the File Output */
            CreateOutputFile(file_error_path, 0, 0, current_omfproject->first_segment, current_omfproject);

            /* Memory release */
            mem_free_omfproject(current_omfproject);
            my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
            my_File(FILE_FREE,NULL);

            /* Code Error */
            return(error);
        }

        /*** Mono Segment: Link the Source Files for 1 Segment ***/
        printf("  + Link project files...\n");
        error = Link65c816Segment(current_omfproject,current_omfproject->first_segment);
        if(error)
        {
            /* Memory release: OMF Project + OMF Segment */
            mem_free_omfproject(current_omfproject);
            my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
            my_File(FILE_FREE,NULL);

            /* Code Error */
            return(error);
        }

        /** Create OMF File / Binary File **/
        if(current_omfproject->first_segment->is_omf == 1)
        {
            /* OMF Mono Segment */
            printf("    o Build OMF output file...\n");
            BuildOMFFile(param->current_folder_path,current_omfproject);
        }
        else
        {
            /* Fixed address binary */
            printf("    o Build Binary output file...\n");
            BuildObjectFile(param->current_folder_path,current_omfproject->first_segment,current_omfproject);
        }

        /** Dump Code as Output Text File **/
        if(verbose_mode >= 0)
        {
            /* File name _Output.txt */
            sprintf(param->output_file_path,"%s%s_Output.txt",param->current_folder_path,current_omfproject->first_segment->object_name);         /* Mono Segment */

            /* Create the Output File */
            printf("  + Create Output Text file...\n");
            CreateOutputFile(param->output_file_path, verbose_mode, symbol_mode, current_omfproject->first_segment, current_omfproject);
        }

        /* Memory release */
        mem_free_omfproject(current_omfproject);
        my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
        my_File(FILE_FREE,NULL);
    }
    else
    {
        /** Multi Segments: Assemble + Link all Segments **/
        printf("  + Loading Link file...\n");

        /** Loading the File Link: 1 omh_hearder + N omf_segment **/
        current_omfproject = BuildLinkFile(master_file);
        if(current_omfproject == NULL)
        {
            mem_free_sourcefile(master_file,1);
            sprintf(param->buffer_error,"Impossible to load Link file '%s'",master_file_path);
            my_RaiseError(ERROR_RAISE,param->buffer_error);
        }

        /* Memory release */
        mem_free_sourcefile(master_file,1);

        /*** We will chain the assembly of all the Segments ***/
        current_omfsegment=current_omfproject->first_segment;
        for(int i=0; current_omfsegment; current_omfsegment=current_omfsegment->next,i++)
        {
            /* Init File */
            my_File(FILE_INIT,NULL);

            /* Declares the current OMF Segment */
            my_Memory(MEMORY_SET_OMFSEGMENT,current_omfsegment,NULL,NULL);

            /* Gives a number to the Segment (decal in case of ExpressLoad) */
            current_omfsegment->segment_number = i + 1 + ((current_omfproject->express_load == 1 && current_omfproject->nb_segment > 1) ? 1 : 0);

            /* Gives an ORG address for Fixed-Address Single-Binary */
            if(current_omfproject->is_single_binary == 1)
            {
                current_omfsegment->has_org_address = 1;
                current_omfsegment->org_address = current_omfproject->org_address_tab[current_omfsegment->file_number-1] + org_offset;
            }

            /*** Segment #n: Assembles Source Files + Create the Output File.txt for 1 Segment ***/
            printf("  + Assemble project files for Segment #%02X :\n",current_omfsegment->segment_number);
            error = Assemble65c816Segment(current_omfproject,current_omfsegment,macro_folder_path);
            if(error)
            {
                /* Create the Output File */
                CreateOutputFile(file_error_path, 0, 0, current_omfsegment, current_omfproject);

                /* Memory release : OMF Project + OMF Segment */
                mem_free_omfproject(current_omfproject);
                my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
                my_File(FILE_FREE,NULL);

                /* Code Error */
                return(error);
            }
            
            /* Update the next segment's ORG Address (for Fixed-Address SingleBinary), then reset to zero between Files */
            org_offset += current_omfsegment->object_length;
            if(current_omfsegment->next != NULL)
                if(current_omfsegment->file_number != current_omfsegment->next->file_number)
                    org_offset = 0;
        }

        /*** We will generate the OMF (Header + Body) or Fixed Address Segments ***/
        for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
        {
            /*** Segment #n: Link Source Files for 1 Segment ***/
            printf("  + Link project files for Segment #%02X...\n",current_omfsegment->segment_number);
            error = Link65c816Segment(current_omfproject,current_omfsegment);
            if(error)
            {
                /* Memory release : OMF Project + OMF Segment */
                mem_free_omfproject(current_omfproject);
                my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
                my_File(FILE_FREE,NULL);

                /* Code Error */
                return(error);
            }
        }

        /** Create the Segment ~ExpressLoad **/
        if(current_omfproject->is_multi_fixed != 1 && current_omfproject->express_load == 1 && current_omfproject->nb_segment > 1)
        {
            printf("  + Build ExpressLoad into Segment #01...\n");
            error = BuildExpressLoadSegment(current_omfproject);
            if(error)
            {
                /* Memory release : OMF Project + OMF Segment */
                mem_free_omfproject(current_omfproject);
                my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
                my_File(FILE_FREE,NULL);
                return(error);
            }
        }

        /** Create OMF File Multi-Segments / OMF File Mono-Segment / Binary Multi-Fixed files **/
        if(current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 0)
        {
            /** We will create the Binary Files of all Segments **/
            printf("  + Build Binary output files...\n");
            for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
            {
                /* Create the Fixed address binary file */
                BuildObjectFile(param->current_folder_path,current_omfsegment,current_omfproject);
            }
        }
        else if(current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 1)
        {
            /** We will create the Binary File of all concatenated Segments **/
            printf("  + Build Binary output file%s...\n",(current_omfproject->nb_file==1)?"":"s");

            /* Create the Files Fixed address binary files and ensemble */
            for(int i=0; i<current_omfproject->nb_file; i++)
                BuildSingleObjectFile(param->current_folder_path,i,current_omfproject);
        }
        else
        {
            /** Create OMF File Multi-Segments (or Mono Segment but with its Link File) **/
            printf("  + Build OMF output file...\n");
            BuildOMFFile(param->current_folder_path,current_omfproject);
        }

        /** Dump the Output Text File **/
        if(verbose_mode >= 0)
        {
            /* Create the Output Text file */
            printf("  + Create Output Text file%s...\n",(current_omfproject->nb_segment == 1)?"":"s");

            /** We will create the Output.txt Files of all the Segments (except the ExpressLoad) **/
            for(current_omfsegment=current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
            {
                /* We do not dump the ExpressLoad */
                if(current_omfsegment->segment_number == 1 && !my_stricmp(current_omfsegment->segment_name,"~ExpressLoad"))
                    continue;

                /* File name _Output.txt */
                if(current_omfproject->nb_segment == 1)                                                                                                 /* Mono Segment */
                    sprintf(param->output_file_path,"%s%s_Output.txt",param->current_folder_path,current_omfsegment->object_name);
                else if(current_omfproject->nb_segment > 1 && current_omfproject->is_multi_fixed == 0)                                                  /* Multi Segment OMF */
                    sprintf(param->output_file_path,"%s%s_S%02X_%s_Output.txt",param->current_folder_path,current_omfproject->dsk_name_tab[0],current_omfsegment->segment_number,current_omfsegment->object_name);
                else if(current_omfproject->nb_segment > 1 && current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 0)     /* Multi Segment Fixed */
                    sprintf(param->output_file_path,"%s%s_S%02X_Output.txt",param->current_folder_path,(strlen(current_omfsegment->segment_name)==0)?current_omfsegment->object_name:current_omfsegment->segment_name,current_omfsegment->segment_number);
                else if(current_omfproject->nb_segment > 1 && current_omfproject->is_multi_fixed == 1 && current_omfproject->is_single_binary == 1)     /* Multi Segment Fixed Single Binary */
                    sprintf(param->output_file_path,"%s%s_S%02X_%s_Output.txt",param->current_folder_path,current_omfproject->dsk_name_tab[current_omfsegment->file_number-1],current_omfsegment->segment_number,(strlen(current_omfsegment->segment_name)==0)?current_omfsegment->object_name:current_omfsegment->segment_name);

                /* Create the Output File */
                CreateOutputFile(param->output_file_path, verbose_mode, symbol_mode, current_omfsegment, current_omfproject);
            }
        }

        /* Memory release : OMF Project + OMF Segment */
        mem_free_omfproject(current_omfproject);
        my_Memory(MEMORY_SET_OMFSEGMENT,NULL,NULL,NULL);
        my_File(FILE_FREE,NULL);
    }

    /* Code Error */
    return(error);
}


/*****************************************************************************/
/*  Assemble65c816Segment() :  Assembles 65c816 Source Files from a Segment. */
/*****************************************************************************/
static int Assemble65c816Segment(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment, char *macro_folder_path)
{
    int error = 0;
    struct source_file *first_file = NULL;
    struct source_line *current_line = NULL;
    struct relocate_address *current_address = NULL;
    struct relocate_address *next_address = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* Init */
    my_Memory(MEMORY_FREE_EQUIVALENCE,NULL,NULL,current_omfsegment);

    /* Initializing the counter of unique labels */
    GetUNID("INIT=1");

    /* Retrieves the Source Files Directory */
    GetFolderFromPath(current_omfsegment->master_file_path,param->source_folder_path);

    /* Creating quick lookup tables */
    BuildReferenceTable(current_omfsegment);

    /** Loading all Source Files / Identifies lines in Comment + Empty **/
    printf("    o Loading Sources files...\n");
    LoadAllSourceFile(current_omfsegment->master_file_path,macro_folder_path,current_omfsegment);

    /** Loading Macro Files **/
    printf("    o Loading Macro files...\n");
    LoadSourceMacroFile(macro_folder_path,current_omfsegment);

    /** Search for additional Macro defined directly in the Source **/
    GetMacroFromSource(current_omfsegment);

    /** Sort all Macro **/
    my_Memory(MEMORY_SORT_MACRO,NULL,NULL,current_omfsegment);

    /** Finding duplicate Macro **/
    printf("    o Check for duplicated Macros...\n");
    CheckForDuplicatedMacro(current_omfsegment);

    /** Determine the type of lines (Code, Macro, Directive, Equivalence ...) **/
    printf("    o Decoding lines types...\n");
    my_Memory(MEMORY_GET_FILE,&first_file,NULL,current_omfsegment);
    error = DecodeLineType(first_file->first_line,NULL,current_omfsegment,current_omfproject);
    if(error)
        return(1);

    /** Replaces labels :local / ]variable by unid_ in Code and Macros **/
    printf("    o Process local/variable Labels...\n");
    ProcessAllLocalLabel(current_omfsegment);
    ProcessAllVariableLabel(current_omfsegment);

    /** Replace * with Labels in Code and Macros **/
    printf("    o Process Asterisk lines...\n");
    error = ProcessAllAsteriskLine(current_omfsegment);
    if(error)
        return(1);

    /** Creation of the External table **/
    printf("    o Build External table...\n");
    BuildExternalTable(current_omfsegment);

    /** Creation of the equivalence table **/
    printf("    o Build Equivalence table...\n");
    BuildEquivalenceTable(current_omfsegment);

    /** Creation of the table of variables ]LP **/
    printf("    o Build Variable table...\n");
    BuildVariableTable(current_omfsegment);

    /** Replaces Equivalences **/
    printf("    o Process Equivalence values...\n");
    ProcessEquivalence(current_omfsegment);

    /** Replace Macro with their code **/
    printf("    o Replace Macros with Code...\n");
    error = ReplaceMacroWithContent(current_omfsegment,current_omfproject);
    if(error)
        return(1);

    /** Replaces LUP with code sequence (this comes after Macro because LUP parameter can be one of Macro's call parameters) **/
    printf("    o Replace Lup with code...\n");
    ReplaceLupWithCode(current_omfsegment);

    /** We redid Equivalences, Variables and Replacements in the code brought by Macro and Loop **/
    BuildEquivalenceTable(current_omfsegment);
    BuildVariableTable(current_omfsegment);
    ProcessEquivalence(current_omfsegment);

    /** Process MX directives **/
    printf("    o Process MX directives...\n");
    ProcessMXDirective(current_omfsegment);

    /** Process Conditional directives **/
    printf("    o Process Conditional directives...\n");
    ProcessConditionalDirective(current_omfsegment);

    /** Build Label table **/
    printf("    o Build Label table...\n");
    BuildLabelTable(current_omfsegment);

    /** Check for duplicated Labels **/
    printf("    o Check for duplicated Labels...\n");
    error = CheckForDuplicatedLabel(current_omfsegment);
    if(error)
        return(1);

    /** Detect unknown Source lines **/
    printf("    o Check for unknown Source lines...\n");
    error = CheckForUnknownLine(current_omfsegment);
    if(error)
        return(1);

    /** Check for Dum lines **/
    printf("    o Check for Dum lines...\n");
    error = CheckForDumLine(current_omfsegment);
    if(error)
        return(1);

    /**** The automatic detection of the Direct Page forces us to iterate several times ****/
    printf("    + Building Code Lines:\n");
    int modified = 1;
    int has_error = 0;
    int pass = 0;
    while(modified == 1 || has_error == 1)
    {
        printf("      Pass %i...\n", ++pass);

        /* Error while generating the code */
        strcpy(param->buffer_latest_error,"");
        has_error = 0;

        /*** Generation of the Opcode + Size Calculation code for each line of Code ***/
        printf("      o Compute Operand Code size...\n");
        BuildAllCodeLineSize(current_omfsegment);

        /*** Calculation of the size for each line of Data ***/
        printf("      o Compute Operand Data size...\n");
        BuildAllDataLineSize(current_omfsegment);

        /*** Calculate the addresses of each line + Analyzes the ORG / OBJ / REL / DUM ***/
        printf("      o Compute Line address...\n");
        ComputeLineAddress(current_omfsegment,current_omfproject);

        /*** Generate Binary for Code Lines (LINE_CODE) ***/
        printf("      o Build Code Line...\n");
        BuildAllCodeLine(&has_error,current_omfsegment,current_omfproject);

        /** Compact Code for Direct Page (unless the address is relocatable OMF) **/
        printf("      o Compact Code for Direct Page Lines...\n");
        modified = CompactDirectPageCode(current_omfsegment);

        /* Compact code gives nothing, raise an error condition */
        if(has_error == 1 && modified == 0)
            my_RaiseError(ERROR_RAISE,param->buffer_latest_error);

        /** We must redo everything => we will delete all addresses & relocate **/
        if(modified == 1 || has_error == 1)
        {
            for(current_address=current_omfsegment->first_address; current_address; )
            {
                next_address = current_address->next;
                free(current_address);
                current_address = next_address;
            }
            current_omfsegment->first_address = NULL;
            current_omfsegment->last_address = NULL;
            current_omfsegment->nb_address = 0;
        }
    }
    printf("      Done\n");

    /** We will evaluate the ERR lines **/
    printf("    o Check for Err lines...\n");
    error = CheckForErrLine(current_omfsegment);
    if(error)
        return(1);

    /** We will check the direct page addresses **/
    printf("    o Check for Direct Page Lines...\n");
    error = CheckForDirectPageLine(current_omfsegment);
    if(error)
        return(1);

    /*** Binary Generation for Data Lines (LINE_DATA) ***/
    printf("    o Build Data Line...\n");
    BuildAllDataLine(current_omfsegment);

    /** Create Object Code (LINE_CODE + LINE_DATA) **/
    printf("    o Build Object Code...\n");
    BuildObjectCode(current_omfsegment);

    /** Transform directive lines with Label into Empty line **/
    ProcessDirectiveWithLabelLine(current_omfsegment);

    /** If we do not have a Segment Name or Load Name, we use the Master Source directives **/
    if(current_omfsegment->segment_name == NULL)
        current_omfsegment->segment_name = strdup(current_omfsegment->object_name);
    if(current_omfsegment->load_name == NULL)
        current_omfsegment->load_name = strdup(current_omfsegment->object_name);
    if(current_omfsegment->segment_name == NULL || current_omfsegment->load_name == NULL)
    {
        printf("      => Error, Can't allocate memory...\n");
        return(1);
    }

    /** If in multi-segment Fixed, have to go up the org address of the segment **/
    if(current_omfproject->is_multi_fixed == 1)
    {
        /* Look for the first line with something in it */
        for(current_line=current_omfsegment->first_file->first_line; current_line; current_line=current_line->next)
        {
            if(current_line->nb_byte > 0)
            {
                current_omfsegment->org = current_line->address;
                break;
            }
        }
    }

    /* OK */
    return(0);
}


/**********************************************************************/
/*  Link65c816Segment() :  Link the 65c816 Source Files of a Segment. */
/**********************************************************************/
static int Link65c816Segment(struct omf_project *current_omfproject, struct omf_segment *current_omfsegment)
{
    int nb_external = 0;
    struct label *external_label = NULL;
    struct relocate_address *current_address = NULL;
    struct omf_segment *external_omfsegment = NULL;
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /** We will check that all External used in this segment are solved **/
    my_Memory(MEMORY_GET_EXTERNAL_NB,&nb_external,NULL,current_omfsegment);
    if(nb_external > 0)
    {
        /** Check all relocatable addresses **/
        for(current_address = current_omfsegment->first_address; current_address; current_address=current_address->next)
            if(current_address->external != NULL)
                if(current_address->external->external_segment == NULL)
                {
                    /** We go through all the Segments to find the Global label associated with this External **/
                    for(external_omfsegment = current_omfproject->first_segment; external_omfsegment; external_omfsegment = external_omfsegment->next)
                    {
                        /* Search a Global Label */
                        my_Memory(MEMORY_SEARCH_LABEL,current_address->external->name,&external_label,external_omfsegment);
                        if(external_label != NULL)
                            if(external_label->is_global == 1)
                            {
                                /* If we have already found one, it means that there is at least 2 Global Label with the same name => Error */
                                if(current_address->external->external_segment != NULL)
                                {
                                    printf("     => Error : We have found 2 External Labels with the same name '%s' (File '%s', Line %d).\n",current_address->external->name,external_label->line->file->file_path,external_label->line->file_line_number);
                                    return(1);
                                }

                                /* We keep the one */
                                current_address->external->external_segment = external_omfsegment;
                                current_address->external->external_label = external_label;
                            }
                    }

                    /** We could not find the Segment containing this External Label :-( **/
                    if(current_address->external->external_segment == NULL)
                    {
                        printf("     => Error : Can't find External Label named '%s' (File '%s', Line %d).\n",current_address->external->name,current_address->external->source_line->file->file_path,current_address->external->source_line->file_line_number);
                        return(1);
                    }
                }
    }

    /** Size of the Body of the Segment (we take wider for the OMF) **/
    if(current_omfproject->is_multi_fixed == 1)
        current_omfsegment->segment_body_length = current_omfsegment->object_length;
    else
        current_omfsegment->segment_body_length = 1024 + current_omfsegment->object_length + CRECORD_SIZE*current_omfsegment->nb_address + END_SIZE;

    /** Allocate memory Segment Body **/
    current_omfsegment->segment_body_file = (unsigned char *) calloc(current_omfsegment->segment_body_length,sizeof(unsigned char));
    if(current_omfsegment->segment_body_file == NULL)
    {
        printf("     => Error : Can't allocate memory to build Body File buffer.\n");
        return(1);
    }

    /** Create the Segment Body **/
    if(current_omfproject->is_multi_fixed == 1)
    {
        /* Relocate fixed external addresses */
        RelocateExternalFixedAddress(current_omfsegment);

        /* Conserve le code objet */
        memcpy(current_omfsegment->segment_body_file,current_omfsegment->object_code,current_omfsegment->object_length);
    }
    else
        current_omfsegment->body_length = BuildOMFBody(current_omfsegment);

    /* Create the Segment Header */
    if(current_omfproject->is_multi_fixed == 1)
        current_omfsegment->header_length = 0;
    else
        current_omfsegment->header_length = BuildOMFHeader(current_omfsegment);

    /* OK */
    return(0);
}


/***************************************************************/
/*  IsLinkFile() :  Determine if a Source file is a File Link. */
/***************************************************************/
static int IsLinkFile(struct source_file *master_file)
{
    int found = 0;
    struct source_line *current_line = NULL;
    char *opcode_link[] = {"DSK","TYP","AUX","XPL","ASM","DS","KND","ALI","LNA","SNA","ORG","BSZ",NULL};      /* Opcode exclusifs au File Link */

    /* Valid File? */
    if(master_file->first_line == NULL)
        return(0);

    /** Pass all lines in to review **/
    for(current_line = master_file->first_line; current_line; current_line = current_line->next)
    {
        /* Comment / Empty */
        if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY)
            continue;

        /* Recognize Link Opcode */
        for(int i=0; opcode_link[i]!=NULL; i++)
        {
            if(!my_stricmp(current_line->opcode_txt,opcode_link[i]))
            {
                found = 1;
                break;
            }
        }
        if(found)
            break;
    }

    return(found);
}


/************************************************************/
/* BuildSingleSegment() : Creation of a single-segment OMF. */
/************************************************************/
static struct omf_project *BuildSingleSegment(char *master_file_path)
{
    /* OMF Header */
    struct omf_project *current_omfproject = (struct omf_project *) calloc(1,sizeof(struct omf_project));
    if(current_omfproject == NULL)
        return(NULL);

    /** OMF Segment **/
    current_omfproject->nb_segment = 1;
    current_omfproject->first_segment = mem_alloc_omfsegment();
    if(current_omfproject->first_segment == NULL)
    {
        mem_free_omfproject(current_omfproject);
        return(NULL);
    }
    current_omfproject->first_segment->master_file_path = strdup(master_file_path);
    if(current_omfproject->first_segment->master_file_path == NULL)
    {
        mem_free_omfproject(current_omfproject);
        return(NULL);
    }

    /* In mono-segment, there is no ExpressLoad, the first segment has the number 1 */
    current_omfproject->first_segment->segment_number = 1;

    /* Return the structure */
    return(current_omfproject);
}


/**************************************************/
/*  BuildLinkFile() :  Retrieving File Link Data. */
/**************************************************/
static struct omf_project *BuildLinkFile(struct source_file *link_file)
{
    struct omf_project *current_omfproject = NULL;
    struct omf_segment *new_omfsegment = NULL;
    struct omf_segment *current_omfsegment = NULL;
    struct source_line *current_line = NULL;
    char **dsk_name_tab = NULL;
    DWORD *org_address_tab = NULL;
    DWORD *file_size_tab = NULL;
    char file_path[1024];
    struct parameter *param;
    my_Memory(MEMORY_GET_PARAM, &param, NULL, NULL);

    /* OMF Header allocation */
    current_omfproject = (struct omf_project *) calloc(1,sizeof(struct omf_project));
    if(current_omfproject == NULL)
    {
        printf("     => Error, Impossible to allocate memory to process Link file.\n");
        return(NULL);
    }

    /* Default values */
    current_omfproject->type = 0xB3;          /* GS/OS Application */
    current_omfproject->aux_type = 0x0000;

    /*** Pass all the lines of the Link File in review ***/
    for(current_line = link_file->first_line; current_line; current_line = current_line->next)
    {
        /* Comment / Empty */
        if(current_line->type == LINE_COMMENT || current_line->type == LINE_EMPTY)
            continue;

        /** New Header **/
        /** TYP: File type **/
        if(!my_stricmp(current_line->opcode_txt,"TYP"))
        {
            /* Decode the value */
            current_omfproject->type = GetByteValue(current_line->operand_txt);
            continue;
        }
        
        /** Aux: AuxType of the File **/
        if(!my_stricmp(current_line->opcode_txt,"AUX"))
        {
            /* Decode the value */
            current_omfproject->aux_type = GetWordValue(current_line->operand_txt);
            continue;
        }
        
        /** XPL : Express Load **/
        if(!my_stricmp(current_line->opcode_txt,"XPL"))
        {
            current_omfproject->express_load = 1;
            continue;
        }

        /** DSK: New Project / File **/
        if(!my_stricmp(current_line->opcode_txt,"DSK"))
        {
            /*** ORG File + Table Name Table ***/
            /* Allocation */
            dsk_name_tab = (char **) calloc(current_omfproject->nb_file+1,sizeof(char *));
            if(dsk_name_tab == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }
            org_address_tab = (DWORD *) calloc(current_omfproject->nb_file+1,sizeof(DWORD));
            if(org_address_tab == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                free(dsk_name_tab);
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }
            file_size_tab = (DWORD *) calloc(current_omfproject->nb_file+1,sizeof(DWORD));
            if(file_size_tab == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                free(dsk_name_tab);
                free(org_address_tab);
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }
            /* Fill out structure */
            for(int i=0; i<current_omfproject->nb_file; i++)
            {
                dsk_name_tab[i] = current_omfproject->dsk_name_tab[i];
                org_address_tab[i] = current_omfproject->org_address_tab[i];
            }
            /* Replacement */
            if(current_omfproject->dsk_name_tab != NULL)
                free(current_omfproject->dsk_name_tab);
            current_omfproject->dsk_name_tab = dsk_name_tab;
            if(current_omfproject->org_address_tab != NULL)
                free(current_omfproject->org_address_tab);
            current_omfproject->org_address_tab = org_address_tab;
            if(current_omfproject->file_size_tab != NULL)
                free(current_omfproject->file_size_tab);
            current_omfproject->file_size_tab = file_size_tab;

            /* One more File */
            current_omfproject->nb_file++;

            /* New values */
            current_omfproject->org_address_tab[current_omfproject->nb_file-1] = 0xFFFFFFFF;
            current_omfproject->dsk_name_tab[current_omfproject->nb_file-1] = strdup(current_line->operand_txt);
            if(current_omfproject->dsk_name_tab[current_omfproject->nb_file-1] == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }

            /* Next line */
            continue;
        }

        /** ORG: Assembly Address of the File Fixed Address **/
        if(!my_stricmp(current_line->opcode_txt,"ORG") && current_omfproject->nb_file > 0)
        {
            /* Decode the value */
            current_omfproject->org_address_tab[current_omfproject->nb_file-1] = GetDwordValue(current_line->operand_txt);

            /* Verify the value */
            if(current_omfproject->org_address_tab[current_omfproject->nb_file-1] > 0xFFFFFF)
            {
                printf("     => Error, Invalid ORG value : %X.\n",(int)current_omfproject->org_address_tab[current_omfproject->nb_file-1]);
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }
            continue;
        }

        /** ASM: New Segment **/
        if(!my_stricmp(current_line->opcode_txt,"ASM"))
        {
            /* Segment memory allocation */
            new_omfsegment = mem_alloc_omfsegment();
            if(new_omfsegment == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }

            /* Source file name */
            BuildAbsolutePath(current_line->operand_txt,param->current_folder_path,file_path);
            new_omfsegment->master_file_path = strdup(file_path);
            if(new_omfsegment->master_file_path == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }

            /* File number */
            new_omfsegment->file_number = current_omfproject->nb_file;

            /* Attachment of the Segment to the list */
            if(current_omfproject->first_segment == NULL)
                current_omfproject->first_segment = new_omfsegment;
            else
                current_omfproject->last_segment->next = new_omfsegment;
            current_omfproject->last_segment = new_omfsegment;
            current_omfproject->nb_segment++;

            /* Next line */
            continue;
        }

        /** DS: Number of 0 to add at the end of the segment **/
        if(!my_stricmp(current_line->opcode_txt,"DS") && current_omfproject->last_segment != NULL)
        {
            current_omfproject->last_segment->ds_end = atoi(current_line->operand_txt);
            continue;
        }

        /** KND: Type and Segment Attributes **/
        if(!my_stricmp(current_line->opcode_txt,"KND") && current_omfproject->last_segment != NULL)
        {
            /* Decode the value */
            current_omfproject->last_segment->type_attributes = GetWordValue(current_line->operand_txt);

            /* Verify the type value */
            if(!(((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0000) ||
                 ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0001) ||
                 ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0002) ||
                 ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0004) ||
                 ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0008) ||
                 ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0010) ||
                 ((current_omfproject->last_segment->type_attributes & 0x00FF) == 0x0012)))
            {
                printf("     => Error, Invalid Link file : Unknown Type value for Directive KND (%04X) at line %d.\n",current_omfproject->last_segment->type_attributes,current_line->file_line_number);
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }

            /* Next value */
            continue;
        }

        /** ALI : Alignement **/
        if(!my_stricmp(current_line->opcode_txt,"ALI") && current_omfproject->last_segment != NULL)
        {
            if(!my_stricmp(current_line->operand_txt,"BANK"))
                current_omfproject->last_segment->alignment = ALIGN_BANK;
            else if(!my_stricmp(current_line->operand_txt,"PAGE"))
                current_omfproject->last_segment->alignment = ALIGN_PAGE;
            else if(!my_stricmp(current_line->operand_txt,"NONE"))
                current_omfproject->last_segment->alignment = ALIGN_NONE;
            else
            {
                printf("     => Error, Invalid Link file : Unknown value for Directive ALI (%s) at line %d.\n",current_line->operand_txt,current_line->file_line_number);
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }

            /* Next line */
            continue;
        }

        /** LNA : Load Name **/
        if(!my_stricmp(current_line->opcode_txt,"LNA") && current_omfproject->last_segment != NULL)
        {
            if(current_omfproject->last_segment->load_name == NULL)
            {
                current_omfproject->last_segment->load_name = strdup(current_line->operand_txt);
                if(current_omfproject->last_segment->load_name == NULL)
                {
                    printf("     => Error, Impossible to allocate memory to process Link file.\n");
                    mem_free_omfproject(current_omfproject);
                    return(NULL);
                }

                /* Remove " or ' */
                CleanUpName(current_omfproject->last_segment->load_name);
            }
            else
            {
                /* We already have a LNA */
                if(!my_stricmp(current_omfproject->last_segment->load_name,current_line->operand_txt))
                    continue;   /* Same value, nothing is said */
                else
                {
                    printf("     => Error, Invalid Link file : Two LNA directives found for Segment '%s'.\n",current_omfproject->last_segment->master_file_path);
                    mem_free_omfproject(current_omfproject);
                    return(NULL);
                }
            }

            /* Next line */
            continue;
        }

        /** SNA : Segment Name **/
        if(!my_stricmp(current_line->opcode_txt,"SNA") && current_omfproject->last_segment != NULL)
        {
            if(current_omfproject->last_segment->segment_name == NULL)
            {
                current_omfproject->last_segment->segment_name = strdup(current_line->operand_txt);
                if(current_omfproject->last_segment->segment_name == NULL)
                {
                    printf("     => Error, Impossible to allocate memory to process Link file.\n");
                    mem_free_omfproject(current_omfproject);
                    return(NULL);
                }

                /* Remove " or ' */
                CleanUpName(current_omfproject->last_segment->segment_name);
            }
            else
            {
                /* We already have a SNA */
                if(!my_stricmp(current_omfproject->last_segment->segment_name,current_line->operand_txt))
                    continue;   /* Same value, nothing is said */
                else
                {
                    printf("     => Error, Invalid Link file : Two SNA directives found for Segment '%s'.\n",current_omfproject->last_segment->master_file_path);
                    mem_free_omfproject(current_omfproject);
                    return(NULL);
                }
            }

            /* Next line */
            continue;
        }

        /** The command is recognized, but we do not have a Segment as support **/
        if(current_omfproject->last_segment == NULL &&
           (!my_stricmp(current_line->opcode_txt,"DS") || !my_stricmp(current_line->opcode_txt,"KND") || !my_stricmp(current_line->opcode_txt,"ALI") ||
            !my_stricmp(current_line->opcode_txt,"LNA") || !my_stricmp(current_line->opcode_txt,"SNA")))
        {
            printf("     => Error, The directive %s is valid but a previous directive ASM is missing in the Link file.\n",current_line->opcode_txt);
            mem_free_omfproject(current_omfproject);
            return(NULL);
        }

        /** Unknown directive or order **/
        printf("     => Error, Invalid Link file : Unknown (or misordered) directive found (%s) at line %d.\n",current_line->opcode_txt,current_line->file_line_number);
        mem_free_omfproject(current_omfproject);
        return(NULL);
    }

    /** We validate the values of Header, Projects / Files and Segments **/
    /*Have a non-OMF Multi-Segment */
    if(current_omfproject->type == 0x0006)     /* BIN = Fixed Address : MultiBinary or SingleBinary */
    {
        if(current_omfproject->dsk_name_tab != NULL)
        {
            /** Single Binary : All Segments will be pasted behind each other, in 1 or more Files **/
            current_omfproject->is_omf = 0;
            current_omfproject->is_multi_fixed = 1;
            current_omfproject->is_single_binary = 1;
            current_omfproject->express_load = 0;

            /* Check the names */
            for(int i=0; i<current_omfproject->nb_file; i++)
            {
                if(IsProdosName(current_omfproject->dsk_name_tab[i]) == 0)
                {
                    printf("     => Error, Bad Link file name '%s' : Invalid Prodos file name (15 chars max, letters/numbers/. allowed).\n",current_omfproject->dsk_name_tab[i]);
                    mem_free_omfproject(current_omfproject);
                    return(NULL);
                }
            }
            
            /* Verify the Org Address */
            for(int i=0; i<current_omfproject->nb_file; i++)
            {
                if(current_omfproject->org_address_tab[i] == 0xFFFFFFFF)
                {
                    printf("     => Error, Invalid Link file : Directive ORG is missing for File #%d.\n",i+1);
                    mem_free_omfproject(current_omfproject);
                    return(NULL);
                }
            }
        }
        else if(current_omfproject->dsk_name_tab == NULL)           /* No DSK or ORG in the Header */
        {
            /** Multi Binary: We will generate as many File Binary as there are Segments **/
            current_omfproject->is_omf = 0;
            current_omfproject->is_multi_fixed = 1;
            current_omfproject->is_single_binary = 0;
            current_omfproject->express_load = 0;
        }
    }
    else
    {
        /** OMF : We will generate a Relogeable Program OMF v2.1 **/
        current_omfproject->is_omf = 1;
        current_omfproject->is_multi_fixed = 0;
        current_omfproject->is_single_binary = 0;

        /** We eliminate doubtful cases **/
        if(current_omfproject->nb_file > 1)
        {
            printf("     => Error, Invalid Link file : Too many DSK directives (only one for an OMF File).\n");
            mem_free_omfproject(current_omfproject);
            return(NULL);
        }
        if(current_omfproject->dsk_name_tab == NULL)
        {
            printf("     => Error, Invalid Link file : Directive DSK is missing (Target OMF File name).\n");
            mem_free_omfproject(current_omfproject);
            return(NULL);
        }
        if(current_omfproject->org_address_tab[0] != 0xFFFFFFFF)
        {
            printf("     => Error, Invalid Link file : Directive ORG is not allowed (Target is a Relocatable OMF File).\n");
            mem_free_omfproject(current_omfproject);
            return(NULL);
        }
        
        /* Verify the file name */
        if(IsProdosName(current_omfproject->dsk_name_tab[0]) == 0)
        {
            printf("     => Error, Bad Link file name '%s' : Invalid Prodos file name (15 chars max, letters/numbers/. allowed).\n",current_omfproject->dsk_name_tab[0]);
            mem_free_omfproject(current_omfproject);
            return(NULL);
        }
    }

    /** Segments **/
    if(current_omfproject->nb_segment == 0)
    {
        printf("     => Error, Invalid Link file : No Segment defined (use ASM directive).\n");
        mem_free_omfproject(current_omfproject);
        return(NULL);
    }

    /** Check all Segments: we put an empty string for missing parameters **/
    for(current_omfsegment = current_omfproject->first_segment; current_omfsegment; current_omfsegment=current_omfsegment->next)
    {
        /* Load Name */
        if(current_omfsegment->load_name == NULL)
        {
            current_omfsegment->load_name = strdup("");
            if(current_omfsegment->load_name == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }
        }

        /* Segment Name */
        if(current_omfsegment->segment_name == NULL)
        {
            current_omfsegment->segment_name = strdup("");
            if(current_omfsegment->segment_name == NULL)
            {
                printf("     => Error, Impossible to allocate memory to process Link file.\n");
                mem_free_omfproject(current_omfproject);
                return(NULL);
            }
        }
    }

    /* OK */
    return(current_omfproject);
}

/***********************************************************************/
