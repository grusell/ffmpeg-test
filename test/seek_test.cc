#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include <stdio.h>
#include <exception>

struct ReadPacketResult {
  int ret = 0;
  AVPacket *packet = nullptr;;
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

  AVPacket *seek( int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags = 0) {
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

TEST_CASE( "Test seek" ) {
  /*
    Timebase: 90000
    Stream start: 1.4
    Keyframes at:
    1.400000   126000
    6.400000   576000
    11.400000  1026000
    16.400000  1476000
    21.400000
    26.400000
    31.400000
    36.400000
    41.400000
    46.400000
    51.400000
    56.400000


   */
  const int64_t FIRST_IFRAME_TS = 126000;
  const int64_t SECOND_IFRAME_TS = 576000;
  const int64_t LAST_IFRAME_TS = 5076000;
  
  TestContext ctx("../testdata/data1/a.m3u8");

  SECTION( "Ts and max Ts is first keyframe " ) {
    //   test_seek(formatContext, stream_index, 0, 126000, 126000, 0, 126000);
    AVPacket* pkt = ctx.seek(0, 0, FIRST_IFRAME_TS, FIRST_IFRAME_TS, NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION( "Ts on first keyframe, ts_max before second keyframe") {
    AVPacket* pkt = ctx.seek(0, 0, FIRST_IFRAME_TS, SECOND_IFRAME_TS - 1000, NO_FLAGS);
    REQUIRE(pkt->dts == FIRST_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts before second iframe") {
     AVPacket* pkt = ctx.seek(0, 0, SECOND_IFRAME_TS - 76000, 1000000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

  SECTION("Seek second iframe, ts after second iframe") {
     AVPacket* pkt = ctx.seek(0, 0, SECOND_IFRAME_TS + 76000, 1000000, NO_FLAGS);
    REQUIRE(pkt->dts == SECOND_IFRAME_TS);
    REQUIRE(pkt->flags & AV_PKT_FLAG_KEY);
  }

}
