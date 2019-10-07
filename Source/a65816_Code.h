/***********************************************************************/
/*                                                                     */
/*  a65816_Code.h : Header for the generation of the object code.      */
/*                                                                     */
/***********************************************************************/
/*  Author : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

int BuildAllCodeLineSize(struct omf_segment *current_omfsegment);
int BuildAllCodeLine(int *,struct omf_segment *current_omfsegment,struct omf_project *);
int CompactDirectPageCode(struct omf_segment *current_omfsegment);

/***********************************************************************/
