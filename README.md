# PLAY BAD APPLE IN TERMNIMAL --IN LINUX --RELY ON FFMPEG

## only play char picture:
build code:
>make build_bad_apple

play :
>make test_bad_apple

or 
>./test_bad_apple path/of/video [xscale yscale]

>the output resolution will be width/xscale X height/yscale

## play with music:

build code:
>make build_audio

play:
>make start

- if your system in ALSA architecture,type **modprobe snd-pcm-oss** first


*ps: the program can also play other videos,but for better output best input black_and_white videos*

**References**:<https://github.com/leandromoreira/ffmpeg-libav-tutorial>

