#pragma once


//
// CNTFSVolumeSearcher::SetFlags() Flags
#define FILE_SEARCH_FLAG_FIND_FILES			0x00000001 /* Include file results in the volume search (Default) */
#define FILE_SEARCH_FLAG_FIND_DIRECTORIES	0x00000002 /* Include directory results in the volume search */
//#define FILE_SEARCH_FLAG_CACHE_RESULTS	0x00000004 

/* Search for files & directories */
#define FILE_SEARCH_FLAG_FIND_ALL			FILE_SEARCH_FLAG_FIND_FILES | FILE_SEARCH_FLAG_FIND_DIRECTORIES


class CNTFSVolumeSearcher
{
private:
	/* Basic File Search Filter */
	struct FileFilterData
	{
		NTFS_FILTER_OPERATOR	fOperator;
		NTFS_FILTER_FACTOR 		fFactorType;
		union
		{
			LPCWSTR lpszFilterValue;
			INT64	llFilterValue;
		};
	};

	CNTFSVolume					*m_pVolume;
	DWORD						m_dwFlags;
	std::vector<FileFilterData>	m_rgFilters;
	CMemoryPool					m_FileNameMemoryPool;
	UINT64						m_nFileNameOffset;
	MFTDataRunList				m_rgDataRuns;
	NTFSFileMap					m_DirectoryMap;
	NTFSFileMap					m_FileMap;

public:
	CNTFSVolumeSearcher();

	~CNTFSVolumeSearcher();

	BOOL AddFileFilter(NTFS_FILTER_OPERATOR fOperator, NTFS_FILTER_FACTOR iFactorType, LPCWSTR lpszFilterString);

	BOOL AddFileFilter(NTFS_FILTER_OPERATOR fOperator, NTFS_FILTER_FACTOR iFactorType, INT64 llFilterValue);

	VOID ClearFileFilters();

	/* Call HeapFree() or NTFSVolumeSearcher::FreeFileEntries() */
	BOOL FindFilesW(DWORD dwFindFlags, PNTFS_FILE_ENTRYW *ppFileEntries, UINT64 *pnNumberOfEntries);

	/* Call HeapFree() or NTFSVolumeSearcher::FreeFileEntries() */
	BOOL FindFilesA(DWORD dwFindFlags, PNTFS_FILE_ENTRYA *ppFileEntries, UINT64 *pnNumberOfEntries);

#ifdef UNICODE
#define FindFiles FindFilesW
#else
#define FindFiles FindFilesA
#endif // UNICODE

	VOID FreeFileEntries(PNTFS_FILE_ENTRYW pFileEntries);

	BOOL SetFlags(DWORD dwFlags);

	BOOL SetVolume(CNTFSVolume *pVolume);

private:
	VOID ParseRecordChunk(DWORD dwFindFlags, PBYTE pbRecordChunk, UINT64 cbRecordChunk);

	LPWSTR DuplicateFileName(PMFT_FILENAME_ATTRIBUTE_HDR pFileName);

	VOID RemoveFileName(PNTFS_FILE_ENTRYW pFileEntry);
	
	BOOL CheckFileCriteria(PNTFS_FILE_ENTRYW pEntry, BOOL bFinalPath);

	VOID UpdateFileRecordMapDirectoryNames(NTFSFileMap &mpDirectoryMap);

	VOID UpdateFileRecordMapFileNames(NTFSFileMap &mpFileMap, NTFSFileMap mpDirectoryMap);
};
