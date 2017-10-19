#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#include "stdafx.h"
#include "MFEncoderH264.h"

#ifndef IF_FAILED_GOTO
#define IF_FAILED_GOTO(hr, label) if (FAILED(hr)) { goto label; }
#endif

#define CHECK_HR(hr) IF_FAILED_GOTO(hr, done)

CEncoderH264::CEncoderH264(const char* fileName, BOOL bWriteMP4, BOOL bWriteH264)
: m_pTransform(NULL)
, m_pH264File(NULL)
, m_hnsSampleTime(NULL)
, m_hnsSampleDuration(NULL)
, m_pMFBSOutputFile(NULL)
, m_pMediaSink(NULL)
, m_pStreamSink(NULL)
, m_pSinkWriter(NULL)
, m_pPauseSample(NULL)
, m_pDisconnectedSample(nullptr)
, m_pStoppedSample(nullptr)
, m_bSampleAfterPause(FALSE)
, m_pCodecApi(NULL)
, m_bExiting(FALSE)
{
	// Initialize Media Foundation
	HRESULT hr = MFStartup(MF_VERSION);

	// Create IMFTransform for h.264 encoder
	if(IsWindows8OrGreater()) {
		CComPtr<IUnknown> spXferUnk;
		HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&spXferUnk);
		if (SUCCEEDED(hr))
			hr = spXferUnk->QueryInterface(IID_PPV_ARGS(&m_pTransform));
		if (FAILED(hr))
			m_pTransform = NULL;
	} else {
		HRESULT hr = S_OK;
		UINT32 count = 0;
		IMFActivate ** activate = NULL;
		MFT_REGISTER_TYPE_INFO info = { 0 };

		info.guidMajorType = MFMediaType_Video;
		info.guidSubtype = MFVideoFormat_H264;
		UINT32 flags = MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_TRANSCODE_ONLY | MFT_ENUM_FLAG_SORTANDFILTER;

		CHECK_HR (hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER,
					   flags,
					   NULL,
					   &info,
					   &activate,
					   &count));

		if (count == 0) {
			goto done;
		}
		CHECK_HR (hr = activate[count-1]->ActivateObject(IID_PPV_ARGS(&m_pTransform)));

	done:
		for (UINT32 idx = 0; idx < count; idx++) {
			activate[idx]->Release();
		}
		CoTaskMemFree(activate);
	}

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char szFileOut[_MAX_PATH];
	wchar_t	pszOutputFile[_MAX_PATH];
	_splitpath(fileName, drive, dir, fname, NULL);

	if(bWriteH264) {
		_makepath(szFileOut, drive, dir, fname, ".h264" );
		m_pH264File = fopen (szFileOut,"wb");
	}

	if(bWriteMP4) {
		_makepath(szFileOut, drive, dir, fname, ".mp4" );
		int flen = ::MultiByteToWideChar(CP_UTF8, 0, szFileOut, -1, pszOutputFile, arraySize(pszOutputFile));
		pszOutputFile[flen] = 0;

		HRESULT hr = MFCreateFile(
			MF_ACCESSMODE_READWRITE,
			MF_OPENMODE_DELETE_IF_EXIST,
			MF_FILEFLAGS_NONE,
			pszOutputFile,
			&m_pMFBSOutputFile);
		if(FAILED(hr))
		{
			m_pMFBSOutputFile = NULL;
		}
	}
}

CEncoderH264::~CEncoderH264()
{
	SafeRelease(&m_pTransform);
	SafeRelease(&m_pStreamSink);
	SafeRelease(&m_pMediaSink);
	SafeRelease(&m_pSinkWriter);
	SafeRelease(&m_pMFBSOutputFile);
	SafeRelease(&m_pPauseSample);
	SafeRelease(&m_pDisconnectedSample);
	SafeRelease(&m_pStoppedSample);
	SafeRelease(&m_pCodecApi);

	if(m_pH264File) {
		fclose(m_pH264File);
		m_pH264File = NULL;
	}

	HRESULT hr = MFShutdown();
}

