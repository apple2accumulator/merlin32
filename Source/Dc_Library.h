/***********************************************************************/
/*                                                                     */
/*  Dc_Library.h : Header pour la bibliothèque générique de fonctions. */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#if defined(WIN32)
typedef unsigned long       DWORD;    /* Unsigned 32 bit */
#else
typedef uint32_t            DWORD;    /* Unsigned 32 bit */
#endif
typedef unsigned short      WORD;     /* Unsigned 16 bit */
typedef unsigned char       BYTE;     /* Unsigned 8 bit */

/** Byte Order **/
#define BYTE_ORDER_INTEL       1     /* Little Indian */
#define BYTE_ORDER_MOTOROLA    2     /* Big Endian */

/** Platform dependent code **/
#if defined(WIN32) || defined(WIN64) 
  /* Windows */
  #define FOLDER_SEPARATOR  "\\"
#else
  /* Linux + MacOS */
  #define FOLDER_SEPARATOR  "/"
#endif

#define ERROR_NOT_READY_YET         0
#define ERROR_READY                 1

#define ERROR_INIT                  1
#define ERROR_END                   2
#define ERROR_GET_STRING            3
#define ERROR_RAISE                 4

#define FILE_INIT                   1
#define FILE_FREE                   2

#define FILE_DECLARE_DIRECTORY     10
#define FILE_FREE_DIRECTORY        11

#define MEMORY_INIT                 1
#define MEMORY_FREE                 2

#define MEMORY_DECLARE_ALLOC        3
#define MEMORY_FREE_ALLOC           4

#define MEMORY_SET_PARAM           10
#define MEMORY_GET_PARAM           11

#define MEMORY_SET_FILE            15
#define MEMORY_GET_FILE            16
#define MEMORY_FREE_FILE           17

#define MEMORY_SET_OMFSEGMENT      18
#define MEMORY_GET_OMFSEGMENT      19

#define MEMORY_ADD_OPCODE          20
#define MEMORY_GET_OPCODE_NB       21
#define MEMORY_GET_OPCODE          22
#define MEMORY_SORT_OPCODE         23
#define MEMORY_SEARCH_OPCODE       24
#define MEMORY_FREE_OPCODE         25

#define MEMORY_ADD_DATA            30
#define MEMORY_GET_DATA_NB         31
#define MEMORY_GET_DATA            32
#define MEMORY_SORT_DATA           33
#define MEMORY_SEARCH_DATA         34
#define MEMORY_FREE_DATA           35

#define MEMORY_ADD_DIRECTIVE       40
#define MEMORY_GET_DIRECTIVE_NB    41
#define MEMORY_GET_DIRECTIVE       42
#define MEMORY_SORT_DIRECTIVE      43
#define MEMORY_SEARCH_DIRECTIVE    44
#define MEMORY_FREE_DIRECTIVE      45

#define MEMORY_ADD_DIREQU          50
#define MEMORY_GET_DIREQU_NB       51
#define MEMORY_GET_DIREQU          52
#define MEMORY_SORT_DIREQU         53
#define MEMORY_SEARCH_DIREQU       54
#define MEMORY_FREE_DIREQU         55

#define MEMORY_ADD_MACRO           60
#define MEMORY_GET_MACRO_NB        61
#define MEMORY_GET_MACRO           62
#define MEMORY_SORT_MACRO          63
#define MEMORY_SEARCH_MACRO        64
#define MEMORY_FREE_MACRO          65

#define MEMORY_ADD_LABEL           70
#define MEMORY_GET_LABEL_NB        71
#define MEMORY_GET_LABEL           72
#define MEMORY_SORT_LABEL          73
#define MEMORY_SEARCH_LABEL        74
#define MEMORY_FREE_LABEL          75

#define MEMORY_ADD_VARIABLE        80
#define MEMORY_GET_VARIABLE_NB     81
#define MEMORY_GET_VARIABLE        82
#define MEMORY_SORT_VARIABLE       83
#define MEMORY_SEARCH_VARIABLE     84
#define MEMORY_FREE_VARIABLE       85

#define MEMORY_ADD_EQUIVALENCE     90
#define MEMORY_GET_EQUIVALENCE_NB  91
#define MEMORY_GET_EQUIVALENCE     92
#define MEMORY_SORT_EQUIVALENCE    93
#define MEMORY_SEARCH_EQUIVALENCE  94
#define MEMORY_FREE_EQUIVALENCE    95

#define MEMORY_ADD_EXTERNAL       100
#define MEMORY_GET_EXTERNAL_NB    101
#define MEMORY_GET_EXTERNAL       102
#define MEMORY_SORT_EXTERNAL      103
#define MEMORY_SEARCH_EXTERNAL    104
#define MEMORY_FREE_EXTERNAL      105

#define MEMORY_ADD_GLOBAL         110
#define MEMORY_GET_GLOBAL_NB      111
#define MEMORY_GET_GLOBAL         112
#define MEMORY_FREE_GLOBAL        115

struct parameter
{
  int byte_order;         /* Processor Byte Order : Intel | Motorola */

  int org_address;        /* Default is $8000 */

  char source_folder_path[1024];  /* Dossier contenant les fichiers sources */

  char date_1[64];       /* 29-DEC-88 (9 bytes) */
  char date_2[64];       /* 12/29/88 (8 bytes) */
  char date_3[64];       /* 29-DEC-88   4:18:37 PM (22 bytes) */
  char date_4[64];       /* 12/29/88   4:18:37 PM (21 bytes) */

  char *buffer_line;     /* 64 KB buffer */
  char *buffer_value;

  char *buffer_folder_path;
  char *buffer_file_path;
  char *buffer_file_name;

