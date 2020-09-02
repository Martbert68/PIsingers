/* singer.c */
/* One sings can the other find a harmony? */

#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#define SA struct sockaddr 
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAX 80 
#define PORT 9080 

/* sound */
void *spkr();
void *comms();

static float notes[108]={
0,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,
32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,55.00,58.27,61.74,
65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.8,110.0,116.5,123.5,
130.8,138.6,146.8,155.6,164.8,174.6,185.0,196.0,207.7,220.0,233.1,246.9,
261.6,277.2,293.7,311.1,329.6,349.2,370.0,392.0,415.3,440.0,466.2,493.9,
523.3,554.4,587.3,622.3,659.3,698.0,740.0,784.0,830.6,880.0,932.3,987.8,
1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 }; 

struct output { short *waveform; long where; int *lno; int *rno;};

void usage ()
{
	printf("usage: singer\n");
	exit (1);
}

int main(int argc,char *argv[])
{
 
	int *fhead,len,chan,sample_rate,bits_pers,byte_rate,ba,size;
	int number,along,osc,note;
	char stop;
	struct output *out;

        out=(struct output *)malloc(sizeof(struct output ));

        fhead=(int *)malloc(sizeof(int)*11);

        len=60*60; //1 hour
        chan=2;
        sample_rate=44100;
        bits_pers=16;
        byte_rate=(sample_rate*chan*bits_pers)/8;
        ba=((chan*bits_pers)/8)+bits_pers*65536;
        size=chan*len*sample_rate;

        out->waveform=(short *)malloc(sizeof(short)*size);
        out->lno=(int *)malloc(sizeof(int));
        out->rno=(int *)malloc(sizeof(int));

	//printf ("waiting \n");
	//scanf("%c",&stop);

       pthread_t spkr_id,comms_id;

       struct timespec tim, tim2;
               tim.tv_sec = 0;
        tim.tv_nsec = 100L;

        pthread_create(&spkr_id, NULL, spkr, out);
        pthread_create(&comms_id, NULL, comms, out);

	long my_point,ahead,chunk,psum,dtot,avg;
	chunk=2206;
	ahead=4410;
	my_point=50*ahead;
	int vamp,lamp,ramp,lnote,rnote,lcount,tnote,minnote,lchoice,a,lattack,rattack,decay;
	float min,lp,rp,fl,fr,fd,ft,thresh;

	lnote=48;lcount=0;
	rnote=60;tnote=60;
	lamp=0;ramp=0;
	minnote=0;
	lchoice=0;
	a=28820;
	lattack=1;rattack=1;
	decay=0;
	vamp=300;

	thresh=1.1;

	fr=2*M_PI*notes[lnote]/44100;

	float losc,rosc,tosc;
	losc=0;rosc=0;tosc=0;
	avg=0; fd=0;
	min=100000;
	while (1>0)
	{
		long trigger,diff;
		int wcount;
		trigger=my_point-ahead;
		wcount=0;
		while (out->where<trigger)
		{
                       nanosleep(&tim , &tim2);
		       wcount++;

		}
		if (wcount < 20){printf ("%d\n",wcount);}

		lcount++;
		//if (lcount>200){lcount=0;lnote++;if(lnote>59){lnote=48;}printf("%d\n",lnote);}
		//if (lcount>150){lcount=0;lnote=55+rand()%5;printf("%d\n",lnote);}
		if (lcount%100==0){a-=1500;vamp=0;if(a<4410){a=88200;}}
		thresh-=0.0003;if (thresh<0.1){thresh=1.1;}

		fl=M_PI*notes[lnote]/22050;
		fr=M_PI*(notes[rnote])/22050;
		ft=M_PI*(notes[tnote])/22050;
		long from,to,sum;
		from=my_point;
		to=my_point+chunk;
		sum=0;
		for (my_point=from;my_point<to;my_point+=2)
		{
			short left,right,test;
			float leftf,rightf;
			int tot;
			losc+=fl;
			rosc+=fr;
			tosc+=ft;
			//if (losc>999.9999){losc=losc-999.026463842;}
			//if (rosc>999.9999){rosc=rosc-999.026463842;}
			//if (tosc>999.9999){tosc=tosc-999.026463842;}
			leftf=(sin(losc));
			rightf=(sin(rosc));
			test=32767*(sin(tosc));
			tot=((leftf+rightf)*16384)+test;
			//threshold it
			
			if (leftf>thresh ){ leftf=1.0;}
			if (leftf<-thresh ){ leftf=-1.0;}
			if (rightf>thresh ){rightf=1.0;}
			if (rightf<-thresh ){rightf=-1.0;}
			// add reverb
			left=lamp*leftf;
			right=ramp*rightf;
			if (vamp<300){ vamp++;}
			out->waveform[my_point]=(left/2)+((vamp*out->waveform[my_point-a+1])/700);
			out->waveform[my_point+1]=(right/2)+((vamp*out->waveform[my_point-a])/700);
			if (tot<0){tot=-tot;}
			sum=sum+tot;
			avg=avg+sum;
			if (decay==1)
			{
				if (lamp>2){lamp-=2;}
				if (ramp>2){ramp-=2;}
				if (lamp<=2 && ramp<=2){ decay=0;}
			}else{
			if (lamp<32767 && my_point%lattack==0 ){lamp++;}
			if (ramp<32767 && my_point%rattack==0  ){ramp++;}
			}
		}
		diff=sum-psum;
		psum=sum;
		if (diff<0){diff=-diff;}
		dtot+=diff;
		if (lcount%15==0){
			float v;
			v=(float)(1000000*dtot)/(float)avg;
			//printf ("%ld %d %f\n",dtot, tnote,v);
			dtot=0;
			avg=0;
			if (v<min){ minnote=tnote;min=v;}
			tnote++;
			tosc=0;
			if (tnote>71){tnote=60;
				min=1000000;
				if (minnote==lchoice){ lnote=minnote-12;losc=0;rnote=0;}else{ rnote=minnote;rosc=0;}
				printf ("chosing l %d r %d a %d thresh %f\n",lnote,rnote,a,thresh);
				lchoice=minnote;
				decay=0;
				out->lno[0]=lnote;out->rno[0]=rnote;
			}
			//lattack++;
			//if (lattack>20){lattack=1;}
			//rattack=21-lattack;
		}
	} 
}