HRESULT CEncoderH264::ConfigureEncoder(IMFMediaType *pSourceType, IMFMediaType *pOutputType, UINT nVBRQuality, UINT nMeanBitrate, UINT nMaxBitrate)
{
	HRESULT hr = S_OK;
	DWORD i = 0;
	GUID subtype = { 0 };
	GUID subtypeSource = { 0 };
	IMFMediaType* pInputType = NULL;
	UINT32 width = 0;
	UINT32 height = 0;
	LONG lStride = 0;

	if (!pSourceType)
		return E_POINTER;

	if (!pOutputType)
		return E_POINTER;

	if (!m_pTransform)
		return E_POINTER;

	//Create MP4 MediaSink, ByteStreamSink, and SinkWriter
	if (m_pMFBSOutputFile) {
		CHECK_HR(hr = MFCreateMPEG4MediaSink(
			m_pMFBSOutputFile,
			pOutputType,   //Video
			NULL,          //Audio
			&m_pMediaSink));

		CHECK_HR(hr = m_pMediaSink->GetStreamSinkByIndex(0, &m_pStreamSink));
		CHECK_HR(hr = MFCreateSinkWriterFromMediaSink(m_pMediaSink, NULL, &m_pSinkWriter));
	}
	CHECK_HR(hr = pSourceType->GetGUID(MF_MT_SUBTYPE, &subtypeSource));

	//Configure encoder parameters via ICodecAPI
	VARIANT var;
	CHECK_HR(hr = m_pTransform->QueryInterface<ICodecAPI>(&m_pCodecApi));
	//Set requested bitrate on media type
	CHECK_HR(hr = pOutputType->SetUINT32(MF_MT_AVG_BITRATE, nMeanBitrate));
/*
	//Configure encoder differently for VOD versus Live outputs
	if (m_pStreamer) {
		//Setting for Live streaming
		//In low-latency mode the encoder is expected to not add any sample delay due to frame reordering in encoding process, 
		//and one input sample shall produce one output sample.
		VariantInit(&var);
		var.vt = VT_BOOL;
		var.boolVal = VARIANT_TRUE;
		CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVLowLatencyMode, &var));
		VariantClear(&var);
		//Set B-Frames count to zero
		VariantInit(&var);
		var.vt = VT_UI4;
		var.ulVal = 0;
		CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncMPVDefaultBPictureCount, &var));
		VariantClear(&var);
		//		if(nMaxBitrate > NULL) {
		if (FALSE) {	//TEMP - Forcing to false to use unconstrained VBR
						//Peak Constrained VBR using Maximum bitrate in addition to avarage bitrate
			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = eAVEncCommonRateControlMode_PeakConstrainedVBR;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var));
			VariantClear(&var);

			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = nMeanBitrate;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &var));
			VariantClear(&var);

			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = nMaxBitrate;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonMaxBitRate, &var));
			VariantClear(&var);
		} else {
			//Unconstrained VBR using avarage bitrate
			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = eAVEncCommonRateControlMode_UnconstrainedVBR;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var));
			VariantClear(&var);

			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = nMeanBitrate;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &var));
			VariantClear(&var);
		}
	} else {
*/
		//Quality-based VBR for VOD recordings using the VBR quality parameter
		VariantInit(&var);
		var.vt = VT_UI4;
		var.ulVal = eAVEncCommonRateControlMode_Quality;
		CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var));
		VariantClear(&var);

		VariantInit(&var);
		var.vt = VT_UI4;
		var.ulVal = nVBRQuality;
		CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonQuality, &var));
		VariantClear(&var);

		VariantInit(&var);
		var.vt = VT_UI4;
		var.ulVal = nMeanBitrate;
		CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &var));
		VariantClear(&var);
//	}

	//Set Output type on the encoder
	CHECK_HR(hr = m_pTransform->SetOutputType(0, pOutputType, 0));

	//Look for compatible input type that the encoder supports
	while (SUCCEEDED(hr))
	{
		hr = m_pTransform->GetInputAvailableType(0, i, &pInputType);
		if (SUCCEEDED(hr))
		{
			hr = pInputType->GetGUID(MF_MT_SUBTYPE, &subtype);
			if (SUCCEEDED(hr))
			{
				if (IsEqualGUID(subtype, subtypeSource))
				{
					hr = m_pTransform->SetInputType(0, pInputType, 0);
					if (SUCCEEDED(hr))
					{
						break;
					}
				}
			}
		}
		i++;
	}
