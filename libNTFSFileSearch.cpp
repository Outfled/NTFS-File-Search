#include "pch.h"
#include "NTFSVolumeSearcher.h"
#include "mft_filerecord.h"

#define FILE_RECORDS_PER_FILE_BUF			65536

#define FILENAME_MEM_POOL_BLOCK_SIZE        30 * 1024 * 1024                                    /* 30 MB */
#define FILENAME_MEM_POOL_MAX_BLOCKS		999 / FILENAME_MEM_POOL_BLOCK_SIZE / 1024 / 1024    /* 999 / 30 (33) */

#define ROOT_VOLUME_FILE_RECORD_NUMBER      5

#define FILENAME_NAMESPACE_POSIX			(BYTE)0
#define FILENAME_NAMESPACE_WIN32			(BYTE)1
#define FILENAME_NAMESPACE_DOS				(BYTE)2
#define FILENAME_NAMESPACE_WIN32_AND_DOS	(BYTE)3

static inline void CopyFileEntry(PNTFS_FILE_ENTRYW pDestination, PNTFS_FILE_ENTRYW pSource, UINT64 *pnOffset, UINT64 cbTotalSize)
{
    LPWSTR pFileName = (LPWSTR)(pDestination + 1);
    CopyMemory(pFileName, pSource->lpszFileName, (wcslen(pSource->lpszFileName) + 1) * sizeof(WCHAR));

    DWORD dwTotalSize = sizeof(NTFS_FILE_ENTRYW) + ((wcslen(pFileName) + 1) * sizeof(WCHAR));
    *pnOffset += dwTotalSize;

    pDestination->AllocatedFileSize     = pSource->AllocatedFileSize;
    pDestination->FileAttributes        = pSource->FileAttributes;
    pDestination->MFTFileId             = pSource->MFTFileId;
    pDestination->ParentDirectoryRecord = pSource->ParentDirectoryRecord;
    pDestination->IsDirectory           = pSource->IsDirectory;
    pDestination->lpszFileName          = pFileName;
    pDestination->NextEntryOffset       = (*pnOffset < cbTotalSize) ? *pnOffset : 0;
}

static inline void CopyFileEntry(PNTFS_FILE_ENTRYA pDestination, PNTFS_FILE_ENTRYW pSource, UINT64 *pnOffset, UINT64 cbTotalSize)
{
    DWORD cbNameSize = WideCharToMultiByte(CP_UTF8, 0, pSource->lpszFileName, -1, NULL, 0, NULL, NULL);
    if (cbNameSize > 0)
    {
        LPSTR pFileName = (LPSTR)(pDestination + 1);
        WideCharToMultiByte(CP_UTF8, 0, pSource->lpszFileName, -1, pFileName, cbNameSize, NULL, NULL);

        DWORD dwTotalSize = sizeof(NTFS_FILE_ENTRYA) + cbNameSize;
        *pnOffset += dwTotalSize;

        pDestination->AllocatedFileSize     = pSource->AllocatedFileSize;
        pDestination->FileAttributes        = pSource->FileAttributes;
        pDestination->MFTFileId             = pSource->MFTFileId;
        pDestination->ParentDirectoryRecord = pSource->ParentDirectoryRecord;
        pDestination->IsDirectory           = pSource->IsDirectory;
        pDestination->lpszFileName          = pFileName;
        pDestination->NextEntryOffset       = (*pnOffset < cbTotalSize) ? *pnOffset : 0;
    }
}


CMFTReader::CMFTReader() : m_pVolume(NULL), m_nFileNameOffset(0)
{
    m_FileNameMemoryPool.Initialize(FILENAME_MEM_POOL_BLOCK_SIZE, FILENAME_MEM_POOL_MAX_BLOCKS);
}
CMFTReader::~CMFTReader()
{
    ClearFileFilters();
    m_FileNameMemoryPool.Uninitialize();
}

