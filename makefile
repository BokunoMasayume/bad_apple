
.PONY : build_bad_apple
build_bad_apple : bad_apple.c
	gcc bad_apple.c -o bad_apple -lm -lavformat -lavcodec -lavfilter -lavdevice -lswresample -lswscale -lavutil -lpthread

.PONY : test_bad_apple
test_bad_apple: bad_apple
	./bad_apple /mnt/hgfs/opesys/BadApple.flv

.PONY : build_audio
build_audio : ba_with_audio.c
	gcc ba_with_audio.c -o ba_with_audio -lm -lavformat -lavcodec -lavfilter -lavdevice -lswresample -lswscale -lavutil -lpthread
#cat /dev/urandom | padsp tee /dev/audio >/dev/null

.PONY : start
start : ba_with_audio
	./ba_with_audio /mnt/hgfs/opesys/BadApple.flv




