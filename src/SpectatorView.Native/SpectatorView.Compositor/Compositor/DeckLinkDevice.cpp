/* -LICENSE-START-
** Copyright (c) 2013 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
**
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/

#include "pch.h"

#if defined(INCLUDE_BLACKMAGIC)
#include <comutil.h>
#include "DeckLinkDevice.h"

using namespace std;


//const INT32_UNSIGNED kFrameDuration = 1000;

class OutputScheduler : public IDeckLinkVideoOutputCallback
{
public:

    bool enabled;
    bool started;
    unsigned long framesQueued;
    LONGLONG frameTime;

    LONGLONG frameDuration;
    LONGLONG timeScale;

#define MAX_NUM_OUTPUT_FRAMES 20
    IDeckLinkMutableVideoFrame* outputVideoFrames[MAX_NUM_OUTPUT_FRAMES];
    int outputWriteIndex = 0;

    OutputScheduler()
    {
        enabled = false;
        started = false;
        outputVideoFrames[0] = NULL;
    }

    void InitScaleAndDeltaFromDisplayMode(BMDDisplayMode videoDisplayMode)
    {
        timeScale = 60000;
        frameDuration = 1001;

        switch (videoDisplayMode)
        {
        case bmdModeNTSC:
        case bmdModeHD1080p2997:
        case bmdModeHD1080i5994:
        case bmdMode4K2160p2997:
            timeScale = 30000;
            frameDuration = 1001;
            break;
        case bmdModeNTSC2398:
        case bmdModeHD1080p2398:
        case bmdMode2k2398:
        case bmdMode2kDCI2398:
        case bmdMode4K2160p2398:
            timeScale = 24000;
            frameDuration = 1001;
            break;
        case bmdModeNTSCp:
        case bmdModeHD720p5994:
        case bmdModeHD1080p5994:
            timeScale = 60000;
            frameDuration = 1001;
            break;
        case bmdModePAL:
        case bmdModeHD1080p25:
        case bmdModeHD1080i50:
        case bmdMode2k25:
        case bmdMode2kDCI25:
        case bmdMode4K2160p25:
            timeScale = 25000;
            frameDuration = 1000;
            break;
        case bmdModePALp:
        case bmdModeHD720p50:
        case bmdModeHD1080p50:
        case bmdMode4K2160p50:
            timeScale = 50000;
            frameDuration = 1000;
            break;
        case bmdModeHD720p60:
        case bmdModeHD1080p6000:
            timeScale = 60000;
            frameDuration = 1000;
            break;
        case bmdModeHD1080p24:
        case bmdMode2k24:
        case bmdMode2kDCI24:
        case bmdMode4K2160p24:
            timeScale = 24000;
            frameDuration = 1000;
            break;
        case bmdModeHD1080p30:
        case bmdModeHD1080i6000:
        case bmdMode4K2160p30:
            timeScale = 30000;
            frameDuration = 1000;
            break;
        }

       /*    Mode Width Height Frames per Second Fields per Frame Suggested Time Scale Display Duration
            bmdModeNTSC 720 486 30 / 1.001 2 30000 1001
            bmdModeNTSC2398 720 486 30 / 1.001 * 2 24000 * 1001
            bmdModeNTSCp 720 486 60 / 1.001 1 60000 1001
            bmdModePAL 720 576 25 2 25000 1000
            bmdModePALp 720 576 50 1 50000 1000
            bmdModeHD720p50 1280 720 50 1 50000 1000
            bmdModeHD720p5994 1280 720 60 / 1.001 1 60000 1001
            bmdModeHD720p60 1280 720 60 1 60000 1000
            bmdModeHD1080p2398 1920 1080 24 / 1.001 1 24000 1001
            bmdModeHD1080p24 1920 1080 24 1 24000 1000
            bmdModeHD1080p25 1920 1080 25 1 25000 1000
            bmdModeHD1080p2997 1920 1080 30 / 1.001 1 30000 1001
            bmdModeHD1080p30 1920 1080 30 1 30000 1000
            bmdModeHD1080i50 1920 1080 25 2 25000 1000
            bmdModeHD1080i5994 1920 1080 30 / 1.001 2 30000 1001
            bmdModeHD1080i6000 1920 1080 30 2 30000 1000
            bmdModeHD1080p50 1920 1080 50 1 50000 1000
            bmdModeHD1080p5994 1920 1080 60 / 1.001 1 60000 1001
            bmdModeHD1080p6000 1920 1080 60 1 60000 1000
            bmdMode2k2398 2048 1556 24 / 1.001 1 24000 1001
            bmdMode2k24 2048 1556 24 1 24000 1000
            bmdMode2k25 2048 1556 25 1 25000 1000
            bmdMode2kDCI2398 2048 1080 24 / 1.001 1 24000 1001
            bmdMode2kDCI24 2048 1080 24 1 24000 1000
            bmdMode2kDCI25 2048 1080 25 1 25000 1000
            bmdMode4K2160p2398 3840 2160 24 / 1.001 1 24000 1001
            bmdMode4K2160p24 3840 2160 24 1 24000 1000
            bmdMode4K2160p25 3840 2160 25 1 25000 1000
            bmdMode4K2160p2997 3840 2160 30 / 1.001 1 30000 1001
            bmdMode4K2160p30 3840 2160 30 1 30000 1000
            bmdMode4K2160p50 3840 2160 50 1 50000 1000
            */
    }

    bool Init(IDeckLinkOutput* deckLinkOutput, BMDDisplayMode videoDisplayMode, BMDPixelFormat pixelFormat)
    {
        m_deckLinkOutput = deckLinkOutput;

        for (int i = 0; i < MAX_NUM_OUTPUT_FRAMES; i++)
        {
            HRESULT result = m_deckLinkOutput->CreateVideoFrame(
                FRAME_WIDTH, FRAME_HEIGHT, FRAME_WIDTH * ((pixelFormat == BMDPixelFormat::bmdFormat8BitYUV) ? FRAME_BPP_YUV : FRAME_BPP_RGBA),
                pixelFormat, bmdVideoInputFlagDefault,
                &outputVideoFrames[i]);
        }
        outputWriteIndex = 0;

        InitScaleAndDeltaFromDisplayMode(videoDisplayMode);

        framesQueued = 0;
        frameTime = 0;

        // Set the callback object to the DeckLink device's output interface
        HRESULT result = m_deckLinkOutput->SetScheduledFrameCompletionCallback(this);
        if (result != S_OK)
        {
            fprintf(stderr, "Could not set callback - result = %08x\n", result);
            return false;
        }

        // Enable video output
        BMDVideoOutputFlags videoOutputFlags = bmdVideoOutputFlagDefault;

        result = m_deckLinkOutput->EnableVideoOutput(videoDisplayMode, videoOutputFlags);
        if (result != S_OK)
        {
            fprintf(stderr, "Could not enable video output - result = %08x\n", result);
            return false;
        }
        enabled = true;

        return true;
    }

    IDeckLinkMutableVideoFrame* GetAvailableVideoFrame()
    {
        if (framesQueued >= MAX_NUM_OUTPUT_FRAMES)
        {
            return NULL;
        }

        return outputVideoFrames[(outputWriteIndex++) % MAX_NUM_OUTPUT_FRAMES];
    }


    void QueueFrame(IDeckLinkVideoFrame* newFrame)
    {
        if (!enabled)
            return;

        LONGLONG duration = frameDuration;

        if (started)
        {
            //Try to fill the queue back up
            if (framesQueued < MAX_NUM_OUTPUT_FRAMES - 5)
                duration += 20;

            //Make sure we dont fall behind on big hitches
            BMDTimeValue streamTime;
            double playbackSpeed;
            m_deckLinkOutput->GetScheduledStreamTime(timeScale, &streamTime, &playbackSpeed);
            if (streamTime > frameTime)
                frameTime = streamTime;
        }

        HRESULT result = m_deckLinkOutput->ScheduleVideoFrame(newFrame, frameTime, duration, timeScale);
        if (result != S_OK)
        {
            fprintf(stderr, "Could not schedule video frame - result = %08x\n", result);
        }
        else
        {
            frameTime += duration;
            InterlockedIncrement(&framesQueued);

            if (!started)
            {
                //Buffer frames, then start
                if (framesQueued > MAX_NUM_OUTPUT_FRAMES - 4)
                {
                    // Start
                    result = m_deckLinkOutput->StartScheduledPlayback(0, timeScale, 1.0);
                    if (result == S_OK)
                    {
                        started = true;
                    }
                }
            }
        }
    }

    virtual ~OutputScheduler(void)
    {
    }

    void Clear()
    {
        if (!enabled)
            return;

        // Stop capture
        if(started)
            m_deckLinkOutput->StopScheduledPlayback(0, NULL, 0);

        // Disable the video input interface
        m_deckLinkOutput->DisableVideoOutput();

        enabled = false;
        started = false;

        for (int i = 0; i < MAX_NUM_OUTPUT_FRAMES; i++)
        {
            if (outputVideoFrames[i] != NULL)
            {
                outputVideoFrames[i]->Release();
                outputVideoFrames[i] = NULL;
            }
        }

    }

    HRESULT	STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result)
    {
        InterlockedDecrement(&framesQueued);
        return S_OK;
    }

    HRESULT	STDMETHODCALLTYPE ScheduledPlaybackHasStopped(void)
    {
        return S_OK;
    }
    // IUnknown needs only a dummy implementation
    HRESULT	STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
    {
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return 1;
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        return 1;
    }
