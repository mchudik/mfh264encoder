#pragma once
#include <D3D9Types.h>
#include "atlbase.h"
#include "Codecapi.h"
#include <Wmcodecdsp.h>
#include <Wincodec.h>
#include <VersionHelpers.h>

//Media Foundation Includes
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mfplay.h>
#include <strsafe.h>
//Media Foundation Includes

//Media Foundation Libraries
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "mfplay")
#pragma comment(lib, "windowscodecs")
#pragma comment(lib, "wmcodecdspuuid")
//Media Foundation Libraries

#define CHECK_HR(val) { if ( (val) != S_OK ) { goto done; } }

template <typename T, unsigned S>
inline unsigned arraySize(const T(&arr)[S]) { return S; }

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { SafeRelease(&(p)); }
#endif 

class CEncoderH264
{
public:
	CEncoderH264(const char* fileName, BOOL bWriteMP4, BOOL bWriteH264);
	virtual ~CEncoderH264();
	void Exiting() { m_bExiting = true; };

	HRESULT ConfigureEncoder(DWORD fccFormat, UINT32 width, UINT32 height, MFRatio frameRate, UINT nVBRQuality, UINT nMeanBitrate, UINT nMaxBitrate);
	HRESULT Encode(BYTE *pData, UINT32 dataSize, LONGLONG sampleTime, LONGLONG sampleDuration);
	HRESULT Start();
	HRESULT Stop();

private:
	HRESULT CreateMFSampleFromMediaType(IMFMediaType* pMediaType, IMFSample** pSample);
	HRESULT CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key);
	HRESULT CreateUncompressedVideoType(
		DWORD                fccFormat,  // FOURCC or D3DFORMAT value.     
		UINT32               width,
		UINT32               height,
		MFVideoInterlaceMode interlaceMode,
		const MFRatio&       frameRate,
		const MFRatio&       par,
		IMFMediaType         **ppType
	);

	IMFTransform*	m_pTransform;
	FILE*			m_pH264File;
	LONGLONG        m_hnsSampleTime;
	LONGLONG        m_hnsSampleDuration;
	IMFByteStream*  m_pMFBSOutputFile;
	IMFMediaSink*	m_pMediaSink;
	IMFStreamSink*	m_pStreamSink;
	IMFSinkWriter*	m_pSinkWriter;
	IMFSample*		m_pSample;
	ICodecAPI*		m_pCodecApi;
	BOOL			m_bExiting;

};
