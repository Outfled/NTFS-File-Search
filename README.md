# NTFS-File-Search

Relatively lightweight app used for quickly querying files/directories located on an NTFS volume.
It works by reading and parsing file records located in the MFT (master file table) 

### Designed for Windows Operating System
While Linux support NTFS drives, this program was specifically designed for Windows.

The majority of the Linux-incompatible code is just because of the Win32 API data types.
So it really shouldn't be too difficult to convert the data types, and (few) function calls, to their supported counterparts

## Usage/Examples
**View _NTFSFileSearch\Sample.cpp_ for more in depth usage example code**

*Variables*

	NTFSVolumeSearcher 	vFileSearcher;
   	UINT64 			nFileResults;
  	PNTFS_FILE_ENTRYA 	pFileResults;
   
### 1: Search for File(s) by Name
  	 /* Add file search filter to find files with the name of "SampleSearchFile.txt" */
 	vFileSearcher.AddFileFilter(
  		FF_OPERATOR_EQUAL, // Equal to operator; ie files that have 'equal to' (in terms of name; same as) names as the provided operand below ("SampleSearchFile.txt")
  		FF_FACTOR_NAME, // Specifies the operand file-content type is its name (not including path)
    		L"SampleSearchFile.txt" // The filter operand (the name of the file that each entry must be the same as)
       	);

 	// TODO:
	// Create CNTFSVolume object & call vFileSearcher.SetVolume()
	// ...

 	/* Locate files that match the criteria */
  	vFileSearcher.FindFilesA(
   		FILE_SEARCH_FLAG_FIND_FILES,	// Search for files only (exclude directories)
		&pFileResults,
		&nFileResults
	);

### 2: Search for File(s) by File Size
  	/* Look for files >= 1 GB */
 	vFileSearcher.AddFileFilter(
  		FF_OPERATOR_GREATER_THAN_OR_EQ, // Greater than or equal to operator; search for files that have a greater than or equal to value as the operand value below (1GB)
  		FF_FACTOR_SIZE, // The operand file content type. FF_FACTOR_SIZE specifies the file content type is it's allocated size on the disk
    		GB_TO_BYTE(1) // The operand value; ie the operand value used alongside each file-entry result found in the operator 
       	);

  	// TODO:
	// Create CNTFSVolume object & call vFileSearcher.SetVolume()
	// ...

 	/* Locate files that match the criteria */
  	vFileSearcher.FindFilesA(
   		FILE_SEARCH_FLAG_FIND_FILES,	// Search for files only (exclude directories)
		&pFileResults,
		&nFileResults
	);

### 3: Search for all Files & Directories
	// No search filter needed

	// TODO:
	// Create CNTFSVolume object & call vFileSearcher.SetVolume()
	// ...
 
 	/* Search for files & directories */
  	vFileSearcher.FindFilesA(
   		FILE_SEARCH_FLAG_FIND_ALL,	// Search for both files & directories (FILE_SEARCH_FLAG_FIND_FILES | FILE_SEARCH_FLAG_FIND_DIRECTORIES)
		&pFileResults,
		&nFileResults
	);

 
## Performance Testing
Quick performance benchmarks were created to determine the length, in milliseconds, NTFS-File-Search took to locate _all files and directories_ on my drives. Indicated by '_Elapsed Time_'

**The software used to determine the read/write speeds of each drive was Samsung Magician (Range: 1GiB, Duration: 2 Minutes, Chunk size: 128KiB)**

### Samsung SSD 870 EVO 500GB - SATA

	Windows Installation Drive: 	No
 	Used Space:			342 GB
	Sequential Read:		560 MB/s
	Sequential Write:		531 MB/s
 
 	Number of Files:	45,109
  	Number of Directories: 	8,198
   	Elapsed Time:		1,250 Milliseconds

### Samsung SSD 970 EVO 250GB - NVME

	Windows Installation Drive: 	Yes
  	Used Space:			207 GB
	Sequential Read:		3,485 MB/s
	Sequential Write:		1,367 MB/s
 
 	Number of Files:	677,259
  	Number of Directories: 	208,226
   	Elapsed Time:		20,141 Milliseconds

### Western Digital (WD) Black 1TB HDD - SATA

	Windows Installation Drive: 	No
  	Used Space:			777 GB
	Sequential Read:		147 MB/s
	Sequential Write:		141 MB/s

  	Number of Files:	1,268,893
  	Number of Directories: 	228,485
   	Elapsed Time:		33,625 Milliseconds

## Cited Sources\Helpful References
* https://www.ntfs.com/ntfs-mft.htm
* https://learn.microsoft.com/en-us/windows/win32/devnotes/master-file-table
* https://learn.microsoft.com/en-us/windows/win32/fileio/master-file-table
* https://handmade.network/forums/articles/t/7002-tutorial_parsing_the_mft
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/attribute_header.html#:~:text=Overview,attribute%20depends%20on%20two%20things.
* https://en.wikipedia.org/wiki/BIOS_parameter_block
* https://www.ntfs.com/ntfs-partition-boot-sector.htm
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/file_record.html
* http://inform.pucp.edu.pe/~inf232/Ntfs/ntfs_doc_v0.5/attributes/standard_information.html
* https://flatcap.github.io/linux-ntfs/ntfs/attributes/file_name.html
* https://stackoverflow.com/questions/62248173/how-are-dos-filenames-handled-in-the-mft-in-windows-10
* https://github.com/farfella/ntfs-cpu-search/tree/master
* https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
* https://man7.org/linux/man-pages/man7/namespaces.7.html