private:
    IDeckLinkOutput * m_deckLinkOutput;
};

static OutputScheduler s_outputScheduler;


DeckLinkDevice::DeckLinkDevice(IDeckLink* device) :
    m_deckLink(device),
    m_deckLinkInput(NULL),
    m_deckLinkOutput(NULL),
    m_supportsFormatDetection(false),
    m_refCount(1),
    m_currentlyCapturing(false)
{

    for (int i = 0; i < MAX_NUM_CACHED_BUFFERS; i++)
    {
        bufferCache[i].buffer = new BYTE[FRAME_BUFSIZE_RGBA];
        bufferCache[i].timeStamp = 0;
    }

    if (m_deckLink != NULL)
    {
        m_deckLink->AddRef();
    }

    InitializeCriticalSection(&m_captureCardCriticalSection);
    InitializeCriticalSection(&m_outputCriticalSection);
}

DeckLinkDevice::~DeckLinkDevice()
{
    StopCapture();
    s_outputScheduler.Clear();

    if (m_deckLinkInput != NULL)
    {
        m_deckLinkInput->Release();
        m_deckLinkInput = NULL;
    }

    if (supportsOutput && m_deckLinkOutput != NULL)
    {
        m_deckLinkOutput->Release();
        m_deckLinkOutput = NULL;
    }

    if (m_deckLink != NULL)
    {
        m_deckLink->Release();
        m_deckLink = NULL;
    }

    DeleteCriticalSection(&m_captureCardCriticalSection);
    DeleteCriticalSection(&m_outputCriticalSection);

    for (int i = 0; i < MAX_NUM_CACHED_BUFFERS; i++)
    {
        delete[] bufferCache[i].buffer;
    }

    delete[] stagingBuffer;
    delete[] outputBuffer;
    delete[] outputBufferRaw;
}

