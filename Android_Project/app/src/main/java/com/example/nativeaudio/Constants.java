package com.example.nativeaudio;

import android.content.Context;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.AsyncTask;
import android.os.CountDownTimer;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.widget.NestedScrollView;

import java.util.ArrayList;

public class Constants {
//    public static int bufferSize = 192*25; // 4800
//public static int bufferSize = 192*60; // 11520
//    public static int bufferSize = 192*250; // 48000
//    public static int bufferSize = 192*500; // 96000
    public static int minbuffersize;
    public static int bufferSize,bigBufferSize,bigBufferTimes;
    public static boolean stop=false;
    public static boolean initsleeping=false;
    static EditText et1,et2,et3,et4,et5,et6,et7,et8,et9,et10,et13,et14,et15,et16; //et11,et12,
    public static NestedScrollView sview;
    static int fs=44100;
    static float naiserThresh=0.3f, naiserShoulder=0.8f;
    static int calibWait=10;
    static int win_size=3600;
    static int bias=240;
    static int initSleep=0;
    static float xcorrthresh=2f;
    public static float replyDelay = 3.0f; // for sender this is sending period, for replier, this is the minimum reply interval
//    static int calibSigLen = 4800;;
    static boolean water,reply,naiser=true,runxcorr=true;
    static float vol=0.01f, minPeakDistance=1.5f;
    static int fileID=0;
    static int recTime=30;
    static int N0;
    static int Ns;
    static int N_FSK;
    static boolean CP;
    static TextView tv,tv2,distOut,debugPane,tv11;
    static Switch sw1,sw2,sw3,sw4;
    static Button recButton,stopButton,clearButton;
    static AsyncTask task;
    static CountDownTimer timer;
    static double[] pre1,pre2;
    static double[] leader_pre1,leader_pre2;

    static short[] sig=null;
    static short[] leader_sig=null;
    static int seekback=960;
    static int user_id = 0;
    static float pthresh=.65f;
    static int rounds = 1;
    static float initialDelay = 3f;
    static float bufSizeInSeconds=.25f;
    static int bufferSize_spk=960*2;
    static float bigBufferSizeInSeconds = .25f;
    static ConstraintLayout clayout;

    static ArrayList<Long> time_acc,time_gyro,time_acc_uncalib,time_gyro_uncalib,time_mag,time_mag_uncalib,time_pressure,time_linear_acc,time_rot;
    static ArrayList<Float> accx,accy,accz;
    static ArrayList<Float> gyrox,gyroy,gyroz;
    static ArrayList<Float> accx_uncalib,accy_uncalib,accz_uncalib;
    static ArrayList<Float> gyrox_uncalib,gyroy_uncalib,gyroz_uncalib;
    static ArrayList<Float> magx,magy,magz;
    static ArrayList<Float> magx_uncalib,magy_uncalib,magz_uncalib;
    static ArrayList<Float> linearaccx,linearaccy,linearaccz;
    static ArrayList<Float> rotx,roty,rotz,rot4;
    static ArrayList<Float> pressure_data;
    static boolean recordImu=false;
    static long tt;
    static int numsym=8;

    public static void init(Context cxt) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(cxt);

        Constants.vol=prefs.getFloat("vol",vol);
        Constants.recTime=prefs.getInt("recTime",recTime);
        Constants.replyDelay=prefs.getFloat("replyDelay",replyDelay);
        Constants.initSleep=prefs.getInt("initSleep",initSleep);

        Constants.water=prefs.getBoolean("water",water);
        Constants.reply=prefs.getBoolean("reply",reply);
        Constants.naiser=prefs.getBoolean("naiser",naiser);
        Constants.xcorrthresh=prefs.getFloat("xcorrthresh",xcorrthresh);
        Constants.minPeakDistance=prefs.getFloat("minPeakDistance",minPeakDistance);
        Constants.fileID=prefs.getInt("fileID",fileID);
        Constants.naiserThresh=prefs.getFloat("naiserThresh",naiserThresh);
        Constants.naiserShoulder=prefs.getFloat("naiserShoulder",naiserShoulder);
//        Constants.win_size=prefs.getInt("win_size",win_size);
//        Constants.bias=prefs.getInt("bias",bias);
//        Constants.seekback=prefs.getInt("seekback",seekback);
        Constants.pthresh=prefs.getFloat("pthresh",pthresh);
        Constants.rounds=prefs.getInt("rounds",rounds);
        Constants.runxcorr=prefs.getBoolean("runxcorr",runxcorr);
        Constants.initialDelay=prefs.getFloat("initialDelay",initialDelay);
        Constants.user_id = prefs.getInt("user_id",user_id);
        Constants.calibWait = prefs.getInt("calibWait",calibWait);


