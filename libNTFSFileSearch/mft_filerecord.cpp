#include "pch.h"
#include "mft_filerecord.h"

CMFTFileRecord::CMFTFileRecord(CNTFSVolume *pVolume, PMFT_FILE_RECORD_HEADER pFileRecord) :
	m_pVolume(pVolume),
	m_pFileRecord(pFileRecord)
{
}
CMFTFileRecord::~CMFTFileRecord()
{
}


UINT64 CMFTFileRecord::GetDataRuns(PMFT_ATTRIBUTE_HEADER pAttribute, MFTDataRunList *prgDataRuns)
{
	if (!pAttribute || !prgDataRuns)
	{
		return 0;
	}

	prgDataRuns->clear();

	VCN_t nStartVCN		= MFT_NONRESIDENT_ATTR(pAttribute).StartVCN;
	VCN_t nLastVCN		= MFT_NONRESIDENT_ATTR(pAttribute).LastVCN;
	VCN_t nCurrentVCN	= 0;

	PBYTE pbCurrent = POINTER_ADD(PBYTE, pAttribute, MFT_NONRESIDENT_ATTR(pAttribute).DataRunOffset);
	do
	{
		MFT_DATARUN DataRun = { 0 };

		int LengthSize = *pbCurrent & 0xF;
		int OffsetSize = *pbCurrent >> 4;
		if ((LengthSize < 1 || LengthSize > 8) || (OffsetSize < 1 || OffsetSize > 8))
		{
			prgDataRuns->clear();
			return 0;
		}

		++pbCurrent;
		CopyMemory(&DataRun.Length, pbCurrent, LengthSize);

		pbCurrent += LengthSize;
		if (pbCurrent[OffsetSize - 1] & 0x80) {
			DataRun.Offset = -1;
		}
		CopyMemory(&DataRun.Offset, pbCurrent, OffsetSize);

		prgDataRuns->push_back(DataRun);

		nCurrentVCN += DataRun.Length;
		pbCurrent	+= OffsetSize;
	} while (nCurrentVCN <= nLastVCN);

	return prgDataRuns->size();
}
