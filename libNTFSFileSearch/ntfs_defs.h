#ifndef _NTFS_DEFS_H_
#define _NTFS_DEFS_H_

typedef UINT64	VCN_t;	/* Virtual Cluster Number */
typedef INT64	LCN_t;	/* Logical Cluster Number */

//--------------------------------------------------------------------------------------
//
// File Record Attribute Types
//
//--------------------------------------------------------------------------------------
#define	MFT_FILERECORD_ATTR_STANDARD_INFO           0x10	
#define	MFT_FILERECORD_ATTR_ATTRIBUTE_LIST          0x20
#define MFT_FILERECORD_ATTR_FILENAME                0x30
#define	MFT_FILERECORD_ATTR_OBJECT_ID               0x40
#define MFT_FILERECORD_ATTR_SECURITY_DESCRIPTOR	    0x50
#define	MFT_FILERECORD_ATTR_VOLUME_NAME	            0x60
#define	MFT_FILERECORD_ATTR_VOLUME_INFORMATION	    0x70
#define	MFT_FILERECORD_ATTR_DATA	                0x80
#define	MFT_FILERECORD_ATTR_INDEX_ROOT	            0x90
#define	MFT_FILERECORD_ATTR_INDEX_ALLOCATION	    0xA0
#define	MFT_FILERECORD_ATTR_BITMAP	                0xB0
#define	MFT_FILERECORD_ATTR_REPARSE_POINT	        0xC0
#define MFT_FILERECORD_ATTR_EA_INFORMATION	        0xD0
#define	MFT_FILERECORD_ATTR_EA	                    0xE0
#define	MFT_FILERECORD_ATTR_LOGGED_UTILITY_STREAM	0x100
#define	MFT_FILERECORD_ATTR_STOP_TAG		        0xFFFFFFFF


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

#pragma pack( push, 8 )

/*
* Resident Attribute Header Layout
* https://flatcap.github.io/linux-ntfs/ntfs/concepts/attribute_header.html#:~:text=Overview,attribute%20depends%20on%20two%20things.
*/
typedef struct MFT_NONRESIDENT_ATTRIBUTE_HDR
{
	VCN_t					StartVCN;				// Starting Virtual Cluster Number (VCN)
	VCN_t					LastVCN;				// Last Virtual Cluster Number (VCN)
	WORD					DataRunOffset;			// Starting offset of the Data Runs
	WORD					CompressionUnitSize;
	DWORD         Padding;
	ULONGLONG     AllocatedSize;
	ULONGLONG     RealSize;
	ULONGLONG     StreamDataSize;
}*PMFT_NONRESIDENT_ATTRIBUTE_HDR;

/*
* Non-Resident Attribute Header Layout
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
	DWORD		Type;			// Attribute Type
	DWORD		TotalSize;		
	BYTE		FormCode;	// 0 = Resident 1 = Non resident
	BYTE		NameLength;		// Attribute name length
	WORD		NameOffset;		// Attribute name offset
	WORD		Flags;
	WORD		AttributeId;	// Unique Id

	union
	{
		MFT_RESIDENT_ATTRIBUTE_HDR		Resdient;
		MFT_NONRESIDENT_ATTRIBUTE_HDR	NonResident;
	};
}*PMFT_ATTRIBUTE_HEADER;

#endif //!_NTFS_DEFS_H_
