/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* This is a JNI example where we use native methods to play sounds
 * using OpenSL ES. See the corresponding Java source file located at:
 *
 *   src/com/example/nativeaudio/NativeAudio/NativeAudio.java
 */
#include <time.h>
#include <android/log.h>
#include <stdlib.h>
#include <assert.h>
#include <jni.h>
#include <string.h>
#include <pthread.h>
#include <fftw3.h>
#include <inttypes.h>

// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
// #include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

// pre-recorded sound clips, both are 8 kHz mono 16-bit signed little endian
static const char hello[] =
#include "hello_clip.h"
;

static const char android[] =
#include "android_clip.h"
;

static int64_t now_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return (now.tv_sec*1000000000LL + now.tv_nsec);
}

JNIEXPORT int64_t JNICALL
Java_com_example_nativeaudio_NativeAudio_nowms2(JNIEnv *env, jclass clazz) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return (now.tv_sec*1000000000LL + now.tv_nsec);
}

// thread lock
pthread_mutex_t speaker_mutex;

// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

//int bias=200;
//int win_size=2400;
int filt_offset=127;
int xcorr_counter=0;
int naiser_index_to_process=-1;
int speed=-1;
int sendDelay=-1;
jboolean responder=JNI_FALSE;

int* xcorr_helper2(void* context, short* data, int globalOffset, int N);
jboolean freed = JNI_FALSE;

float initialCalibrationDelay=0;
int chirpsPlayed = 0;
int period_wait = 2; // the interval between different recv signal detection
int recv_indexes[150];
double dtx = .8/100;
double drx = 3.3/100;
typedef struct mycontext{
    int totalSegments;
    int queuedSegments;
    int processedSegments;
    int playOffset;
    int bufferSize;
    int timingOffset;
    int initialOffset;
    short* data;
    short* bigdata;
    short* refData;
    char* topfilename;
    char* bottomfilename;
    char* meta_filename;
    double* naiserTx1;
    double* naiserTx2;
    int naiserTx1Count;
    int naiserTx2Count;
    int speed;
    float minPeakDistance;
    jboolean getOneMoreFlag;
    jboolean waitforFSK;
    int sendDelay;
    jboolean runXcorr;
    jboolean sendReply;
    jboolean responder;
    jboolean naiser;
    jint warmdownTime;
    jint preamble_len;
    jfloat xcorrthresh;
    jint Ns;
    jint N0;
    jint N_FSK;
    jboolean CP;
    jfloat naiserThresh;
    jfloat naiserShoulder;
    jint win_size;
    jint bias;
    int seekback;
    double pthresh;
    int recorder_offset;
    int filenum;
    int dataSize;
    int initialDelay;
    int64_t* mic_ts;
    int64_t* speaker_ts;
    int ts_len;
    char* speaker_ts_fname;
    char* mic_ts_fname;
    int bigBufferSize;
    int bigBufferTimes;
    JNIEnv *env;
    jclass clazz;
    int numSyms;
    int calibWait;
}mycontext;

jboolean wroteToDisk=JNI_FALSE;
mycontext* cxt=NULL;
mycontext* cxt2=NULL;
// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;
static SLmilliHertz bqPlayerSampleRate = 0;
static short *resampleBuf = NULL;
jboolean forcewrite=JNI_FALSE;
static int lock=0;
static int next_segment_num = 0;
// a mutext to guard against re-entrance to record & playback
// as well as make recording and playing back to be mutually exclusive
// this is to avoid crash at situations like:
//    recording is in session [not finished]
//    user presses record button and another recording coming in
// The action: when recording/playing back is not finished, ignore the new request
static pthread_mutex_t  audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
        SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

// URI player interfaces
static SLObjectItf uriPlayerObject = NULL;

// file descriptor player interfaces
static SLObjectItf fdPlayerObject = NULL;

// recorder interfaces
static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord;
static SLAndroidSimpleBufferQueueItf recorderBufferQueue;

// synthesized sawtooth clip
#define SAWTOOTH_FRAMES 8000
static short sawtoothBuffer[SAWTOOTH_FRAMES];

// 5 seconds of recorded audio at 16 kHz mono, 16-bit signed little endian
jint FS=0;
static short* recorderBuffer;
static unsigned recorderSize = 0;

double distance1=0;
double distance2=0;
double distance3=0;

int self_chirp_idx=-1;
int last_chirp_idx=-1;
double chirp_indexes[5] = {-1,-1,-1,-1,-1};
// short PN_seq[7] = {1, 1, 1, -1, 1, -1, -1};
//PN_seq[5] = [1, 1, -1, 1, -1];
int fre_idx[6][18] = {0};
int N_fre = 18;

short PN_seq[7] = {0};


int lastidx=0;
double last_xcorr_val=0;
double last_naiser_val=0;

jboolean reply_ready=JNI_FALSE;
// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;

int receivedIdx=-1;
int replyIdx1 = -1;
int replyIdx2 = -1;
int replyIdx3 = -1;

char* getString_d(double* data, int N) {
    int maxCharLength = 16;
    int maxLineLength = 1023;
    char* str=calloc(maxCharLength*N,sizeof(char));
    int index = 0;
    for (int i = 0; i < N; i++) {
        index += sprintf(&str[index],"%.5f,",(data[i]));
    }
    return str;
}

char* getString_s(short* data, int N) {
    int maxCharLength = 11;
    int maxLineLength = 1023;
    char* str=calloc(maxCharLength*N,sizeof(char));
    int index = 0;
    for (int i = 0; i < N; i++) {
        index += sprintf(&str[index],"%d,",(int)(data[i]));
    }
    return str;
}

// synthesize a mono sawtooth wave and place it into a buffer (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void)
{
    unsigned i;
    for (i = 0; i < SAWTOOTH_FRAMES; ++i) {
        sawtoothBuffer[i] = 32768 - ((i % 100) * 660);
    }
}

void releaseResampleBuf(void) {
    if( 0 == bqPlayerSampleRate) {
        /*
         * we are not using fast path, so we were not creating buffers, nothing to do
         */
        return;
    }

    free(resampleBuf);
    resampleBuf = NULL;
}

/*
 * Only support up-sampling
 */
short* createResampledBuf(uint32_t idx, uint32_t srcRate, unsigned *size) {
    short  *src = NULL;
    short  *workBuf;
    int    upSampleRate;
    int32_t srcSampleCount = 0;

    if(0 == bqPlayerSampleRate) {
        return NULL;
    }
    if(bqPlayerSampleRate % srcRate) {
        /*
         * simple up-sampling, must be divisible
         */
        return NULL;
    }
    upSampleRate = bqPlayerSampleRate / srcRate;

    switch (idx) {
        case 0:
            return NULL;
        case 1: // HELLO_CLIP
            srcSampleCount = sizeof(hello) >> 1;
            src = (short*)hello;
            break;
        case 2: // ANDROID_CLIP
            srcSampleCount = sizeof(android) >> 1;
            src = (short*) android;
            break;
        case 3: // SAWTOOTH_CLIP
            srcSampleCount = SAWTOOTH_FRAMES;
            src = sawtoothBuffer;
            break;
        case 4: // captured frames
            srcSampleCount = recorderSize / sizeof(short);
            src =  recorderBuffer;
            break;
        default:
            assert(0);
            return NULL;
    }

    resampleBuf = (short*) malloc((srcSampleCount * upSampleRate) << 1);
    if(resampleBuf == NULL) {
        return resampleBuf;
    }
    workBuf = resampleBuf;
    for(int sample=0; sample < srcSampleCount; sample++) {
        for(int dup = 0; dup  < upSampleRate; dup++) {
            *workBuf++ = src[sample];
        }
    }

    *size = (srcSampleCount * upSampleRate) << 1;     // sample format is 16 bit
    return resampleBuf;
}
// z = abs(c)
double Complex_abs(fftw_complex c){
    return sqrt(pow(c[0], 2 ) + pow(c[1], 2 ) );
}

// z = power(c, 2)
double Complex_power(fftw_complex c){
    return pow(c[0], 2 ) + pow(c[1], 2 );
}

void abs_complex(double * out, fftw_complex* in, unsigned long int Nu){
    unsigned long int  i = 0;
    for( i = 0; i < Nu ; ++i){
        out[i] = Complex_abs(in[i]);
    }
}

double magnitude_to_decibels(double magnitude) {
    return 20 * log10(magnitude);
}

// Apply the FSk to check the user id of packets.
int check_user_id(short* sig, unsigned long int sig_len){
    fftw_complex *in , *out;
    fftw_plan p;
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * sig_len);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * sig_len);
    double*  result = calloc(sig_len, sizeof(double));
    for (int i = 0; i < sig_len; i++) {
        out[i][0] = 0;
        out[i][1] = 0;
    }

    for (int i = 0; i < sig_len; i++) {
        in[i][0] = (double)sig[i];
        in[i][1] = 0;
    }

    p = fftw_plan_dft_1d(sig_len, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    abs_complex(result, out, sig_len);

    // iterate through all users
    int max_user = -1;
    double max_snr = -99;
    double snr = 0;
    int i, j, k = 0;
    int middle_idx = 0;
    int tmp_sum = 0;
    double ratio = 0;
    int nearby = 7;
    if(N_fre == 18){
        nearby = 7;
    }else{
        nearby = 12;
    }
    for(i = 0; i < 6; ++i){
        snr = 0;
        for(j = 0; j < N_fre; ++j){
            middle_idx = fre_idx[i][j] - 1;
            tmp_sum = 0;
            for(k = middle_idx - nearby; k < middle_idx; k++){
                tmp_sum += result[k];
            }
            for(k = middle_idx + 1; k < middle_idx + nearby+1; k++){
                tmp_sum += result[k];
            }
            ratio = result[middle_idx]/(tmp_sum/(nearby*2.0));
            snr += magnitude_to_decibels(ratio);
        }
        snr = snr/N_fre;
        if(snr > max_snr){
            max_snr = snr;
            max_user = i;
        }
        __android_log_print(ANDROID_LOG_VERBOSE,"user id","user id = %d %.3f",i,snr);

    }


    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);
    free(result);
    return max_user;
}



speakerCounter=0;
// Callback function for speaker buffer, it loads data from the Java buffer to the hardware speaker buffer.
void static bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    mycontext* cxt = (mycontext*)context;

    cxt->speaker_ts[speakerCounter++] = now_ms();

    assert(bq == bqPlayerBufferQueue);

    SLresult result;
    if (cxt->queuedSegments<cxt->totalSegments) {
//        __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","%d %d %d %d",cxt->queuedSegments,cxt->totalSegments,cxt->playOffset,replyIdx1);
        pthread_mutex_lock(&speaker_mutex);
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, cxt->data+cxt->playOffset, cxt->bufferSize*sizeof(short));
        pthread_mutex_unlock(&speaker_mutex);
        assert(SL_RESULT_SUCCESS == result);

        cxt->playOffset += cxt->bufferSize;
        cxt->queuedSegments+=1;
    }
    else {
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug2", "write speaker %s %d",cxt->speaker_ts_fname,cxt->ts_len);
        FILE* fp = fopen(cxt->speaker_ts_fname,"w+");
        fp = fopen(cxt->speaker_ts_fname,"w+");
        for (int i = 0; i < cxt->ts_len-1; i++) {
//            fprintf(fp,"%f\n",cxt->speaker_ts[i]);
//            fprintf(fp,"%d\n",cxt->speaker_ts[i]);
            fprintf(fp, "%" PRId64 "\n", cxt->speaker_ts[i]);
        }
        fclose(fp);
        free(cxt->speaker_ts);
        __android_log_print(ANDROID_LOG_VERBOSE, "debug2", "write speaker done");
    }
}


