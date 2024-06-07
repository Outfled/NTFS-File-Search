#pragma once

#define POINTER_ADD(type, base, offset)                 (type)((PBYTE)base + offset)
#define POINTER_SUB(base, length)                       ((PBYTE)base - (PBYTE)length)


//----------------------------------
// Forward declarations

class	CNTFSVolume;
class	CMFTReader;
class	CMFTFileRecord;


typedef enum NTFS_FILTER_OPERAND
{
	FF_OPERAND_EQUAL,	
	FF_OPERAND_NOT_EQUAL,
	FF_OPERAND_GREATER_THAN,
	FF_OPERAND_GREATER_THAN_OR_EQ,
	FF_OPERAND_LESS_THAN,
	FF_OPERAND_LESS_THAN_OR_EQ,
};
typedef enum NTFS_FILTER_FACTOR
{
	FF_FACTOR_NAME,
	FF_FACTOR_NAME_AND_PATH,
	FF_FACTOR_SIZE,
	FF_FACTOR_RECORD_NUMBER,
	FF_FACTOR_PARENT_RECORD_NUMER
};


typedef struct NTFS_FILE_ENTRYW
{
	DWORD		AllocatedFileSize;		// File size on disk (bytes)
	DWORD		FileAttributes;			
	DWORD		NextEntryOffset;
	MFT_FILE_ID	MFTFileId;				// $MFT Record Number
	UINT64		ParentDirectoryRecord;	// Parent Directory $MFT-Record Number
	BOOL		IsDirectory;
	LPWSTR		lpszFileName;			// Full path of file/directory
}*PNTFS_FILE_ENTRYW;
typedef struct NTFS_FILE_ENTRYA
{
	DWORD		AllocatedFileSize;		// File size on disk (bytes)
	DWORD		FileAttributes;
	DWORD		NextEntryOffset;
	MFT_FILE_ID	MFTFileId;				// $MFT Record Number
	UINT64		ParentDirectoryRecord;	// Parent Directory $MFT-Record Number
	BOOL		IsDirectory;
	LPSTR		lpszFileName;			// Full path of file/directory
}*PNTFS_FILE_ENTRYA;
#ifdef _UNICODE
typedef NTFS_FILE_ENTRYW	NTFS_FILE_ENTRY;
typedef PNTFS_FILE_ENTRYW	PNTFS_FILE_ENTRY;
#else
typedef NTFS_FILE_ENTRYA	NTFS_FILE_ENTRY;
typedef PNTFS_FILE_ENTRYA	PNTFS_FILE_ENTRY;
#endif


typedef std::map<UINT64, PMFT_ATTRIBUTE_HEADER>			FileRecordAttrMap;
typedef std::multimap<UINT64, PMFT_ATTRIBUTE_HEADER>	RecordAttrMultiMap;
typedef std::vector<MFT_DATARUN>						MFTDataRunList;

typedef UINT64			FileMapKey_t;
typedef NTFS_FILE_ENTRY FileMapValue_t;
typedef std::unordered_map<FileMapKey_t, FileMapValue_t> NTFSFileMap; /* File/directory entry map */

class CMemoryPool
{
public:
	int						m_nMaximumBlocks;
	UINT64					m_cbFixedBlockSize;
	LPVOID					m_lpBaseAddress;
	LPVOID					m_lpPreviousBlockEndPtr;
	std::vector<LPVOID>		m_rgCommittedBlocks;

public:
	CMemoryPool() : m_nMaximumBlocks(0), m_cbFixedBlockSize(0), m_lpBaseAddress(NULL), m_lpPreviousBlockEndPtr(NULL)
	{

	}
	CMemoryPool(UINT32 cbBlockLength, int nMaximumBlockCount) : CMemoryPool()
	{
		Initialize(cbBlockLength, nMaximumBlockCount);
	}

	~CMemoryPool()
	{
		Uninitialize();
	}

	/* Reserve's nMaximumBlockCount amount of memory-blocks each with a length of cbBlockLength */
	BOOL Initialize(UINT32 cbBlockLength, int nMaximumBlockCount)
	{
		if (IsValid() || !cbBlockLength || !nMaximumBlockCount)
		{
			return FALSE;
		}

		m_rgCommittedBlocks.clear();

		//
		// Align the length
		//
		DWORD dwPageSize = GetSystemPageSize();
		while (cbBlockLength % dwPageSize != 0) {
			++cbBlockLength;
		}

		/* Reserve continiguous memory */
		m_lpBaseAddress = VirtualAlloc(NULL, cbBlockLength * nMaximumBlockCount, MEM_RESERVE, PAGE_READWRITE);
		if (m_lpBaseAddress)
		{
			m_cbFixedBlockSize		= cbBlockLength;
			m_nMaximumBlocks		= nMaximumBlockCount;
			m_lpPreviousBlockEndPtr	= m_lpBaseAddress;
		}

		return (m_lpBaseAddress != NULL) ? TRUE : FALSE;
	}

