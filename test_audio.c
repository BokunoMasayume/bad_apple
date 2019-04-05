#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<linux/soundcard.h>
#include<fcntl.h>

int length = 3;
int rate = 44100;
int size = 16;
int channels = 2;

unsigned char buff[3*44100*2*2];

int main(){
	int fp;
	int status;
//	fp = open("/proc/asound/pcm",O_RDONLY);
	fp = open("/dev/dsp",O_RDONLY);
	if(fp<0){
		fprintf(stderr , "fail to open sound file\n");
		return -1;

	}
	status = ioctl(fp,SOUND_PCM_WRITE_BITS,&size);
if(status<0){printf("fail ioctl");}
	status = ioctl(fp , SNDCTL_DSP_STEREO,&channels);
if(status<0){printf("fail ioctl");}

	status = ioctl(fp , SNDCTL_DSP_SPEED , &rate);
if(status<0){printf("fail ioctl\n");}

	printf("say it\n");
	status = read(fp , buff , sizeof(buff));
	close(fp);
	printf("u saied:\n");

	fp = open("/dev/dsp",O_WRONLY);
	if(fp<0){
		printf("failed to open write\n");
		return -1;
	}


	status = ioctl(fp,SOUND_PCM_WRITE_BITS,&size);
if(status<0){printf("fail ioctl");}

	status = ioctl(fp , SNDCTL_DSP_STEREO,&channels);
if(status<0){printf("fail ioctl");}

	status = ioctl(fp , SNDCTL_DSP_SPEED , &rate);
if(status<0){printf("fail ioctl");}
	
	status = write(fp , buff,sizeof(buff));
	close(fp);

	return 0;
}