BOOL CMFTReader::SetVolume(CNTFSVolume *pVolume)
{
    m_pVolume = pVolume;
    if (m_pVolume)
    {
        //
        // Read the $MFT
        //
        PBYTE pbMFTBuffer = new BYTE[m_pVolume->RecordSize()]{};
        if (m_pVolume->RecordSize() == m_pVolume->ReadBytes(pbMFTBuffer, m_pVolume->RecordSize(), $MFT_LCN(m_pVolume->BootSector())))
        {
            CMFTFileRecord mftFileRecord(m_pVolume, (PMFT_FILE_RECORD_HEADER)pbMFTBuffer);
            if (mftFileRecord.IsValid())
            {
                mftFileRecord.ApplyFixup();

                /* Get attributes map */
                RecordAttrMultiMap mpMFTAttributes = mftFileRecord.GetAttributes();

                /* Read the $DATA attribute and get the data runs */
                if (mpMFTAttributes.find(MFT_FILERECORD_ATTR_DATA) != mpMFTAttributes.end()) {
                    mftFileRecord.GetDataRuns(mpMFTAttributes.find(MFT_FILERECORD_ATTR_DATA)->second, &m_rgDataRuns);
                }
            }
        }

        delete[] pbMFTBuffer;
        return (m_rgDataRuns.size() > 0) ? TRUE : FALSE;
    }

    return TRUE;
}

BOOL CMFTReader::AddFileFilter(NTFS_FILTER_OPERAND iOperand, NTFS_FILTER_FACTOR iFactorType, LPCWSTR lpszFilterString)
{
    FileFilterData Filter;

    if (iOperand != FF_OPERAND_EQUAL && iOperand != FF_OPERAND_NOT_EQUAL)
    {
        return FALSE;
    }
    if (iFactorType != FF_FACTOR_NAME && iFactorType != FF_FACTOR_NAME_AND_PATH || !lpszFilterString)
    {
        return FALSE;
    }

    Filter.fOperand         = iOperand;
    Filter.fFactorType      = iFactorType;
    Filter.lpszFilterValue  = _wcsdup(lpszFilterString);

    m_rgFilters.push_back(Filter);
    return TRUE;
}

BOOL CMFTReader::AddFileFilter(NTFS_FILTER_OPERAND iOperand, NTFS_FILTER_FACTOR iFactorType, INT64 ullFilterValue)
{
    FileFilterData Filter;
    if ((iFactorType == FF_FACTOR_NAME) && (iFactorType == FF_FACTOR_NAME_AND_PATH))
    {
        return FALSE;
    }

    Filter.fOperand         = iOperand;
    Filter.fFactorType      = iFactorType;
    Filter.llFilterValue    = ullFilterValue;

    m_rgFilters.push_back(Filter);
    return TRUE;
}

VOID CMFTReader::ClearFileFilters()
{
    for (int i = 0; i < m_rgFilters.size(); ++i) {
        if (m_rgFilters[i].fFactorType == FF_FACTOR_NAME || m_rgFilters[i].fFactorType == FF_FACTOR_NAME_AND_PATH) {
            free((LPWSTR)m_rgFilters[i].lpszFilterValue);
        }
    }

    m_rgFilters.clear();
}

