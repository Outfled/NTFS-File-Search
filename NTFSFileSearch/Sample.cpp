//#define NTFS_UNORDERED_FILE_MAPPING

#include "pch.h"
#include "ntfsvolume.h"
#include "ntfsvolumesearcher.h"

#pragma comment( lib, "Shlwapi.lib" )


CNTFSVolumeSearcher	g_vSearcher;
CNTFSVolume			*g_pSourceVolume;

void FindFilesWithSameName(LPCWSTR lpszFileName);
void FindFoldersWithSameName(LPCWSTR lpszFolderName);
void FindFirst100FileRecords();

int main()
{
	g_pSourceVolume = new CNTFSVolume;

	//
	//  Open the first available volume
	//
	DWORD dwDriveBitmask = GetLogicalDrives();
	for (int i = 'Z'; i <= 'Z'; ++i)
	{
		if ((dwDriveBitmask & (1 << (i - 'A'))))
		{
			const WCHAR szVolume[7] = { L'\\', L'\\', L'.', L'\\', (wchar_t)i, L':' };
			if (g_pSourceVolume->Open(szVolume) && g_vSearcher.SetVolume(g_pSourceVolume))
			{
				break;
			}
		}
		if (i == 'Z')
		{
			wprintf(L"Failed to find/open volume\n");
			return 1;
		}
	}

	//FindFilesWithSameName(L"Sample.cpp");
	//FindFoldersWithSameName(L"TestFolder");

	/* Print the first 100 MFT file records */
	FindFirst100FileRecords();

	/* Close the volume */
	g_pSourceVolume->Close();
	delete g_pSourceVolume;

	return 0;
}


//--------------------------------------------------------------
// Sample Functions

void FindFilesWithSameName(LPCWSTR lpszFileName)
{
	PNTFS_FILE_ENTRY	pFileResults;
	UINT64				nResultCount;

	g_vSearcher.ClearFileFilters();
	if (g_vSearcher.AddFileFilter(FF_OPERATOR_EQUAL, FF_FACTOR_FILENAME, lpszFileName))
	{
		wprintf(L"Failed to add file filter\n");
		return;
	}

	BOOL bSuccess = g_vSearcher.FindFiles(FILE_SEARCH_FLAG_FIND_FILES,
		&pFileResults,
		&nResultCount
	);
	if (bSuccess)
	{
		if (!nResultCount)
		{
			wprintf(L"No files found\n");
		}
		else
		{
			PNTFS_FILE_ENTRY pEnumResult = pFileResults;
			for (UINT64 i = 0; i < nResultCount; ++i)
			{
				wprintf(L"%I64u: %s\n", i + 1, pEnumResult->lpszFileName);
				if (pEnumResult->NextEntryOffset == 0)
				{
					wprintf(L"\n");
					break;
				}

				pEnumResult = POINTER_ADD(PNTFS_FILE_ENTRY, pFileResults, pEnumResult->NextEntryOffset);
			}
		}

		g_vSearcher.FreeFileEntries(pFileResults);
		return;
	}

	wprintf(L"Error: failed to search volume files\n");
}

void FindFoldersWithSameName(LPCWSTR lpszFolderName)
{
	PNTFS_FILE_ENTRY	pFileResults;
	UINT64				nResultCount;

	g_vSearcher.ClearFileFilters();
	if (g_vSearcher.AddFileFilter(FF_OPERATOR_EQUAL, FF_FACTOR_FILENAME, lpszFolderName))
	{
		wprintf(L"Failed to add file (directory) filter\n");
		return;
	}

	BOOL bSuccess = g_vSearcher.FindFiles(FILE_SEARCH_FLAG_FIND_FILES,
		&pFileResults,
		&nResultCount
	);
	if (bSuccess)
	{
		if (!nResultCount)
		{
			wprintf(L"No directories found\n");
		}
		else
		{
			PNTFS_FILE_ENTRY pEnumResult = pFileResults;
			for (UINT64 i = 0; i < nResultCount; ++i)
			{
				wprintf(L"%I64u: %s\n", i + 1, pEnumResult->lpszFileName);
				if (pEnumResult->NextEntryOffset == 0)
				{
					wprintf(L"\n");
					break;
				}

				pEnumResult = POINTER_ADD(PNTFS_FILE_ENTRY, pFileResults, pEnumResult->NextEntryOffset);
			}
		}

		g_vSearcher.FreeFileEntries(pFileResults);
		return;
	}

	wprintf(L"Error: failed to search volume files\n");
}

void FindFirst100FileRecords()
{
	PNTFS_FILE_ENTRY	pFileResults;
	UINT64				nResultCount;

	g_vSearcher.ClearFileFilters();
	if (!g_vSearcher.AddFileFilter(FF_OPERATOR_LESS_THAN_OR_EQ, FF_FACTOR_RECORD_NUMBER, 100ULL))
	{
		wprintf(L"Failed to add file filter\n");
		return;
	}

	BOOL bSuccess = g_vSearcher.FindFiles(FILE_SEARCH_FLAG_FIND_FILES,
		&pFileResults,
		&nResultCount
	);
	if (bSuccess)
	{
		if (!nResultCount)
		{
			wprintf(L"No files found\n");
		}
		else
		{
			PNTFS_FILE_ENTRY pEnumResult = pFileResults;
			for (UINT64 i = 0; i < nResultCount; ++i)
			{
				wprintf(L"[MFT Record Number %I64u] -> %s\n", pEnumResult->MFTFileId.MftRecordIndex, pEnumResult->lpszFileName);
				if (pEnumResult->NextEntryOffset == 0) {
					break;
				}

				pEnumResult = POINTER_ADD(PNTFS_FILE_ENTRY, pFileResults, pEnumResult->NextEntryOffset);
			}
		}

		g_vSearcher.FreeFileEntries(pFileResults);
		return;
	}

	wprintf(L"Error: failed to search volume files\n");
}
