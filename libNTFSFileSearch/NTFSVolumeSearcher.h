#pragma once

/* Search for files */
#define FILE_SEARCH_FLAG_FIND_FILES			0x00000001

/* Search for directories */
#define FILE_SEARCH_FLAG_FIND_DIRECTORIES	0x00000002

/* Search for files & directories */
#define FILE_SEARCH_FLAG_FIND_ALL			FILE_SEARCH_FLAG_FIND_FILES | FILE_SEARCH_FLAG_FIND_DIRECTORIES


class NTFSVolumeSearcher
{
private:
	/* Basic File Search Filter */
	struct FileFilterData
	{
		NTFS_FILTER_OPERAND fOperand;
		NTFS_FILTER_FACTOR	fFactorType;
		union
		{
			LPCWSTR lpszFilterValue;
			INT64	llFilterValue;
		};
	};

	CNTFSVolume					*m_pVolume;
	std::vector<FileFilterData>	m_rgFilters;
	CMemoryPool					m_FileNameMemoryPool;
	UINT64						m_nFileNameOffset;
	MFTDataRunList				m_rgDataRuns;
	NTFSDirectoryMap			m_DirectoryMap;
	NTFSFileMap					m_FileMap;

public:
	CMFTReader();

	~CMFTReader();

	BOOL SetVolume(CNTFSVolume *pVolume);

	BOOL AddFileFilter(NTFS_FILTER_OPERAND iOperand, NTFS_FILTER_FACTOR iFactorType, LPCWSTR lpszFilterString);

	BOOL AddFileFilter(NTFS_FILTER_OPERAND iOperand, NTFS_FILTER_FACTOR iFactorType, INT64 llFilterValue);

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

private:
	VOID ParseRecordChunk(DWORD dwFindFlags, PBYTE pbRecordChunk, UINT64 cbRecordChunk);

	LPWSTR DuplicateFileName(PMFT_FILENAME_ATTRIBUTE_HDR pFileName);

	VOID RemoveFileName(PNTFS_FILE_ENTRYW pFileEntry);
	
	BOOL CheckFileCriteria(PNTFS_FILE_ENTRYW pEntry, BOOL bFinalPath);

	VOID UpdateFileRecordMapDirectoryNames(NTFSDirectoryMap &mpDirectoryMap);

	VOID UpdateFileRecordMapFileNames(NTFSFileMap &mpFileMap, NTFSDirectoryMap mpDirectoryMap);
};

};