/*
	hr = CreateMFSampleFromBitmap(pSourceType, IDB_PNG_PAUSED, &m_pPauseSample);
	if (FAILED(hr)) {
		SafeRelease(&m_pPauseSample);
	}
	hr = CreateMFSampleFromBitmap(pSourceType, IDB_PNG_DISCONNECTED, &m_pDisconnectedSample);
	if (FAILED(hr)) {
		SafeRelease(&m_pDisconnectedSample);
	}
	hr = CreateMFSampleFromBitmap(pSourceType, IDB_PNG_STOPPED, &m_pStoppedSample);
	if (FAILED(hr)) {
		SafeRelease(&m_pStoppedSample);
	}
*/
done:
#ifdef DEBUG_MEDIA_TYPES
	wprintf(L"CEncoderH264::ConfigureEncoder:pSourceType Media Type:\n");
	LogMediaType(pSourceType);
	wprintf(L"CEncoderH264::ConfigureEncoder:pInputType Media Type:\n");
	LogMediaType(pInputType);
	wprintf(L"CEncoderH264::ConfigureEncoder:pOutputType Media Type:\n");
	LogMediaType(pOutputType);
#endif
	SafeRelease(&pInputType);
	return hr;
}

HRESULT CEncoderH264::Encode(IMFSample *pSample, BOOL bPause)
{
	if (!pSample)
	{
		return E_INVALIDARG;
	}

	if (!m_pTransform)
	{
		return MF_E_NOT_INITIALIZED;
	}
	
	HRESULT hr = S_OK, hrRes = S_OK;
	DWORD dwStatus = 0;
	DWORD cbTotalLength = 0, cbCurrentLength = 0;
	BYTE *pData = NULL;
	LONGLONG phnsSampleDuration = NULL;
	IMFMediaBuffer* pBufferOut = NULL;
	IMFSample* pSampleOut = NULL;
	IMFMediaType* pMediaType = NULL;
	LONGLONG rtStart = NULL;
	LONGLONG rtDuration = NULL;

	//get the size of the output buffer processed by the encoder.
	//There is only one output so the output stream id is 0.
	MFT_OUTPUT_STREAM_INFO mftStreamInfo;
	ZeroMemory(&mftStreamInfo, sizeof(MFT_OUTPUT_STREAM_INFO));
	CHECK_HR (hr =  m_pTransform->GetOutputStreamInfo(0, &mftStreamInfo));
	
	//Send input to the encoder.
	if(bPause) {
		//Copy timing information to pause sample.
		CHECK_HR (hr = pSample->GetSampleTime(&rtStart));
		CHECK_HR (hr = pSample->GetSampleDuration(&rtDuration));
		if (m_bExiting) {
			CHECK_HR(hr = m_pStoppedSample->SetSampleTime(rtStart));
			CHECK_HR(hr = m_pStoppedSample->SetSampleDuration(rtDuration));
			CHECK_HR(hr = m_pTransform->ProcessInput(0, m_pStoppedSample, 0));
		} else {
			CHECK_HR(hr = m_pPauseSample->SetSampleTime(rtStart));
			CHECK_HR(hr = m_pPauseSample->SetSampleDuration(rtDuration));
			CHECK_HR(hr = m_pTransform->ProcessInput(0, m_pPauseSample, 0));
		}
		if(!m_bSampleAfterPause) {
			VARIANT var;
			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = 1;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncVideoForceKeyFrame, &var));
			CHECK_HR(hr = pSample->SetUINT32(MFSampleExtension_Discontinuity, TRUE));
		}
		m_bSampleAfterPause = TRUE;
	} else {
		if(m_bSampleAfterPause) {
			VARIANT var;
			VariantInit(&var);
			var.vt = VT_UI4;
			var.ulVal = 1;
			CHECK_HR(hr = m_pCodecApi->SetValue(&CODECAPI_AVEncVideoForceKeyFrame, &var));
			CHECK_HR(hr = pSample->SetUINT32(MFSampleExtension_Discontinuity, TRUE));
		}
		m_bSampleAfterPause = FALSE;
		CHECK_HR (hr = m_pTransform->ProcessInput(0, pSample, 0));
	}

	//Generate the output sample
	MFT_OUTPUT_DATA_BUFFER mftOutputData;
	ZeroMemory(&mftOutputData, sizeof(mftOutputData));
	CHECK_HR (hr = MFCreateMemoryBuffer(mftStreamInfo.cbSize, &pBufferOut));
	CHECK_HR (hr = MFCreateSample(&pSampleOut));
	CHECK_HR (hr = pSampleOut->AddBuffer(pBufferOut));
	mftOutputData.pSample = pSampleOut;
	mftOutputData.dwStreamID = 0;
	hrRes =  m_pTransform->ProcessOutput(0, 1, &mftOutputData, &dwStatus);

	//If more input is needed there was no output to process. Return and repeat
	if(hrRes != MF_E_TRANSFORM_NEED_MORE_INPUT) {

//#ifdef DEBUG_MEDIA_TYPES
//		CHECK_HR (hr =  m_pTransform->GetOutputCurrentType(0, &pMediaType));  
//		wprintf(L"CEncoderH264::Encode: Actual Compressed Media Type:\n");
//		LogMediaType(pMediaType);
//#endif
		
		//Get a pointer to the memory
		CHECK_HR (hr = pBufferOut->Lock(&pData, &cbTotalLength, &cbCurrentLength)); 

//		if(m_pStreamer) {
			//Extract the actual Timestamp and Duration on the sample from before the compression
			//The CMSH264EncoderMFT encoder does not seem to output the timestamps in ascending order
			CHECK_HR(hr = pSample->GetSampleTime(&m_hnsSampleTime)); 
			CHECK_HR(hr = pSample->GetSampleDuration(&m_hnsSampleDuration)); 
			//Send the buffer to the Streamer with timestamp adjusted to UTC time
//			m_pStreamer->SendSample(pData, cbCurrentLength, m_hnsSampleTime, m_hnsSampleDuration);
//		}

		//Write elementary h.264/AAC/AVC data to file
		if(m_pH264File && !bPause) {
			fwrite (pData , sizeof(BYTE), cbCurrentLength, m_pH264File);
		}

		CHECK_HR (hr = pBufferOut->Unlock());
		pData = NULL;

		//Send Sample to MP4 StreamSink for writing
		if(m_pStreamSink && !bPause) {
			hr = m_pStreamSink->ProcessSample(pSampleOut);
		}
	}

