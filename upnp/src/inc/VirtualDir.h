

#ifndef VIRTUALDIR_H
#define VIRTUALDIR_H


/** The {\bf VirtualDirCallbacks} structure contains the pointers to
 *  file-related callback functions a device application can register to
 *  virtualize URLs.  
 */
struct VirtualDirCallbacks
{
	/** Called by the web server to query information on a file.  The callback
	 *  should return 0 on success or -1 on an error. */
	VDCallback_GetInfo get_info;

	/** Called by the web server to open a file.  The callback should return
	 *  a valid handle if the file can be opened.  Otherwise, it should return
	 *  {\tt NULL} to signify an error. */
	VDCallback_Open open;

	/** Called by the web server to perform a sequential read from an open
	 *  file.  The callback should copy {\bf buflen} bytes from the file into
	 *  the buffer.
	 *  @return [int] An integer representing one of the following:
	 *    \begin{itemize}
	 *      \item {\tt 0}:  The file contains no more data (EOF).
	 *      \item {\tt >0}: A successful read of the number of bytes in the 
	 *                      return code.
	 *      \item {\tt <0}: An error occurred reading the file.
	 *    \end{itemzie}
	 */
	VDCallback_Read read;

	/** Called by the web server to perform a sequential write to an open
	 *  file.  The callback should write {\bf buflen} bytes into the file from
	 *  the buffer.  It should return the actual number of bytes written, 
	 *  which might be less than {\bf buflen} in the case of a write error.
	 */
	VDCallback_Write write;

	/** Called by the web server to move the file pointer, or offset, into
	 *  an open file.  The {\bf origin} parameter determines where to start
	 *  moving the file pointer.  A value of {\tt SEEK_CUR} moves the
	 *  file pointer relative to where it is.  The {\bf offset} parameter can
	 *  be either positive (move forward) or negative (move backward).  
	 *  {\tt SEEK_END} moves relative to the end of the file.  A positive 
	 *  {\bf offset} extends the file.  A negative {\bf offset} moves backward 
	 *  in the file.  Finally, {\tt SEEK_SET} moves to an absolute position in 
	 *  the file. In this case, {\bf offset} must be positive.  The callback 
	 *  should return 0 on a successful seek or a non-zero value on an error.
	 */
	VDCallback_Seek seek;

	/** Called by the web server to close a file opened via the {\bf open}
	 *  callback.  It should return 0 on success, or a non-zero value on an 
	 *  error.
	 */
	VDCallback_Close close;
};


typedef struct virtual_Dir_List
{
	struct virtual_Dir_List *next;
	char dirName[NAME_SIZE];
} virtualDirList;


#endif /* VIRTUALDIR_H */