BOOL CMFTReader::FindFilesW(DWORD dwFindFlags, PNTFS_FILE_ENTRYW *ppFileEntries, UINT64 *pnNumberOfEntries)
{
    PBYTE           pbFileRecordBuffer;
    DWORD           cbFileRecordBuffer;
	UINT64			ullVolumeOffset;
    DWORD           cbTotalSizeNeeded;

	m_nFileNameOffset	= 0;
	ullVolumeOffset		= 0;
    cbTotalSizeNeeded   = 0;

    if (!ppFileEntries)
    {
        return FALSE;
    }

    *ppFileEntries = NULL;
    (pnNumberOfEntries) ? *pnNumberOfEntries = 0: 0;

    /* Invalid call */
    if (!m_pVolume || !m_rgDataRuns.size())
    {
        return FALSE;
    }

	//
	// Allocate buffers
	//
    cbFileRecordBuffer = FILE_RECORDS_PER_FILE_BUF * m_pVolume->RecordSize();
	if (!(pbFileRecordBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbFileRecordBuffer)))
	{
        return FALSE;
	}
    if (!m_FileNameMemoryPool.IsValid())
    {
        if (!m_FileNameMemoryPool.Initialize(FILENAME_MEM_POOL_BLOCK_SIZE, FILENAME_MEM_POOL_MAX_BLOCKS))
        {
            HeapFree(GetProcessHeap(), 0, pbFileRecordBuffer);
            return FALSE;
        }
    }

    /* Commit first file name memory block */
    m_FileNameMemoryPool.CommitNextMemoryBlock();

    /* Enumerate the data runs */
	for (size_t i = 0; i < m_rgDataRuns.size(); ++i)
	{
        //UINT64 nRemainingFiles = RunLength * m_pVolume->ClusterSize() / m_pVolume->RecordSize();

		VCN_t RunLength = m_rgDataRuns[i].Length;
		ullVolumeOffset += m_rgDataRuns[i].Offset;

		/* Read the files in chunks */
		for (UINT64 nChunkOffset = 0; nChunkOffset < RunLength * m_pVolume->ClusterSize();)
		{
            /* Determine the number of files to read */
            UINT64 cbReadLength = cbFileRecordBuffer;
			if (RunLength * m_pVolume->ClusterSize() - nChunkOffset < cbReadLength) {
				cbReadLength = RunLength * m_pVolume->ClusterSize() - nChunkOffset;
			}

            /* Read the file records from the volume */
			DWORD cbBytesRead = m_pVolume->ReadBytes(pbFileRecordBuffer, cbReadLength, ullVolumeOffset * m_pVolume->ClusterSize() + nChunkOffset);
			if (!cbBytesRead) {
				break;
			}

			nChunkOffset += cbReadLength;
			
            /* Parse the records chunk */
            ParseRecordChunk(dwFindFlags, pbFileRecordBuffer, cbReadLength);
            //nRemainingFiles -= cbReadLength / m_pVolume->RecordSize();
		}
	}

    /* Free the file record buffer */
    HeapFree(GetProcessHeap(), 0, pbFileRecordBuffer);

    //
    // Build the full paths of each file record entry
    //
    UpdateFileRecordMapDirectoryNames(m_DirectoryMap);
    UpdateFileRecordMapFileNames(m_FileMap, m_DirectoryMap);

    /* Free the file name buffer now that the file names have been updated */
    m_FileNameMemoryPool.DecommitAllMemoryBlocks();

    //
    // Determine the total size needed for the file entries array & free up unnecessary memory
    //
    for (auto &fDirEntry : m_DirectoryMap)
    {
        if ((dwFindFlags & FILE_SEARCH_FLAG_FIND_DIRECTORIES) && CheckFileCriteria(&fDirEntry.second, TRUE))
        {
            cbTotalSizeNeeded += sizeof(NTFS_FILE_ENTRY) +
                ((wcslen(fDirEntry.second.lpszFileName)) + 1) * sizeof(WCHAR);
        }
        else
        {
            free(fDirEntry.second.lpszFileName);
            fDirEntry.second.lpszFileName = NULL;
        }
    }
    for (auto &fFileEntry : m_FileMap)
    {
        if ((dwFindFlags & FILE_SEARCH_FLAG_FIND_FILES) && CheckFileCriteria(&fFileEntry.second, TRUE))
        {
            cbTotalSizeNeeded += sizeof(NTFS_FILE_ENTRY) +
                ((wcslen(fFileEntry.second.lpszFileName)) + 1) * sizeof(WCHAR);
        }
        else
        {
            free(fFileEntry.second.lpszFileName);
            fFileEntry.second.lpszFileName = NULL;
        }
    }
    
    /* Copy the entries */
    if (cbTotalSizeNeeded)
    {
        LPVOID lpEntries;
        UINT64 nEntryOffset;

        /* Allocate contiguous block of memory for the entries array */
        lpEntries = HeapAlloc(GetProcessHeap(), 0, cbTotalSizeNeeded);
        if (lpEntries)
        {
            nEntryOffset    = 0;
            *ppFileEntries  = (PNTFS_FILE_ENTRY)lpEntries;

            if (dwFindFlags & FILE_SEARCH_FLAG_FIND_DIRECTORIES)
            {
                for (auto &fDirEntry : m_DirectoryMap)
                {
                    NTFS_FILE_ENTRY fDirInfo = fDirEntry.second;
                    if (fDirInfo.lpszFileName)
                    {
                        CopyFileEntry(POINTER_ADD(PNTFS_FILE_ENTRY, lpEntries, nEntryOffset),
                            &fDirInfo,
                            &nEntryOffset,
                            cbTotalSizeNeeded
                        );
                        (pnNumberOfEntries) ? ++(*pnNumberOfEntries) : 0;

                        /* Free original file (directory) name */
                        free(fDirInfo.lpszFileName);
                        fDirInfo.lpszFileName = NULL;
                    }
                }
            }
            if (dwFindFlags & FILE_SEARCH_FLAG_FIND_FILES)
            {
                for (auto &fFileEntry : m_FileMap)
                {
                    NTFS_FILE_ENTRY fFileInfo = fFileEntry.second;
                    if (fFileInfo.lpszFileName)
                    {
                        CopyFileEntry(POINTER_ADD(PNTFS_FILE_ENTRY, lpEntries, nEntryOffset),
                            &fFileInfo,
                            &nEntryOffset,
                            cbTotalSizeNeeded
                        );
                        (pnNumberOfEntries) ? ++(*pnNumberOfEntries) : 0;

                        /* Free original file name */
                        free(fFileInfo.lpszFileName);                    
                    }
                }
            }
        }
    }

    m_DirectoryMap.clear();
    m_FileMap.clear();

    return (*ppFileEntries != NULL) ? TRUE : (cbTotalSizeNeeded == 0 ? TRUE : FALSE);
}