DeckLinkDevice::BufferCache& DeckLinkDevice::GetOldestBuffer()
{
    return bufferCache[(captureFrameIndex + 1) % MAX_NUM_CACHED_BUFFERS];
}


HRESULT    STDMETHODCALLTYPE DeckLinkDevice::QueryInterface(REFIID iid, LPVOID *ppv)
{
    HRESULT result = E_NOINTERFACE;

    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    // Initialise the return result
    *ppv = NULL;

    // Obtain the IUnknown interface and compare it the provided REFIID
    if (iid == IID_IUnknown)
    {
        *ppv = this;
        AddRef();
        result = S_OK;
    }
    else if (iid == IID_IDeckLinkInputCallback)
    {
        *ppv = (IDeckLinkInputCallback*)this;
        AddRef();
        result = S_OK;
    }
    else if (iid == IID_IDeckLinkNotificationCallback)
    {
        *ppv = (IDeckLinkNotificationCallback*)this;
        AddRef();
        result = S_OK;
    }

    return result;
}

ULONG STDMETHODCALLTYPE DeckLinkDevice::AddRef(void)
{
    return InterlockedIncrement((LONG*)&m_refCount);
}

ULONG STDMETHODCALLTYPE DeckLinkDevice::Release(void)
{
    int newRefValue;

    newRefValue = InterlockedDecrement((LONG*)&m_refCount);
    if (newRefValue == 0)
    {
        delete this;
        return 0;
    }

    return newRefValue;
}