        et1.setText(vol+"");
        et2.setText(recTime+"");
        et3.setText(replyDelay+"");
        et4.setText(initSleep+"");
        et5.setText(xcorrthresh+"");
        et6.setText(minPeakDistance+"");
        et7.setText(fileID+"");
        et8.setText(naiserThresh+"");
        et9.setText(naiserShoulder+"");
        et10.setText(user_id+"");
        et13.setText(pthresh+"");
        et14.setText(rounds+"");
        et15.setText(initialDelay+"");
        et16.setText(calibWait+"");
        sw1.setChecked(water);
        sw2.setChecked(reply);
        sw3.setChecked(naiser);
        sw4.setChecked(runxcorr);

        if (sw1.isChecked()) {
            Constants.sw1.setText("Water");
        }
        else {
            Constants.sw1.setText("Air");
        }

        AudioManager am = (AudioManager) cxt.getSystemService(Context.AUDIO_SERVICE);
        minbuffersize=Integer.parseInt(am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER));

        // buffersize is how often the speaker/microphone callback is invoked
        float bufSizeInSamples = fs*bufSizeInSeconds;
        bufferSize=((int)Math.ceil(bufSizeInSamples/minbuffersize))*minbuffersize;
//        bufferSize *= 10;

        // big buffer size is the buffer size used for xcorr processing
        float bigBufferSizeInSamples = fs*bigBufferSizeInSeconds;
        bigBufferTimes=((int)Math.ceil(bigBufferSizeInSamples/bufferSize));
        bigBufferSize = bigBufferTimes*bufferSize;
        Log.e("asdf","BUFFER "+minbuffersize+","+bufferSize+","+bigBufferSize);

        loadData(cxt);
    }

    public static void toggleUI() {
        Constants.recButton.setEnabled(!Constants.recButton.isEnabled());
        Constants.stopButton.setEnabled(!Constants.stopButton.isEnabled());
        Constants.clearButton.setEnabled(!Constants.clearButton.isEnabled());
        Constants.sw1.setEnabled(!Constants.sw1.isEnabled());
        Constants.sw2.setEnabled(!Constants.sw2.isEnabled());
        Constants.sw3.setEnabled(!Constants.sw3.isEnabled());
        Constants.sw4.setEnabled(!Constants.sw4.isEnabled());
        Constants.et1.setEnabled(!Constants.et1.isEnabled());
        Constants.et2.setEnabled(!Constants.et2.isEnabled());
        Constants.et3.setEnabled(!Constants.et3.isEnabled());
        Constants.et4.setEnabled(!Constants.et4.isEnabled());
        Constants.et5.setEnabled(!Constants.et5.isEnabled());
        Constants.et6.setEnabled(!Constants.et6.isEnabled());
        Constants.et7.setEnabled(!Constants.et7.isEnabled());
        Constants.et8.setEnabled(!Constants.et8.isEnabled());
        Constants.et9.setEnabled(!Constants.et9.isEnabled());
        Constants.et10.setEnabled(!Constants.et10.isEnabled());
//        Constants.et11.setEnabled(!Constants.et11.isEnabled());
//        Constants.et12.setEnabled(!Constants.et12.isEnabled());
        Constants.et13.setEnabled(!Constants.et13.isEnabled());
        Constants.et14.setEnabled(!Constants.et14.isEnabled());
        Constants.et15.setEnabled(!Constants.et15.isEnabled());
        Constants.et16.setEnabled(!Constants.et16.isEnabled());
    }

    public static void loadData(Context cxt) {
        String txt = "null";


        if (Constants.fileID==0) {
            txt=" seq5_FSK_1_480_1000_5000";
            N0 = 540;
            Ns = 1920;
            N_FSK = 2720;
            CP = false;
            numsym=4;
            leader_sig=FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_0_1920_540_2720_1000_2720);
            leader_pre1 =new double[Ns]; // this is the single OFDM symbol from 0-1
            for (int i = 0; i < Ns; i++) {
                leader_pre1[i]=leader_sig[i+N0]/31000.0;
            }
            leader_pre2 = new double[numsym*(Ns + N0)]; // this is the 4 OFDM symbols from 0-1
            for (int i = 0; i < leader_pre2.length; i++) {
                leader_pre2[i]=leader_sig[i]/31000.0;
            }

            if(!Constants.reply) {
                sig=FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_0_1920_540_2720_1000_2720);
                pre1 = new double[Ns];
                for (int i = 0; i < Ns; i++) {
                    pre1[i] = leader_pre1[i]/31000;
                }
                pre2 = new double[leader_pre2.length];
                for (int i = 0; i < leader_pre2.length; i++) {
                    pre2[i] = leader_pre2[i]/31000.0;
                }
            }
            else if(Constants.user_id == 0){
                sig=FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_2_1920_540_2720_1000_2720);
                pre1 = new double[Ns];
                pre2 = new double[sig.length];

                for (int i = 0; i < Ns; i++) {
                    pre1[i]=sig[i+N0]/31000;
                }
                for (int i = 0; i < pre2.length; i++) {
                    pre2[i]=sig[i]/31000.0;
                }
            }
            else if(Constants.user_id == 1){
                sig =FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_4_1920_540_2720_1000_2720);
                pre1 = new double[Ns];
                pre2 = new double[sig.length];

                for (int i = 0; i < Ns; i++) {
                    pre1[i]=sig[i+N0]/31000;
                }
                for (int i = 0; i < pre2.length; i++) {
                    pre2[i]=sig[i]/31000.0;
                }
            }
            else if(Constants.user_id == 2){
                sig=FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_6_1920_540_2720_1000_2720);
                pre1 = new double[Ns];
                pre2 = new double[sig.length];

                for (int i = 0; i < Ns; i++) {
                    pre1[i]=sig[i+N0]/31000;
                }
                for (int i = 0; i < pre2.length; i++) {
                    pre2[i]=sig[i]/31000.0;
                }
            }
            else if(Constants.user_id == 3){
                sig=FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_8_1920_540_2720_1000_2720);
                pre1 = new double[Ns];
                pre2 = new double[sig.length];

                for (int i = 0; i < Ns; i++) {
                    pre1[i]=sig[i+N0]/31000;
                }
                for (int i = 0; i < pre2.length; i++) {
                    pre2[i]=sig[i]/31000.0;
                }
            }
            else if(Constants.user_id == 4){
                sig=FileOperations.readrawasset_binary(cxt, R.raw.seq5_fsk_10_1920_540_2720_1000_2720);
                pre1 = new double[Ns];
                pre2 = new double[sig.length];

                for (int i = 0; i < Ns; i++) {
                    pre1[i]=sig[i+N0]/31000;
                }
                for (int i = 0; i < pre2.length; i++) {
                    pre2[i]=sig[i]/31000.0;
                }
            }
        }
        else if (Constants.fileID==1) {  // use long chirp as preamble
            txt="FMCW";
            Ns = 9300;
            N0 = 0;
            N_FSK = 0;
            CP = false;
            numsym = 4;

            sig=FileOperations.readrawasset_binary(cxt, R.raw.fmcw);
            leader_sig=FileOperations.readrawasset_binary(cxt, R.raw.fmcw);
            leader_pre1 =new double[Ns]; // this is the single OFDM symbol from 0-1
            leader_pre2 = new double[Ns]; // this is the 4 OFDM symbols from 0-1
            for (int i = 0; i < Ns; i++) {
                leader_pre1[i]=leader_sig[i]/31000.0;
                leader_pre2[i]=leader_sig[i]/31000.0;
            }

        }



        String finalTxt = txt;
        (NativeAudio.av).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                tv11.setText(finalTxt);
            }
        });
        Log.e("asdf","load file "+sig.length);
    }
}