BOOL CMFTReader::FindFilesA(DWORD dwFindFlags, PNTFS_FILE_ENTRYA *ppFileEntries, UINT64 *pnNumberOfEntries)
{
    BOOL                bSuccess;
    PNTFS_FILE_ENTRYW   pWideEntries;
    UINT64              nWideEntries;

    if (!ppFileEntries)
    {
        return FALSE;
    }

    *ppFileEntries = NULL;
    (pnNumberOfEntries) ? *pnNumberOfEntries = 0 : 0;

    bSuccess = FindFilesW(dwFindFlags, &pWideEntries, &nWideEntries);
    if (bSuccess && pWideEntries && nWideEntries)
    {
        UINT64              cbTotalSizeNeeded;
        PNTFS_FILE_ENTRYW   pWideEntry;
        UINT64              ullOffset;

        cbTotalSizeNeeded   = sizeof(NTFS_FILE_ENTRYA) * nWideEntries;
        pWideEntry          = pWideEntries;
        ullOffset           = 0;

        for (UINT64 nEntry = 0; pWideEntry && nEntry < nWideEntries; ++nEntry)
        {
            DWORD cbStringSize = WideCharToMultiByte(CP_UTF8, 0, pWideEntry->lpszFileName, -1, NULL, 0, NULL, NULL);
            cbTotalSizeNeeded += cbStringSize;

            if (pWideEntry->NextEntryOffset != 0)
            {
                pWideEntry = POINTER_ADD(PNTFS_FILE_ENTRYW, pWideEntries, pWideEntry->NextEntryOffset);
                continue;
            }

            break;
        }

        *ppFileEntries = (PNTFS_FILE_ENTRYA)HeapAlloc(GetProcessHeap(), 0, cbTotalSizeNeeded);
        if (*ppFileEntries)
        {
            pWideEntry = pWideEntries;
            for (UINT64 nEntry = 0; pWideEntry && nEntry < nWideEntries; ++nEntry)
            {
                CopyFileEntry(POINTER_ADD(PNTFS_FILE_ENTRYA, *ppFileEntries, ullOffset),
                    pWideEntry,
                    &ullOffset,
                    cbTotalSizeNeeded
                );
                (pnNumberOfEntries) ? ++(*pnNumberOfEntries) : 0;

                if (pWideEntry->NextEntryOffset != 0)
                {
                    pWideEntry = POINTER_ADD(PNTFS_FILE_ENTRYW, pWideEntries, pWideEntry->NextEntryOffset);
                    continue;
                }
                
                break;
            }
        }
    }

    FreeFileEntries(pWideEntries);
    return bSuccess;
}

VOID CMFTReader::FreeFileEntries(PNTFS_FILE_ENTRYW pFileEntries)
{
    if (pFileEntries) {
        HeapFree(GetProcessHeap(), 0, pFileEntries);
    }
}

