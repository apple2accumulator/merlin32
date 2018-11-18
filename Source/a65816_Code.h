/***********************************************************************/
/*                                                                     */
/*  a65816_Code.h : Header pour la génération du Code objet.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

int BuildAllCodeLineSize(struct omf_segment *current_omfsegment);
int BuildAllCodeLine(int *,struct omf_segment *current_omfsegment,struct omf_project *);
int CompactDirectPageCode(struct omf_segment *current_omfsegment);

/***********************************************************************/