done:

	if (pData && FAILED(hr))
	{
		pBufferOut->Unlock();
	}

	SAFE_RELEASE(pBufferOut);
	SAFE_RELEASE(pSampleOut);
	SAFE_RELEASE(pMediaType);
	if(hr == MF_E_TRANSFORM_NEED_MORE_INPUT) hr=S_OK;	//Do not log valid error
	return hr;
}

HRESULT CEncoderH264::Start() 
{
	HRESULT hr = S_OK;

	if (!m_pTransform)
		return E_POINTER;

	//Start MP4 SinkWriter
	if(m_pSinkWriter) {
		CHECK_HR (hr = m_pSinkWriter->BeginWriting());
	}

	//Start the encoder so it is ready to receive samples on the input
	if (SUCCEEDED(hr))
	{
		CHECK_HR (hr = m_pTransform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL));
		CHECK_HR (hr = m_pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL));
		CHECK_HR (hr = m_pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL));
	}
/*
	//Start Video Streamer
	if(m_pStreamer) {
		CHECK_HR (hr = m_pStreamer->Start());
	}
*/
done:
	return hr;
}

HRESULT CEncoderH264::Stop() 
{
	HRESULT hr = S_OK;

	if (!m_pTransform)
		return E_POINTER;

	//Stop the encoder
	if (SUCCEEDED(hr))
	{
		CHECK_HR (hr = m_pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, NULL));
		CHECK_HR (hr = m_pTransform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, NULL));
		CHECK_HR (hr = m_pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, NULL));
	}

	//Stop MP4 SinkWriter
	if(m_pSinkWriter) {
		CHECK_HR (hr = m_pSinkWriter->Finalize());
	}

