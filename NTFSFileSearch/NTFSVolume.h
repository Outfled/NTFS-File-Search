#pragma once

#define NTFS_BOOT_SIGNATURE         (PBYTE)"NTFS    "
#define NTFS_BOOTSECTOR_SIZE        512

class CNTFSVolume
{
private:
	HANDLE				m_hVolume;
	NTFS_BOOT_SECTOR	m_BootRecord;
	UINT64				m_ullCurrentOffset;
	ULONG32				m_ullClusterSize;
	ULONG32				m_ullRecordSize;
	ULONG32				m_ullBlockSize;
	ULONG32				m_ullSectorSize;
	LPWSTR				m_lpszVolumeRoot;

public:
	CNTFSVolume();

	~CNTFSVolume();

	BOOL Open(LPCWSTR lpszVolumeName);

	BOOL Close();

	BOOL Seek(UINT64 ullAbsoluteOffset)
	{
		LARGE_INTEGER liPosition;
		liPosition.QuadPart = ullAbsoluteOffset;

		if (IsValid())
		{
			LARGE_INTEGER liUpdatedPosition;
			if (SetFilePointerEx(m_hVolume, liPosition, &liUpdatedPosition, FILE_BEGIN))
			{
				m_ullCurrentOffset = liUpdatedPosition.QuadPart;
				return TRUE;
			}
		}

		wprintf(L"%d\n", GetLastError());
		return FALSE;
	}

	DWORD ReadBytes(PVOID pBuffer, DWORD cbReadSize, UINT64 ullAbsoluteOffset = UINT64_MAX)
	{
		if (IsValid())
		{
			DWORD	dwNumberOfBytesRead;
			BOOL	bSuccess;

			if (ullAbsoluteOffset != UINT64_MAX)
			{
				if (!Seek(ullAbsoluteOffset)) {
					return 0;
				}
			}

			bSuccess = ReadFile(m_hVolume, pBuffer, cbReadSize, &dwNumberOfBytesRead, NULL);
			if (bSuccess) {
				m_ullCurrentOffset += cbReadSize;
			}

			return dwNumberOfBytesRead;
		}

		return 0;
	}

	BOOL IsValid() const
	{
		return (m_hVolume != INVALID_HANDLE_VALUE &&
			RtlCompareMemory(m_BootRecord.Signature, NTFS_BOOT_SIGNATURE, sizeof(m_BootRecord.Signature)));
	}

	HANDLE Get() const {
		return m_hVolume;
	}

	UINT64 GetCurrentOffset() const {
		return m_ullCurrentOffset;
	}

	NTFS_BOOT_SECTOR BootSector() const {
		return m_BootRecord;
	}

	UINT64 ClusterSize() const {
		return m_ullClusterSize;
	}

	UINT64 RecordSize() const {
		return m_ullRecordSize;
	}

	UINT64 BlockSize() const {
		return m_ullBlockSize;
	}

	UINT64 SectorSize() const {
		return m_ullSectorSize;
	}

	LPCWSTR RootPath()
	{
		if (IsValid())
		{
			return m_lpszVolumeRoot;
		}

		return NULL;
	}
};
