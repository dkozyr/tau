# Audio File - OPUS Codec Example (Converted)

This folder contains an audio file sourced from the **OPUS Codec website** (https://opus-codec.org/examples/). 

The original file was a **.wav** file named `plc_orig.wav`, which has been converted to **PCM format** using `ffmpeg`. The resulting file is `16khz_mono_pcm_s16le.raw`, a raw audio file with 16kHz sample rate, mono channel, and signed 16-bit little-endian PCM encoding.

```
ffmpeg -i plc_orig.wav -f s16le -acodec pcm_s16le 16khz_mono_pcm_s16le.raw
```

All rights for the original audio file are owned by the **OPUS Codec team**. This conversion is for demonstration and development purposes.
