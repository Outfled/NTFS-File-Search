#ifndef _NTFS_DEFS_H_
#define _NTFS_DEFS_H_

typedef UINT64	VCN_t;	/* Virtual Cluster Number */
typedef INT64	LCN_t;	/* Logical Cluster Number */


//--------------------------------------------------------------------------------------
//
// File Record Attribute Types
//
//--------------------------------------------------------------------------------------
#define	MFT_FILERECORD_ATTR_STANDARD_INFO			0x10
#define	MFT_FILERECORD_ATTR_ATTRIBUTE_LIST			0x20
#define MFT_FILERECORD_ATTR_FILENAME				0x30
#define	MFT_FILERECORD_ATTR_OBJECT_ID				0x40
#define MFT_FILERECORD_ATTR_SECURITY_DESCRIPTOR		0x50
#define	MFT_FILERECORD_ATTR_VOLUME_NAME				0x60
#define	MFT_FILERECORD_ATTR_VOLUME_INFORMATION		0x70
#define	MFT_FILERECORD_ATTR_DATA					0x80
#define	MFT_FILERECORD_ATTR_INDEX_ROOT				0x90
#define	MFT_FILERECORD_ATTR_INDEX_ALLOCATION		0xA0
#define	MFT_FILERECORD_ATTR_BITMAP					0xB0
#define	MFT_FILERECORD_ATTR_REPARSE_POINT			0xC0
#define MFT_FILERECORD_ATTR_EA_INFORMATION			0xD0
#define	MFT_FILERECORD_ATTR_EA						0xE0
#define	MFT_FILERECORD_ATTR_LOGGED_UTILITY_STREAM	0x100
#define	MFT_FILERECORD_ATTR_STOP_TAG				0xFFFFFFFF


//--------------------------------------------------------------------------------------
//
// MFT File Record Header Flags (MFT_FILE_RECORD_HEADER::Flags)
//
//--------------------------------------------------------------------------------------
#define MFT_FILERECORD_FLAG_IN_USE					0x00000001
#define MFT_FILERECORD_FLAG_IS_DIRECTORY			0x00000002
#define MFT_FILERECORD_FLAG_IS_EXTENSION			0x00000004	/* Record is located in the $Extend directory */
#define MFT_FILERECORD_FLAG_SPECIAL_INDEX_PRESENT	0x00000008


//--------------------------------------------------------------------------------------
//
// $STANDARD_INFORMATION Flags
//
//--------------------------------------------------------------------------------------

#define	MFT_STDINFO_FILE_ATTRIBUTE_READONLY			0x00000001
#define	MFT_STDINFO_FILE_ATTRIBUTE_HIDDEN			0x00000002
#define	MFT_STDINFO_FILE_ATTRIBUTE_SYSTEM			0x00000004
#define	MFT_STDINFO_FILE_ATTRIBUTE_ARCHIVE			0x00000020
#define	MFT_STDINFO_FILE_ATTRIBUTE_DEVICE			0x00000040
#define	MFT_STDINFO_FILE_ATTRIBUTE_NORMAL			0x00000080
#define	MFT_STDINFO_FILE_ATTRIBUTE_TEMP				0x00000100
#define	MFT_STDINFO_FILE_ATTRIBUTE_SPARSE			0x00000200
#define	MFT_STDINFO_FILE_ATTRIBUTE_REPARSE			0x00000400
#define	MFT_STDINFO_FILE_ATTRIBUTE_COMPRESSED		0x00000800
#define	MFT_STDINFO_FILE_ATTRIBUTE_OFFLINE			0x00001000
#define	MFT_STDINFO_FILE_ATTRIBUTE_NCI				0x00002000
#define	MFT_STDINFO_FILE_ATTRIBUTE_ENCRYPTED		0x00004000

typedef struct MFT_FILE_REFERENCE
{
	DWORD	LowPart;
	WORD	HighPart;
	WORD	SegmentNumber;
}*PMFT_FILE_REFERENCE;

#pragma pack( push, 1 )

typedef union MFT_FILE_ID
{
	LONGLONG IndexNumber;

	struct
	{
		LONGLONG MftRecordIndex : 48;
		LONGLONG SequenceNumber : 16;
	};
}*PMFT_FILE_ID;

/*
* Non-Resident Attribute Header Layout
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/attribute_header.html#:~:text=Overview,attribute%20depends%20on%20two%20things.
*/
typedef struct MFT_NONRESIDENT_ATTRIBUTE_HDR
{
	VCN_t		StartVCN;				// Starting Virtual Cluster Number (VCN)
	VCN_t		LastVCN;				// Last Virtual Cluster Number (VCN)
	WORD		DataRunOffset;			// Starting offset of the Data Runs
	WORD		CompressionUnitSize;
	DWORD		Padding;
	ULONGLONG	AllocatedSize;
	ULONGLONG	RealSize;
	ULONGLONG	StreamDataSize;
}*PMFT_NONRESIDENT_ATTRIBUTE_HDR;

/*
* Resident Attribute Header Layout
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/attribute_header.html#:~:text=Overview,attribute%20depends%20on%20two%20things.
*/
typedef struct MFT_RESIDENT_ATTRIBUTE_HDR
{
	DWORD	AttributeSize;		// Length of the attribute body
	WORD	AttributeOffset;	// Offset to the Attribute
	BYTE	IndexedFlag;		// Indexed flag
	BYTE	Padding;			// Padding
}*PMFT_RESIDENT_ATTRIBUTE_HDR;