// computer the fft of input array, output a 2xF array where first row is the real part and second row is the imag part.
double** fftcomplexoutnative_double(double* data, jint dataN, jint bigN) {
    fftw_complex *in, *out;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * bigN);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * bigN);

    for (int i = 0; i < bigN; i++) {
        in[i][0] = 0;
        in[i][1] = 0;
        out[i][0] = 0;
        out[i][1] = 0;
    }

    for (int i = 0; i < dataN; i++) {
        in[i][0] = data[i];
    }

    p = fftw_plan_dft_1d(bigN, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

//    __android_log_print(ANDROID_LOG_VERBOSE, "hello", "%f %f %f",data[0],out[0][0],out[1][0]);

    double** outarray = calloc(2,sizeof(double*));
    outarray[0] = calloc(bigN,sizeof(double));
    outarray[1] = calloc(bigN,sizeof(double));

    for (int i = 0; i < bigN; i++) {
        outarray[0][i] = out[i][0];
        outarray[1][i] = out[i][1];
    }

    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);

    return outarray;
}

void conjnative(double** data, int N) {
    for (int i = 0; i < N; i++) {
        data[1][i] = -data[1][i];
    }
}

//compute the muplitcation of two complex number array
double** timesnative(double** c1, double** c2, int N) {
    double* real1 = c1[0];
    double* imag1 = c1[1];
    double* real2 = c2[0];
    double* imag2 = c2[1];

    double** outarray = calloc(2,sizeof(double*));
    outarray[0] = calloc(N,sizeof(double));
    outarray[1] = calloc(N,sizeof(double));

    for (int i = 0; i < N; i++) {
        outarray[0][i] = real1[i]*real2[i]-imag1[i]*imag2[i];
        outarray[1][i] = imag1[i]*real2[i]+real1[i]*imag2[i];
    }

    return outarray;
}

// compute the inverse fft
double* ifftnative(double** data, int N) {
    fftw_complex *in , *out;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

    for (int i = 0; i < N; i++) {
        in[i][0] = 0;
        in[i][1] = 0;
        out[i][0] = 0;
        out[i][1] = 0;
    }

    double *realArray = data[0];
    double *imagArray = data[1];
    for (int i = 0; i < N; i++) {
        in[i][0] = realArray[i];
    }
    for (int i = 0; i < N; i++) {
        in[i][1] = imagArray[i];
    }

    p = fftw_plan_dft_1d(N, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    double* realout = calloc(N,sizeof(double));
    int counter = 0;
    for (int i = 0; i < N; i++) {
//    for (int i = N-1; i >= 0; i--) {
        realout[counter++] = out[i][0]/N;
    }

//    char* str5=getString_d(realout,N/2);

    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);

    return realout;
}


// computer the cross-correlation of 2 input arrays and then seek the peaks in the cross-correlation.
int* xcorr(double* filteredData, double* refData, int filtLength, int refLength, int i, int globalOffset, int xcorrthresh, double minPeakDistance, int seekback, double pthresh, jboolean getOneMoreFlag) {
    double** a=fftcomplexoutnative_double(filteredData, filtLength, filtLength);
    double** b=fftcomplexoutnative_double(refData, refLength, filtLength);

    conjnative(b,filtLength);
    double** multout = timesnative(a, b,filtLength);

    double* corr = ifftnative(multout,filtLength);


    free(a[0]);
    free(a[1]);
    free(a);
    free(b[0]);
    free(b[1]);
    free(b);
    free(multout[0]);
    free(multout[1]);
    free(multout);
    double maxval=-1;
    int maxidx=-1;
//
    for (int i = 0; i < filtLength; i++) {
        if (corr[i] > maxval) {
            maxval=corr[i];
            maxidx=i;
        }
    }

    // seekback, we advance forward in android as the xcorr result is flipped
    for (int i = seekback; i >= 0; i--) {
        int l_new=maxidx-i;
        if (l_new >= 0 && l_new < filtLength) {
            if (corr[l_new] > maxval * pthresh) {
                maxidx = l_new;
                break;
            }
        }
    }

    free(corr);

    last_xcorr_val=maxval;

    // return local index, max val
    int* out = calloc(2,sizeof(int));

    int globalidx=maxidx+globalOffset;


    if (maxval > xcorrthresh && (globalidx - lastidx > minPeakDistance*FS||lastidx==0||globalidx==lastidx||getOneMoreFlag)) {

        out[0] = maxidx;
        out[1] = maxval;
        return out;
    }
    else {
        out[0] = -1;
        out[1] = maxval;
        return out;
    }
}

double* myfir_s(short* data,double* h, int lenData, int lenH) {
    int nconv = lenH+lenData-1;

    double* temp = calloc(nconv,sizeof(double));

    for (int i=0; i<nconv; i++) {
        temp[i]=0;
    }
    for (int n = 0; n < nconv; n++){
        int kmin, kmax;

        temp[n] = 0;

        kmin = (n >= lenH - 1) ? n - (lenH - 1) : 0;
        kmax = (n < lenData - 1) ? n : lenData - 1;

        for (int k = kmin; k <= kmax; k++) {
            temp[n] += data[k] * h[n - k];
        }
    }

    return temp;
}

double* myfir_d(double* data,double* h, int lenData, int lenH) {
    int nconv = lenH+lenData-1;

    double* temp = calloc(nconv,sizeof(double));

    for (int i=0; i<nconv; i++) {
        temp[i]=0;
    }
    for (int n = 0; n < nconv; n++){
        int kmin, kmax;

        temp[n] = 0;

        kmin = (n >= lenH - 1) ? n - (lenH - 1) : 0;
        kmax = (n < lenData - 1) ? n : lenData - 1;

        for (int k = kmin; k <= kmax; k++) {
            temp[n] += data[k] * h[n - k];
        }
    }

    return temp;
}

double sum_multiple(double* seg1, double* seg2, double seg1len, double seg2len) {
    double out = 0;
    for (int i = 0; i < seg1len; i++) {
        out += seg1[i] * seg2[i];
    }
    return out;
}

double* filter_s(short* data, int N) {
//    double h[129]={0.000182981959336989,0.000281596242979397,0.000278146045925432,0.000175593510303356,-4.62343304809110e-19,-0.000200360419689133,-0.000360951066953434,-0.000412991328953827,-0.000300653514269367,1.50969613210241e-18,0.000465978441137468,0.00102258147190892,0.00155366635073282,0.00192784355834049,0.00203610307379658,0.00183119111593167,0.00135603855040258,0.000749280342023432,0.000220958680427305,-2.30998316092680e-19,0.000264751350126403,0.00107567949517942,0.00233229627684471,0.00377262258405226,0.00502311871694995,0.00569227410155340,0.00548603174290836,0.00431270688583350,0.00234309825736876,0,-0.00213082326080853,-0.00345315860003227,-0.00353962427138790,-0.00228740687362881,3.04497082814396e-18,0.00264042059820580,0.00471756289619728,0.00531631859929438,0.00379212348265709,-1.99178361782373e-17,-0.00558925290045386,-0.0119350370250123,-0.0176440086545525,-0.0213191620435004,-0.0219592383672450,-0.0193021014035209,-0.0140079988312124,-0.00761010753938763,-0.00221480214028708,-3.05997847316821e-18,-0.00261988600696407,-0.0106615919949044,-0.0233019683686087,-0.0382766989959210,-0.0522058307502582,-0.0612344107673335,-0.0618649720104803,-0.0518011604210193,-0.0306049219949348,2.05563094310381e-17,0.0362729247397242,0.0730450660051671,0.104645449260725,0.125975754818137,0.133506082359307,0.125975754818137,0.104645449260725,0.0730450660051671,0.0362729247397242,2.05563094310381e-17,-0.0306049219949348,-0.0518011604210193,-0.0618649720104803,-0.0612344107673335,-0.0522058307502582,-0.0382766989959210,-0.0233019683686087,-0.0106615919949044,-0.00261988600696407,-3.05997847316821e-18,-0.00221480214028708,-0.00761010753938763,-0.0140079988312124,-0.0193021014035209,-0.0219592383672450,-0.0213191620435004,-0.0176440086545525,-0.0119350370250123,-0.00558925290045386,-1.99178361782373e-17,0.00379212348265709,0.00531631859929438,0.00471756289619728,0.00264042059820580,3.04497082814396e-18,-0.00228740687362881,-0.00353962427138790,-0.00345315860003227,-0.00213082326080853,0,0.00234309825736876,0.00431270688583350,0.00548603174290836,0.00569227410155340,0.00502311871694995,0.00377262258405226,0.00233229627684471,0.00107567949517942,0.000264751350126403,-2.30998316092680e-19,0.000220958680427305,0.000749280342023432,0.00135603855040258,0.00183119111593167,0.00203610307379658,0.00192784355834049,0.00155366635073282,0.00102258147190892,0.000465978441137468,1.50969613210241e-18,-0.000300653514269367,-0.000412991328953827,-0.000360951066953434,-0.000200360419689133,-4.62343304809110e-19,0.000175593510303356,0.000278146045925432,0.000281596242979397,0.000182981959336989};
    double h[257]={-7.75418371456968e-05,8.07613875877651e-06,-3.92526167043826e-05,-0.000196577687451030,-0.000363815429427690,-0.000429011103355452,-0.000344211593290773,-0.000161433405990234,-2.27906993160878e-06,2.21324905722762e-05,-0.000110576061155958,-0.000311661348347424,-0.000434696883769424,-0.000373945469366597,-0.000142686343390440,0.000125526059860333,0.000260911644546323,0.000175459780613039,-6.58953350088675e-05,-0.000276653523425456,-0.000270803670261422,2.76171636530950e-06,0.000409094778853374,0.000705093329509247,0.000703814715600538,0.000415348528110552,5.95769502689691e-05,-7.20325819790529e-05,0.000178836072582780,0.000707880805548229,0.00119598585719552,0.00131796961455718,0.000977094933481562,0.000397469014108926,-7.25350872688717e-06,8.40942390412198e-05,0.000657376794616464,0.00133985891012699,0.00163482563945913,0.00126878481436442,0.000410120695812505,-0.000411562869857602,-0.000655022323947711,-0.000157555976487184,0.000713059390223053,0.00127654946849575,0.00100368894695367,-8.78349944396206e-05,-0.00139727891686581,-0.00212196774639508,-0.00181669359290785,-0.000743094372055135,0.000260732796658281,0.000336747387397122,-0.000775766687272849,-0.00250158378691291,-0.00377229131416849,-0.00375110821922860,-0.00244950041720240,-0.000783946821672791,1.22028724702117e-05,-0.000749781225902183,-0.00267602888721496,-0.00448654067178731,-0.00486180048480386,-0.00338257879880225,-0.000896858215572822,0.000992067265165680,0.00101589067172485,-0.000826917138183567,-0.00317607429010071,-0.00418112513485569,-0.00278805783020865,0.000447875236091432,0.00363839200129231,0.00480975817205362,0.00330192421198659,0.000331793636070950,-0.00174191471604462,-0.00102206334558371,0.00253562740495695,0.00695068161204548,0.00947644401597403,0.00849636178646048,0.00475354493893717,0.000989204169992018,0.000130947535939305,0.00321428726881982,0.00849734359572208,0.0124875619151081,0.0122927532000494,0.00772334699889114,0.00163146724296435,-0.00188914557070325,-0.000327202667360082,0.00532653020483520,0.0110004458963365,0.0121917731549891,0.00714175294801697,-0.00167720293411387,-0.00896015759588886,-0.0101054903654515,-0.00456104382870571,0.00341275770579504,0.00733760762480312,0.00293769160556652,-0.00857852574844589,-0.0208135125902682,-0.0262005495076531,-0.0212830286030305,-0.00964933176381504,-0.000190413898523792,-0.00137902871462172,-0.0151351588915629,-0.0345440205887962,-0.0475222390535327,-0.0446652617466912,-0.0264003884645968,-0.00439713781596734,0.00461270470496120,-0.0103659562820506,-0.0455949628374347,-0.0815323553384488,-0.0915801894325124,-0.0566750715438293,0.0222527355408236,0.121454331780568,0.203704303656161,0.235571062399159,0.203704303656161,0.121454331780568,0.0222527355408236,-0.0566750715438293,-0.0915801894325124,-0.0815323553384488,-0.0455949628374347,-0.0103659562820506,0.00461270470496120,-0.00439713781596734,-0.0264003884645968,-0.0446652617466912,-0.0475222390535327,-0.0345440205887962,-0.0151351588915629,-0.00137902871462172,-0.000190413898523792,-0.00964933176381504,-0.0212830286030305,-0.0262005495076531,-0.0208135125902682,-0.00857852574844589,0.00293769160556652,0.00733760762480312,0.00341275770579504,-0.00456104382870571,-0.0101054903654515,-0.00896015759588886,-0.00167720293411387,0.00714175294801697,0.0121917731549891,0.0110004458963365,0.00532653020483520,-0.000327202667360082,-0.00188914557070325,0.00163146724296435,0.00772334699889114,0.0122927532000494,0.0124875619151081,0.00849734359572208,0.00321428726881982,0.000130947535939305,0.000989204169992018,0.00475354493893717,0.00849636178646048,0.00947644401597403,0.00695068161204548,0.00253562740495695,-0.00102206334558371,-0.00174191471604462,0.000331793636070950,0.00330192421198659,0.00480975817205362,0.00363839200129231,0.000447875236091432,-0.00278805783020865,-0.00418112513485569,-0.00317607429010071,-0.000826917138183567,0.00101589067172485,0.000992067265165680,-0.000896858215572822,-0.00338257879880225,-0.00486180048480386,-0.00448654067178731,-0.00267602888721496,-0.000749781225902183,1.22028724702117e-05,-0.000783946821672791,-0.00244950041720240,-0.00375110821922860,-0.00377229131416849,-0.00250158378691291,-0.000775766687272849,0.000336747387397122,0.000260732796658281,-0.000743094372055135,-0.00181669359290785,-0.00212196774639508,-0.00139727891686581,-8.78349944396206e-05,0.00100368894695367,0.00127654946849575,0.000713059390223053,-0.000157555976487184,-0.000655022323947711,-0.000411562869857602,0.000410120695812505,0.00126878481436442,0.00163482563945913,0.00133985891012699,0.000657376794616464,8.40942390412198e-05,-7.25350872688717e-06,0.000397469014108926,0.000977094933481562,0.00131796961455718,0.00119598585719552,0.000707880805548229,0.000178836072582780,-7.20325819790529e-05,5.95769502689691e-05,0.000415348528110552,0.000703814715600538,0.000705093329509247,0.000409094778853374,2.76171636530950e-06,-0.000270803670261422,-0.000276653523425456,-6.58953350088675e-05,0.000175459780613039,0.000260911644546323,0.000125526059860333,-0.000142686343390440,-0.000373945469366597,-0.000434696883769424,-0.000311661348347424,-0.000110576061155958,2.21324905722762e-05,-2.27906993160878e-06,-0.000161433405990234,-0.000344211593290773,-0.000429011103355452,-0.000363815429427690,-0.000196577687451030,-3.92526167043826e-05,8.07613875877651e-06,-7.75418371456968e-05};
    double* filtered = myfir_s(data,h,N,257);
    return filtered;
}