VOID CMFTReader::ParseRecordChunk(DWORD dwFindFlags, PBYTE pbRecordChunk, UINT64 cbRecordChunk)
{
	for (UINT64 iRecord = 0; iRecord < cbRecordChunk / m_pVolume->RecordSize(); ++iRecord)
	{
		CMFTFileRecord enumFileRecord(m_pVolume, POINTER_ADD(PMFT_FILE_RECORD_HEADER, pbRecordChunk, iRecord * m_pVolume->RecordSize()));
		if (!(enumFileRecord.Flags() & MFT_FILERECORD_FLAG_IN_USE))
		{
			continue;
		}

		enumFileRecord.ApplyFixup();

        /* Get the attributes of the current record  */
		RecordAttrMultiMap mpAttributes = enumFileRecord.GetAttributes();
		auto FileNamesList = mpAttributes.equal_range(MFT_FILERECORD_ATTR_FILENAME);

        /* Find a valid (non-DOS file-name namespace) $FILE_NAME attribute */
		for (auto i = FileNamesList.first; i != FileNamesList.second; ++i)
		{
			PMFT_FILENAME_ATTRIBUTE_HDR pFileName = POINTER_ADD(PMFT_FILENAME_ATTRIBUTE_HDR,
				i->second,
				i->second->Resdient.AttributeOffset
			);
			if (pFileName && pFileName->NameSpaceType != FILENAME_NAMESPACE_DOS)
			{
                NTFS_FILE_ENTRY FileEntry;

                //
                // Set the file entry info
                //
                FileEntry.AllocatedFileSize         = pFileName->AllocatedSize;
                FileEntry.FileAttributes            = 0;
                FileEntry.MFTFileId.MftRecordIndex	= enumFileRecord.RecordNumber();
                FileEntry.MFTFileId.SequenceNumber  = enumFileRecord.SequenceNumber();
                FileEntry.IsDirectory			    = (enumFileRecord.Flags() & MFT_FILERECORD_FLAG_IS_DIRECTORY) ? TRUE : FALSE;
                FileEntry.ParentDirectoryRecord     = pFileName->ParentReference.MftRecordIndex;
                FileEntry.lpszFileName			    = DuplicateFileName(pFileName);
                if (enumFileRecord.FindAttribute(MFT_FILERECORD_ATTR_STANDARD_INFO))
                {
                    PMFT_STANDARD_INFORMATION_ATTRIBUTE_HDR pStandardInfo = POINTER_ADD(
                        PMFT_STANDARD_INFORMATION_ATTRIBUTE_HDR,
                        enumFileRecord.FindAttribute(MFT_FILERECORD_ATTR_STANDARD_INFO),
                        enumFileRecord.FindAttribute(MFT_FILERECORD_ATTR_STANDARD_INFO)->Resdient.AttributeOffset
                    );
                    FileEntry.FileAttributes = pStandardInfo->FileAttributes;
                }

                /* Check if file should be skipped */
                if (FileEntry.IsDirectory == FALSE || !FileEntry.lpszFileName)
                {
                    //
                    // Directory entries are always required in order to build the full paths of other entries
                    // Files are not always required and should be checked to prevent extra unnecessary memory consumption
                    //
                    if (!(dwFindFlags & FILE_SEARCH_FLAG_FIND_FILES) || !CheckFileCriteria(&FileEntry, FALSE))
                    {
                        RemoveFileName(&FileEntry);
                        break;
                    }
                }

                /* Add entry to map */
				if (FileEntry.IsDirectory) {
					m_DirectoryMap[enumFileRecord.RecordNumber()] = FileEntry;
				}
				else {
					m_FileMap[enumFileRecord.RecordNumber()] = FileEntry;
				}

				break;
			}
		}
	}
}

LPWSTR CMFTReader::DuplicateFileName(PMFT_FILENAME_ATTRIBUTE_HDR pFileName)
{
    LPVOID pNameBlock = m_FileNameMemoryPool.GetMemoryBlock();
    if (pNameBlock == NULL || (m_nFileNameOffset + (pFileName->NameLength + 1) * sizeof(WCHAR) > m_FileNameMemoryPool.BlockLength()))
    {
        if (m_FileNameMemoryPool.CommittedBlockCount() + 1 > m_FileNameMemoryPool.MaximumBlockCount())
        {
            /* No more buffers left */
            return NULL;
        }

        pNameBlock = (LPWSTR)m_FileNameMemoryPool.CommitNextMemoryBlock();
        if (!pNameBlock)
        {
            return NULL;
        }

        m_nFileNameOffset = 0;
    }

    LPWSTR lpszFileName = POINTER_ADD(LPWSTR, pNameBlock, m_nFileNameOffset);

    CopyMemory(lpszFileName, pFileName->Name, pFileName->NameLength * sizeof(WCHAR));
    lpszFileName[pFileName->NameLength] = L'\0';

	m_nFileNameOffset += (pFileName->NameLength + 1) * sizeof(WCHAR);
	return lpszFileName;
}

