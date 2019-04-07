#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>


#include<libavutil/opt.h>
#include<libavutil/channel_layout.h>
#include<libavutil/samplefmt.h>
#include<libswresample/swresample.h>

#include<sys/types.h>
#include<sys/ioctl.h>
#include<linux/soundcard.h>
#include<fcntl.h>

int fp;

uint8_t **outbuf=NULL;
int xscale = 6;
int yscale = 11;
struct timeval startTime={0,0};
int64_t countFrame = 0;
int64_t pervPts=0;
// print out the steps and errors
static void logging(const char *fmt, ...);
// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// save a frame into a .pgm file
static void show_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize);

int char2int(const char *str){
	int count = 0;
	int i = 0;
	while(str[i] != '\0'){
		count = count*10 + (str[i]-48);
		i++;
	}
	return count;
}

int main(int argc, const char *argv[])
{
 // logging("initializing all the containers, codecs and protocols.");
//uint8_t buf1[4096];
//outbuf=&buf1;
  // AVFormatContext holds the header information from the format (Container)
  // Allocating memory for this component
  // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
  if(argc >3){
	  xscale = char2int(argv[2]);
	  yscale = char2int(argv[3]);
  }
  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (!pFormatContext) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }

 // logging("opening the input file (%s) and loading format (container) header", argv[1]);
  // Open the file and read its header. The codecs are not opened.
  // The function arguments are:
  // AVFormatContext (the component we allocated memory for),
  // url (filename),
  // AVInputFormat (if you pass NULL it'll do the auto detect)
  // and AVDictionary (which are options to the demuxer)
  // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
  if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0) {
    logging("ERROR could not open the file");
    return -1;
  }

  // now we have access to some information about our file
  // since we read its header we can say what format (container) it's
  // and some other information related to the format itself.
 // logging("format %s, duration %lld us, bit_rate %lld", pFormatContext->iformat->name, pFormatContext->duration, pFormatContext->bit_rate);

 // logging("finding stream info from format");
  // read Packets from the Format to get stream information
  // this function populates pFormatContext->streams
  // (of size equals to pFormatContext->nb_streams)
  // the arguments are:
  // the AVFormatContext
  // and options contains options for codec corresponding to i-th stream.
  // On return each dictionary will be filled with options that were not found.
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
  if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
    logging("ERROR could not get the stream info");
    return -1;
  }

  // the component that knows how to enCOde and DECode the stream
  // it's the codec (audio or video)
  // http://ffmpeg.org/doxygen/trunk/structAVCodec.html
  AVCodec *pCodec = NULL;
  AVCodec *pAudioCodec = NULL;
  // this component describes the properties of a codec used by the stream i
  // https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
  AVCodecParameters *pCodecParameters =  NULL;
  AVCodecParameters *pAudioCodecParameters = NULL;
  int video_stream_index = -1;
  int audio_stream_index = -1;

  // loop though all the streams and print its main information
  for (int i = 0; i < pFormatContext->nb_streams; i++)
  {
    AVCodecParameters *pLocalCodecParameters =  NULL;
    pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
  //  logging("AVStream->time_base before open coded %d/%d", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
  //  logging("AVStream->r_frame_rate before open coded %d/%d", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
  //  logging("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
 //   logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

   // logging("finding the proper decoder (CODEC)");

    AVCodec *pLocalCodec = NULL;

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec==NULL) {
      logging("ERROR unsupported codec!");
      return -1;
    }

    // when the stream is a video we store its index, codec parameters and codec
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
      pCodec = pLocalCodec;
      pCodecParameters = pLocalCodecParameters;

     // logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
    } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      printf("Audio Codec: %d channels, sample rate %d sample_width %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate,pLocalCodecParameters->bits_per_coded_sample);
      usleep(2000000);
      audio_stream_index = i;
      pAudioCodec = pLocalCodec;
      pAudioCodecParameters = pLocalCodecParameters;
    }

    // print its name, id and bitrate
  //  logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pCodecParameters->bit_rate);
  }
 
  printf("**************************************************%d\n",pFormatContext->streams[audio_stream_index]->time_base.den);
  // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  AVCodecContext *pAudioCodecContext = avcodec_alloc_context3(pAudioCodec);
  if (!pCodecContext || ! pAudioCodecContext)
  {
    logging("failed to allocated memory for AVCodecContext");
    return -1;
  }

  // Fill the codec context based on the values from the supplied codec parameters
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
  {
    logging("failed to copy codec params to codec context");
    return -1;
  }
  if(avcodec_parameters_to_context(pAudioCodecContext , pAudioCodecParameters) < 0){
    printf("failed to copy codec params to audio codec context");
    return -1;
  }//*************************copy audio parameters

  // Initialize the AVCodecContext to use the given AVCodec.
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
  if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
  {
    logging("failed to open codec through avcodec_open2");
    return -1;
  }

  if(avcodec_open2(pAudioCodecContext , pAudioCodec,NULL) <0){
	  printf("failed to open audio codec through avcodec_open2");
	  return -1;
  }

  // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
  AVFrame *pFrame = av_frame_alloc();
  if (!pFrame)
  {
    logging("failed to allocated memory for AVFrame");
    return -1;
  }
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket)
  {
    logging("failed to allocated memory for AVPacket");
    return -1;
  }
