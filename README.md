# NTFS-File-Search

C++ project used for quickly querying all files/directories located on an NTFS volume/partition.
It works by reading and parsing file records located in the MFT (master file table) 

### Designed for Windows Operating System
While Linux support NTFS drives, this program was specifically designed for Windows.

The majority of the Linux-incompatible code is just because of the Win32 API data types.
So it really shouldn't be too difficult to convert the data types, and very few function calls, to their supported counterparts