VOID CMFTReader::RemoveFileName(PNTFS_FILE_ENTRYW pFileEntry)
{
    if (pFileEntry->lpszFileName)
    {
        LPVOID pPrevStartPtr = POINTER_ADD(LPVOID,
            m_FileNameMemoryPool.GetMemoryBlock(),
            m_nFileNameOffset - ((wcslen(pFileEntry->lpszFileName) + 1) * sizeof(WCHAR))
        );
        RtlFillMemory(pPrevStartPtr, (wcslen(pFileEntry->lpszFileName) + 1) * sizeof(WCHAR), L'\0');

        m_nFileNameOffset -= (wcslen(pFileEntry->lpszFileName) + 1) * sizeof(WCHAR);
    }
}

inline BOOL CMFTReader::CheckFileCriteria(PNTFS_FILE_ENTRYW pEntry, BOOL bFinalPath)
{
    for (size_t i = 0; i < m_rgFilters.size(); ++i)
    {
        INT64   iOperatorValue;
        BOOL    bValid;

        bValid = TRUE;

        auto Filter = m_rgFilters[i];
        switch (Filter.fFactorType)
        {
        case FF_FACTOR_NAME_AND_PATH:
            if (!bFinalPath)
            {
                if (Filter.fOperand == FF_OPERAND_EQUAL && (wcscmp(PathFindFileName(Filter.lpszFilterValue), pEntry->lpszFileName) != 0)) {
                    return FALSE;
                }
                else if (Filter.fFactorType == FF_OPERAND_NOT_EQUAL && !(wcscmp(PathFindFileName(Filter.lpszFilterValue), pEntry->lpszFileName) == 0)) {
                    return FALSE;
                }

                continue;
            }

        case FF_FACTOR_NAME:
            if (Filter.fOperand == FF_OPERAND_EQUAL && (wcscmp(Filter.lpszFilterValue, PathFindFileName(pEntry->lpszFileName)) != 0)) {
                return FALSE;
            }
            else if (Filter.fFactorType == FF_OPERAND_NOT_EQUAL && !(wcscmp(Filter.lpszFilterValue, PathFindFileName(pEntry->lpszFileName)) == 0)) {
                return FALSE;
            }

            continue;

        case FF_FACTOR_SIZE:
            iOperatorValue = pEntry->AllocatedFileSize;
            break;
        case FF_FACTOR_RECORD_NUMBER:
            iOperatorValue = pEntry->MFTFileId.MftRecordIndex;
            break;
        case FF_FACTOR_PARENT_RECORD_NUMER:
            iOperatorValue = pEntry->ParentDirectoryRecord;
            break;
        default:
            continue;
        }

        /* Arithmetic operations */
        switch (Filter.fOperand)
        {
        case FF_OPERAND_EQUAL:
            bValid = (iOperatorValue == Filter.llFilterValue) ? TRUE : FALSE;
            break;
        case FF_OPERAND_NOT_EQUAL:
            bValid = (iOperatorValue != Filter.llFilterValue) ? TRUE : FALSE;
            break;
        case FF_OPERAND_GREATER_THAN:
            bValid = (iOperatorValue > Filter.llFilterValue) ? TRUE : FALSE;
            break;
        case FF_OPERAND_GREATER_THAN_OR_EQ:
            bValid = (iOperatorValue >= Filter.llFilterValue) ? TRUE : FALSE;
            break;
        case FF_OPERAND_LESS_THAN:
            bValid = (iOperatorValue < Filter.llFilterValue) ? TRUE : FALSE;
            break;
        case FF_OPERAND_LESS_THAN_OR_EQ:
            bValid = (iOperatorValue <= Filter.llFilterValue) ? TRUE : FALSE;
            break;
        }
        if (!bValid) {
            return FALSE;
        }
    }

    return TRUE;
}