bool DeckLinkDevice::Init(ID3D11ShaderResourceView* colorSRV, ID3D11Texture2D* outputTexture, bool useCPU, bool passthroughOutput)
{
    IDeckLinkAttributes*            deckLinkAttributes = NULL;
    IDeckLinkDisplayModeIterator*   displayModeIterator = NULL;
    IDeckLinkDisplayMode*           displayMode = NULL;
    BSTR                            deviceNameBSTR = NULL;

    ZeroMemory(rawBuffer, FRAME_BUFSIZE_YUV);
    ZeroMemory(outputBuffer, FRAME_BUFSIZE_RGBA);
    ZeroMemory(outputBufferRaw, FRAME_BUFSIZE_YUV);

    for (int i = 0; i < MAX_NUM_CACHED_BUFFERS; i++)
        ZeroMemory(bufferCache[i].buffer, FRAME_BUFSIZE_RGBA);

    captureFrameIndex = 0;

    _useCPU = useCPU;
    _passthroughOutput = passthroughOutput;

    _colorSRV = colorSRV;
    _outputTexture = outputTexture;

    if (colorSRV != nullptr)
    {
        colorSRV->GetDevice(&device);
    }

    // Get input interface
    if (m_deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&m_deckLinkInput) != S_OK)
        return false;

    if (m_deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&m_deckLinkOutput) != S_OK)
    {
        supportsOutput = false;
        // Returning false here prevent the rest of the init function executing
        // But we can still continue without an output interface
    }

    // Check if input mode detection is supported.
    if (m_deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes) == S_OK)
    {
        if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &m_supportsFormatDetection) != S_OK)
        {
            m_supportsFormatDetection = false;
        }

        deckLinkAttributes->Release();
    }

    return true;
}

bool DeckLinkDevice::StartCapture(BMDDisplayMode videoDisplayMode)
{
    if (m_deckLinkInput == NULL)
    {
        return false;
    }

    OutputDebugString(L"Start Capture.\n");
    BMDVideoInputFlags videoInputFlags = bmdVideoInputFlagDefault;

    // Enable input video mode detection if the device supports it
    if (m_supportsFormatDetection == TRUE)
    {
        videoInputFlags |= bmdVideoInputEnableFormatDetection;
    }

    // Set capture callback
    m_deckLinkInput->SetCallback(this);

    // Set the video input mode
    if (m_deckLinkInput->EnableVideoInput(videoDisplayMode, bmdFormat8BitYUV, videoInputFlags) != S_OK)
    {
        OutputDebugString(L"Unable to set the chosen video mode.\n");
        return false;
    }

    // Start the capture
    if (m_deckLinkInput->StartStreams() != S_OK)
    {
        OutputDebugString(L"Unable to start capture.\n");
        return false;
    }

    m_currentlyCapturing = true;

    SetupVideoOutputFrame(videoDisplayMode);

    return true;
}

void DeckLinkDevice::SetupVideoOutputFrame(BMDDisplayMode videoDisplayMode)
{
    s_outputScheduler.Clear();

    if (supportsOutput)
    {
        if(!s_outputScheduler.Init(m_deckLinkOutput, videoDisplayMode, (pixelFormat == PixelFormat::YUV) ? BMDPixelFormat::bmdFormat8BitYUV : BMDPixelFormat::bmdFormat8BitBGRA))
            supportsOutput = false;
    }
}


void DeckLinkDevice::StopCapture()
{
    EnterCriticalSection(&m_captureCardCriticalSection);

    OutputDebugString(L"Stop Capture.\n");

    if (m_deckLinkInput != NULL)
    {
        // Stop the capture
        m_deckLinkInput->StopStreams();

        // Delete capture callback
        m_deckLinkInput->SetCallback(NULL);
    }

    if (supportsOutput && m_deckLinkOutput != NULL)
    {
        m_deckLinkOutput->StopScheduledPlayback(0, NULL, 0);
        m_deckLinkOutput->DisableVideoOutput();
    }

    m_currentlyCapturing = false;
    LeaveCriticalSection(&m_captureCardCriticalSection);
}