double* filter_d(double* data, int N) {
//    double h[129]={0.000182981959336989,0.000281596242979397,0.000278146045925432,0.000175593510303356,-4.62343304809110e-19,-0.000200360419689133,-0.000360951066953434,-0.000412991328953827,-0.000300653514269367,1.50969613210241e-18,0.000465978441137468,0.00102258147190892,0.00155366635073282,0.00192784355834049,0.00203610307379658,0.00183119111593167,0.00135603855040258,0.000749280342023432,0.000220958680427305,-2.30998316092680e-19,0.000264751350126403,0.00107567949517942,0.00233229627684471,0.00377262258405226,0.00502311871694995,0.00569227410155340,0.00548603174290836,0.00431270688583350,0.00234309825736876,0,-0.00213082326080853,-0.00345315860003227,-0.00353962427138790,-0.00228740687362881,3.04497082814396e-18,0.00264042059820580,0.00471756289619728,0.00531631859929438,0.00379212348265709,-1.99178361782373e-17,-0.00558925290045386,-0.0119350370250123,-0.0176440086545525,-0.0213191620435004,-0.0219592383672450,-0.0193021014035209,-0.0140079988312124,-0.00761010753938763,-0.00221480214028708,-3.05997847316821e-18,-0.00261988600696407,-0.0106615919949044,-0.0233019683686087,-0.0382766989959210,-0.0522058307502582,-0.0612344107673335,-0.0618649720104803,-0.0518011604210193,-0.0306049219949348,2.05563094310381e-17,0.0362729247397242,0.0730450660051671,0.104645449260725,0.125975754818137,0.133506082359307,0.125975754818137,0.104645449260725,0.0730450660051671,0.0362729247397242,2.05563094310381e-17,-0.0306049219949348,-0.0518011604210193,-0.0618649720104803,-0.0612344107673335,-0.0522058307502582,-0.0382766989959210,-0.0233019683686087,-0.0106615919949044,-0.00261988600696407,-3.05997847316821e-18,-0.00221480214028708,-0.00761010753938763,-0.0140079988312124,-0.0193021014035209,-0.0219592383672450,-0.0213191620435004,-0.0176440086545525,-0.0119350370250123,-0.00558925290045386,-1.99178361782373e-17,0.00379212348265709,0.00531631859929438,0.00471756289619728,0.00264042059820580,3.04497082814396e-18,-0.00228740687362881,-0.00353962427138790,-0.00345315860003227,-0.00213082326080853,0,0.00234309825736876,0.00431270688583350,0.00548603174290836,0.00569227410155340,0.00502311871694995,0.00377262258405226,0.00233229627684471,0.00107567949517942,0.000264751350126403,-2.30998316092680e-19,0.000220958680427305,0.000749280342023432,0.00135603855040258,0.00183119111593167,0.00203610307379658,0.00192784355834049,0.00155366635073282,0.00102258147190892,0.000465978441137468,1.50969613210241e-18,-0.000300653514269367,-0.000412991328953827,-0.000360951066953434,-0.000200360419689133,-4.62343304809110e-19,0.000175593510303356,0.000278146045925432,0.000281596242979397,0.000182981959336989};
    double h[257]={-7.75418371456968e-05,8.07613875877651e-06,-3.92526167043826e-05,-0.000196577687451030,-0.000363815429427690,-0.000429011103355452,-0.000344211593290773,-0.000161433405990234,-2.27906993160878e-06,2.21324905722762e-05,-0.000110576061155958,-0.000311661348347424,-0.000434696883769424,-0.000373945469366597,-0.000142686343390440,0.000125526059860333,0.000260911644546323,0.000175459780613039,-6.58953350088675e-05,-0.000276653523425456,-0.000270803670261422,2.76171636530950e-06,0.000409094778853374,0.000705093329509247,0.000703814715600538,0.000415348528110552,5.95769502689691e-05,-7.20325819790529e-05,0.000178836072582780,0.000707880805548229,0.00119598585719552,0.00131796961455718,0.000977094933481562,0.000397469014108926,-7.25350872688717e-06,8.40942390412198e-05,0.000657376794616464,0.00133985891012699,0.00163482563945913,0.00126878481436442,0.000410120695812505,-0.000411562869857602,-0.000655022323947711,-0.000157555976487184,0.000713059390223053,0.00127654946849575,0.00100368894695367,-8.78349944396206e-05,-0.00139727891686581,-0.00212196774639508,-0.00181669359290785,-0.000743094372055135,0.000260732796658281,0.000336747387397122,-0.000775766687272849,-0.00250158378691291,-0.00377229131416849,-0.00375110821922860,-0.00244950041720240,-0.000783946821672791,1.22028724702117e-05,-0.000749781225902183,-0.00267602888721496,-0.00448654067178731,-0.00486180048480386,-0.00338257879880225,-0.000896858215572822,0.000992067265165680,0.00101589067172485,-0.000826917138183567,-0.00317607429010071,-0.00418112513485569,-0.00278805783020865,0.000447875236091432,0.00363839200129231,0.00480975817205362,0.00330192421198659,0.000331793636070950,-0.00174191471604462,-0.00102206334558371,0.00253562740495695,0.00695068161204548,0.00947644401597403,0.00849636178646048,0.00475354493893717,0.000989204169992018,0.000130947535939305,0.00321428726881982,0.00849734359572208,0.0124875619151081,0.0122927532000494,0.00772334699889114,0.00163146724296435,-0.00188914557070325,-0.000327202667360082,0.00532653020483520,0.0110004458963365,0.0121917731549891,0.00714175294801697,-0.00167720293411387,-0.00896015759588886,-0.0101054903654515,-0.00456104382870571,0.00341275770579504,0.00733760762480312,0.00293769160556652,-0.00857852574844589,-0.0208135125902682,-0.0262005495076531,-0.0212830286030305,-0.00964933176381504,-0.000190413898523792,-0.00137902871462172,-0.0151351588915629,-0.0345440205887962,-0.0475222390535327,-0.0446652617466912,-0.0264003884645968,-0.00439713781596734,0.00461270470496120,-0.0103659562820506,-0.0455949628374347,-0.0815323553384488,-0.0915801894325124,-0.0566750715438293,0.0222527355408236,0.121454331780568,0.203704303656161,0.235571062399159,0.203704303656161,0.121454331780568,0.0222527355408236,-0.0566750715438293,-0.0915801894325124,-0.0815323553384488,-0.0455949628374347,-0.0103659562820506,0.00461270470496120,-0.00439713781596734,-0.0264003884645968,-0.0446652617466912,-0.0475222390535327,-0.0345440205887962,-0.0151351588915629,-0.00137902871462172,-0.000190413898523792,-0.00964933176381504,-0.0212830286030305,-0.0262005495076531,-0.0208135125902682,-0.00857852574844589,0.00293769160556652,0.00733760762480312,0.00341275770579504,-0.00456104382870571,-0.0101054903654515,-0.00896015759588886,-0.00167720293411387,0.00714175294801697,0.0121917731549891,0.0110004458963365,0.00532653020483520,-0.000327202667360082,-0.00188914557070325,0.00163146724296435,0.00772334699889114,0.0122927532000494,0.0124875619151081,0.00849734359572208,0.00321428726881982,0.000130947535939305,0.000989204169992018,0.00475354493893717,0.00849636178646048,0.00947644401597403,0.00695068161204548,0.00253562740495695,-0.00102206334558371,-0.00174191471604462,0.000331793636070950,0.00330192421198659,0.00480975817205362,0.00363839200129231,0.000447875236091432,-0.00278805783020865,-0.00418112513485569,-0.00317607429010071,-0.000826917138183567,0.00101589067172485,0.000992067265165680,-0.000896858215572822,-0.00338257879880225,-0.00486180048480386,-0.00448654067178731,-0.00267602888721496,-0.000749781225902183,1.22028724702117e-05,-0.000783946821672791,-0.00244950041720240,-0.00375110821922860,-0.00377229131416849,-0.00250158378691291,-0.000775766687272849,0.000336747387397122,0.000260732796658281,-0.000743094372055135,-0.00181669359290785,-0.00212196774639508,-0.00139727891686581,-8.78349944396206e-05,0.00100368894695367,0.00127654946849575,0.000713059390223053,-0.000157555976487184,-0.000655022323947711,-0.000411562869857602,0.000410120695812505,0.00126878481436442,0.00163482563945913,0.00133985891012699,0.000657376794616464,8.40942390412198e-05,-7.25350872688717e-06,0.000397469014108926,0.000977094933481562,0.00131796961455718,0.00119598585719552,0.000707880805548229,0.000178836072582780,-7.20325819790529e-05,5.95769502689691e-05,0.000415348528110552,0.000703814715600538,0.000705093329509247,0.000409094778853374,2.76171636530950e-06,-0.000270803670261422,-0.000276653523425456,-6.58953350088675e-05,0.000175459780613039,0.000260911644546323,0.000125526059860333,-0.000142686343390440,-0.000373945469366597,-0.000434696883769424,-0.000311661348347424,-0.000110576061155958,2.21324905722762e-05,-2.27906993160878e-06,-0.000161433405990234,-0.000344211593290773,-0.000429011103355452,-0.000363815429427690,-0.000196577687451030,-3.92526167043826e-05,8.07613875877651e-06,-7.75418371456968e-05};
    double* filtered = myfir_d(data,h,N,257);
    return filtered;
}


