
.PONY : build_bad_apple
build_bad_apple : bad_apple.c
	gcc bad_apple.c -o bad_apple -lm -lavformat -lavcodec -lavfilter -lavdevice -lswresample -lswscale -lavutil -lpthread

.PONY : test_bad_apple
test_bad_apple: bad_apple
	./bad_apple /mnt/hgfs/opesys/BadApple.flv


#cat /dev/urandom | padsp tee /dev/audio >/dev/null

