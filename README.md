# ffmpeg-test
Some code for testing seeking in hls input with ffmpeg/libavformat, created to test a fix for ffmpeg ticket [7485](https://trac.ffmpeg.org/ticket/7485).

## Usage
To run the tests against your own FFmpeg build, modify `FFMPEG_DIR`in `meson.build` to point to your FFmpeg dir.

## Testdata
Generated with

% ffmpeg -y -s 640x480 -f rawvideo -pix_fmt rgb24 -r 25 -i /dev/zero -f lavfi -i "sine=frequency=440:duration=60:r=48000" -vf "drawtext=fontfile=/usr/share/fonts/truetype/freefont/FreeMono.ttf: text=%{n}: x=(w-tw)/2: y=h-(2*lh): fontcolor=white: box=1: boxcolor=0x000000FF" -muxdelay 0 -muxpreload 0 -acodec libfdk_aac -ac 2 -vcodec libx264 -preset medium -tune stillimage -crf 24 -pix_fmt yuv420p -shortest -force_key_frames "expr:gte(t,n_forced*2.5)" -bf 0 -hls_time 5 -hls_list_size 0 -hls_wrap 0 -hls_allow_cache 1 -hls_segment_filename "testdata/data3/a_%04d.ts" -t 60 testdata/data3/a.m3u8

    % ffmpeg -y -s 640x480 -f rawvideo -pix_fmt rgb24 -r 25 -i /dev/zero -f lavfi -i "sine=frequency=440:duration=64:r=48000" -muxdelay 4.97867 -muxpreload 0 -vf "drawtext=fontfile=/usr/share/fonts/truetype/freefont/FreeMono.ttf: text=%{n}: x=(w-tw)/2: y=h-(2*lh): fontcolor=white: box=1: boxcolor=0x000000FF"  -acodec libfdk_aac -ac 2 -vcodec libx264 -preset medium -tune stillimage -crf 24 -pix_fmt yuv420p -shortest -force_key_frames "expr:not(mod(n,80))" -t 64 -bf 0 -hls_time 6.4 -hls_list_size 0 -hls_wrap 0 -hls_allow_cache 1 -hls_segment_filename "stream_%v_%04d.ts" -hls_flags independent_segments -master_pl_name master.m3u8 -var_stream_map "a:0,agroup:audio v:0,agroup:audio" stream_%v.m3u8



