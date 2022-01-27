#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include <stdio.h>
#include <exception>
#include <string>

struct ReadPacketResult {
  int ret = 0;
  AVPacket *packet = nullptr;
  bool keyframe = false;
};

class TestContext {
public:
  TestContext(std::string file):
    formatContext(nullptr),
    packet(av_packet_alloc()) {
    if(avformat_open_input(&formatContext,
                           file.c_str(),
                           NULL,
                           NULL) != 0) {
      throw std::runtime_error("failed to open file: " + file);

      //   avformat_find_stream_info(formatContext, NULL);
    }
  };
  ~TestContext() {
    if (formatContext) {
      avformat_close_input(&this->formatContext);
    }
    if (packet) {
      av_packet_unref(packet);
    }
  };

  void readPacket() {
    int ret = av_read_frame(formatContext, packet);
    if (ret < 0) {
        throw std::runtime_error("Read frame failed: " + std::to_string(ret));
    }
    av_packet_unref(packet);
  };

  AVPacket *seekFile( int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags = 0) {
    int ret = avformat_seek_file(formatContext, stream_index, min_ts, ts, max_ts, flags);
    if (ret < 0) {
      throw std::runtime_error("Seek failed: " + std::to_string(ret));
    }
    while(1) {
      ret = av_read_frame(formatContext, packet);
      if (ret < 0) {
        throw std::runtime_error("Read frame failed: " + std::to_string(ret));
      }
      if (packet->stream_index == stream_index) {
        return packet;
      } else {
        av_packet_unref(packet);
      }
    }
  };

  AVPacket *seekFrame( int stream_index, int64_t ts, int flags = 0) {
    int ret = av_seek_frame(formatContext, stream_index, ts, flags);
    if (ret < 0) {
      throw std::runtime_error("Seek failed: " + std::to_string(ret));
    }
    ret = av_read_frame(formatContext, packet);
    if (ret < 0) {
      throw std::runtime_error("Read frame failed: " + std::to_string(ret));
    }
    return packet;
  };

private:
  AVFormatContext *formatContext;
  AVPacket *packet;
};

const int NO_FLAGS = 0;

/*
  Timebase: 90000
  Video stream start: 10.0
  Gop-size: 80 frames/ 3.2s
  Segment length: 6.4s
  Keyframes at:
  10.0        900000
  13.2        1188000
  16.4        1476000
  ...


*/
const int64_t STREAM_START_TS = 900000;
const int64_t FIRST_IFRAME_TS = STREAM_START_TS;
const int64_t SECOND_IFRAME_TS = 1188000;
const int64_t THIRD_IFRAME_TS = 1476000;

const int64_t AUDIO_FRAME_DURATION = 1024 * 90000 / 48000;
const int64_t FIRST_AUDIO_TS = 896160;
const int64_t SECOND_AUDIOFRAME_TS = FIRST_AUDIO_TS + AUDIO_FRAME_DURATION;
const int VIDEO_STREAM_INDEX = 1;
const int AUDIO_STREAM_INDEX = 0;

const std::string testdata("../testdata/data3/master.m3u8");

TEST_CASE( "Seek videostream with avformat_seek_file" ) {
  TestContext ctx(testdata);
  ctx.readPacket();  // Need to read packet so that first segment ts is known when we call seek
  
  SECTION( "Ts and max Ts is first keyframe " ) {
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, FIRST_IFRAME_TS, FIRST_IFRAME_TS, NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION( "Ts on first keyframe, ts_max before second keyframe") {
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, FIRST_IFRAME_TS, SECOND_IFRAME_TS - 1000, NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts before second iframe") {
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, SECOND_IFRAME_TS - 3600, SECOND_IFRAME_TS + 36000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts after second iframe") {
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, SECOND_IFRAME_TS + 1000, SECOND_IFRAME_TS + 40000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts after second iframe") {
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, SECOND_IFRAME_TS + 1000, SECOND_IFRAME_TS + 40000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek third iframe, ts just before third iframe") {
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, THIRD_IFRAME_TS -3600, THIRD_IFRAME_TS + 36000, NO_FLAGS);
    REQUIRE(pkt->dts == THIRD_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }
}


TEST_CASE("Test for bug in hls.c find_timestamp_in_playlist", "[.]") {
  // Because HlsContext -> first_timestamp will be taken from audiostream,
  // finding the correct segement for a ts in videostream will not be accurate.
  // For the last frame in first segment, find_timestamp_in_playlist will
  // return index of second segment. So below test is expected to fail
  
  TestContext ctx(testdata);
  ctx.readPacket();

  SECTION("Seek just before third iframe, SEEK ANY") {
    int64_t ts = THIRD_IFRAME_TS - 3600;
    AVPacket* pkt = ctx.seekFile(VIDEO_STREAM_INDEX, STREAM_START_TS, ts , THIRD_IFRAME_TS + 36000, AVSEEK_FLAG_ANY);
    REQUIRE(pkt->stream_index == VIDEO_STREAM_INDEX);
    REQUIRE(pkt->dts == ts);
  }

}

TEST_CASE( "Seek audiostream with avformat_seek_file" ) { 
  TestContext ctx(testdata);
  ctx.readPacket();

  SECTION( "Ts and max Ts is first keyframe " ) {
    AVPacket* pkt = ctx.seekFile(AUDIO_STREAM_INDEX, FIRST_AUDIO_TS, SECOND_AUDIOFRAME_TS, SECOND_AUDIOFRAME_TS, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_AUDIOFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION( "Ts at third frame, ts_max before fourth frame") {
    int64_t ts = FIRST_AUDIO_TS + 3 * AUDIO_FRAME_DURATION;
    AVPacket* pkt = ctx.seekFile(AUDIO_STREAM_INDEX, 0, ts, ts + AUDIO_FRAME_DURATION/2, NO_FLAGS);
    REQUIRE(pkt->dts == ts);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

}


TEST_CASE( "Test av_seek_frame" ) { 
  TestContext ctx(testdata);
  ctx.readPacket();
  
  SECTION( "Ts is first keyframe " ) {
    AVPacket* pkt = ctx.seekFrame(VIDEO_STREAM_INDEX, FIRST_IFRAME_TS,  NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION( "Ts before second keyframe, seek forward") {
    AVPacket* pkt = ctx.seekFrame(VIDEO_STREAM_INDEX, FIRST_IFRAME_TS + 200,  NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Ts before second iframe, seek backwards") {
    AVPacket* pkt = ctx.seekFrame(VIDEO_STREAM_INDEX, SECOND_IFRAME_TS - 14400, AVSEEK_FLAG_BACKWARD);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Ts just before second iframe, seek backwards") {
    AVPacket* pkt = ctx.seekFrame(VIDEO_STREAM_INDEX, SECOND_IFRAME_TS - 3600, AVSEEK_FLAG_BACKWARD);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

}