/*
* File-Record Attribute Header
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/attribute_header.html#:~:text=Overview,attribute%20depends%20on%20two%20things.
*/
typedef struct MFT_ATTRIBUTE_HEADER
{
	DWORD	Type;			// Attribute Type
	DWORD	TotalSize;
	BYTE	FormCode;		// 0 = Resident 1 = Non resident
	BYTE	NameLength;		// Attribute name length
	WORD	NameOffset;		// Attribute name offset
	WORD	Flags;
	WORD	AttributeId;	// Unique Id

	union
	{
		MFT_RESIDENT_ATTRIBUTE_HDR		Resdient;
		MFT_NONRESIDENT_ATTRIBUTE_HDR	NonResident;
	};
}*PMFT_ATTRIBUTE_HEADER;


/*
* BIOS Parameter Block (BPB)
* https://en.wikipedia.org/wiki/BIOS_parameter_block
* https://www.ntfs.com/ntfs-partition-boot-sector.htm
*/
typedef struct NTFS_BOOT_SECTOR
{
	/* Jump instruction */
	BYTE Jmp[3];

	/* Signature */
	BYTE Signature[8];

	//
	// BPB and extended BPB
	//

	WORD		BytesPerSector;
	BYTE		SectorsPerCluster;
	WORD		ReservedSectors;
	BYTE		Zeros1[3];
	WORD		Unused1;
	BYTE		MediaDescriptor;
	WORD		Zeros2;
	WORD		SectorsPerTrack;
	WORD		NumberOfHeads;
	DWORD		HiddenSectors;
	DWORD		Unused2;
	DWORD		Unused3;
	ULONGLONG	TotalSectors;
	ULONGLONG	MFT_LCN;		/* $MFT Logical Cluster Number (LCN) */
	ULONGLONG	MFTMirr_LCN;	/* $MFTMirr Logical Cluster Number (LCN) */
	DWORD		ClustersPerFileRecord;
	DWORD		ClustersPerIndexBlock;
	BYTE		VolumeSN[8];

	/* Boot Code */
	BYTE BootCode[430];

	//
	// 0xAA55
	//

	BYTE _AA;
	BYTE _55;
};


#define MFT_FILE_RECORD_MAGIC	0x454C4946

/*
* $MFT File Record Header Layout
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/file_record.html
*/
typedef struct MFT_FILE_RECORD_HEADER
{
	DWORD		Magic;					// "FILE" (0x454C4946)
	WORD		UpdateSequenceOffset;	// Update Sequence offset
	WORD		SizeOfUpdateSequence;	// Size in words of Update Sequence 
	ULONGLONG	LogSequenceNumber;		// $LogFile Sequence Number (LSN)
	WORD		SequenceNumber;			// Sequence number
	WORD		HardLinkCount;			// Hard link count
	WORD		FirstAttributeOffset;	// First attribute offset
	WORD		Flags;					
	DWORD		RealSize;				// Real size of the FILE record
	DWORD		AllocatedSize;			// Allocated size of the FILE record
	ULONGLONG	FileReference;			// File reference to the base FILE record
	WORD		NextAttributeId;		
	WORD		Align;
	DWORD		RecordNumber;			// MFT Record Number
}*PMFT_FILE_RECORD_HEADER;

/* Cluster info of Non-Residental Attributes (Data runs) */
typedef struct MFT_DATARUN
{
	UINT64	Length;
	INT64	Offset;
}*PMFT_DATARUN;

/*
* $MFT Standard Information Attribute
* http://inform.pucp.edu.pe/~inf232/Ntfs/ntfs_doc_v0.5/attributes/standard_information.html
*/
typedef struct MFT_STANDARD_INFORMATION_ATTRIBUTE_HDR
{
	LONGLONG	CreationTime;
	LONGLONG	ChangeTime;
	LONGLONG	LastWriteTime;
	LONGLONG	LastAccessTime;
	ULONG		FileAttributes;
	ULONG		Unknown[3];
	ULONG		QuotaId;
	ULONG		SecurityId;
	ULONGLONG	QuotaChange;
	USN			Usn;
}*PMFT_STANDARD_INFORMATION_ATTRIBUTE_HDR;

/*
* $FILE_NAME Attribute Header Layout
* https://flatcap.github.io/linux-ntfs/ntfs/attributes/file_name.html
*/
typedef struct MFT_FILENAME_ATTRIBUTE_HDR
{
	MFT_FILE_ID	ParentReference;	// File reference to the parent directory
	ULONGLONG	CreationTime;		// File creation time
	ULONGLONG	ModiciationTime;	// File altered time
	ULONGLONG	MFTTime;			// MFT changed time
	ULONGLONG	ReadTime;			// File read time
	ULONGLONG	AllocatedSize;		// Allocated size of the file
	ULONGLONG	RealSize;			// Real size of the file
	DWORD		Flags;				// Flags
	DWORD		ER;
	BYTE		NameLength;			// Filename length in characters
	BYTE		NameSpaceType;		// File namespace type
	WCHAR		Name[1];			// Filename
}*PMFT_FILENAME_ATTRIBUTE_HDR;

#pragma pack( pop )

/* $MFT Logical Cluster Number (Absolute offset of $MFT on the volume) */
#define $MFT_LCN(bootSector)	(UINT64)(bootSector.MFT_LCN * bootSector.BytesPerSector * bootSector.SectorsPerCluster)

#endif //!_NTFS_DEFS_H_