/*************audio frame***********/
  AVFrame *pAudioFrame = av_frame_alloc();
  if(!pAudioFrame){
	  printf("failed to alloc mem audio");
	  return -1;
  }
  AVPacket *pAudioPacket = av_packet_alloc();
  if(!pAudioPacket){
	  printf("fail allocate audio packet");
	  return -1;
  }
  int response = 0;
  int how_many_packets_to_process = -1;

  gettimeofday(&startTime ,NULL);
  // fill the Packet with data from the Stream
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
 /* while (av_read_frame(pFormatContext, pPacket) >= 0)
  {
    // if it's the video stream
    if (pPacket->stream_index == video_stream_index) {
 //   logging("AVPacket->pts %" PRId64, pPacket->pts);
      response = decode_packet(pPacket, pCodecContext, pFrame);
      if (response < 0)
        break;
      // stop it, otherwise we'll be saving hundreds of frames
      if (--how_many_packets_to_process == 0) break;
    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
    av_packet_unref(pPacket);
  }*/

  fp = open("/dev/dsp",O_WRONLY);
  if(fp <0){printf("fail to open device\n");return -1;}
  while (av_read_frame(pFormatContext, pAudioPacket) >= 0)
  {
    // if it's the video stream
    if (pAudioPacket->stream_index == audio_stream_index) {
 //   logging("AVPacket->pts %" PRId64, pPacket->pts);
      response = decode_packet(pAudioPacket, pAudioCodecContext, pAudioFrame);
      if (response < 0)
        break;
      // stop it, otherwise we'll be saving hundreds of frames
      if (--how_many_packets_to_process == 0) break;
    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
    av_packet_unref(pAudioPacket);
  }


//  logging("releasing all the resources");

  avformat_close_input(&pFormatContext);
  avformat_free_context(pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pCodecContext);
  return 0;
}

