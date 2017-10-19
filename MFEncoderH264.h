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
	IMFSample* GetSavedSample() { return m_pDisconnectedSample; };
	void Exiting() { m_bExiting = true; };

	HRESULT ConfigureEncoder(IMFMediaType *pSourceType, IMFMediaType *pOutputType, UINT nVBRQuality, UINT nMeanBitrate, UINT nMaxBitrate = NULL); 
	HRESULT CreateMFSampleFromBitmap(IMFMediaType* pMediaType, UINT nResourceId, IMFSample** pSample);
	HRESULT ColorConvertMFSample(IMFSample* pSampleIn, IMFSample** pSampleOut, const GUID& guidOutputType, const RECT& rcFile, const RECT& rcFrame);
	HRESULT CreateVideoMediaType(D3DFORMAT D3DFmt, DWORD dwWidth, DWORD dwHeight, IMFMediaType **ppMediaType);
	HRESULT Encode(IMFSample *pSampleIn, BOOL bPause);
	HRESULT Start();
	HRESULT Stop();

private:
	IMFTransform*	m_pTransform;		
	FILE*			m_pH264File;
	LONGLONG        m_hnsSampleTime;
	LONGLONG        m_hnsSampleDuration;
	IMFByteStream*  m_pMFBSOutputFile;
	IMFMediaSink*	m_pMediaSink;
	IMFStreamSink*	m_pStreamSink;
	IMFSinkWriter*	m_pSinkWriter;
	IMFSample*		m_pPauseSample;
	IMFSample*		m_pDisconnectedSample;
	IMFSample*		m_pStoppedSample;
	BOOL			m_bSampleAfterPause;
	ICodecAPI*		m_pCodecApi;
	BOOL			m_bExiting;

};