// according to the user ID set the reply interval and then write the reply packet into the speaker buffer.
void setReply(int idx, mycontext* cxt0, int user_id) {
    // don't trigger on the calibration chirp
    clock_t t0 = clock();
    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "setReply at %.3f", (double)(t0)/CLOCKS_PER_SEC);
    if (cxt0->responder && idx > FS && idx-replyIdx1 >= FS/4) {
        receivedIdx = idx;
        int max_user = 6;
        int init_delay = cxt0->initialDelay;
        int each_delay = (int)(0.32*FS);
        if(user_id <= 0 ){
            replyIdx1 = idx - cxt0->timingOffset + cxt0->sendDelay;
        }else{
            replyIdx1 = idx - cxt0->timingOffset + (cxt0->sendDelay - init_delay) + (max_user - user_id + 1) * each_delay;
        }


        if (replyIdx1 < cxt0->dataSize-cxt0->preamble_len-cxt0->bufferSize) {
            reply_ready = JNI_TRUE;
            cxt0->sendReply = JNI_FALSE;
            if(2 + 3*chirpsPlayed < 150){
                recv_indexes[3*chirpsPlayed] = idx;
                recv_indexes[3*chirpsPlayed + 1] = replyIdx1;
                recv_indexes[3*chirpsPlayed + 2] = user_id;
                chirpsPlayed+=1;
            }


            if (reply_ready && replyIdx1 >= 0) {
                pthread_mutex_lock(&speaker_mutex);
                __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","Write to speaker buffer from %d to %d: %d, %d, speaker offset %d", replyIdx1, replyIdx1+cxt->preamble_len, idx, cxt0->timingOffset, cxt->playOffset);
                memcpy(cxt->data + replyIdx1, cxt->refData, cxt->preamble_len*sizeof(short));
                pthread_mutex_unlock(&speaker_mutex);
                reply_ready=JNI_FALSE;
                cxt0->sendReply=JNI_TRUE;
            }

        }
    }

}

// this method is called to record the microphone/speaker delay from the calibration signal
jboolean timeOffsetUpdated=JNI_FALSE;
void updateTimingOffset(int global_xcorr_idx, int local_chirp_idx, mycontext* cxt) {
    double oneSampleDelay = 1.0/FS;
    int transmitDelay = (int) (((33.0 / 1000) / cxt->speed)/oneSampleDelay); // comment TUOCHAO: NEED TO VERIFY
    cxt->timingOffset = global_xcorr_idx - (cxt->initialOffset); // - transmitDelay;
    if(3*chirpsPlayed + 2< 150){
        recv_indexes[3*chirpsPlayed] = global_xcorr_idx;
        recv_indexes[3*chirpsPlayed + 1] = cxt->timingOffset;
        recv_indexes[3*chirpsPlayed + 2] = 0;
        chirpsPlayed+=1;
    }


    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","Initial calibration %d to %d, %d --- %d", global_xcorr_idx, cxt->initialOffset, transmitDelay, cxt->timingOffset);
    //__android_log_print(ANDROID_LOG_VERBOSE,"debug_tau","detect leader chirp in phase 2 = %d", global_xcorr_idx);

    timeOffsetUpdated=JNI_TRUE;
}

// Process the incoming chunk from the microphone buffer. If there is a possible preamble then further signal processing will be executed.
void* xcorr_thread2(void* context) {
    mycontext* cxt = (mycontext*)context;
    clock_t t0 = clock();
    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "thread created %d at %.3f", cxt->processedSegments, (double)(t0)/CLOCKS_PER_SEC);

    if (cxt->runXcorr && cxt->processedSegments >= next_segment_num) {
        int global_xcorr_idx=-1;
        int local_xcorr_idx=-1;

        if (!cxt->getOneMoreFlag && cxt->processedSegments > 0) { // not wait for one more flag
            int globalOffset=0;
            short *data = cxt->data + (cxt->bigBufferSize * (cxt->processedSegments - 1));

            globalOffset = (cxt->processedSegments - 1) * cxt->bigBufferSize;
            clock_t t0 = clock();
//            __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "if condition 2 before xcorr");
            int *result = xcorr_helper2(context, data, globalOffset, cxt->bigBufferSize * 2);
//            __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "if condition 2 after xcorr");
            clock_t t1 = clock();

            __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "return idx in phase 1 %d to %d, %d, with time %.3f",  result[0], result[1], result[2], (double)(t1 - t0)/CLOCKS_PER_SEC );
            local_xcorr_idx = result[0];
            int naiser_idx = result[2];

            if (local_xcorr_idx >= 0 && naiser_idx >= 0) {
                int synclag = cxt->seekback;
                __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "judge if need one more %d, %d",  local_xcorr_idx, cxt->bigBufferSize);

                jboolean c1 = cxt->processedSegments > 0 && local_xcorr_idx - cxt->bigBufferSize  >= 1000;
                jboolean c2 = cxt->processedSegments > 0 && local_xcorr_idx - cxt->bigBufferSize  >= 1000;

                if ((c1 || c2)) { // wait for next chunk
                    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "get one more flag set to true");
                    cxt->getOneMoreFlag = JNI_TRUE;
                }
                else {
                    global_xcorr_idx = result[0] + globalOffset;
                    last_chirp_idx = global_xcorr_idx;
                    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","detect leader chirp in phase 1 = %d, and local_idx=%d, tolerance = %d", global_xcorr_idx, local_xcorr_idx, 2*cxt->bigBufferSize - cxt->numSyms*(cxt->N0 + cxt->Ns) - 200);

//                    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","detect leader chirp in phase 1 = %d", global_xcorr_idx);
                    if (cxt->timingOffset == 0) {
                        self_chirp_idx = global_xcorr_idx;
                        updateTimingOffset(global_xcorr_idx, local_xcorr_idx, cxt);
                        next_segment_num = cxt->processedSegments + 4*cxt->calibWait; //calibWait is avoid others' calibrate signal affects it
                    } else{
                            setReply(global_xcorr_idx, cxt, 0);
                            next_segment_num = cxt->processedSegments + period_wait*4 + 2;
                    }
                }
            }
            free(result);

        }
        else if(cxt->processedSegments > 0) {//  wait for one more flag
            // look back half a second
            int globalOffset = (cxt->processedSegments-1) * (cxt->bigBufferSize);

            short *data = cxt->data +
                          (int) ((cxt->bigBufferSize * (double) (cxt->processedSegments - 1)));
            clock_t t0 = clock();
            //__android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "if condition 3 before xcorr");
            int *result = xcorr_helper2(context, data, globalOffset, cxt->bigBufferSize*2);
            //__android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "if condition 3 after xcorr");
            clock_t t1 = clock();
            __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "return idx in phase 2 %d to %d, %d, with time %.3f",  result[0], result[1], result[2], (double)(t1 - t0)/CLOCKS_PER_SEC );

            local_xcorr_idx = result[0];
            int naiser_out = result[2];
            if (local_xcorr_idx >= 0 && naiser_out >= 0) {
                global_xcorr_idx = result[0]+globalOffset;

                last_chirp_idx = global_xcorr_idx;

                cxt->getOneMoreFlag = JNI_FALSE;
                __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","detect leader chirp in phase 2 = %d, and local_idx=%d, tolerance = %d", global_xcorr_idx, local_xcorr_idx, 2*cxt->bigBufferSize - cxt->numSyms*(cxt->N0 + cxt->Ns) - 200);
                if (cxt->timingOffset==0) {
                    self_chirp_idx = global_xcorr_idx;
                    updateTimingOffset(global_xcorr_idx,local_xcorr_idx,cxt);
                    next_segment_num = cxt->processedSegments + 4*cxt->calibWait;
                }else{
                    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","end time t %.3f", (double)clock()/CLOCKS_PER_SEC);
                    setReply(global_xcorr_idx, cxt, 0);
                    next_segment_num = cxt->processedSegments + period_wait*4 + 2;  // assume the
                }

            } else if (local_xcorr_idx < 0 || naiser_out < 0) {
                // occurs in the case of noise
//                __android_log_print(ANDROID_LOG_VERBOSE, "debug", "get one more flag set to false 2");
                cxt->getOneMoreFlag = JNI_FALSE;
            }
            free(result);
        }

    }
    cxt->processedSegments+=1;
//    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "thread finish");
}




double getdist(int earier_chirp_idx, int later_chirp_index,int delay,int dtx,int drx, double speed) {
    double diff = (later_chirp_index - earier_chirp_idx) - delay;
    double delta = diff / (double)FS * speed;
    double distance = (delta / 2) + dtx + drx;
    return distance;
}

void getdistall() {
    if (!responder) {
        distance1 = -1;
        distance2 = -1;
        distance3 = -1;

        if (chirp_indexes[1]!=-1) {
            distance1 = getdist(self_chirp_idx, chirp_indexes[1], sendDelay, dtx, drx,
                                speed);
        }
        if (chirp_indexes[2]!=-1) {
            distance2 = getdist(self_chirp_idx, chirp_indexes[2], sendDelay * 2, dtx, drx,
                                speed);
        }
        if (chirp_indexes[3]!=-1) {
            distance3 = getdist(self_chirp_idx, chirp_indexes[3], sendDelay * 3, dtx,
                                drx, speed);
        }
    }
}

JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_forcewrite(JNIEnv *env, jclass clazz) {
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "forcewrite");
    forcewrite=JNI_TRUE;
    getdistall();
}

void stopithelper() {
    if (!freed) {
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "stopithelper");
        SLresult result;
        result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
        assert(SL_RESULT_SUCCESS == result);

        result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        assert(SL_RESULT_SUCCESS == result);

//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "freeing1");
//    free(chirp_indexes);
        if (cxt != NULL) {
            free(cxt->data);
            free(cxt->speaker_ts);
            free(cxt->refData);
            free(cxt);
        }
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "freeing2");
        if (cxt2 != NULL) {
            free(cxt2->mic_ts);
            free(cxt2->bigdata);
            free(cxt2->data);
            free(cxt2);
        }
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "freeing3");

//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "done freeing");
        freed = JNI_TRUE;
    }
}

int smallBufferIdx=0;
int micCounter=0;