  char *buffer_label;
  char *buffer_opcode;
  char *buffer_operand;
  char *buffer_comment;

  char buffer_string[64000];   /* Buffer utilise pour la conversion de String */

  char output_file_path[1024];
  char current_folder_path[1024];   /* Dossier où se trouvait le fichier Link */

  char *buffer_error;

  char buffer_latest_error[2048];

  unsigned char *buffer;
};

#define MIN(a,b)  ((a)>(b) ? (b) : (a))

#define IS_DOLLAR      1    /* $ */
#define IS_DASH        2    /* # */
#define IS_POURCENT    3    /* % */

#define IS_VALUE       1
#define IS_CONSTANT    2    /* ( ) [ ] $ # % ,X ,Y ,S + - */
#define IS_NUMERIC     3    /* 0123456789ABCDEF */
#define IS_EXPANDABLE  4    /* la valeur doit être éclatée */

#define TYPE_DATA       1
#define TYPE_SEPARATOR  2
 
#define SEPARATOR_REPLACE_LABEL        0   /* Remplace un Label dans une Operande (tous les sépatareurs possibles) */
#define SEPARATOR_REPLACE_VARIABLE     1   /* Remplace une ]Variable dans une Opérande (tous les séparateurs sauf le ]) */
#define SEPARATOR_EVALUATE_EXPRESSION  2   /* Prépare l'évaluation d'une expression (on découpe selon les opérateurs : + - / * & . ! < = > #) et on gère les opérateurs ambigus (#><*) ou unaire (-) */
#define SEPARATOR_DATA_VALUES          3   /* Sépare les différentes valeurs d'une ligne Data (juste le ,) */

#define ASCII_TABLE    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?`abcdefghijklmnopqrstuvwxyz{|}~"    /* Valide pour ASC, STR... */
#define INVFLS_TABLE   "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?"                                   /* Valide pour INV et FLS */

#define SET_FILE_VISIBLE 1
#define SET_FILE_HIDDEN  2

#define STATUS_DONT       0    /* Evaluation d'une condition */
#define STATUS_DO         1
#define STATUS_UNKNWON    2

struct item
{
  char *name;
  int type;

  struct item *next;
};

/* Pre-Déclaration des structures */
struct omf_project;
struct omf_segment;
struct source_line;
struct external;

void my_RaiseError(int,void *);
void my_Memory(int,void *,void *,struct omf_segment *);
void my_File(int,void *);
int my_stricmp(char *,char *);
int my_strnicmp(char *,char *,size_t);
void my_printf64(int64_t,char *);
int64_t my_atoi64(char *);
int my_IsFileExist(char *);
void bo_memcpy(void *,void *,size_t);
char *GetFileProperCasePath(char *);
char **GetFolderFileList(char *,int *,int *);
unsigned char *LoadTextFileData(char *,int *);
unsigned char *LoadBinaryFileData(char *,int *);
int GetLabelFromLine(char *,int,char *);
int GetOpcodeFromLine(char *,int,char *);
void CleanBuffer(char *);
void CleanUpName(char *);
void GetFolderFromPath(char *,char *);
int IsDecimal(char *,int *);
int IsHexaDecimal(char *,int *);
int IsBinary(char *,int *);
int IsAscii(char *,int *);
int IsVariable(char *,int *,struct omf_segment *);
int IsLabel(char *,int *,struct omf_segment *);
int IsExternal(char *,int *,struct omf_segment *);
void GetUNID(char *);
void ProcessOZUNIDLine(char *);
char *ReplaceInOperand(char *,char *,char *,int,struct source_line *);
char **DecodeOperandeAsElementTable(char *,int *,int,struct source_line *);
struct item *ExtractAllIem(char *);
int IsSeparator(char,int);
BYTE GetByteValue(char *);
WORD GetWordValue(char *);
DWORD GetDwordValue(char *);
int64_t GetVariableValue(char *,int *,struct omf_segment *);
int64_t GetBinaryValue(char *);
int64_t GetDecimalValue(char *,int *);
int64_t GetHexaValue(char *);
int64_t GetAsciiValue(char *);
int64_t GetAddressValue(char *,int,struct external **,int *,int *,struct omf_segment *);
int QuickConditionEvaluate(struct source_line *,int64_t *,struct omf_segment *);
int64_t GetQuickValue(char *,struct source_line *,int *,struct omf_segment *);
int64_t GetQuickVariable(char *,struct source_line *,int *,struct omf_segment *);
int64_t EvalExpressionAsInteger(char *,char *,struct source_line *,int,int *,BYTE *,BYTE *,WORD *,DWORD *,struct external **,struct omf_segment *);
int64_t EvaluateAlgebricExpression(char **,int,int,int,int *);
int HasPriority(char *,char *);
int BuildBestMVXWord(DWORD,DWORD);
int IsPageDirectOpcode(char *);
int IsPageDirectAddressMode(int);
int IsDirectPageLabel(char *,struct omf_segment *);
int UseCurrentAddress(char *,char *,struct source_line *);
void ReplaceCurrentAddressInOperand(char **,char *,char *,struct source_line *);
void my_SetFileAttribute(char *,int);
int CreateBinaryFile(char *,unsigned char *,int);
void my_DeleteFile(char *);
char **BuildUniqueListFromFile(char *,int *);
char **BuildUniqueListFromText(char *,char,int *);
int IsProdosName(char *);
void BuildAbsolutePath(char *,char *,char *);
void mem_free_list(int,char **);
struct parameter *mem_alloc_param(void);
struct item *mem_alloc_item(char *,int);
void mem_free_param(struct parameter *);
void mem_free_item(struct item *);
void mem_free_item_list(struct item *);
void mem_free_table(int, char**);

/***********************************************************************/
