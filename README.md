# NTFS-File-Search

Relatively lightweight app used for quickly querying files/directories located on an NTFS volume.
It works by reading and parsing file records located in the MFT (master file table) 

### Designed for Windows Operating System
While Linux support NTFS drives, this program was specifically designed for Windows.

The majority of the Linux-incompatible code is just because of the Win32 API data types.
So it really shouldn't be too difficult to convert the data types, and (few) function calls, to their supported counterparts

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