VOID CMFTReader::UpdateFileRecordMapDirectoryNames(NTFSDirectoryMap &mpDirectoryMap)
{
    using namespace std;
    vector<UINT64> rgEraseRecords;

    /* Update the volume root path (ie C:, D:, F:, etc) */
    mpDirectoryMap.at(ROOT_VOLUME_FILE_RECORD_NUMBER).lpszFileName = _wcsdup(m_pVolume->RootPath());

    for (auto i = mpDirectoryMap.begin(); i != mpDirectoryMap.end(); ++i)
    {
        FileMapKey_t    fRecordNumber;
        FileMapValue_t  fEntryInfo;
        vector<LPWSTR>  rgAllPaths;
        SIZE_T          cchFullPath;
        LPWSTR          lpszFullPath;
        BOOL            bAddRootPath;
        FileMapValue_t  fParentEntryInfo;

        fRecordNumber   = i->first;
        fEntryInfo      = i->second;
        bAddRootPath    = TRUE;
        cchFullPath     = 0;
        ZeroMemory(&fParentEntryInfo, sizeof(fParentEntryInfo));

        /* The root volume path file record */
        if (fRecordNumber == ROOT_VOLUME_FILE_RECORD_NUMBER)
        {
            continue;
        }

        //
        // Check if directory entry should be skipped
        //
        if (mpDirectoryMap.end() != mpDirectoryMap.find(fEntryInfo.ParentDirectoryRecord))
        {
            fParentEntryInfo = mpDirectoryMap[fEntryInfo.ParentDirectoryRecord];
        }
        if (fParentEntryInfo.lpszFileName == NULL)
        {
            /* Parent directory file-record not found in map */
            rgEraseRecords.push_back(fRecordNumber);
            continue;
        }
        if (wcsstr(fEntryInfo.lpszFileName, L"\\") != NULL)
        {
            /* Current directory has already been updated */
            continue;
        }

        /* The parent path of the record has already been updated to the full path */
        if (wcsstr(fParentEntryInfo.lpszFileName, L":") != NULL)
        {
            cchFullPath = wcslen(fParentEntryInfo.lpszFileName) + wcslen(L"\\") + wcslen(fEntryInfo.lpszFileName);
            lpszFullPath    = (LPWSTR)malloc((cchFullPath + 1) * sizeof(WCHAR));

            if (!lpszFullPath)
            {
                rgEraseRecords.push_back(fRecordNumber);
                continue;
            }

            /* Append the current record name to the end of the parent path */
            wcscpy_s(lpszFullPath, cchFullPath + 1, fParentEntryInfo.lpszFileName);
            if (lpszFullPath[wcslen(lpszFullPath) - 1] != L'\\') {
                wcscat_s(lpszFullPath, cchFullPath + 1, L"\\");
            }

            wcscat_s(lpszFullPath, cchFullPath + 1, fEntryInfo.lpszFileName);

            /* Update the path (do not free()) */
            mpDirectoryMap.at(fRecordNumber).lpszFileName = lpszFullPath;
            continue;
        }

        /* Loop & get all parent path names until the root path is hit */
        for (FileMapValue_t fEnumParent = fParentEntryInfo; fEnumParent.MFTFileId.MftRecordIndex != 5;)
        {
            rgAllPaths.push_back(fEnumParent.lpszFileName);
            if (wcsstr(fEnumParent.lpszFileName, L"\\") != NULL)
            {
                /* This parent path has already been updated to the full path */
                bAddRootPath = FALSE;
                break;
            }

            if (find(rgEraseRecords.begin(), rgEraseRecords.end(), fEnumParent.ParentDirectoryRecord) != rgEraseRecords.end() ||
                mpDirectoryMap.find(fEnumParent.ParentDirectoryRecord) == mpDirectoryMap.end() ||
                mpDirectoryMap.at(fEnumParent.ParentDirectoryRecord).lpszFileName == NULL)
            {
                rgAllPaths.clear();
                break;
            }

            fEnumParent = mpDirectoryMap.at(fEnumParent.ParentDirectoryRecord);
        }

        if (rgAllPaths.size() > 0)
        {
            /*
            * None of the parent paths of the current directory have been updated so we
            * append the root volume path to the path array
            */
            if (bAddRootPath)
            {
                rgAllPaths.push_back(mpDirectoryMap[ROOT_VOLUME_FILE_RECORD_NUMBER].lpszFileName);
            }

            /* Reverse the path array to correct the append order for the full path */
            reverse(rgAllPaths.begin(), rgAllPaths.end());

            /* Get the size needed of the full path */
            cchFullPath = wcslen(fEntryInfo.lpszFileName);
            for (size_t i = 0; i < rgAllPaths.size(); ++i)
            {
                cchFullPath += wcslen(rgAllPaths[i]);
                if (rgAllPaths[i][wcslen(rgAllPaths[i]) - 1] != L'\\') {
                    cchFullPath += wcslen(L"\\");
                }
            }

            //
            // Build the full path
            //
            lpszFullPath = (LPWSTR)malloc((cchFullPath + 1) * sizeof(WCHAR));
            if (lpszFullPath)
            {
                //wcscpy_s(lpszFullPath, cchFullPath + 1, rgAllPaths[0]);
                *lpszFullPath = 0;
                for (size_t i = 0; i < rgAllPaths.size(); ++i)
                {
                    wcscat_s(lpszFullPath, cchFullPath + 1, rgAllPaths[i]);

                    if (i + 1 == rgAllPaths.size() && NULL == wcsstr(fParentEntryInfo.lpszFileName, L"\\"))
                    {
                        /* Update the parent full path record */
                        mpDirectoryMap.at(fEntryInfo.ParentDirectoryRecord).lpszFileName = _wcsdup(lpszFullPath);
                    }
                    if (rgAllPaths[i][wcslen(rgAllPaths[i]) - 1] != L'\\')
                    {
                        wcscat_s(lpszFullPath, cchFullPath + 1, L"\\");
                    }
                }

                /* Update the path (do not free original name since the pointer is from a block of memory) */
                wcscat_s(lpszFullPath, cchFullPath + 1, fEntryInfo.lpszFileName);
                mpDirectoryMap.at(fRecordNumber).lpszFileName = lpszFullPath;

                continue;
            }
        }

        /* Erase this entry */
        rgEraseRecords.push_back(fRecordNumber);
    }

    /* Erase any invalid records from the directory map */
    for (size_t i = 0; i < rgEraseRecords.size(); ++i) {
        mpDirectoryMap.erase(rgEraseRecords[i]);
    }
}

