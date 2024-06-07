#pragma once

#include "NTFSVolume.h"

class CMFTFileRecord
{
private:
    PMFT_FILE_RECORD_HEADER m_pFileRecord;
    CNTFSVolume             *m_pVolume;

public:
    CMFTFileRecord(CNTFSVolume *pVolume, PMFT_FILE_RECORD_HEADER pFileRecord);

	~CMFTFileRecord();

	VOID ApplyFixup()
	{
		WORD    wUpdateSize;
		PWORD   pUpdateSequence;
		DWORD	dwOffset;

		wUpdateSize = m_pFileRecord->SizeOfUpdateSequence;
		pUpdateSequence = POINTER_ADD(PWORD, m_pFileRecord, m_pFileRecord->UpdateSequenceOffset);

		dwOffset = m_pVolume->SectorSize();
		for (DWORD i = 1; i < wUpdateSize; i++)
		{
			if (dwOffset <= m_pVolume->RecordSize())
			{
				((PWORD)m_pFileRecord)[(dwOffset - 2) / sizeof(WORD)] = pUpdateSequence[i];
				dwOffset += m_pVolume->SectorSize();
			}
			else {
				break;
			}
		}
	}

	RecordAttrMultiMap GetAttributes()
	{
		RecordAttrMultiMap mpAttributes;

		PMFT_ATTRIBUTE_HEADER pCurAttribute = POINTER_ADD(PMFT_ATTRIBUTE_HEADER,
			m_pFileRecord,
			m_pFileRecord->FirstAttributeOffset
		);
		do
		{
			if (!pCurAttribute || POINTER_SUB(pCurAttribute, m_pFileRecord) >= m_pVolume->RecordSize())
			{
				break;
			}

			mpAttributes.insert(std::make_pair(pCurAttribute->Type, pCurAttribute));
			pCurAttribute = POINTER_ADD(PMFT_ATTRIBUTE_HEADER, pCurAttribute, pCurAttribute->TotalSize);
		} while (pCurAttribute->Type != MFT_FILERECORD_ATTR_STOP_TAG);

		return mpAttributes;
	}

	PMFT_ATTRIBUTE_HEADER FindAttribute(DWORD dwType, DWORD dwIndex = 0)
	{
		PMFT_ATTRIBUTE_HEADER pAttribute = POINTER_ADD(PMFT_ATTRIBUTE_HEADER,
			m_pFileRecord,
			m_pFileRecord->FirstAttributeOffset
		);
		while (pAttribute->Type != MFT_FILERECORD_ATTR_STOP_TAG && pAttribute->TotalSize)
		{
			if (pAttribute->Type == dwType)
			{
				if (dwIndex == 0) {
					return pAttribute;
				}

				--dwIndex;
			}

			pAttribute = POINTER_ADD(PMFT_ATTRIBUTE_HEADER, pAttribute, pAttribute->TotalSize);
		}

		return NULL;
	}

	UINT64 GetDataRuns(PMFT_ATTRIBUTE_HEADER pAttribute, MFTDataRunList *prgDataRuns);

	BOOL IsValid() const
	{
		if (m_pFileRecord)
		{
			if (m_pFileRecord->Magic == MFT_FILE_RECORD_MAGIC && m_pFileRecord->RealSize % 8 == 0)
			{
				if (m_pFileRecord->AllocatedSize > m_pVolume->ClusterSize()) {
					return (m_pFileRecord->AllocatedSize % m_pVolume->ClusterSize() == 0) ? TRUE : FALSE;
				}
				else {
					return (m_pVolume->ClusterSize() % m_pFileRecord->AllocatedSize == 0) ? TRUE : FALSE;
				}
			}
		}

		return FALSE;
	}

	DWORD Flags() const {
		return m_pFileRecord->Flags;
	}

	DWORD RecordNumber() const {
		return m_pFileRecord->RecordNumber;
	}

	DWORD SequenceNumber() const {
		return m_pFileRecord->SequenceNumber;
	}
};