HRESULT DeckLinkDevice::VideoInputFormatChanged(/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
    EnterCriticalSection(&m_captureCardCriticalSection);
    EnterCriticalSection(&m_outputCriticalSection);

    OutputDebugString(L"Changing Formats to: ");
    OutputDebugString(std::to_wstring(newMode->GetDisplayMode()).c_str());
    OutputDebugString(L"\n");

    // If we do not have the correct dimension frames - loop until user changes camera settings.
    if (newMode->GetWidth() != FRAME_WIDTH || newMode->GetHeight() != FRAME_HEIGHT)
    {
        OutputDebugString(L"Invalid frame dimensions detected.\n");
        OutputDebugString(L"Actual Frame Dimensions: ");
        OutputDebugString(std::to_wstring(newMode->GetWidth()).c_str());
        OutputDebugString(L", ");
        OutputDebugString(std::to_wstring(newMode->GetHeight()).c_str());
        OutputDebugString(L"\n");

        OutputDebugString(L"Expected Frame Dimensions: ");
        OutputDebugString(std::to_wstring(FRAME_WIDTH).c_str());
        OutputDebugString(L", ");
        OutputDebugString(std::to_wstring(FRAME_HEIGHT).c_str());
        OutputDebugString(L"\n");

        LeaveCriticalSection(&m_captureCardCriticalSection);
        LeaveCriticalSection(&m_outputCriticalSection);
        return E_PENDING;
    }

    pixelFormat = PixelFormat::YUV;
    BMDPixelFormat bmdPixelFormat = bmdFormat8BitYUV;

    if ((detectedSignalFlags & bmdDetectedVideoInputRGB444) != 0)
    {
        pixelFormat = PixelFormat::BGRA;
        bmdPixelFormat = bmdFormat8BitBGRA;
    }

    // Stop the capture
    m_currentlyCapturing = false;

    m_deckLinkInput->StopStreams();
    m_deckLinkInput->FlushStreams();

    if (supportsOutput && m_deckLinkOutput != NULL)
    {
        m_deckLinkOutput->StopScheduledPlayback(0, NULL, 0);
        m_deckLinkOutput->DisableVideoOutput();
    }

    // Set the video input mode
    if (m_deckLinkInput->EnableVideoInput(newMode->GetDisplayMode(), bmdPixelFormat, bmdVideoInputEnableFormatDetection) != S_OK)
    {
        OutputDebugString(L"Could not enable video input when restarting capture with detected input.\n");
        goto bail;
    }

    // Start the capture
    if (m_deckLinkInput->StartStreams() != S_OK)
    {
        OutputDebugString(L"Could not start streams when restarting capture with detected input.\n");
        goto bail;
    }

    if (m_deckLinkOutput != NULL && m_deckLinkOutput->EnableVideoOutput(newMode->GetDisplayMode(), bmdVideoOutputFlagDefault) != S_OK)
    {
        OutputDebugString(L"Unable to set video output.\n");
        supportsOutput = false;
    }

    m_currentlyCapturing = true;

bail:
    OutputDebugString(L"Done changing formats.\n");

    SetupVideoOutputFrame(newMode->GetDisplayMode());

    LeaveCriticalSection(&m_captureCardCriticalSection);
    LeaveCriticalSection(&m_outputCriticalSection);
    return S_OK;
}