static void logging(const char *fmt, ...)
{
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
  int response = avcodec_send_packet(pCodecContext, pPacket);
  int linesize;
  struct timeval now;
  int sleeptime = 0;
  long st=0;
//uint8_t **outbuf=NULL;
  if (response < 0) {
    logging("Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }
printf("hello1\n");
  while (response >= 0)
  {
    // Return decoded output data (into a frame) from a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }

    if (response >= 0) {
  /*    logging(
          "Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
          pCodecContext->frame_number,
          av_get_picture_type_char(pFrame->pict_type),
          pFrame->pkt_size,
          pFrame->pts,
          pFrame->key_frame,
          pFrame->coded_picture_number
      );
      
      char frame_filename[1024];
      snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);*/
      // save a grayscale frame into a .pgm file
      
 //     show_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height);
     // ioctl(fp , SOUND_PCM_WRITE_BITS,&pFrame->);
//	printf("bit depth: %d\n",av_get_bytes_per_sample(pCodecContext->sample_fmt));

      
//usleep(2000000);
	
	    SwrContext *swr_ctx = swr_alloc();
	    if(!swr_ctx){
		    printf("fail to alloc swr_ctx mem\n");
		    return -1;
	    }
	    av_opt_set_int(swr_ctx,"in_channel_layout",AV_CH_LAYOUT_NATIVE , 0);
	    av_opt_set_int(swr_ctx , "in_sample_rate" , pFrame->sample_rate , 0);
	    av_opt_set_sample_fmt(swr_ctx,"in_sample_fmt" , pCodecContext->sample_fmt , 0);
	    av_opt_set_int(swr_ctx , "out_channel_layout", AV_CH_LAYOUT_MONO ,0);
	    av_opt_set_int(swr_ctx , "out_sample_rate" , pFrame->sample_rate , 0);
	    av_opt_set_sample_fmt(swr_ctx , "out_sample_fmt" , AV_SAMPLE_FMT_S16,0);

printf("hello2\n");
	    int ret = swr_init(swr_ctx);
	    if(ret< 0){
		    printf("fail to init swr\n");
		    return -1;
	    }
printf("hello3\n");
  if(outbuf ==NULL){	
printf("hello\n");

	int size = 16;
printf("sample depth %d\n",size);
	ioctl(fp , SOUND_PCM_WRITE_BITS,&size);
	size = 0;
	ioctl(fp , SNDCTL_DSP_STEREO ,&size );
	size = pFrame->sample_rate;
printf("sample rate %d\n",size);
	ioctl(fp , SNDCTL_DSP_SPEED , &size);
if(pFrame->data[1]!=NULL){printf("we got second channel\n");}
else{printf("no second\n");}
printf("channels is %d\n",pFrame->channels);
       	 ret = av_samples_alloc_array_and_samples(&outbuf,&linesize,1,pFrame->nb_samples , AV_SAMPLE_FMT_S16,0);
  
	 if(ret<0){
		 printf("fail alloc dst sample mem\n");
		 return -1;
	 }
  }
	  ret =   swr_convert(swr_ctx , outbuf , pFrame->nb_samples , pFrame->data , pFrame->nb_samples);
	if(ret <0){
		printf("fail to convert\n");
		return -1;
	}
//	int size =8* av_get_bytes_per_sample(pCodecContext->sample_fmt);
printf("hello\n");
	write(fp , outbuf[0] , pFrame->nb_samples*2);
printf("pframe pts: %ld\n",pFrame->pts);
//	av_freep(&outbuf[0]);
//	av_freep(&outbuf);
	int delay = pFrame->pts - pervPts;
	//usleep(delay*1000);
	pervPts = pFrame->pts;
      av_frame_unref(pFrame);
    }
  }
 /*
  // Supply raw packet data as input to a decoder
  // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
  int response = avcodec_send_packet(pCodecContext, pPacket);
  
  struct timeval now;
  int sleeptime = 0;
  long st=0;
  if (response < 0) {
    logging("Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  while (response >= 0)
  {
    // Return decoded output data (into a frame) from a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }

    if (response >= 0) {*/
  /*    logging(
          "Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
          pCodecContext->frame_number,
          av_get_picture_type_char(pFrame->pict_type),
          pFrame->pkt_size,
          pFrame->pts,
          pFrame->key_frame,
          pFrame->coded_picture_number
      );

      char frame_filename[1024];
      snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);*/
      // save a grayscale frame into a .pgm file
      
 //     show_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height);
      
     /* if(countFrame % 10 ==0){
      gettimeofday(&now ,NULL);
//      if(pervTime.tv_sec!=0 || pervTime.tv_usec!=0){
//	sleeptime = (pFrame->pts - pervPts)*1000;
//	printf("1*********************###########sleeptime: %d\n",sleeptime);
	sleeptime = (pFrame->pts -  (now.tv_sec - startTime.tv_sec)*1000+(now.tv_usec - startTime.tv_usec)/1000);

//	printf("2*********************###########sleeptime: %d\n",sleeptime);
//	printf("###################tv_[u]sec: %d\n",(now.tv_sec - pervTime.tv_sec)*1000000+(now.tv_usec - pervTime.tv_usec));
       if(sleeptime>=0){
       	  
      show_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height);
       }else{
       	sleeptime=0;
       }
//	printf("3*********************###########sleeptime: %d\n",sleeptime);
	usleep(sleeptime*1000);
//      }
      }else{

      show_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height);
      	usleep((pFrame->pts-pervPts)*1000);
      }
      countFrame++;
	pervPts = pFrame->pts;
//	pervTime = now;
     // usleep(33000);
      av_frame_unref(pFrame);
    }
  }
 */
  return 0;
}

static void show_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize)
{
	char linestr[xsize/xscale +1];
	int i,j;
	for( i=0;i<ysize; i =i+yscale){
		for(j=0 ; j<xsize ; j=j+xscale){
			if(*(buf+i*wrap+j)>127){
				linestr[j/xscale] = 'a';
			}else{
				linestr[j/xscale] = ' ';
			}
		}
		linestr[j/xscale] = '\0';
		printf("%s\n",linestr);
	}
//	printf("\033[%dA",ysize/yscale);
	printf("\033[0;0H");				
}