void *spkr(void *o) {
        // This handles the speakers.

  struct output *out;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;

  out=(struct output *)o;

  //char *buffer;

  /* Open PCM device for playback. */
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }
  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);
  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);
  /* Set the desired hardware parameters. */
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);
  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
  /* Set period size to 32 frames. */
  frames = 64;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
  /* Write the parameters to the aux */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  //size = frames * 4; /* 2 bytes/sample, 2 channels 
  // size as in number of data points along
  size = frames * 2;

  snd_pcm_hw_params_get_period_time(params, &val, &dir);

  while (1 > 0) {
    rc = snd_pcm_writei(handle, (out->waveform+out->where), frames);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
    out->where+=size;
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  //free(buffer);

  return 0;
}



// Function designed for chat between client and server. 
void func(int sockfd, int *lno, int *rno) 
{ 
	char buff[MAX]; 
	int n; 
	// infinite loop for chat 
	for (;;) { 
		bzero(buff, MAX); 

		// read the message from client and copy it in buffer 
		read(sockfd, buff, sizeof(buff)); 
		// print buffer which contains the client contents 
		//printf("Request From client \n"); 
		bzero(buff, MAX); 
		n = 0; 
		// copy server message in the buffer 
		//while ((buff[n++] = getchar()) != '\n') 
		//	; 

		// and send that buffer to client 
		write(sockfd, lno, sizeof(int)); 
		write(sockfd, rno, sizeof(int)); 
		write(sockfd, buff, sizeof(buff)); 

		// if msg contains "Exit" then server exit and chat ended. 
		if (strncmp("exit", buff, 4) == 0) { 
			printf("Server Exit...\n"); 
			break; 
		} 
	} 
} 

// Server  function 
void *comms (void *o) 
{ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 
	struct output *out;
  	out=(struct output *)o;

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n"); 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else{
		printf("Server listening..\n"); 
	}
	len = sizeof(cli); 

	// Accept the data packet from client and verification 
	connfd = accept(sockfd, (SA*)&cli, &len); 
	if (connfd < 0) { 
		printf("server acccept failed...\n"); 
		exit(0); 
	} 
	else 
		printf("server acccept the client...\n"); 


	// Function for chatting between client and server 
	func(connfd, out->lno, out->rno); 
	

	// After chatting close the socket 
	close(sockfd); 
} 

