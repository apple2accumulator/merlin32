/***********************************************************************/
/*                                                                     */
/*  a65816_Line.h : Header for line management.                        */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#define LINE_UNKNOWN     0       /* Line of which we do not yet know the type / that we have not managed to decode */
#define LINE_COMMENT     1       /* Line containing only a comment */
#define LINE_DIRECTIVE   2       /* Assembly directive (PUT, USE, IF, XC...) */
#define LINE_EQUIVALENCE 3       /* Line defining a constant (EQU, =) */
#define LINE_VARIABLE    4       /* Line defining a variable (for loops...) */
#define LINE_CODE        5       /* Line with at least one Opcode */
#define LINE_DATA        6       /* Line storing variables (HEX, DA, ASC, STR...)*/
#define LINE_MACRO       7       /* Line calling a macro */
#define LINE_EMPTY       8       /* Line with only a Label */
#define LINE_GLOBAL      9       /* Global Label Defining an Entry Point for the InterSeg */
#define LINE_EXTERNAL   10       /* External label defining an address of another segment */

#define LINE_MACRO_DEF   1

#define AM_UNKOWN                                0  /* Not recognized */
#define AM_IMPLICIT                              1  /* A          Implicit                             */
#define AM_ABSOLUTE                              2  /* addr2      Absolute                             */
#define AM_ABSOLUTE_INDEXED_X_INDIRECT           3  /* (addr2,X)  Absolute Indexed,X Indirect          */
#define AM_ABSOLUTE_INDEXED_X                    4  /* addr2,X    Absolute Indexed,X                   */
#define AM_ABSOLUTE_INDEXED_Y                    5  /* addr2,Y    Absolute Indexed,Y                   */
#define AM_ABSOLUTE_INDIRECT                     6  /* (addr2)    Absolute Indirect                    */
#define AM_ABSOLUTE_INDIRECT_LONG                7  /* [addr2]    Absolute Indirect Long               */
#define AM_ABSOLUTE_LONG                         8  /* addr3      Absolute Long                        */
#define AM_ABSOLUTE_LONG_INDEXED_X               9  /* addr3,X    Absolute Long Indexed,X              */
#define AM_DIRECT_PAGE                          10  /* dp         Direct Page                          */
#define AM_DIRECT_PAGE_INDEXED_X                11  /* dp,X       Direct Page Indexed,X                */
#define AM_DIRECT_PAGE_INDEXED_Y                12  /* dp,Y       Direct Page Indexed,Y                */
#define AM_DIRECT_PAGE_INDIRECT                 13  /* (dp)       Direct Page Indirect                 */
#define AM_DIRECT_PAGE_INDIRECT_LONG            14  /* [dp]       Direct Page Indirect Long            */
#define AM_DIRECT_PAGE_INDEXED_X_INDIRECT       15  /* (dp,X)     Direct Page Indexed,X Indirect       */
#define AM_DIRECT_PAGE_INDIRECT_INDEXED_Y       16  /* (dp),Y     Direct Page Indirect Indexed,Y       */
#define AM_DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y  17  /* [dp],Y     Direct Page Indirect Long Indexed,Y  */
#define AM_IMMEDIATE_8                          18  /* #const 8   Immediate 8 bit                      */
#define AM_IMMEDIATE_16                         19  /* #const 16  Immediate 16 bit                      */
#define AM_PC_RELATIVE                          20  /* relative1  Program Counter Relative 8 bit (BRA, BEQ, BPL...) */
#define AM_PC_RELATIVE_LONG                     21  /* relative2  Program Counter Relative Long (BRL and PER)  */
#define AM_STACK_RELATIVE_INDIRECT_INDEXED_Y    22  /* (sr,S),Y   Stack Relative Indirect Indexed,Y    */
#define AM_STACK_PC_RELATIVE_LONG               23  /* label      Stack PC Relative Long               */
#define AM_STACK_RELATIVE                       24  /* sr,S       Stack Relative                       */
#define AM_BLOCK_MOVE                           25  /* Src,Dst    Block Move (MVN, MVP)                */

#define FORMAT_DECIMAL    1
#define FORMAT_HEXA       2
#define FORMAT_BINARY     3
#define FORMAT_ASCII      4
#define FORMAT_LABEL      5
#define FORMAT_VARIABLE   6
#define FORMAT_EXTERNAL   7      /* External Label to Module = Long Address */

struct source_line
{
    char *line_data;               /* Line as it was in the Source file */
    int line_number;               /* Number of the global line */

    int file_line_number;          /* Number of the line in the File */
    struct source_file *file;

    int type;                      /* Type of line : LINE_COMMENT, LINE_DIRECTIVE, LINE_CODE, LINE_MACRO */
    int type_aux;                  /* Auxiliary Type : LINE_MACRO_DEF */
    int is_in_source;              /* This line is in the original source code (and not in a Macro that has been developed) */
    int is_valid;                  /* This line will be kept in the final code */
    int no_direct_page;            /* You do not have to generate any DP code : LDA\ $0000 */
    int use_direct_page;           /* We must generate the code for DP : LDA $00 */

    int was_local_label;           /* This label: local has been replaced by a unique name */

    int is_inside_macro;           /* This Code Line is part of a declaration of a Macro present in the Source file */

    int is_dum;                    /* This line is stored between 2 DUM - DEND */
    struct source_line *dum_line;  /* Line declaring the DUM */