HRESULT DeckLinkDevice::VideoInputFrameArrived(/* in */ IDeckLinkVideoInputFrame* frame, /* in */ IDeckLinkAudioInputPacket* audioPacket)
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    if (frame == nullptr)
    {
        return S_OK;
    }

    BMDPixelFormat framePixelFormat = frame->GetPixelFormat();

    EnterCriticalSection(&m_captureCardCriticalSection);

    //TODO: Create conversion to RGBA for any other pixel format your camera outputs at.
    if (framePixelFormat == BMDPixelFormat::bmdFormat8BitYUV)
    {
        if (frame->GetBytes((void**)&rawBuffer) == S_OK)
        {
            captureFrameIndex++;
            BYTE* buffer = bufferCache[captureFrameIndex % MAX_NUM_CACHED_BUFFERS].buffer;
            // Always return the latest buffer when using the CPU.
            if (_useCPU)
            {
                DirectXHelper::ConvertYUVtoBGRA(rawBuffer, buffer, FRAME_WIDTH, FRAME_HEIGHT, true);
            }
            else
            {
                memcpy(buffer, rawBuffer, FRAME_BUFSIZE_YUV);
            }
        }
    }
    else if (framePixelFormat == BMDPixelFormat::bmdFormat8BitBGRA)
    {
        if (frame->GetBytes((void**)&localFrameBuffer) == S_OK)
        {
            captureFrameIndex++;
            BYTE* buffer = bufferCache[captureFrameIndex % MAX_NUM_CACHED_BUFFERS].buffer;
            if (_useCPU)
            {
                //TODO: Remove this block if R and B components are swapped in color feed.
                memcpy(stagingBuffer, localFrameBuffer, FRAME_BUFSIZE_RGBA);
                DirectXHelper::ConvertBGRAtoRGBA(stagingBuffer, FRAME_WIDTH, FRAME_HEIGHT, true);
                // Do not cache frames when using the CPU
                memcpy(buffer, stagingBuffer, FRAME_BUFSIZE_RGBA);
            }
            else
            {
                memcpy(buffer, localFrameBuffer, FRAME_BUFSIZE_RGBA);
            }
        }
    }

    bufferCache[captureFrameIndex % MAX_NUM_CACHED_BUFFERS].ComputePixelChange(bufferCache[(captureFrameIndex-1) % MAX_NUM_CACHED_BUFFERS].buffer, framePixelFormat);

    LONGLONG t;
    frame->GetStreamTime(&t, &frameDuration, QPC_MULTIPLIER);

    // Get frame time.
    bufferCache[captureFrameIndex % MAX_NUM_CACHED_BUFFERS].timeStamp = time.QuadPart;

    if (supportsOutput && m_deckLinkOutput != NULL)
    {
        if (_passthroughOutput)
        {
            if (supportsOutput && m_deckLinkOutput != NULL)
            {
                m_deckLinkOutput->DisplayVideoFrameSync(frame);
            }
        }
    }

    LeaveCriticalSection(&m_captureCardCriticalSection);

    return S_OK;
}

int DeckLinkDevice::GetNumQueuedOutputFrames()
{
    return s_outputScheduler.framesQueued;
}


void DeckLinkDevice::Update(int compositeFrameIndex)
{
    if (_colorSRV != nullptr &&
        device != nullptr)
    {
        const BufferCache& buffer = bufferCache[compositeFrameIndex % MAX_NUM_CACHED_BUFFERS];
        if (buffer.buffer != nullptr)
        {
            DirectXHelper::UpdateSRV(device, _colorSRV, buffer.buffer, FRAME_WIDTH * FRAME_BPP_RGBA);
        }

        if (!_passthroughOutput && compositeFrameIndex != lastCompositorFrameIndex)
        {
            lastCompositorFrameIndex = compositeFrameIndex;
            //Output to video recording screen
            if (supportsOutput && device != nullptr && _outputTexture != nullptr && s_outputScheduler.enabled)
            {
                unsigned char* outBytes = NULL;
                EnterCriticalSection(&m_outputCriticalSection);

                if (outputTextureBuffer.IsDataAvailable())
                {
                    IDeckLinkMutableVideoFrame* videoFrame = s_outputScheduler.GetAvailableVideoFrame();
                    if (videoFrame)
                    {
                        videoFrame->GetBytes((void**)&outBytes);
                        if (pixelFormat == PixelFormat::YUV)
                        {
                            outputTextureBuffer.FetchTextureData(device, outBytes, FRAME_BPP_YUV);
                        }
                        else if (pixelFormat == PixelFormat::BGRA)
                        {
                            outputTextureBuffer.FetchTextureData(device, outBytes, FRAME_BPP_RGBA);
                        }
                        s_outputScheduler.QueueFrame(videoFrame);
                    }
                }
                outputTextureBuffer.PrepareTextureFetch(device, _outputTexture);
                LeaveCriticalSection(&m_outputCriticalSection);
            }
        }
    }
}

