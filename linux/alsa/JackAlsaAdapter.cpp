/*
Copyright (C) 2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "JackAlsaAdapter.h"

namespace Jack
{

int JackAlsaAdapter::Open()
{
    if (fAudioInterface.open() == 0) {
        fAudioInterface.longinfo();
        fThread.AcquireRealTime(85);
        fThread.StartSync();
        return 0;
    } else {
        return -1;
    }
}

int JackAlsaAdapter::Close()
{
#ifdef DEBUG
    fTable.Save();
#endif
    fThread.Stop();
    return fAudioInterface.close();
}

bool JackAlsaAdapter::Init()
{
    fAudioInterface.write();
    fAudioInterface.write();
    return true;
}
            
bool JackAlsaAdapter::Execute()
{
    if (fAudioInterface.read() < 0)
        return false;

    bool failure = false;
    jack_nframes_t time1, time2; 
    ResampleFactor(time1, time2);
  
    for (int i = 0; i < fCaptureChannels; i++) {
        fCaptureRingBuffer[i]->SetRatio(time1, time2);
        if (fCaptureRingBuffer[i]->WriteResample(fAudioInterface.fInputSoftChannels[i], fBufferSize) < fBufferSize)
            failure = true;
    }
    
    for (int i = 0; i < fPlaybackChannels; i++) {
        fPlaybackRingBuffer[i]->SetRatio(time2, time1);
        if (fPlaybackRingBuffer[i]->ReadResample(fAudioInterface.fOutputSoftChannels[i], fBufferSize) < fBufferSize)
            failure = true;
    }

#ifdef DEBUG
    fTable.Write(time1, time2, double(time1) / double(time2), double(time2) / double(time1),
        fCaptureRingBuffer[0]->ReadSpace(), fPlaybackRingBuffer[0]->WriteSpace());
#endif
        
    if (fAudioInterface.write() < 0)
        return false;
     
    // Reset all ringbuffers in case of failure
    if (failure) {
        jack_error("JackAlsaAdapter::Execute ringbuffer failure... reset");
        ResetRingBuffers();
    }
    return true;
}

int JackAlsaAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    JackAudioAdapterInterface::SetBufferSize(buffer_size);
    Close();
    return Open();
}
        
}
