/***********************************************************************/
/*                                                                     */
/*  a65816_OMF.h : Header for the management of File OMF.              */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#define CRECORD_SIZE                7       /* Size of a CRecord */
#define END_SIZE                    1       /* Size of an END */

struct omf_project
{
  BYTE type;               /* Type of File ($06 for the Multi-Segment Fixed-Address) */
  WORD aux_type;           /* AuxType of File */
  int express_load;        /* Adds the ExpressLoad Segment at the Beginning */

  /** List of Files Multi-Segments (OMF or Fixed Address) **/
  int nb_file;             /* Number of Files for the Multi-Segment Fixed-Address OneBinaryFile */
  char **dsk_name_tab;     /* Name of project = File name à create (in OMF or in Multi-Segment Fixed-Address SingleBinary) */
  DWORD *org_address_tab;  /* for the Multi-Segment Fixed-Address OneBinaryFile, the ORG is fixed by the File LINK */
  DWORD *file_size_tab;    /* File Size for the Multi-Segment Fixed-Address OneBinaryFile */
  
  /** List of Segments OMF / Fixed Address **/
  int nb_segment;
  struct omf_segment *first_segment;
  struct omf_segment *last_segment;

  /** Data of Project **/
  DWORD project_buffer_length;
  unsigned char *project_buffer_file;

  /* File Size Project */
  DWORD project_file_length;

  /* Type of Program */
  int is_omf;                    /* This is a relocatable OMF project v2.1 */
  int is_multi_fixed;            /* This is a Multi-Segment Fixed-Address project */
  int is_single_binary;          /* At the end, we stick all the fixed-address segments together, one behind the other (in 1 or more files) */
};

#define ALIGN_BANK   2
#define ALIGN_PAGE   1
#define ALIGN_NONE   0

struct omf_segment
{
  char *master_file_path;    /* Path of Source file Master */

  /*****************************************************/
  /*  Values used in the OMF Header of Segment  */
  /*****************************************************/
  WORD type_attributes;      /* Type + Attributs */
  int bank_size;             /* Bank Size (64KB for code, 0-64 KB for Data, O=can cross boundaries) */
  int org;                   /* Absolute address to load the segment, 0=anywhere */
  int alignment;             /* Boundary Alignement */
  int ds_end;                /* Number of 0s to add at the end of Segment */

  char *load_name;
  char *segment_name;

  int file_number;           /* File number (for the Fixed-Address Single-Binary */

  /***************************************************/
  /*  Values used in the OMF Body of Segment  */
  /***************************************************/
  /* Number of Segment */
  int segment_number;      /* 1-N */

   /* for the Multi-Segment Fixed-Address OneBinaryFile, the ORG is fixed by the File LINK */
  int has_org_address;
  DWORD org_address;

  /* Type of File Out: OMF or Binary */
  int is_omf;
  int is_relative;     /* We have an REL => Pas of Direct Page for the Label (unless the assembly is managed via a Link.txt Fixed Address) */

  /** List of addresses to be patched **/
  int nb_address;
  struct relocate_address *first_address;
  struct relocate_address *last_address;

  /* File to create */
  char object_name[256];

  /* Object code */
  int object_length;
  unsigned char *object_code;

  /*** Data of Segment: Header + Body ***/
  DWORD header_length;
  unsigned char segment_header_file[1024];                       /* we make it large */

  DWORD segment_body_length;                                     /* Size of the body segment */
  DWORD body_length;
  unsigned char *segment_body_file;

  /* Header stored in ExpressLoad */
  DWORD xpress_data_offset;
  DWORD xpress_data_length;
  DWORD xpress_reloc_offset;
  DWORD xpress_reloc_length;

  DWORD header_xpress_length;
  unsigned char header_xpress_file[1024];                        /* we make it large */

  /**************************************/
  /*  Set of structures of data memory  */
  /**************************************/
  void *alloc_table[1024];
  struct source_file *first_file;         /* Premier Source file */
  int nb_opcode;                          /* List of opcode */
  struct item *first_opcode;
  struct item *last_opcode;
  struct item **tab_opcode;
  int nb_data;                            /* List of data */
  struct item *first_data;
  struct item *last_data;
  struct item **tab_data;
  int nb_directive;                       /* List of directives */
  struct item *first_directive;
  struct item *last_directive;
  struct item **tab_directive;
  int nb_direqu;                          /* List of Equivalence Directives */
  struct item *first_direqu;
  struct item *last_direqu;
  struct item **tab_direqu;
  struct item local_item;
  struct item *local_item_ptr;
  int nb_macro;                           /* Macro list */
  struct macro *first_macro;
  struct macro *last_macro;
  struct macro **tab_macro;
  struct macro local_macro;
  struct macro *local_macro_ptr;
  int nb_label;                           /* List of labels */
  struct label *first_label;
  struct label *last_label;
  struct label **tab_label;
  struct label local_label;
  struct label *local_label_ptr;
  int nb_equivalence;                     /* List of equivalences */
  struct equivalence *first_equivalence;
  struct equivalence *last_equivalence;
  struct equivalence **tab_equivalence;
  struct equivalence local_equivalence;
  struct equivalence *local_equivalence_ptr;
  int nb_variable;                        /* List of variables */
  struct variable *first_variable;
  struct variable *last_variable;
  struct variable **tab_variable;
  struct variable local_variable;
  struct variable *local_variable_ptr;
  int nb_external;                        /* List of external EXT */
  struct external *first_external;
  struct external *last_external;
  struct external **tab_external;
  struct external local_external;
  struct external *local_external_ptr;
  int nb_global;                          /* List of global ENT */
  struct global *first_global;
  struct global *last_global;
  
  struct omf_segment *next;
};

DWORD BuildOMFHeader(struct omf_segment *);
DWORD BuildOMFBody(struct omf_segment *);
void RelocateExternalFixedAddress(struct omf_segment *);
int BuildOMFFile(char *,struct omf_project *);
int BuildExpressLoadSegment(struct omf_project *);
void UpdateFileInformation(char *,char *,struct omf_project *);
void mem_free_omfproject(struct omf_project *);
struct omf_segment *mem_alloc_omfsegment(void);
void mem_free_omfsegment(struct omf_segment *);

/***********************************************************************/