done:
	return hr;
}
/*
HRESULT CEncoderH264::CreateMFSampleFromBitmap(IMFMediaType* pMediaType, UINT nResourceId, IMFSample** pSample)
{
	HRESULT hr = S_OK;
	IStream* pImageStream = NULL;
	IWICBitmapSource *pBitmap = NULL;
	UINT uiWidth = 0, uiHeight = 0;
	RECT rcFrame = {0, 0, 0, 0};
	RECT rFile = {0, 0, 0, 0};
	IMFSample *pRGBSample = NULL;
	IMFSample *pYUVSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;
	BYTE *pData = NULL;

	if((pImageStream = CreateStreamOnResource(MAKEINTRESOURCE(nResourceId), L"PNG")) == NULL) {
		return E_FAIL;
	}
	if((pBitmap = LoadBitmapFromStream(pImageStream)) == NULL) {
		SafeRelease(&pImageStream);
		return E_FAIL;
	}
	CHECK_HR(hr = pBitmap->GetSize((UINT*)&rFile.right, (UINT*)&rFile.bottom));
	UINT cbStride = rFile.right * 4;
	UINT cbBufferSize = cbStride * rFile.bottom;
	CHECK_HR(hr = MFCreateMemoryBuffer(cbBufferSize, &pBuffer));
	CHECK_HR(hr = pBuffer->Lock(&pData, NULL, NULL));
	CHECK_HR(hr = pBitmap->CopyPixels(NULL,
		cbStride,
		cbBufferSize,
		reinterpret_cast<BYTE*>(pData)));
	CHECK_HR(hr = pBuffer->Unlock());
	CHECK_HR(hr = pBuffer->SetCurrentLength(cbBufferSize));
	CHECK_HR(hr = MFCreateSample(&pRGBSample));
	CHECK_HR(hr = pRGBSample->AddBuffer(pBuffer));
	CHECK_HR(hr = pRGBSample->SetSampleTime(NULL));
	CHECK_HR(hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, (UINT32*)&rcFrame.right, (UINT32*)&rcFrame.bottom));
	ColorConvertMFSample(pRGBSample, &pYUVSample, MFVideoFormat_YUY2, rFile, rcFrame);
	CHECK_HR(hr = pYUVSample->SetSampleTime(NULL));

done:
	if (pYUVSample)
	{
		*pSample = pYUVSample;
		(*pSample)->AddRef();
	}
	SafeRelease(&pRGBSample);
	SafeRelease(&pYUVSample);
	SafeRelease(&pImageStream);
	SafeRelease(&pBitmap);
	return hr;
}

HRESULT CEncoderH264::ColorConvertMFSample(IMFSample* pSampleIn, IMFSample** pSampleOut, const GUID& guidOutputType, const RECT& rcFile, const RECT& rcFrame)
{
	HRESULT hr = S_OK;
	IMFMediaType *pTypeIn = NULL;
	IMFMediaType *pTypeOut = NULL;
	IMFSample *pSample = NULL;

	CVideoProcessor* pVideoProcessor = new CVideoProcessor();
	CHECK_HR(hr = CreateVideoMediaType(D3DFMT_A8R8G8B8, rcFile.right, rcFile.bottom, &pTypeIn));
	CHECK_HR(hr = pVideoProcessor->ConfigureTranscoder(pTypeIn, MFVideoFormat_YUY2, rcFrame, &pTypeOut));
	CHECK_HR(hr = pVideoProcessor->Transcode(pSampleIn, &pSample));

done:
	if(pSample) {
		*pSampleOut = pSample;
		(*pSampleOut)->AddRef();
	}
	SafeRelease(&pSample);
	SafeRelease(&pTypeIn);
	SafeRelease(&pTypeOut);
	SafeRelease(&pTypeIn);
	if(pVideoProcessor) {
		delete pVideoProcessor;
	}
	return hr;
}
*/
HRESULT CEncoderH264::CreateVideoMediaType(D3DFORMAT D3DFmt, DWORD dwWidth, DWORD dwHeight, IMFMediaType **ppMediaType)
{
	HRESULT hr = S_OK;
	IMFMediaType *pMediaType = NULL;
	MFVIDEOFORMAT format;

	CHECK_HR(hr = MFInitVideoFormat_RGB(&format, dwWidth, dwHeight, D3DFmt));
	CHECK_HR(hr = MFCreateMediaType(&pMediaType));
	CHECK_HR(hr = MFInitMediaTypeFromMFVideoFormat(pMediaType, &format, sizeof(format)));

done:
	if(pMediaType) {
		*ppMediaType = pMediaType;
		(*ppMediaType)->AddRef();
	}
	SafeRelease(&pMediaType);
	return hr;
}
