#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include <stdio.h>
#include <exception>

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

  AVPacket *seekFile( int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags = 0) {
    int ret = avformat_seek_file(formatContext, stream_index, min_ts, ts, max_ts, flags);
    if (ret < 0) {
      throw std::runtime_error("Seek failed: " + std::to_string(ret));
    }
    ret = av_read_frame(formatContext, packet);
    if (ret < 0) {
      throw std::runtime_error("Read frame failed: " + std::to_string(ret));
    }
    return packet;
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

int test_seek(AVFormatContext *formatContext, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int expected_error, int64_t expected_ts) {
  int ret;
  int success = 0;
  char message[1024];

//  printf("Seeking for ts %d in [%d, %d]\n", ts, min_ts, max_ts);
  if ((ret = avformat_seek_file(formatContext, stream_index, min_ts, ts, max_ts, 0)) < 0) {
    sprintf(message, "Seek failed: %d", ret);
    success = (ret == expected_error);
  } else {
    AVPacket *packet = av_packet_alloc();
    av_read_frame(formatContext, packet);
//    printf("Got packet with dts=%d, keyframe=%d\n", packet->dts, packet->flags & AV_PKT_FLAG_KEY);
    success = (expected_ts == packet->dts);
    sprintf(message, "expected %ld, got %ld", expected_ts, packet->dts);
  }
  if (success) {
    printf("%70s%s\n", " ", "SUCCESS");
  } else {
    printf("%-70s%s\n\n", message, "FAIL");
  }
  return -1;
}

const int NO_FLAGS = 0;

  /*
    Timebase: 90000
    Stream start: 0
    Keyframes at:
    0.0        0
    5.0        450000
    10.0       900000
    ...

   */
  const int64_t FIRST_IFRAME_TS = 0;
  const int64_t SECOND_IFRAME_TS = 450000;
  const int64_t LAST_IFRAME_TS = 4950000;

TEST_CASE( "Test avformat_seek_file" ) { 
  TestContext ctx("../testdata/data1/b.m3u8");

  SECTION( "Ts and max Ts is first keyframe " ) {
    AVPacket* pkt = ctx.seekFile(0, 0, FIRST_IFRAME_TS, FIRST_IFRAME_TS, NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION( "Ts on first keyframe, ts_max before second keyframe") {
    AVPacket* pkt = ctx.seekFile(0, 0, FIRST_IFRAME_TS, SECOND_IFRAME_TS - 1000, NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts before second iframe") {
     AVPacket* pkt = ctx.seekFile(0, 0, SECOND_IFRAME_TS - 1000, SECOND_IFRAME_TS + 40000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts after second iframe") {
    AVPacket* pkt = ctx.seekFile(0, 0, SECOND_IFRAME_TS + 1000, SECOND_IFRAME_TS + 40000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }
}


TEST_CASE( "Test av_seek_frame" ) { 
  TestContext ctx("../testdata/data1/b.m3u8");

  SECTION( "Ts is first keyframe " ) {
    AVPacket* pkt = ctx.seekFrame(0, FIRST_IFRAME_TS,  NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION( "Ts before second keyframe, seek forward") {
    AVPacket* pkt = ctx.seekFrame(0, FIRST_IFRAME_TS + 200,  NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Ts before second iframe, seek backwards") {
     AVPacket* pkt = ctx.seekFrame(0, SECOND_IFRAME_TS - 1000, AVSEEK_FLAG_BACKWARD);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

}