    char *label_txt;               /* line cut into 4 parts : Label  Opcode Operand   ; Comment */
    char *opcode_txt;
    char *operand_txt;
    char *comment_txt;

    int cond_level;                /* Condition level 0->N IF [ELSE] FIN | DO [ELSE] FIN */
    int do_level;                  /* Level of IF ELSE FIN condition for Preliminary Assessment */

    char m[2];                     /* M : 0/1/? = Acc size (0=16bit, 1=8bit) */
    char x[2];                     /* X : 0/1/? = X,Y size (0=16bit, 1=8bit) */

    struct variable *variable;     /* If this line is a variable, we point to its value */

    struct macro *macro;           /* If this line is a call to a Macro, point to it */

    int bank;                      /* Bank memory of the line */
    int address;                   /* Address of the line */
    int is_fix_address;            /* This line has a fixed address (case of Lines located in [ORG $ Addr ORG] for an OMF or a Single Binary */
    int global_bank;
    int global_address;            /* Address of the line if the ORG $ Addr was not useful (to relocate addresses between a [ORG $ Addr ORG] */

    int nb_byte;                   /* Size of the line: 1 + Operand Size */
    unsigned char opcode_byte;     /* Opcode */
    int address_mode;              /* Addressing mode AM_ */
    int address_is_rel;            /* The address pointed by the opcode is an address of a relative zone: if <0x100, it will not make it switch to DP */
    unsigned char operand_byte[4]; /* Operand */
    unsigned char *data;           /* Data */

    DWORD operand_value;           /* Results of the evaluation of the operand, before the truncation due to the number of byte of the operand */
    DWORD operand_address_long;    /* If the address points to an address, here is the long version of the address: Bank / HighLow */
    struct relocate_address *external_address;  /* When using an External address, we keep a pointer */

    char reloc[16];                /* Relocate Information : Nb Byte >> Bit Shift */

    struct source_line *next;
};

struct label
{
    char *name;
    struct source_line *line;      /* The is_dum of the line allows to know if this label comes from a section DUM (= Fixed address = no relogeable) */

    int is_global;                 /* This label is an entry point into the Segment */

    struct label *next;
};

struct external
{
    /* Internal Reference */
    char *name;
    struct source_line *source_line;

    /* External reference */
    struct omf_segment *external_segment;
    struct label *external_label;

    struct external *next;
};

struct global
{
    char *name;
    struct source_line *source_line;    /* Source line containing ENT Label1, Label2... */

    struct global *next;
};

struct equivalence
{
    char *name;
    char *value;

    struct source_line *source_line;    /* If the equivalence comes from source, we point the line (it can also come from the Macro Files) */

    struct equivalence *next;
};

struct variable
{
    char *name;

    int is_pound;         /* # */
    int is_dollar;        /* $ */
    int is_pound_dollar;  /* #$ */

    int64_t value;

    struct variable *next;
};

struct relocate_address
{
    BYTE ByteCnt;               /* Number of Bytes to be relocated (1,2,3 or 4) */
    BYTE BitShiftCnt;           /* Operations >> or << */
    WORD OffsetPatch;           /* Offset of the first Byte to be Patched */
    WORD OffsetReference;       /* Address */

    struct external *external;  /* External Label is used (NULL if internal) */
    unsigned char *object_line; /* Address of the area to be patched in the line (for external labels) */

    int processed;              /* Already processed */

    struct relocate_address *next;
};

int DecodeLineType(struct source_line *,struct macro *,struct omf_segment *,struct omf_project *);
void BuildReferenceTable(struct omf_segment *);
int BuildLabelTable(struct omf_segment *);
int BuildEquivalenceTable(struct omf_segment *);
int BuildExternalTable(struct omf_segment *);
int BuildVariableTable(struct omf_segment *);
int ProcessAllLocalLabel(struct omf_segment *);
int ProcessAllVariableLabel(struct omf_segment *);
int ProcessAllAsteriskLine(struct omf_segment *);
int ProcessEquivalence(struct omf_segment *);
int ProcessLineEquivalence(struct source_line *,struct omf_segment *);
int CheckForDuplicatedLabel(struct omf_segment *);
int CheckForUnknownLine(struct omf_segment *);
int CheckForDumLine(struct omf_segment *);
int CheckForErrLine(struct omf_segment *);
int CheckForDirectPageLine(struct omf_segment *);
int ProcessDirectiveWithLabelLine(struct omf_segment *);
int ProcessMXDirective(struct omf_segment *);
int EvaluateVariableLine(struct source_line *,struct omf_segment *);
int ComputeLineAddress(struct omf_segment *,struct omf_project *);
struct relocate_address *BuildRelocateAddress(BYTE,BYTE,WORD,WORD,struct external *,struct omf_segment *);
struct source_line *BuildSourceLine(struct source_file *,int);
struct source_line *DuplicateSourceLine(struct source_line *);
struct source_line *BuildEmptyLabelLine(char *,struct source_line *);
void DecodeLine(char *,char *,char *,char *,char *);
void mem_free_sourceline(struct source_line *);
void mem_free_sourceline_list(struct source_line *);
void mem_free_label(struct label *);
void mem_free_equivalence(struct equivalence *);
void mem_free_variable(struct variable *);
void mem_free_external(struct external *);
void mem_free_global(struct global *);

/***********************************************************************/
