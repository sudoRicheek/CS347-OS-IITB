#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
    struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	for (size_t i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, inode);
		if (strcmp(inode->name, filename)==0) {
			free(inode);
			return -1;
		}
	}
	
	int inode_num = simplefs_allocInode();
	if (inode_num == -1) { // No free Inode
		free(inode);
		return -1;
	}

	// Initialise the inode nicely!
	inode->status = INODE_IN_USE;
	memcpy(inode->name, filename, MAX_NAME_STRLEN);
	inode->file_size = 0;
	for(int i=0; i < MAX_FILE_SIZE; i++)
        inode->direct_blocks[i] = -1;
	// End initialise

	simplefs_writeInode(inode_num, inode);
	free(inode);
    return inode_num;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	
	size_t i; // inode number
	for (i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, inode);
		if (strcmp(inode->name, filename)==0)
			break;
	}

	if (i==NUM_INODES) { // filename not found
		free(inode);
		return; 
	}

	for (size_t i = 0; i < MAX_FILE_SIZE; i++) // free data blocks referenced by inode
		if (inode->direct_blocks[i]!=-1)
			simplefs_freeDataBlock(inode->direct_blocks[i]);
	
	simplefs_freeInode(i);
	free(inode);
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	
	size_t i; // inode number
	for (i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, inode);
		if (strcmp(inode->name, filename)==0)
			break;
	}
	free(inode);

	if (i==NUM_INODES) // filename not found
		return -1;

	for (int j = 0; j < MAX_OPEN_FILES; j++) {
		if (file_handle_array[j].inode_number==-1) 
		{
			file_handle_array[j].inode_number = i;
			file_handle_array[j].offset = 0;
			return j;
		}
	}
	
    return -1;
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
	buf[0] = '\0'; // we know buf is of BLOCKSIZE

	int i_num = file_handle_array[file_handle].inode_number;
	int of_st = file_handle_array[file_handle].offset;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(i_num, inode);

	if (inode->file_size-of_st < nbytes) {
		free(inode);
		return -1;
	}

	int bytes_read = 0;
	int bl_no = of_st/BLOCKSIZE; // start block number: possibilities-> 0, 1, 2, 3
	int of_in = of_st%BLOCKSIZE; // offset in the start block: 0 .. 63
	for (size_t i = bl_no; i < MAX_FILE_SIZE; i++) {
    	char tempBuf[BLOCKSIZE+1];
		simplefs_readDataBlock(inode->direct_blocks[i], tempBuf);
		tempBuf[BLOCKSIZE] = '\0';

		int len = BLOCKSIZE-of_in;
		bytes_read += BLOCKSIZE-of_in;
		if (bytes_read > nbytes) {
			len = BLOCKSIZE+nbytes-bytes_read-of_in;
			tempBuf[len+of_in] = '\0';
			bytes_read = nbytes;
		}
		strncat(buf, tempBuf+of_in, len);

		if (bytes_read==nbytes)
			break;		
		of_in=0; // start from the beginning in the next block
	}

	free(inode);
    return 0;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
	int i_num = file_handle_array[file_handle].inode_number;
	int of_st = file_handle_array[file_handle].offset;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(i_num, inode);

	int new_sz = of_st + nbytes;
	if (new_sz > BLOCKSIZE*MAX_FILE_SIZE) {
		free(inode);
		return -1;
	}
	
	int blocks_needed = new_sz/BLOCKSIZE + (new_sz%BLOCKSIZE>0);
	int new_blocks[MAX_FILE_SIZE];
	for (size_t i = 0; i < MAX_FILE_SIZE; i++)
		new_blocks[i] = -1;

	// Handle blocks, allocate and free as necessary
	for (size_t i = 0; i < blocks_needed; i++) {
		if (inode->direct_blocks[i]==-1) {
			if ((inode->direct_blocks[i]=simplefs_allocDataBlock())==-1) 
			{
				for (size_t j = 0; j < MAX_FILE_SIZE; j++)
					if (new_blocks[j]!=-1)
						simplefs_freeDataBlock(new_blocks[j]);	
				free(inode);								
				return -1;					
			}
			new_blocks[i] = inode->direct_blocks[i];
		}
	}
	// end block handling

	int bytes_written = 0;
	int bl_no = of_st/BLOCKSIZE; // start block number: possibilities-> 0, 1, 2, 3
	int of_in = of_st%BLOCKSIZE; // offset in the start block: 0 .. 63
	
	// handling offset of the first block
	char tempBuf[BLOCKSIZE];
	if (of_in!=0)
		simplefs_readDataBlock(inode->direct_blocks[bl_no], tempBuf);
	tempBuf[of_in] = '\0';
	
	int len = BLOCKSIZE-of_in; // when nbytes >= BLOCKSIZE-of_in
	if (nbytes < len)
		len = nbytes;

	char tempWriteBuf[BLOCKSIZE+1];
	strncpy(tempWriteBuf, buf, len);
	tempWriteBuf[len] = '\0';
	
	strncat(tempBuf, tempWriteBuf, len);
	simplefs_writeDataBlock(inode->direct_blocks[bl_no], tempBuf);

	bytes_written += BLOCKSIZE-of_in;
	for (size_t i = bl_no+1; i < blocks_needed; i++) {
		len = BLOCKSIZE;
		if (bytes_written + BLOCKSIZE > nbytes)
			len = nbytes - bytes_written;

		strncpy(tempBuf, buf+bytes_written, len);
		tempBuf[len] = '\0';

		simplefs_writeDataBlock(inode->direct_blocks[i], tempBuf);

		bytes_written += len;
		if (bytes_written==nbytes)
			break;		
	}

	inode->file_size += nbytes;
	simplefs_writeInode(i_num, inode);
	free(inode);
    return 0;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/
	int i_num = file_handle_array[file_handle].inode_number;
	int of_st = file_handle_array[file_handle].offset;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(i_num, inode);

	int n_ofst = of_st + nseek;
	if (n_ofst < 0 || n_ofst > inode->file_size)
		return -1;
	
	file_handle_array[file_handle].offset = n_ofst;
	
	free(inode);
    return 0;
}