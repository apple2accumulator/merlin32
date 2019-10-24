/***********************************************************************/
/*                                                                     */
/*  a65816_File.h : Header for file management.                        */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

struct source_file 
{
  char *file_path;
  char *file_name;
  int file_number;

  unsigned char *data;

  int nb_line;
  char **tab_line;

  struct source_line *first_line;    /* Line of this Source file */
  struct source_line *last_line;

  struct source_file *next;
};

int LoadAllSourceFile(char *,char *,struct omf_segment *);
struct source_file *LoadOneSourceFile(char *,char *,int);
struct source_file *LoadOneBinaryFile(char *,char *,int);
int BuildObjectCode(struct omf_segment *);
int CreateOutputFile(char *,int verbose_mode, int symbol_mode, struct omf_segment *,struct omf_project *);
int BuildObjectFile(char *,struct omf_segment *,struct omf_project *);
int BuildSingleObjectFile(char *,int,struct omf_project *);
void mem_free_sourcefile(struct source_file *,int);

/***********************************************************************/
