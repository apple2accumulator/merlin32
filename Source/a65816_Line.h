/***********************************************************************/
/*                                                                     */
/*  a65816_Line.h : Header pour la gestion des lignes.                 */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#define LINE_UNKNOWN     0       /* Ligne dont on ne connait pas encore le type / qu'on a rrive pas à décoder */
#define LINE_COMMENT     1       /* Ligne ne contenant qu'un commentaire */
#define LINE_DIRECTIVE   2       /* Directive d'assemblage (PUT, USE, IF, XC...) */
#define LINE_EQUIVALENCE 3       /* Ligne définissant une constante (EQU, =) */
#define LINE_VARIABLE    4       /* Ligne définissant une variable (pour les boucles...) */
#define LINE_CODE        5       /* Ligne de code avec au moins un Opcode */
#define LINE_DATA        6       /* Ligne stockant des variables (HEX, DA, ASC, STR...)*/
#define LINE_MACRO       7       /* Ligne appelant une macro */
#define LINE_EMPTY       8       /* Ligne avec seulement un Label */
#define LINE_GLOBAL      9       /* Label Global définissant un point d'entrée pour les InterSeg */
#define LINE_EXTERNAL   10       /* Label Externe définissant une adresse d'un autre segment */

#define LINE_MACRO_DEF   1

#define AM_UNKOWN                                0  /* Not recognize */
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
#define AM_PC_RELATIVE_LONG                     21  /* relative2  Program Counter Relative Long (BRL et PER)  */
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
#define FORMAT_EXTERNAL   7      /* Label Externe au module = Adresse Longue */

struct source_line
{
  char *line_data;               /* Ligne telle qu'elle était dans le fichier Source */
  int line_number;               /* Numéro de la ligne global */

  int file_line_number;          /* Numéro de la ligne dans le fichier */
  struct source_file *file;

  int type;                      /* Type de ligne : LINE_COMMENT, LINE_DIRECTIVE, LINE_CODE, LINE_MACRO */
  int type_aux;                  /* Type Auxiliaire : LINE_MACRO_DEF */
  int is_in_source;              /* Cette ligne est dans le code source d'origine (et non pas dans une Macro qui a été développée) */
  int is_valid;                  /* Cette ligne sera conservé dans le code final */
  int no_direct_page;            /* On ne doit pas générer de code Page Direct : LDA\ $0000 */
  int use_direct_page;           /* On doit générer de code Page Direct : LDA $00 */

  int was_local_label;           /* Ce label :local a été remplacé par un nom unique */

  int is_inside_macro;           /* Cette ligne de code fait partie d'une déclaration d'une Macro présente dans le fichier Source */

  int is_dum;                    /* Cette ligne est stockée entre 2 DUM - DEND */
  struct source_line *dum_line;  /* Ligne déclarant le DUM */

  char *label_txt;               /* Découpage de la ligne en 4 parties : Label  Opcode Operand   ; Comment */
  char *opcode_txt;
  char *operand_txt;
  char *comment_txt;

  int cond_level;                /* Niveau de la conditiion 0->N IF [ELSE] FIN | DO [ELSE] FIN */
  int do_level;                  /* Niveau de la condition IF ELSE FIN pour l'évaluation préliminaire */

  char m[2];                     /* M : 0/1/? = Acc size (0=16bit, 1=8bit) */
  char x[2];                     /* X : 0/1/? = X,Y size (0=16bit, 1=8bit) */

  struct variable *variable;     /* Si cette ligne est une variable, on pointe sur sa valeur */

  struct macro *macro;           /* Si cette ligne est un appel à une Macro, on pointe vers elle */

  int bank;                      /* Bank mémoire de la ligne */
  int address;                   /* Adresse de la ligne */
  int is_fix_address;            /* Cette ligne a une adresse fixe (cas des lignes situées dans [ORG $Addr ORG] pour un OMF ou un Single Binary */
  int global_bank;
  int global_address;            /* Adresse de la ligne si le ORG $Addr n'était pas là (utile pour reloger les adresses situées entre un [ORG $Addr ORG] */

  int nb_byte;                   /* Taille de la ligne : 1 + Operand Size */
  unsigned char opcode_byte;     /* Opcode */
  int address_mode;              /* Mode d'addressage AM_ */
  int address_is_rel;            /* L'adresse pointée par l'opcode est une adresse d'une zone relative : si < 0x100, cela ne la fera pas basculer dans du Page Direct */
  unsigned char operand_byte[4]; /* Operande */
  unsigned char *data;           /* Data */

  DWORD operand_value;           /* Résultat de l'évaluation de l'Opérande, avant la troncature due au nb de byte de l'operande */
  DWORD operand_address_long;    /* Si l'opérande pointe vers une adresse, on garde ici la version longue de l'adresse : Bank/HighLow */
  struct relocate_address *external_address;  /* En cas d'utilisation d'une adresse Externe, on conserve un pointeur */

  char reloc[16];                /* Information de Relocate : Nb Byte >> Bit Shift */

  struct source_line *next;
};

struct label
{
  char *name;
  struct source_line *line;      /* Le is_dum de la ligne permet de savoir si ce label provient d'une section DUM (= Adresse Fixe = Non relogeable) */

  int is_global;                 /* Ce label est un point d'entrée dans le Segment */                 

  struct label *next;
};

struct external
{
  /* Référence interne */
  char *name;
  struct source_line *source_line;

  /* Référence externe */
  struct omf_segment *external_segment;
  struct label *external_label;

  struct external *next;
};

struct global
{
  char *name;
  struct source_line *source_line;    /* Ligne source contenant le ENT Label1,Label2... */

  struct global *next;
};

struct equivalence
{
  char *name;
  char *value;

  struct source_line *source_line;    /* Si l'équivalence provient du source, on pointe la ligne (elle peut aussi venir des fichiers Macro) */

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
  BYTE ByteCnt;               /* Number of Bytes to be relocated (1,2,3 ou 4) */
  BYTE BitShiftCnt;           /* Opérations >> ou << */
  WORD OffsetPatch;           /* Offset of the first Byte to be Patched */
  WORD OffsetReference;       /* Adresse */ 

  struct external *external;  /* On fait appel à un Label Externe (NULL si interne) */
  unsigned char *object_line; /* Adresse de la zone à patcher dans la line (pour les label externes) */

  int processed;              /* Déjà traité */

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
