# PCAP Parser (RTP H264/H265 Depacketizer)

This is a simple command-line tool that parses a `.pcap` file, filters RTP packets by a given Payload Type (PT), and depacketizes **H.264** or **H.265** (HEVC) video NAL units into a raw video file (`raw.h264` or `raw.h265`). The output file can be played using **ffplay** or processed with any compatible decoder.

If everything works correctly, the [pcap dump](../../data/pcap/wilson.pcap) will be depacketized into a valid video file and you'll be able to watch **Wilson**, my dog. 🐾

![Wilson](../../data/pcap/wilson.gif)

---

## Features

* Parses `.pcap` files and extracts RTP streams
* Filters by specified **RTP Payload Type (PT)**
* Supports **H.264** and **H.265** RTP depacketization (RFC 6184 / RFC 7798)
* Writes raw video stream to disk (`raw.h264` or `raw.h265`)
* Example of how to use custom RTP depacketizers in practice

---

## Requirements

* C++17 or later
* `libpcap` 
* ffmpeg (optional, for playback):

---

## Usage

```bash
./pcap-parser-app --pcap_file=wilson.pcap --type=2 --pt=104
```

This will generate `raw.h265`. Or use `help` for the description:

```
./pcap-parser-app --help
  --help                                produce help message
  --pcap_file arg (=./data/pcap/wilson.pcap)
                                        pcap file path
  --type arg (=2)                       processing type: 0 - rtp only, 1 - h264
                                        stream, 2 - h265 stream
  --pt arg (=104)                       target payload type (pt)
  --output_file arg (=raw.h265)         output file path (not used for "rtp 
                                        only" case)
```

## Playback with ffplay

To preview the extracted video:

```bash
ffplay raw.h265
```