VOID CMFTReader::UpdateFileRecordMapFileNames(NTFSFileMap &mpFileMap, NTFSDirectoryMap mpDirectoryMap)
{
    std::vector<UINT64> rgEraseRecords;

    for (auto &eFileEntry : mpFileMap)
    {
        FileMapKey_t    fRecordNumber = eFileEntry.first;
        FileMapValue_t  fEntryInfo = eFileEntry.second;

        /* Locate the parent directory */
        if (mpDirectoryMap.find(fEntryInfo.ParentDirectoryRecord) != mpDirectoryMap.end())
        {
            FileMapValue_t  fParentEntry;
            SIZE_T          cchNeeded;
            LPWSTR          lpszFullPath;

            fParentEntry = mpDirectoryMap.at(fEntryInfo.ParentDirectoryRecord);

            cchNeeded       = wcslen(fParentEntry.lpszFileName) + wcslen(L"\\") + wcslen(fEntryInfo.lpszFileName);
            lpszFullPath    = (LPWSTR)malloc((cchNeeded + 1) * sizeof(WCHAR));

            //
            // Build the full path of the current file record
            //
            if (lpszFullPath)
            {
                /* Append the file name to the parent directory */
                wcscpy_s(lpszFullPath, cchNeeded + 1, fParentEntry.lpszFileName); 
                if (lpszFullPath[wcslen(lpszFullPath)] != L'\\') {
                    wcscat_s(lpszFullPath, cchNeeded + 1,L"\\");
                }
                wcscat_s(lpszFullPath, cchNeeded + 1, fEntryInfo.lpszFileName);

                /* Update the file name */
                mpFileMap.at(fRecordNumber).lpszFileName = lpszFullPath;
                continue;
            }
        }

        /*
        * The parent directory is unknown (usually occurs with non-visible system files like $MFTMirr, $Extend, etc),
        * or malloc failed due to insufficient memory
        * The file will be erased from the map
        */
        rgEraseRecords.push_back(fRecordNumber);
    }

    /* Erase any invalid records from the file map */
    for (size_t i = 0; i < rgEraseRecords.size(); ++i) {
        mpFileMap.erase(rgEraseRecords[i]);
    }
}