// callback function for microphone buffer, it loads data from the microphone hardware buffer to Java buffer
// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
//    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","bqRecorderCallback time t %.3f", (double)clock()/CLOCKS_PER_SEC);
    assert(bq == recorderBufferQueue);

    mycontext* cxt=(mycontext*)context;
    cxt->mic_ts[micCounter++] = now_ms();

    int offset=0;
    // determine whether top/bottom microphone is the odd/even sample in the buffer
    if (smallBufferIdx==0) {
        int maxval1=0;
        int maxval2=0;

        int counter=0;
        for (int i = 0; i < cxt->bufferSize; i++) {
            int val1 = cxt->bigdata[(cxt->processedSegments*cxt->bufferSize*2)+counter];
            int val2 = cxt->bigdata[(cxt->processedSegments*cxt->bufferSize*2)+counter+1];
            if (val1 > maxval1) {
                maxval1=val1;
            }
            if (val2 > maxval2) {
                maxval2=val2;
            }
            counter+=2;
        }
        __android_log_print(ANDROID_LOG_VERBOSE, "maxval", "%d %d",maxval1,maxval2);
        if (maxval1 > maxval2) {
            cxt->recorder_offset = 0;
        }
        else {
            cxt->recorder_offset = 1;
        }
    }

    // we only process data from one microphone, we use the one with higher amplitude (probably bottom one)
    int counter=cxt->recorder_offset;
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug5", "copy %d %d %d %d",smallBufferIdx,cxt->bufferSize,smallBufferIdx*cxt->bufferSize,smallBufferIdx*cxt->bufferSize*2);
    for (int i = 0; i < cxt->bufferSize; i++) {
        cxt->data[smallBufferIdx*cxt->bufferSize+i] = cxt->bigdata[(smallBufferIdx*cxt->bufferSize*2)+counter];
        counter+=2;
    }
    smallBufferIdx+=1;

    if (cxt->queuedSegments<cxt->totalSegments) {
        SLresult result=(*recorderBufferQueue)->Enqueue(recorderBufferQueue,
                                                        (cxt->bigdata)+(cxt->bufferSize*2*cxt->queuedSegments),
                                                        cxt->bufferSize*2*sizeof(short));
        assert(SL_RESULT_SUCCESS == result);
        cxt->queuedSegments+=1;
    }

    // bug: if the time is too short, then this will use 'old' values of an index (i.e. not naiser-refined)
    jboolean c1 = forcewrite&&!wroteToDisk;
    jboolean c2 = !cxt->responder && xcorr_counter==4;
    jboolean c3 = !cxt->responder&&cxt->queuedSegments==cxt->totalSegments;
    jboolean c5 = (cxt->responder&&cxt->queuedSegments==cxt->totalSegments);
    if ((c1 || c2 || c3 || c5) && !wroteToDisk) {
        // write to disk
        wroteToDisk=JNI_TRUE;
        FILE* fp = fopen(cxt->mic_ts_fname,"w+");
        fp = fopen(cxt->mic_ts_fname,"w+");
        for (int i = 0; i < cxt->ts_len-1; i++) {
//            fprintf(fp,"%f\n",cxt->mic_ts[i]);
            fprintf(fp, "%" PRId64 "\n", cxt->mic_ts[i]);
        }
        fclose(fp);
        free(cxt->mic_ts);

        fp = fopen(cxt->bottomfilename,"w+");
        for (int i = cxt->recorder_offset; i < cxt->totalSegments*cxt->bufferSize*2-2; i+=2) {
            fprintf(fp,"%d ",cxt->bigdata[i]);
        }
        fclose(fp);

        fp = fopen(cxt->topfilename,"w+");
        for (int i = -(cxt->recorder_offset-1); i < cxt->totalSegments*cxt->bufferSize*2-2; i+=2) {
            fprintf(fp,"%d ",cxt->bigdata[i]);
        }
        fclose(fp);

        ///////////////////////////////////////////////////////////////////////

        wroteToDisk=JNI_TRUE;

//        __android_log_print(ANDROID_LOG_VERBOSE, "debug2", "finish writing");
        stopithelper();
    }

    if (!wroteToDisk && cxt->runXcorr && cxt->responder) {
        if (micCounter%cxt->bigBufferTimes==0) {
            pthread_t t;
            pthread_create(&t, NULL, xcorr_thread2, context);
        }
    }
}

// create the engine and output mix objects
JNIEXPORT jdoubleArray JNICALL
Java_com_example_nativeaudio_NativeAudio_getDistance(JNIEnv* env, jclass clazz, jboolean reply)
{
    if (reply) {
        jdouble out[5];
        out[0] = self_chirp_idx;
        out[1] = receivedIdx;
        out[2] = replyIdx1;
        out[3] = replyIdx2;
        out[4] = replyIdx3;

        jdoubleArray result;
        result = (*env)->NewDoubleArray(env, 5);
        (*env)->SetDoubleArrayRegion(env, result, 0, 5, out);

        return result;
    }
    else {
        jdouble out[7];
        out[0] = distance1;
        out[1] = distance2;
        out[2] = distance3;
        out[3] = chirp_indexes[0];
        out[4] = chirp_indexes[1];
        out[5] = chirp_indexes[2];
        out[6] = chirp_indexes[3];

        jdoubleArray result;
        result = (*env)->NewDoubleArray(env, 7);
        (*env)->SetDoubleArrayRegion(env, result, 0, 7, out);

        return result;
    }
}

// create the engine and output mix objects
JNIEXPORT jdoubleArray JNICALL
Java_com_example_nativeaudio_NativeAudio_getVal(JNIEnv* env, jclass clazz)
{
    jdouble out[2];
    out[0] = last_xcorr_val;
    out[1] = last_naiser_val;

    jdoubleArray result;
    result = (*env)->NewDoubleArray(env,2);
    (*env)->SetDoubleArrayRegion(env, result, 0, 2, out);

    return result;
}
// create the engine and output mix objects
JNIEXPORT jboolean JNICALL
Java_com_example_nativeaudio_NativeAudio_responderDone(JNIEnv* env, jclass clazz)
{
    return cxt2->responder&&!cxt2->sendReply&&wroteToDisk;
}

JNIEXPORT jboolean JNICALL
Java_com_example_nativeaudio_NativeAudio_replySet(JNIEnv* env, jclass clazz)
{
    return !cxt2->sendReply;
}

JNIEXPORT jint JNICALL
Java_com_example_nativeaudio_NativeAudio_getXcorrCount(JNIEnv* env, jclass clazz)
{
    return xcorr_counter;
}

JNIEXPORT jintArray JNICALL
Java_com_example_nativeaudio_NativeAudio_getReplyIndexes(JNIEnv* env, jclass clazz)
{
    jint out[100];
    int i = 0;
    int total_num = chirpsPlayed;
    for(i = 0; i < 3*total_num; i++){
        out[i] = recv_indexes[i];
    }
    jintArray result;
    result = (*env)->NewIntArray(env,3*total_num);
    (*env)->SetIntArrayRegion(env, result, 0, 3*total_num, out);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_example_nativeaudio_NativeAudio_getQueuedSpeakerSegments(JNIEnv* env, jclass clazz)
{
    return cxt->queuedSegments;
}

// create the engine and output mix objects
JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_createEngine(JNIEnv* env, jclass clazz)
{
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example
}

// create buffer queue audio player
JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_createBufferQueueAudioPlayer(JNIEnv* env, jclass clazz, jint sampleRate, jint bufSize)
{
    SLresult result;
    if (sampleRate >= 0 && bufSize >= 0 ) {
        bqPlayerSampleRate = sampleRate * 1000;
        /*
         * device native buffer size is another factor to minimize audio latency, not used in this
         * sample: we only play one giant buffer here
         */
    }

    // configure audio source
    int numBuffers=5;
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, numBuffers};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if(bqPlayerSampleRate) {
        format_pcm.samplesPerSec = bqPlayerSampleRate;       //sample rate in mili second
    }
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME,
//                                  SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ };

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
                                                bqPlayerSampleRate? 2 : 3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the effect send interface
    bqPlayerEffectSend = NULL;
    if( 0 == bqPlayerSampleRate) {
        result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
                                                 &bqPlayerEffectSend);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
}

// create audio recorder: recorder is not in fast path
//    like to avoid excessive re-sampling while playing back from Hello & Android clip
JNIEXPORT jboolean JNICALL
Java_com_example_nativeaudio_NativeAudio_createAudioRecorder(JNIEnv* env, jclass clazz, jint micInterface)
{
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "start recorder setup");

    SLresult result;

    int numchannels=2;
    int mics=0;
    if (numchannels==1) {
        mics=SL_SPEAKER_FRONT_CENTER;
    }
    else {
        mics=SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    }

    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, numchannels, SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   mics, SL_BYTEORDER_LITTLEENDIAN};
    if (FS==48000) {
        format_pcm.samplesPerSec=SL_SAMPLINGRATE_48;
    }

    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[2] = {SL_IID_ANDROIDCONFIGURATION, SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc,
                                                  &audioSnk, 2, id, req);
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    // Configure the voice recognition preset which has no
    // signal processing for lower latency.
    SLAndroidConfigurationItf inputConfig;
    result = (*recorderObject)
            ->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION,
                           &inputConfig);

    if (SL_RESULT_SUCCESS == result) {
        SLuint32 presetValue = 0;
        if (micInterface == 0) {
            presetValue = SL_ANDROID_RECORDING_PRESET_CAMCORDER;
        }
        else if (micInterface == 1) {
            presetValue = SL_ANDROID_RECORDING_PRESET_UNPROCESSED;
        }
        __android_log_print(ANDROID_LOG_VERBOSE, "maxval", "preset");
        (*inputConfig)
                ->SetConfiguration(inputConfig, SL_ANDROID_KEY_RECORDING_PRESET,
                                   &presetValue, sizeof(SLuint32));
    }

    // realize the audio recorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    // get the record interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                             &recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "finish recorder setup");

    return JNI_TRUE;
}

void shutdownhelper() {
    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "shutdown1");
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "destroy player1");

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "destroy player2");
    // destroy file descriptor audio player object, and invalidate all associated interfaces
    if (fdPlayerObject != NULL) {
        (*fdPlayerObject)->Destroy(fdPlayerObject);
        fdPlayerObject = NULL;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "destroy player3");
    // destroy URI audio player object, and invalidate all associated interfaces
    if (uriPlayerObject != NULL) {
        (*uriPlayerObject)->Destroy(uriPlayerObject);
        uriPlayerObject = NULL;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "destroy recorder");

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "destroy outputmix");
    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "destroy engine");
    // destroy engine object, and invalidate all associated interfaces
    if (lock==1) {
        pthread_mutex_destroy(&audioEngineLock);
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "shutdown2");
}

// shut down the native audio system
JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_shutdown(JNIEnv* env, jclass clazz)
{
    shutdownhelper();
}

JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_stopit(JNIEnv *env, jclass clazz) {
    stopithelper();
}

JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_reset(JNIEnv *env, jclass clazz) {
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "reset");

    SLresult result;
    result=(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "reset 1");
    result=(*bqPlayerBufferQueue)->Clear(bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "reset 2");

//    if (recorderRecord!=NULL) {
        result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "reset rec %d",result);
        assert(SL_RESULT_SUCCESS == result);
//    }

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "reset 3");
    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "reset done");
}
/////////////////////////////////////////////////////////////////////////////////////////

JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_calibrate(JNIEnv *env, jclass clazz,jshortArray tempLeadData,
                                                   jshortArray tempRefData, jint bSize_spk, jint bSize_mic, jint recordTime,
                                                   jstring ttfilename, jstring tbfilename, jstring tmeta_filename,
                                                   jint initialOffset, jint warmdownTime,
                                                   jint preamble_len, jboolean water,
                                                   jboolean reply, jboolean naiser,
                                                   jint tempSendDelay, jfloat xcorrthresh, jfloat minPeakDistance,
                                                   jint fs,
                                                   jdoubleArray tnaiserTx1, jdoubleArray tnaiserTx2,
                                                   jint Ns, jint N0, jboolean CP, jint N_FSK,
                                                   jfloat naiserThresh, jfloat naiserShoulder,
                                                   jint win_size, jint bias, jint seekback, jdouble pthresh, int round,
                                                   int filenum, jboolean runxcorr, jint initialDelay, jstring mic_ts_fname,
                                                   jstring speaker_ts_fname, int bigBufferSize,int bigBufferTimes,int numSym, int calibWait) {
    freed=JNI_FALSE;
    timeOffsetUpdated=JNI_FALSE;
    int round0 = 0;
    smallBufferIdx=0;
    micCounter=0;
    speakerCounter=0;
    FS=fs;
    chirpsPlayed=0;
    memset(recv_indexes, 0, 100*sizeof(int));
    forcewrite=JNI_FALSE;
    responder = reply;
    int bufferSize_mic = bSize_mic;
    int bufferSize_spk = bSize_spk;


    if(numSym == 7){
        //{1, 1, 1, -1, 1, -1, -1};
        PN_seq[0] = 1;
        PN_seq[1] = 1;
        PN_seq[2] = 1;
        PN_seq[3] = -1;
        PN_seq[4] = 1;
        PN_seq[5] = -1;
        PN_seq[6] = -1;


    }else if(numSym == 5){
        //PN_seq[5] = [1, 1, -1, 1, -1];
        PN_seq[0] = 1;
        PN_seq[1] = 1;
        PN_seq[2] = -1;
        PN_seq[3] = 1;
        PN_seq[4] = -1;
    }
    else if(numSym == 4){
        PN_seq[0] = 1;
        PN_seq[1] = 1;
        PN_seq[2] = -1;
        PN_seq[3] = 1;
    }

    if(N_FSK == 1920){
        N_fre = 18;
        int tmp[6][18] =
        {
            {60, 68, 76, 84, 92, 100, 108, 116, 124, 132, 140, 148, 156, 164, 172, 180, 188, 196 },
            {61, 69, 77, 85, 93, 101, 109, 117, 125, 133, 141, 149, 157, 165, 173, 181, 189, 197  },
            {62, 70, 78, 86, 94, 102, 110, 118, 126, 134, 142, 150, 158, 166, 174, 182, 190, 198 },
            {63, 71, 79, 87, 95, 103, 111, 119, 127, 135, 143, 151, 159, 167, 175, 183, 191, 199  },
            {64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200  },
            {65, 73, 81, 89, 97, 105, 113, 121, 129, 137, 145, 153, 161, 169, 177, 185, 193, 201  }
        };
        for(int i = 0; i < 6 ; ++i){
            for(int  j = 0 ; j < N_fre; ++j){
                fre_idx[i][j] =tmp[i][j];
            }
        }
    }
    else{
        N_fre = 16;
        int tmp[6][16] =
        {
                {71,85,99,113,127,141,155,169,183,197,211,225,239,253,267,281 },
                {73,87,101,115,129,143,157,171,185,199,213,227,241,255,269,283},
                {75,89,103,117,131,145,159,173,187,201,215,229,243,257,271,285},
                {77,91,105,119,133,147,161,175,189,203,217,231,245,259,273,287 },
                {79,93,107,121,135,149,163,177,191,205,219,233,247,261,275,289 },
                {81,95,109,123,137,151,165,179,193,207,221,235,249,263,277,291 }
        };
        for(int i = 0; i < 6 ; ++i){
            for(int  j = 0 ; j < N_fre; ++j){
                fre_idx[i][j] =tmp[i][j];
            }
        }
    }



    reply_ready=JNI_FALSE;
    receivedIdx=-1;
    replyIdx1=-1;
    replyIdx2=-1;
    replyIdx3=-1;
    next_segment_num = 0;
    SLresult result;
    recorderSize = 0;
    self_chirp_idx=-1;
    last_chirp_idx=-1;
    naiser_index_to_process=-1;

    jint N = (*env)->GetArrayLength(env, tempLeadData);
    jint N_ref = (*env)->GetArrayLength(env, tempRefData);

    jint naiserTx1Count = (*env)->GetArrayLength(env, tnaiserTx1);
    jint naiserTx2Count = (*env)->GetArrayLength(env, tnaiserTx2);
    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "naiser %d %d", naiserTx1Count,naiserTx2Count);

    double* naiserTx1 = (double*)((*env)->GetDoubleArrayElements(env, tnaiserTx1, NULL));
    double* naiserTx2 = (double*)((*env)->GetDoubleArrayElements(env, tnaiserTx2, NULL));

    int totalSpeakerLoops = (recordTime*FS)/bufferSize_spk;
    int totalRecorderLoops = (recordTime*FS)/bufferSize_mic;

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "total speaker/recorder loop %d %d", totalSpeakerLoops,totalRecorderLoops);

    short* data = (short*)((*env)->GetShortArrayElements(env, tempLeadData, NULL));
    short* refData = (short*)((*env)->GetShortArrayElements(env, tempRefData, NULL));


    /// ctx is the context for the speaker
    if (round==round0) {
        cxt = calloc(1, sizeof(mycontext));
        cxt->sendDelay=tempSendDelay;
        cxt->preamble_len = preamble_len;
        cxt->data=calloc(bufferSize_spk * totalSpeakerLoops, sizeof(short));
        cxt->initialDelay = initialDelay;
        cxt->calibWait=calibWait;
        cxt->refData=calloc(N_ref, sizeof(short));
        memcpy(cxt->refData,refData, N_ref*sizeof(short));

        // copy at beginning for init calibrate
        if (responder) {
            memcpy(cxt->data + initialOffset, data, N_ref * sizeof(short));
        }

        // for the leader copy the data into the buffers to period sent
        if (!runxcorr && !responder){
            for (int index = 0; index < bufferSize_spk * totalSpeakerLoops; index += tempSendDelay) {
                __android_log_print(ANDROID_LOG_VERBOSE, "debug", "FILLING %d %d",index, bufferSize_spk * totalSpeakerLoops);
                for (int i = 0; i < N_ref; i++) {
                    cxt->data[index+i] = refData[i];
                }
            }
        }
    }
    else {
        memset(cxt->data,0,bufferSize_spk * totalSpeakerLoops*sizeof(short));
    }
//    char* str1=getString_s(cxt->data,12000);
    cxt->warmdownTime=warmdownTime;
    cxt->initialOffset=initialOffset;
    cxt->bufferSize=bufferSize_spk;
    cxt->playOffset=bufferSize_spk;
    cxt->totalSegments=totalSpeakerLoops;
    cxt->queuedSegments=1;
    cxt->responder=reply;
    cxt->waitforFSK = JNI_FALSE;
    char* speaker_ts_filename_str = (*env)->GetStringUTFChars(env, speaker_ts_fname, NULL);
    cxt->speaker_ts_fname=speaker_ts_filename_str;
    cxt->speaker_ts = calloc((recordTime*FS)/bufferSize_spk,sizeof(double));
    cxt->ts_len=(recordTime*FS)/bufferSize_spk;
    cxt->env = env;
    cxt->clazz = clazz;
    cxt->Ns=Ns;
    cxt->N_FSK=N_FSK;
    cxt->N0=N0;

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "populate cxt");

    if (round==round0) {
        result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, cxt);
        assert(SL_RESULT_SUCCESS == result); //this fails
        (void)result;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    char* topfilename = (*env)->GetStringUTFChars(env, ttfilename, NULL);
    char* bottomfilename = (*env)->GetStringUTFChars(env, tbfilename, NULL);
    char* meta_filename = (*env)->GetStringUTFChars(env, tmeta_filename, NULL);
    char* mic_ts_filename_str = (*env)->GetStringUTFChars(env, mic_ts_fname, NULL);

    wroteToDisk=JNI_FALSE;
    lastidx=0;
    last_xcorr_val=0;
    last_naiser_val=0;
    distance1=-1;
    distance2=-1;
    distance3=-1;
    xcorr_counter=0;
    //cxt is for the speaker

    /* -------------------------------------------------    */
    //cxt2 is for the microphone
    if (round==round0) {
        cxt2 = calloc(1, sizeof(mycontext));
        cxt2->bigdata = calloc(bufferSize_mic * 2 * totalRecorderLoops, sizeof(short));
        cxt2->data = calloc(bufferSize_mic * totalRecorderLoops, sizeof(short));
        cxt2->dataSize = bufferSize_mic*totalRecorderLoops;
    }
    else {
        memset(cxt2->bigdata,0,bufferSize_mic * 2 * totalRecorderLoops * sizeof(short));
        memset(cxt2->data,0,bufferSize_mic * totalRecorderLoops * sizeof(short));
    }
    cxt2->calibWait=calibWait;
    cxt2->numSyms=numSym;
    cxt2->env = env;
    cxt2->clazz = clazz;
    cxt2->ts_len=(recordTime*FS)/bufferSize_mic;
    cxt2->bigBufferSize=bigBufferSize;
    cxt2->bigBufferTimes=bigBufferTimes;
    cxt2->mic_ts = calloc((recordTime*FS)/bufferSize_mic,sizeof(double));
    cxt2->initialDelay = initialDelay;
    cxt2->xcorrthresh=xcorrthresh;
    cxt2->naiserTx1=naiserTx1;
    cxt2->naiserTx2=naiserTx2;
    cxt2->filenum=filenum;
    cxt2->naiserTx1Count=naiserTx1Count;
    cxt2->naiserTx2Count=naiserTx2Count;
    cxt2->refData=refData;
    cxt2->recorder_offset=0;
    cxt2->totalSegments=totalRecorderLoops;
    cxt2->naiser=naiser;
    cxt2->preamble_len = preamble_len;
    cxt2->topfilename = topfilename;
    cxt2->processedSegments=0;
    cxt2->bottomfilename = bottomfilename;
    cxt2->meta_filename = meta_filename;
    cxt2->mic_ts_fname=mic_ts_filename_str;
    cxt2->bufferSize=bufferSize_mic;
    cxt2->queuedSegments=1;
    cxt2->initialOffset=initialOffset;
    cxt2->warmdownTime = warmdownTime;
    cxt2->naiserThresh=naiserThresh;
    cxt2->naiserShoulder=naiserShoulder;
    cxt2->Ns=Ns;
    cxt2->N_FSK=N_FSK;
    cxt2->N0=N0;
    cxt2->CP=CP;
    cxt2->win_size=win_size;
    cxt2->bias=bias;
    cxt2->seekback=seekback;
    cxt2->pthresh=pthresh;

    if (water) {
        cxt2->speed = 1500;
    }
    else {
        cxt2->speed=340;
    }
    cxt2->getOneMoreFlag = JNI_FALSE;
    cxt2->waitforFSK = JNI_FALSE;
    cxt2->sendDelay=tempSendDelay;
    cxt2->minPeakDistance=minPeakDistance;
    cxt2->runXcorr=runxcorr;
    cxt2->sendReply=JNI_TRUE;
    cxt2->responder=reply;
    speed=cxt2->speed;
    sendDelay=cxt2->sendDelay;
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "populate cxt2");

    for (int i = 0; i < 5; i++) {
        chirp_indexes[i]=-1;
    }


    if (round==round0) {
        result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback,
                                                          cxt2);
        assert(SL_RESULT_SUCCESS == result);
    }

    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, cxt2->bigdata, bufferSize_mic * 2 * sizeof(short));
    assert(SL_RESULT_SUCCESS == result);


    result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, cxt->data, bufferSize_spk * sizeof(short));
    assert(SL_RESULT_SUCCESS == result);

    result=(*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);

//    usleep(30*1000);

    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "finish setup");
}

JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_testFileWrite(JNIEnv *env, jclass clazz, jstring tfilename) {
    char* filename = (*env)->GetStringUTFChars(env, tfilename, NULL);
    FILE* fp = fopen(filename,"w+");
    clock_t begin = clock();

    for (int i = 0; i < 48000*30; i++) {
        fprintf(fp,"%d, ",i);
    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "time %f", time_spent);

    fclose(fp);
}


//conduct the division between 2 complex number array H = Y/X
void estimate_H(fftw_complex* H, fftw_complex* X, fftw_complex * Y, int fft_begin, int fft_end, unsigned long int Nu){
    unsigned long int  i = 0;
    for( i = 0; i < Nu ; ++i){
        if(i >= fft_begin && i < fft_end){
            double y_pow = Complex_power(X[i]);
            H[i][0] = (Y[i][0]*X[i][0] + X[i][1]*Y[i][1])/y_pow;
            H[i][1] = (Y[i][1]*X[i][0] - Y[i][0]*X[i][1])/y_pow;
        }
        else{
            H[i][0] = 0;
            H[i][1] = 0;
        }
    }
}


// do the channel estimation in the frequency domain, tx is the sending signal and the rx is the recv signal
void channel_estimation_freq_single(fftw_complex* H_result, double * tx, double * rx, unsigned long int sig_len, int BW1, int BW2, int fs){

    fftw_complex *rx_fft=(fftw_complex*)fftw_malloc(sig_len*sizeof(fftw_complex));
    if(rx_fft==NULL){fprintf(stderr,"malloc failed\n");exit(1);}

    fftw_complex *tx_fft=(fftw_complex*)fftw_malloc(sig_len*sizeof(fftw_complex));
    if(tx_fft==NULL){fprintf(stderr,"malloc failed\n");exit(1);}


    fftw_plan p1=fftw_plan_dft_r2c_1d(sig_len, tx, tx_fft, FFTW_ESTIMATE);
    if (p1==NULL){fprintf(stderr,"plan creation failed\n");exit(1);}

    fftw_plan p2=fftw_plan_dft_r2c_1d(sig_len, rx, rx_fft, FFTW_ESTIMATE);
    if (p2==NULL){fprintf(stderr,"plan creation failed\n");exit(1);}


    fftw_execute(p1);
    fftw_execute(p2);
    //cout <<sig_len <<endl;

    double delta_f = (double)fs/(double)sig_len;
    int fft_begin = (int) round(BW1/delta_f);
    int fft_end = (int) round(BW2/delta_f);

    estimate_H(H_result, tx_fft, rx_fft, fft_begin, fft_end, sig_len);

    fftw_destroy_plan(p1);
    fftw_destroy_plan(p2);

    fftw_free(tx_fft);
    fftw_free(rx_fft);

    //delete []abs_h;
}





double sum_sqr(double* in, int begin, int end){
    double sum0 = 0;
    for(int i = begin; i < end; ++i){
        sum0 += in[i]*in[i];
    }
    return sum0;
}


double multiptle_sum(double* in1, int begin1, int end1, double * in2 ,int begin2, int end2){
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "msum %d %d %d %d",begin1,end1,begin2,end2);
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "msum %d %d",end1-begin1,end2-begin2);
    assert(end1-begin1 == end2 - begin2);
    double sum0 = 0;
    for(int i = 0; i < end1 - begin1 ; ++i){
        sum0 += in1[begin1 + i]*in2[begin2+i];
    }

    return sum0;
}

int find_max(double* in, unsigned long int len_in){
//    int maxPosition = max_element(in,in+len_in) - in;
    double maxval=0;
    int maxidx=-1;
    for (int i = 0; i < len_in; i++) {
        if (in[i] > maxval) {
            maxval=in[i];
            maxidx=i;
        }
    }
//    int maxPosition=maxval-in;
    return maxidx;
}

// received signal, total_length is length of signal
// Nu=length of symbol (720)
// N0 = guard interval (480), we will use CP version
// DIVIDE_FACTOR = 2
// Implementation of Naiser correlation
int naiser_corr(double* signal, int total_length , int Nu, int N0, int DIVIDE_FACTOR, jboolean include_zero, double threshold, double nshoulder, int numSym){
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug2", "naiser corr %d",total_length);

    //short PN_seq[8] = {1, -1, -1, -1, -1, -1, 1, -1};
    //short PN_seq[7] = {1, 1, 1, -1, 1, -1, -1};

    int N_both = Nu + N0;
    int preamble_L = numSym*N_both;
//    __android_log_print(ANDROID_LOG_VERBOSE, "xcorr","naiser xcorr: %d, %d, %d, %d", Nu, N0, numSym,total_length );

    if(total_length-preamble_L < 0){
//        cout << "input signal too short" << endl;
        __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug","input signal too short");
        return -1;
    }

    int len_corr = (total_length - preamble_L)/DIVIDE_FACTOR + 2;
    int num = 0;
//    double* Mn = new double[len_corr];
    double* Mn = calloc(len_corr,sizeof(double));
    memset(Mn, 0, sizeof(double)*len_corr);
//    char* sig = getString_d(signal, total_length);
    for(int i = 0; i < total_length - preamble_L - 1; i += DIVIDE_FACTOR){
        //cout << i << ' ' << total_length - preamble_L - 1 << ' '  << len_corr  << endl;
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "it %d/%d",i,total_length - preamble_L - 1);
        double Pd = 0;
        for(int k = 0; k < numSym - 1; ++k){
//            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "it2 %d",k);
            int bk = PN_seq[k]*PN_seq[k+1];
            double temp_P = multiptle_sum(signal + i, k*N_both + N0, (k + 1)*N_both,
                                          signal + i, (k + 1)*N_both + N0, (k+2)*N_both);
            Pd +=  temp_P*bk;
        }
        double Rd = 0;
//        if(include_zero){
//            Rd = (sum_sqr(signal + i, 0, preamble_L)*Nu)/N_both;
//        }
//        else{
            for(int k = 0; k < numSym; ++k){
                Rd += sum_sqr(signal + i, k*N_both + N0, (k + 1)*N_both);
            }
//        }

        Mn[num] = Pd/Rd;
//
        num ++;
    }
//    char* va = getString_d(Mn, len_corr);
    int max_idx = find_max(Mn, (unsigned long int)len_corr);
    if(max_idx < 0){
        return -1;
    }
    double max_value = Mn[max_idx];
    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug","max peak: %d, %.2f, %.2f", max_idx, max_value, threshold);


//    char* str2=getString_d(signal,total_length);

    last_naiser_val = max_value;
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug6","naiser value %.2f %.2f",max_value,threshold);
    if(max_value < threshold){
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug2","naiser_err1 %.2f %.2f",max_value,threshold);
//        delete [] Mn;
        free(Mn);
        return -1;
    }
    else {
        __android_log_print(ANDROID_LOG_VERBOSE, "debug2","naiser_pass1 %.2f %.2f",max_value,threshold);
    }

    double shoulder = nshoulder*max_value;
    int right = -1;
    int left = -1;

    // find the right shoulder point
    for(int i = max_idx; i < len_corr - 1; ++i){
        if(Mn[i] >= shoulder && Mn[i+1] <= shoulder){
            right = i;
            break;
        }
    }

    // find the left shoulder point
    for(int i = max_idx; i > 0; --i){
        if(Mn[i] >= shoulder && Mn[i-1] <= shoulder){
            left = i;
            break;
        }
    }

    if(left == -1 || right == -1){
//        delete [] Mn;
        free(Mn);
        if (max_idx*DIVIDE_FACTOR==0) {
            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "out1 %d", max_idx * DIVIDE_FACTOR);
        }
        return max_idx*DIVIDE_FACTOR;
    }
    else{
        double middle = (right+left)*DIVIDE_FACTOR/2;
//        delete [] Mn;
        free(Mn);
        if (lround(middle)==0) {
            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "out2 %d", lround(middle));
        }
        return lround(middle);
    }
}

// result, tx -> channel length = Ns
// rx -> recv_len = 8*(Ns+N0)
// do the channel estimation for multiple OFDM symbols (call `channel_estimation_freq_single()` multple times)
void channel_estimation_freq_multiple(double* result, double * tx, unsigned long int Ns, double * rx, unsigned long int recv_len, unsigned long int N0, int BW1, int BW2, int fs, int numSym){
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug", "channel estimation %d %d %d",recv_len,Ns,N0);

    assert(  recv_len == numSym*(Ns+N0) );


    fftw_complex *H_avg =(fftw_complex*)fftw_malloc(Ns*sizeof(fftw_complex));
    if(H_avg==NULL){fprintf(stderr,"malloc failed\n");exit(1);}

    for(int i = 0; i<numSym; ++i){
        fftw_complex *H_result =(fftw_complex*)fftw_malloc(Ns*sizeof(fftw_complex));
        if(H_result==NULL){fprintf(stderr,"malloc failed\n");exit(1);}

        channel_estimation_freq_single(H_result, tx, rx + i*(N0+Ns) + N0, Ns, BW1, BW2, fs);
        if(i == 0){
            for(unsigned long int j = 0; j < Ns; ++j ){
                H_avg[j][0] = PN_seq[i]*H_result[j][0];
                H_avg[j][1] = PN_seq[i]*H_result[j][1];
            }
        }
        else{
            for(unsigned long int j = 0; j < Ns; ++j ){
                H_avg[j][0] += PN_seq[i]*H_result[j][0];
                H_avg[j][1] += PN_seq[i]*H_result[j][1];
            }
        }

        fftw_free(H_result);
    }

    for(unsigned long int j = 0; j < Ns; ++j ){
        H_avg[j][0] /= (numSym*Ns); //H_result[j][0];
        H_avg[j][1] /= (numSym*Ns); //H_result[j][1];
    }

    fftw_complex *h=(fftw_complex*)fftw_malloc(Ns*sizeof(fftw_complex));
    if(h==NULL){fprintf(stderr,"malloc failed\n");exit(1);}

    fftw_plan ifft=fftw_plan_dft_1d(Ns, H_avg, h, FFTW_BACKWARD, FFTW_ESTIMATE);

    //double* abs_h = new double[Nu];

    fftw_execute(ifft);

    abs_complex(result, h, Ns);
//    char* out = getString_d(result,720);

    fftw_destroy_plan(ifft);

    fftw_free(H_avg);
    fftw_free(h);
}


int findhpeak(double* h, int Ntx1, int bias) {
    double maxval=-1;
    int maxidx=-1;
    for (int i = 0; i < Ntx1; i++) {
        if (h[i] > maxval) {
            maxval=h[i];
            maxidx=i;
        }
    }
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug","h vals %f %f %f %f %f",h[0],h[1],h[2],h[3],h[4]);
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug","h vals %f %f %f %f %f",h[715],h[716],h[717],h[718],h[719]);
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug","h max %d %f",maxidx,maxval);

    int* peaksidxs = calloc(Ntx1,sizeof(int));
    int peakcounter=0;
    int prevpeakidx=0;
    int prevpeakval=0;
    for (int i = 1; i < maxidx+10; i++) {
        if (i < Ntx1 && h[i-1] < h[i] && h[i] > h[i+1] && h[i] > maxval*.65 && i-prevpeakidx >= 2) {
            peaksidxs[peakcounter++]=i;
            prevpeakidx = i;
        }
    }

    int midx_new=maxidx;
    for (int i = 0; i < peakcounter; i++) {
        if (maxidx-peaksidxs[i] <= bias) {
            midx_new=peaksidxs[i];
            break;
        }
    }

    free(peaksidxs);

    return midx_new;
}