bool DeckLinkDevice::OutputYUV()
{
    return (pixelFormat == PixelFormat::YUV);
}


void DeckLinkDevice::BufferCache::ComputePixelChange(BYTE* prevBuffer, BMDPixelFormat framePixelFormat)
{
    pixelChange = 0;
    int bpp = framePixelFormat == BMDPixelFormat::bmdFormat8BitYUV ? FRAME_BPP_YUV : FRAME_BPP_RGBA;

    for (int x = 50; x < FRAME_WIDTH - 50; x += 100)
    {
        for (int y = 50; y < FRAME_HEIGHT - 50; y += 100)
        {
            int i = (y * FRAME_WIDTH + x) * bpp;
            pixelChange += abs(buffer[i + 1] - prevBuffer[i + 1]) + abs(buffer[i + 2] - prevBuffer[i + 2]);
        } 
    }
}


DeckLinkDeviceDiscovery::DeckLinkDeviceDiscovery()
    : m_deckLinkDiscovery(NULL), m_refCount(1)
{
    if (CoCreateInstance(CLSID_CDeckLinkDiscovery, NULL, CLSCTX_ALL, IID_IDeckLinkDiscovery, (void**)&m_deckLinkDiscovery) != S_OK)
    {
        m_deckLinkDiscovery = NULL;
    }
}

DeckLinkDeviceDiscovery::~DeckLinkDeviceDiscovery()
{
    if (m_deckLinkDiscovery != NULL)
    {
        // Uninstall device arrival notifications and release discovery object
        m_deckLinkDiscovery->UninstallDeviceNotifications();
        m_deckLinkDiscovery->Release();
        m_deckLinkDiscovery = NULL;
    }

    if (m_deckLink != nullptr)
    {
        m_deckLink->Release();
        m_deckLink = NULL;
    }
}

bool DeckLinkDeviceDiscovery::Enable()
{
    HRESULT result = E_FAIL;

    // Install device arrival notifications
    if (m_deckLinkDiscovery != NULL)
    {
        result = m_deckLinkDiscovery->InstallDeviceNotifications(this);
    }

    return result == S_OK;
}

void DeckLinkDeviceDiscovery::Disable()
{
    // Uninstall device arrival notifications
    if (m_deckLinkDiscovery != NULL)
    {
        m_deckLinkDiscovery->UninstallDeviceNotifications();
    }
}

HRESULT DeckLinkDeviceDiscovery::DeckLinkDeviceArrived(/* in */ IDeckLink* deckLink)
{
    if (m_deckLink == nullptr)
    {
        deckLink->AddRef();

        m_deckLink = deckLink;
    }

    return S_OK;
}

HRESULT DeckLinkDeviceDiscovery::DeckLinkDeviceRemoved(/* in */ IDeckLink* deckLink)
{
    deckLink->Release();
    return S_OK;
}

HRESULT    STDMETHODCALLTYPE DeckLinkDeviceDiscovery::QueryInterface(REFIID iid, LPVOID *ppv)
{
    HRESULT result = E_NOINTERFACE;

    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    // Initialise the return result
    *ppv = NULL;

    // Obtain the IUnknown interface and compare it the provided REFIID
    if (iid == IID_IUnknown)
    {
        *ppv = this;
        AddRef();
        result = S_OK;
    }
    else if (iid == IID_IDeckLinkDeviceNotificationCallback)
    {
        *ppv = (IDeckLinkDeviceNotificationCallback*)this;
        AddRef();
        result = S_OK;
    }

    return result;
}

ULONG STDMETHODCALLTYPE DeckLinkDeviceDiscovery::AddRef(void)
{
    return InterlockedIncrement((LONG*)&m_refCount);
}

ULONG STDMETHODCALLTYPE DeckLinkDeviceDiscovery::Release(void)
{
    ULONG newRefValue;

    newRefValue = InterlockedDecrement((LONG*)&m_refCount);
    if (newRefValue == 0)
    {
        delete this;
        return 0;
    }

    return newRefValue;
}
#endif
