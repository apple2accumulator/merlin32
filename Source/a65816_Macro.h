/***********************************************************************/
/*                                                                     */
/*  a65816_Macro.h : Module pour la gestion des Macros  .              */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

struct macro
{
  char *name;

  char *file_name;         /* Nom du fichier Macro contenant cette Macro */
  int file_line_number;    /* Numéro de ligne du fichier Macro où commence cette Macro */

  struct macro_line *first_line;
  struct macro_line *last_line;

  struct macro *next;
};

struct macro_line
{
  char *label;
  char *opcode;
  char *operand;
  char *comment;

  struct macro_line *next;
};

void LoadAllMacroFile(char *,struct omf_segment *);
void LoadSourceMacroFile(char *,struct omf_segment *);
void GetMacroFromSource(struct omf_segment *);
void CheckForDuplicatedMacro(struct omf_segment *);
int ReplaceMacroWithContent(struct omf_segment *,struct omf_project *);
int IsMacroFile(char *,char *,char *);
void mem_free_macro(struct macro *);
void mem_free_macro_list(struct macro *);
void mem_free_macroline(struct macro_line *);

/***********************************************************************/