// if output is positive, good it passes, else... fail
// include the stage (b) pipeline for naiser correlation (`naiser_corr()`) and channel estimation (`channel_estimation_freq_multiple()`)
// naiser correlation and channel estimation
int corr2(int N, int xcorr_idx, double* filteredData, mycontext* cxt2, int globalOffset) {
//    __android_log_print(ANDROID_LOG_VERBOSE, "debug2","corr2");
//    int* out = calloc(2,sizeof(int));

    int outidx = -1;
    int offset=0;
    int Nu=cxt2->naiserTx1Count;
    int Ns2=cxt2->naiserTx2Count;
    unsigned long int N0 = cxt2->N0;
    jboolean CP = cxt2->CP;
    jint win_size = cxt2->win_size;
    jint bias = cxt2->bias;

    jint Nrx=Ns2+win_size;
//    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug","corr2 length %d %d ",N, Ns2 + bias+10);

    if (N > Ns2 + bias+10) {
        double* naiser_sig = calloc(N,sizeof(double));

        memcpy(naiser_sig,filteredData,N*sizeof(double));


        clock_t begin = clock();

        int naiser_idx = -1;
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug2","naiser_threshold %d %d %.2f",cxt->responder,cxt->timingOffset,cxt2->naiserThresh);
        //__android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","naiser_sig begin");
        if(cxt2->responder && cxt2->timingOffset == 0){
            naiser_idx = naiser_corr(naiser_sig, Nrx, Nu, N0, 16, !CP, 0.45, cxt2->naiserShoulder,cxt2->numSyms);
        }else{
            naiser_idx = naiser_corr(naiser_sig, Nrx, Nu, N0, 16, !CP, cxt2->naiserThresh, cxt2->naiserShoulder,cxt2->numSyms);
        }

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        //__android_log_print(ANDROID_LOG_VERBOSE, "time2", "%.4f", time_spent);

//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***nindex %d",naiser_idx);
        if (naiser_idx>0) {
            outidx = xcorr_idx; // we don't care about the exact naiser_idx, only failure case
        }
        else {
            // did not pass naiser, false positive
            free(naiser_sig);
            return -1;
        }

        int start_idx2 = xcorr_idx - bias +1;
        int end_idx2 = start_idx2 + Ns2;
        __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","channel est begin");


        if (start_idx2 >= 0 && end_idx2 < N && naiser_idx > 0) {
            int BW1=1000;
            int BW2=5000;

            int Ntx1=cxt2->naiserTx1Count;
            int Ntx2=cxt2->naiserTx2Count;
            double* h = calloc(Ntx1,sizeof(double));

            double* h_sig = calloc(Ns2,sizeof(double));
            memcpy(h_sig,&filteredData[start_idx2],Ns2*sizeof(double));
//            char* str5=getString_d(h,Ntx1);

//            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***hindex %d %f %f %f %f %f",
//                                xcorr_idx,filteredData[0],filteredData[1],filteredData[2],filteredData[3],filteredData[4]);
//            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***hindex %d %f %f %f %f %f",
//                                xcorr_idx,h_sig[0],h_sig[1],h_sig[2],h_sig[3],h_sig[4]);

            begin = clock();

            channel_estimation_freq_multiple(h, cxt2->naiserTx1, Ntx1,
                                             h_sig, Ntx2,
                                             N0,  BW1,  BW2,  FS, cxt2->numSyms);
//            char*  str1 = getString_d(cxt2->naiserTx1, Ntx1);
            int h_idx=findhpeak(h,Ntx1,cxt2->bias);
            end = clock();
            time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//            __android_log_print(ANDROID_LOG_VERBOSE, "time3", "%.4f", time_spent);
            free(h);
            free(h_sig);
            if (h_idx>0) {
                outidx = xcorr_idx - bias + h_idx;
            }
//            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***hindex %d %d",h_idx,outidx);
        }
        else {
//            __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***h fail %d %d %d %d",naiser_idx, naiser_idx+1-bias, naiser_idx+Ns2-bias, N);
        }
        free(naiser_sig);
    }
    else {
        outidx = -1;
        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***nindex fail %d %d %d %d",xcorr_idx, xcorr_idx-win_size, xcorr_idx+Ns2+win_size, N);
    }

    return outidx;
}

// includes the entire pipeline for the signal processing from cross-correlation to naiser correlation and channel estimation
int* xcorr_helper2(void* context, short* data, int globalOffset, int N) {
    mycontext* cxt = (mycontext*)context;

    int N_ref = cxt->naiserTx2Count;

    double *refData = cxt->naiserTx2;
    clock_t t_minus = clock();
    double *filteredData = filter_s(data, N);
    for (int i = 0; i < N; i++) {
        filteredData[i] /= 10000.0;
    }
    double* filteredData2=filteredData+filt_offset;
    N -= filt_offset;


    clock_t t0 = clock();
    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","xcorr begin");
    int* xcorr_out = xcorr(filteredData2, refData, N, N_ref, cxt->queuedSegments,
                   globalOffset, cxt->xcorrthresh, cxt->minPeakDistance, cxt->seekback,
                   cxt->pthresh,cxt->getOneMoreFlag);
    __android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","xcorr end");
    //idx,val
    //__android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","xcorr end time t %.3f", (double)clock()/CLOCKS_PER_SEC);
    //__android_log_print(ANDROID_LOG_VERBOSE, "xcorr","before naiser %d %d %.0f %d", xcorr_out[0], xcorr_out[1],cxt->xcorrthresh,globalOffset);

    clock_t t1 = clock();



//    if (xcorr_out[0]<0) {
////        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***xcorr output neg %d", xcorr_out[0]);
//    }
//    else {
////        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "***xcorr output pos %d", xcorr_out[0]);
//    }

    int* result = calloc(3,sizeof(int));
    result[0] = xcorr_out[0]; //xcorr out
    result[1] = xcorr_out[1]; //xcorr val
    result[2] = -1;
    //__android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","corr2 begin");
    if (cxt->naiser && xcorr_out[0] > 0) {
        int global_lower_bound = globalOffset + xcorr_out[0] - cxt->win_size;
        if(global_lower_bound < 0){
            global_lower_bound = 0;
        }
        int front_padding = (globalOffset - global_lower_bound);
        int new_corr_id = front_padding + xcorr_out[0];
        short *data2 = cxt->data + global_lower_bound;
        N=cxt->bigBufferSize*2 + front_padding - 1;
        filteredData = filter_s(data2, N);
        for (int i = 0; i < N; i++) {
            filteredData[i] /= 10000.0;
        }
        double* filteredData2=filteredData+filt_offset;
        N -= filt_offset;
        int idx = corr2(N,new_corr_id,filteredData2,cxt,globalOffset);
        if(idx!=-1){
            idx = idx - front_padding;
        }
        result[2] = idx; // naiser out
    }
    else if ((!cxt->naiser) && xcorr_out[0] > 0){
        result[2] =  xcorr_out[0];
    }
    //__android_log_print(ANDROID_LOG_VERBOSE,"speaker_debug","corr2 end");
    clock_t t2 = clock();
    double time_spent0 = (double)(t0 - t_minus) / CLOCKS_PER_SEC;
    double time_spent = (double)(t1 - t0) / CLOCKS_PER_SEC;
    double time_spent2 = (double)(t2 - t1) / CLOCKS_PER_SEC;
//    __android_log_print(ANDROID_LOG_VERBOSE, "speaker_debug", "xcorr time %.4f %.4f, %.4f", time_spent0, time_spent, time_spent2);
    free(xcorr_out);
    free(filteredData);

    return result;
}


JNIEXPORT void JNICALL
Java_com_example_nativeaudio_NativeAudio_testxcorr(JNIEnv *env, jclass clazz,jdoubleArray tempData,
                                                   jdoubleArray tempRefData1, jdoubleArray tempRefData2, jint N0,
                                                   jboolean CP) {
    jint N = (*env)->GetArrayLength(env, tempData);
    jint N_ref1 = (*env)->GetArrayLength(env, tempRefData1);
    jint N_ref2 = (*env)->GetArrayLength(env, tempRefData2);

    jdouble* data = (*env)->GetDoubleArrayElements(env,tempData, NULL);
    jdouble* refData1 = (*env)->GetDoubleArrayElements(env,tempRefData1, NULL);
    jdouble* refData2 = (*env)->GetDoubleArrayElements(env,tempRefData2, NULL);

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug","file lens %d %d %d",N,N_ref1,N_ref2);

    int filt_offset=127;
    for (int i = 0; i < 5; i++) {
        chirp_indexes[i]=-1;
    }
//    char* str1=getString_d(data,N);
//    char* str2=getString_d(refData2,N_ref2);

    double* filtered=filter_d(data,N);

    for (int i = 0; i < N; i++) {
        filtered[i]/=10000.0;
    }
    filtered=filtered+filt_offset;
    N -= filt_offset;

//    int* xcorr(double* filteredData, double* refData, int filtLength, int refLength, int i, int globalOffset, int xcorrthresh, double minPeakDistance, int seekback, double pthresh, jboolean getOneMoreFlag) {
    int* result = xcorr(filtered, refData2, N, N_ref2, 0,
                    0, 4, 1.5, 960, .65,JNI_FALSE);

//    char* str3=getString_d(filtered,N);

    int corr_idx= result[0];

//    __android_log_print(ANDROID_LOG_VERBOSE, "debug","corrected xcorr after filter %d %d",result[0],result[1]);

    int win_size = 4800;
    int Ntx2= N_ref2;
    int Nu=N_ref1;
    int bias = 320;

    jint Nrx=Ntx2+(win_size*2);
    double* signal2 = calloc(Nrx,sizeof(double));

    int start_idx = corr_idx-win_size;
    int end_idx = start_idx+Nrx;
    if (start_idx < 0 || end_idx > N) {
        __android_log_print(ANDROID_LOG_VERBOSE, "debug","naiser bounds error %d %d %d",start_idx,end_idx,Nrx);
    }
    else {
        memcpy(signal2, &filtered[start_idx], Nrx * sizeof(double));

//        char *nstr = getString_d(signal2, Nrx);
        int naiser_idx = naiser_corr(signal2, Nrx, Nu, N0, 8, !CP, .45, .8,8);
//        __android_log_print(ANDROID_LOG_VERBOSE, "debug", "naiser %d", naiser_idx);

        if (naiser_idx >= 0) {
            double *h_sig = calloc(Ntx2, sizeof(double));
            int start_idx2 = corr_idx - bias + 1;
            int end_idx2 = start_idx2 + Ntx2;
            if (start_idx2 < 0 || end_idx2 > N) {
                __android_log_print(ANDROID_LOG_VERBOSE, "debug", "hsig bounds error %d %d %d",
                                    start_idx, end_idx, Nrx);
            } else {
                memcpy(h_sig, &filtered[start_idx2], Ntx2 * sizeof(double));

                //    char* out = getString_d(h_sig, 8640);

                double *h = calloc(Nu, sizeof(double));
                //    void channel_estimation_freq_multiple(double* result, double * tx, unsigned long int Ns, double * rx, unsigned long int recv_len, unsigned long int N0, int BW1, int BW2, int fs){

                channel_estimation_freq_multiple(h, refData1, Nu,
                                                 h_sig, Ntx2,
                                                 N0, 1000, 5000, 44100, 8);
                //    char* out2 = getString_d(h, 720);

                int h_idx = findhpeak(h, Nu, bias);
                int global_idx = corr_idx - bias + h_idx;
//                __android_log_print(ANDROID_LOG_VERBOSE, "debug", "h index %d", h_idx);
//                __android_log_print(ANDROID_LOG_VERBOSE, "debug", "global index %d", global_idx);
            }
        }
    }
}

JNIEXPORT jint JNICALL
Java_com_example_nativeaudio_NativeAudio_getNumChirps(JNIEnv *env, jclass clazz) {
    return chirpsPlayed;
}