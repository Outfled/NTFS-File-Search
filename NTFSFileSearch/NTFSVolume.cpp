#include "pch.h"
#include "NTFSVolume.h"


CNTFSVolume::CNTFSVolume() :
	m_hVolume(NULL),
	m_ullCurrentOffset(0),
	m_ullClusterSize(0),
	m_ullRecordSize(0),
	m_ullBlockSize(0),
	m_ullSectorSize(0),
	m_lpszVolumeRoot(NULL)
{
	ZeroMemory(&m_BootRecord, sizeof(m_BootRecord));
}

CNTFSVolume::~CNTFSVolume()
{
	Close();
}

BOOL CNTFSVolume::Open(LPCWSTR lpszVolumeName)
{
	/* Open volume handle */
	m_hVolume = CreateFile(lpszVolumeName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
		NULL
	);
	if (m_hVolume != INVALID_HANDLE_VALUE)
	{
		//
		// Read the boot sector
		//
		DWORD dwNumberOfBytesRead;
		if (ReadFile(m_hVolume, &m_BootRecord, NTFS_BOOTSECTOR_SIZE, &dwNumberOfBytesRead, NULL))
		{
			m_ullCurrentOffset += dwNumberOfBytesRead;
			if (dwNumberOfBytesRead == NTFS_BOOTSECTOR_SIZE)
			{
				ULONG nReadSectorsPerCluster = (m_BootRecord.SectorsPerCluster > 0x80) ?
					1 << -m_BootRecord.SectorsPerCluster : m_BootRecord.SectorsPerCluster;

				m_ullClusterSize = m_BootRecord.BytesPerSector * nReadSectorsPerCluster;
				m_ullSectorSize = m_BootRecord.BytesPerSector;

				m_ullBlockSize = ((char)m_BootRecord.ClustersPerIndexBlock >= 0) ?
					m_BootRecord.ClustersPerIndexBlock * m_ullClusterSize : 1 << (-(int)m_BootRecord.ClustersPerFileRecord);
				m_ullRecordSize = ((char)m_BootRecord.ClustersPerFileRecord >= 0) ?
					m_BootRecord.ClustersPerFileRecord * m_ullClusterSize : 1 << (-(int)m_BootRecord.ClustersPerFileRecord);

				m_lpszVolumeRoot = StrDupW(lpszVolumeName + wcslen(L"\\\\.\\"));
				return TRUE;
			}
		}

		// Invalid volume
		CloseHandle(m_hVolume);
		m_hVolume = INVALID_HANDLE_VALUE;
	}
}


BOOL CNTFSVolume::Close()
{
	if (m_hVolume && m_hVolume != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hVolume);
		m_hVolume = NULL;

		if (m_lpszVolumeRoot)
		{
			LocalFree(m_lpszVolumeRoot);
			m_lpszVolumeRoot = NULL;
		}

		ZeroMemory(&m_BootRecord, sizeof(m_BootRecord));
		return TRUE;
	}

	return FALSE;
}
