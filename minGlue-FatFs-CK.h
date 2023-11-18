/*  Glue functions for the minIni library, based on the FatFs and Petit-FatFs
 *  libraries, see http://elm-chan.org/fsw/ff/00index_e.html
 *
 *  By CompuPhase, 2008-2019
 *  This "glue file" is in the public domain. It is distributed without
 *  warranties or conditions of any kind, either express or implied.
 *
 *  (The FatFs and Petit-FatFs libraries are copyright by ChaN and licensed at
 *  its own terms.)
 */

#define INI_BUFFERSIZE  256       /* maximum line length, maximum path length */

/* You must set FF_USE_STRFUNC to 1 or 2 in the include file ff.h (or tff.h)
 * to enable the "string functions" fgets() and fputs().
 */

#include "ff_headers.h"
#include "ff_stdio.h"
//#include "ff.h"                   /* include tff.h for Tiny-FatFs */

/* When setting FF_USE_STRFUNC to 2 (for LF to CR/LF translation), INI_LINETERM
 * should be defined to "\n" (otherwise "\r\n" will be translated by FatFS to
 * "\r\r\n").
*/
#if defined FF_USE_STRFUNC && FF_USE_STRFUNC == 2 && !defined INI_LINETERM
  #define INI_LINETERM  "\n"
#endif

#define INI_FILETYPE    FF_FILE*
#define ini_openread(filename, file)  ((*(file) = ff_fopen((filename), "r")) != NULL)
#define ini_openwrite(filename,file)  ((*(file) = ff_fopen((filename), "rw")) != NULL)
#define ini_close(file)               (ff_fclose(*(file)) == FF_ERR_NONE)
#define ini_read(buffer,size,file)    (ff_fgets((buffer),(size), (*file)) != NULL)
#define ini_write(buffer,file)        (ff_fwrite((buffer), 1, strlen(buffer), *(file)) >= 0)
#define ini_remove(filename)          (ff_remove(filename) == FF_ERR_NONE)

#define INI_FILEPOS                   long
#define ini_tell(file,pos)            (*(pos) = ff_ftell((*file)))
#define ini_seek(file,pos)            (ff_fseek(*file, *(pos), FF_SEEK_SET) == 0)

static int ini_rename(char* source, const char* dest)
{
  /* Function f_rename() does not allow drive letters in the destination file */
  const char *drive = strchr(dest, ':');
  drive = (drive == NULL) ? dest : drive + 1;
  return (ff_rename(source, drive, true) == FF_ERR_NONE);
}