	/* Decommit & free all memory blocks and invalidates the CMemoryPool object until Initialize() is re-called */
	BOOL Uninitialize()
	{
		if (IsValid())
		{
			if (m_rgCommittedBlocks.size())
			{
				/* Decommit each block */
				for (int i = m_rgCommittedBlocks.size() - 1; i >= 0; --i)
				{
					DecomitBlock(m_rgCommittedBlocks[i]);
					m_rgCommittedBlocks.erase(m_rgCommittedBlocks.begin() + i);
				}
			}
			
			/* Free the continiguous memory */
			VirtualFree(m_lpBaseAddress, 0, MEM_RELEASE);
			m_lpBaseAddress = NULL;

			/* Reset the pool properties */
			m_lpPreviousBlockEndPtr = NULL;
			m_nMaximumBlocks		= 0;
			m_cbFixedBlockSize		= 0;

			return TRUE;
		}

		return FALSE;
	}

	/* Commit a new block of memory */
	LPVOID CommitNextMemoryBlock()
	{
		LPVOID lpMemoryBlock = NULL;
		if (IsValid())
		{
			/* Ensure more blocks are available */
			if (CommittedBlockCount() < m_nMaximumBlocks)
			{
				/* Commit the next block */
				lpMemoryBlock = VirtualAlloc(m_lpPreviousBlockEndPtr, m_cbFixedBlockSize, MEM_COMMIT, PAGE_READWRITE);
				if (lpMemoryBlock)
				{
					m_lpPreviousBlockEndPtr = POINTER_ADD(LPVOID, lpMemoryBlock, m_cbFixedBlockSize);
					m_rgCommittedBlocks.push_back(lpMemoryBlock);
				}
			}
		}

		return lpMemoryBlock;
	}

	/* Retrieve the most recently committed memory block from CommitNextMemoryBlock() */
	LPVOID GetMemoryBlock()
	{
		LPVOID lpMemoryBlock = NULL;
		if (IsValid())
		{
			if (m_rgCommittedBlocks.size() > 0) {
				lpMemoryBlock = m_rgCommittedBlocks[m_rgCommittedBlocks.size() - 1];
			}
		}

		return lpMemoryBlock;
	}

	/* Get contiguous memory base address of the block pool */
	LPVOID GetContiguousBuffer(UINT64 *pcbTotalLength)
	{
		if (pcbTotalLength == NULL)
		{
			return FALSE;
		}

		*pcbTotalLength = m_rgCommittedBlocks.size() * m_cbFixedBlockSize;
		if (IsValid())
		{
			return (m_rgCommittedBlocks.size() > 0) ? m_lpBaseAddress : 0;
		}

		return FALSE;
	}

	/* Decommit a (previously committed from CommitNextMemoryBlock()) memory block */
	BOOL DecommitMemoryBlock(LPVOID lpMemoryBlockAddress)
	{
		if (IsValid())
		{
			if (m_rgCommittedBlocks.size())
			{
				for (int i = m_rgCommittedBlocks.size() - 1; i >= 0; --i)
				{
					if (m_rgCommittedBlocks[i] == lpMemoryBlockAddress)
					{
						DecomitBlock(m_rgCommittedBlocks[i]);
						m_rgCommittedBlocks.erase(m_rgCommittedBlocks.begin() + i);

						return TRUE;
					}	
				}
			}
		}

		return FALSE;
	}

	/* Decommit all blocks */
	BOOL DecommitAllMemoryBlocks()
	{
		if (IsValid())
		{
			if (IsValid())
			{
				if (m_rgCommittedBlocks.size())
				{
					for (int i = m_rgCommittedBlocks.size() - 1; i >= 0; --i)
					{
						DecomitBlock(m_rgCommittedBlocks[i]);
						m_rgCommittedBlocks.erase(m_rgCommittedBlocks.begin() + i);
					}

					return TRUE;
				}
			}
		}

		return FALSE;
	}

	/* Is the memory pool initialized (reserved in memory) */
	BOOL IsValid() const {
		return (m_lpBaseAddress != NULL) ? TRUE : FALSE;
	}

	/* The fixed length of each memory block */
	UINT64 BlockLength() const {
		return m_cbFixedBlockSize;
	}

	/* Maximum number of memory blocks to be committed from CommitNextMemoryBlock() */
	INT MaximumBlockCount() const {
		return m_nMaximumBlocks;
	}

	/* Current number of committed memory blocks */
	UINT64 CommittedBlockCount() const {
		return m_rgCommittedBlocks.size();
	}

	/* The sum of all currently committed memory blocks */
	UINT64 ContiguousMemoryLength() const {
		return (CommittedBlockCount() * m_cbFixedBlockSize);
	}

private:
	DWORD GetSystemPageSize()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);

		return si.dwPageSize;
	}

	VOID DecomitBlock(LPVOID lpBlock) {
		VirtualFree(lpBlock, m_cbFixedBlockSize, MEM_DECOMMIT);
	}
};
