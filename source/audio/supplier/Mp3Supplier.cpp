/* Mp3Supplier.cpp
Copyright (c) 2025 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Mp3Supplier.h"

#define MA_IMPLEMENTATION
#include <miniaudio.h>

#include <array>
#include <cmath>
#include <cstring>
#include <utility>




Mp3Supplier::Mp3Supplier(std::shared_ptr<std::iostream> data, bool looping) : AsyncAudioSupplier(std::move(data), looping) {}


void Mp3Supplier::Decode()
{
  std::vector<sample_t> samples;
  ma_decoder decoder;

  ma_decoder_config config = ma_decoder_config_init(
      ma_format_s16,  // match sample_t if int16_t
      0,              // auto channels
      0               // auto sample rate
  );

  if (ma_decoder_init(OnRead, nullptr, this, &config, &decoder) != MA_SUCCESS)
    return;

  constexpr ma_uint64 framesPerChunk = 4096;
  std::vector<ma_int16> pcmBuffer(framesPerChunk * 2); // 2 = max stereo

  while (true)
  {
    AwaitBufferSpace();

    if (done)
    {
      PadBuffer();
      break;
    }

    ma_uint64 framesRead = 0;
    ma_result result = ma_decoder_read_pcm_frames(
        &decoder,
        pcmBuffer.data(),
        framesPerChunk,
        &framesRead
    );

    if (result == MA_AT_END || framesRead == 0)
    {
      AddBufferData(samples);
      break;
    }

    size_t totalSamples = framesRead * decoder.outputChannels;
    samples.insert(samples.end(), pcmBuffer.begin(), pcmBuffer.begin() + totalSamples);

    AddBufferData(samples);
    samples.clear();
  }

  ma_decoder_uninit(&decoder);
}

ma_result Mp3Supplier::OnRead(ma_decoder *pDecoder, void *pBufferOut, size_t bytesToRead, size_t *pBytesRead)
{
  Mp3Supplier *self = reinterpret_cast<Mp3Supplier *>(pDecoder->pUserData);

  size_t read = self->ReadInput(reinterpret_cast<char *>(pBufferOut), bytesToRead);

  if(pBytesRead) *pBytesRead = read;

  if(read == 0) return MA_AT_END;

  return MA_SUCCESS;
}
