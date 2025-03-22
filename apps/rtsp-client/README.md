# RTSP Client

A simple RTSP client for receiving video streams from RTSP cameras.

## Features
- Connects to an RTSP camera using the following commands:
  - `OPTIONS`
  - `DESCRIBE`
  - `SETUP`
  - `PLAY`
  - `TEARDOWN`
- Supports **video-only sessions** with **H264 codec**
- Parses and verifies SDP from the camera:
  - Parses `sprop-parameter-sets` if present
- Configures the RTP session and processes the RTP stream:
  - Depacketizes RTP into NAL units and saves them in AnnexB format
  - Detects packet loss and waits for a new key-frame
- Receives and sends RTCP packets
- Saves video to the file in format `{NTP-timestamp}.h264`
- Waits for a key-frame in case of packet loss, which may cause video pauses

## Limitations
- **H264 codec only**
- Does **not** support audio streams

## Usage
1. Run the client with the specified RTSP URL:
   ```sh
   ./bin/rtsp-client-app rtsp://192.168.0.1/ch0_0.h264
   ```
2. The recorded video will be saved as `16972268366203287059.h264`
3. Play the video using `ffplay` or `ffmpeg`:
   ```sh
   ffplay 16972268366203287059.h264
   ```

## Dependencies
- A working RTSP server/camera
- `ffmpeg` (for playing the recorded file)
