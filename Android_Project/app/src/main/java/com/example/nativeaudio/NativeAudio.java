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
 */

package com.example.nativeaudio;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
//import android.support.annotation.NonNull;
//import android.support.v4.app.ActivityCompat;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.app.ActivityCompat;
import androidx.core.widget.NestedScrollView;

import java.util.Date;

public class NativeAudio extends AppCompatActivity
        implements ActivityCompat.OnRequestPermissionsResultCallback,
        SensorEventListener {

    String[] perms = new String[]{Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE};
    //static final String TAG = "NativeAudio";
    private static final int AUDIO_ECHO_REQUEST = 0;

    static final int CLIP_NONE = 0;
    static final int CLIP_HELLO = 1;
    static final int CLIP_ANDROID = 2;
    static final int CLIP_SAWTOOTH = 3;
    static final int CLIP_PLAYBACK = 4;

    static String URI;
    static AssetManager assetManager;

    static boolean isPlayingAsset = false;
    static boolean isPlayingUri = false;

    private static SensorManager sensorManager;
    private Sensor accelerometer,accelerometer_uncalib,gyroscope,gyroscope_uncalib,magnetometer,magnetometer_uncalib,pressure,linear_acc,rotation_vec;

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }

    static Activity av;
    /** Called when the activity is first created. */
    @Override
    @TargetApi(17)
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);

//        NotificationManager notificationManager =
//                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
//
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
//                && !notificationManager.isNotificationPolicyAccessGranted()) {
//
//            Intent intent = new Intent(
//                    android.provider.Settings
//                            .ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
//
//            startActivity(intent);
//        }

        av = NativeAudio.this;
        ActivityCompat.requestPermissions(this,
                perms,
                1234);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        AudioManager am = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        getSupportActionBar().hide();
        NativeAudio.createEngine();
        NativeAudio.createBufferQueueAudioPlayer(Constants.fs, Constants.bufferSize);
        int micInterface = 0;
        if (Build.MODEL.equals("IN2020")) {
            micInterface=1;
        }
        Log.e("interface",micInterface+"");
        NativeAudio.createAudioRecorder(micInterface);

        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);

        accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        accelerometer_uncalib = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER_UNCALIBRATED);
        gyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        gyroscope_uncalib = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED);
        magnetometer = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
        magnetometer_uncalib = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED);
        pressure = sensorManager.getDefaultSensor(Sensor.TYPE_PRESSURE);
        linear_acc = sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
        rotation_vec = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);

//        NativeAudio.init();
        Constants.et1 = (EditText)findViewById(R.id.editTextNumberDecimal);
        Constants.et2 = (EditText)findViewById(R.id.editTextNumberDecimal2);
        Constants.et3 = (EditText)findViewById(R.id.editTextNumberDecimal4);
        Constants.et4 = (EditText)findViewById(R.id.editTextNumber5);
        Constants.et5 = (EditText)findViewById(R.id.editTextNumber2);
        Constants.et7 = (EditText)findViewById(R.id.etFileNumber);
        Constants.et10 = (EditText)findViewById(R.id.editTextNumber6);
        Constants.et16 = (EditText)findViewById(R.id.editTextNumber16);

        Constants.tv = (TextView)findViewById(R.id.textView3);
        Constants.tv2 = (TextView)findViewById(R.id.textView4);
        Constants.tv11 = (TextView)findViewById(R.id.textView11);
        Constants.distOut=(TextView)findViewById(R.id.textView5);
        Constants.debugPane=(TextView)findViewById(R.id.debugPane);
        Constants.sview = (NestedScrollView) findViewById(R.id.scrollView);
        Constants.recButton=(Button) findViewById(R.id.record);
        Constants.stopButton=(Button) findViewById(R.id.button2);
        Constants.clearButton=(Button) findViewById(R.id.button);
        Constants.sw1=(Switch) findViewById(R.id.switch1);
        Constants.sw2=(Switch) findViewById(R.id.switch2);
        Constants.sw3=(Switch) findViewById(R.id.switch3);
        Constants.sw4=(Switch) findViewById(R.id.switch4);
        Constants.clayout = (ConstraintLayout)findViewById(R.id.clayout);

        Constants.debugPane.setMovementMethod(new ScrollingMovementMethod());
//        for (int i = 0; i < 50; i++) {
//            Utils.log(i+"");
//            Log.e("bottom",Constants.debugPane.getBottom()+"");
//        }
//        Constants.sview.setMovementMethod(new ScrollingMovementMethod());

        Constants.init(this);

        assetManager = getAssets();

        Constants.et1.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et1.getText().toString();
                if (Utils.isFloat(ss)) {
                    editor.putFloat("vol", Float.parseFloat(ss));
                    editor.commit();
                    Constants.vol = Float.parseFloat(ss);
                }
            }
        });
        Constants.et2.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et2.getText().toString();
                if (Utils.isInteger(ss)) {
                    editor.putInt("recTime", Integer.parseInt(ss));
                    editor.commit();
                    Constants.recTime = Integer.parseInt(ss);
                }
            }
        });

        Constants.et3.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et3.getText().toString();
                if (Utils.isFloat(ss)) {
                    editor.putFloat("replyDelay", Float.parseFloat(ss));
                    editor.commit();
                    Constants.replyDelay = Float.parseFloat(ss);
                }
            }
        });

        Constants.et4.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et4.getText().toString();
                if (Utils.isInteger(ss)) {
                    editor.putInt("initSleep", Integer.parseInt(ss));
                    editor.commit();
                    Constants.initSleep = Integer.parseInt(ss);
                }
            }
        });

        Constants.et5.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et5.getText().toString();
                if (Utils.isFloat(ss)) {
                    editor.putFloat("xcorrthresh", Float.parseFloat(ss));
                    editor.commit();
                    Constants.xcorrthresh = Float.parseFloat(ss);
                }
            }
        });


        Constants.et7.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et7.getText().toString();
                if (Utils.isInteger(ss)) {
                    editor.putInt("fileID", Integer.parseInt(ss));
                    editor.commit();
                    Constants.fileID = Integer.parseInt(ss);
                    Constants.loadData(NativeAudio.this);
                }
            }
        });


        Constants.et10.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et10.getText().toString();
                if (Utils.isInteger(ss)) {
                    editor.putInt("user_id", Integer.parseInt(ss));
                    editor.commit();
                    Constants.user_id = Integer.parseInt(ss);
                    Constants.loadData(NativeAudio.this);
                }
            }
        });

//        Constants.et11.addTextChangedListener(new TextWatcher() {
//            @Override
//            public void afterTextChanged(Editable s) {
//            }
//            @Override
//            public void beforeTextChanged(CharSequence s, int start,
//                                          int count, int after) {
//            }
//            @Override
//            public void onTextChanged(CharSequence s, int start,
//                                      int before, int count) {
//                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
//                String ss = Constants.et11.getText().toString();
//                if (Utils.isInteger(ss)) {
//                    editor.putInt("bias", Integer.parseInt(ss));
//                    editor.commit();
//                    Constants.bias = Integer.parseInt(ss);
//                }
//            }
//        });
//
//        Constants.et12.addTextChangedListener(new TextWatcher() {
//            @Override
//            public void afterTextChanged(Editable s) {
//            }
//            @Override
//            public void beforeTextChanged(CharSequence s, int start,
//                                          int count, int after) {
//            }
//            @Override
//            public void onTextChanged(CharSequence s, int start,
//                                      int before, int count) {
//                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
//                String ss = Constants.et12.getText().toString();
//                if (Utils.isInteger(ss)) {
//                    editor.putInt("seekback", Integer.parseInt(ss));
//                    editor.commit();
//                    Constants.seekback = Integer.parseInt(ss);
//                }
//            }
//        });




        Constants.et16.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                String ss = Constants.et16.getText().toString();
                if (Utils.isInteger(ss)) {
                    editor.putInt("calibWait", Integer.parseInt(ss));
                    editor.commit();
                    Constants.calibWait = Integer.parseInt(ss);
                }
            }
        });

        Constants.recButton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                Constants.stop=false;
                register_sensors();
                int status = ActivityCompat.checkSelfPermission(NativeAudio.this,
                        Manifest.permission.RECORD_AUDIO);
                if (status != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(
                            NativeAudio.this,
                            new String[]{Manifest.permission.RECORD_AUDIO},
                            AUDIO_ECHO_REQUEST);
                    return;
                }
                closeKeyboard();
                recordAudio();
            }
        });
        Constants.stopButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Constants.recordImu=false;
                deregister_sensors();
                forcewrite();
                FileOperations.writeSensorsToDisk(NativeAudio.this,Constants.tt+"");
                try {
                    if (Constants.recTime==1800) {
                        Thread.sleep(60000);
                    }
                    else {
                        Thread.sleep(3000);
                    }
                }
                catch(Exception e) {
                    Log.e("asdf",e.toString());
                }

                double[] out = NativeAudio.getDistance(Constants.reply);

                if (out.length==5) {
                    Constants.distOut.setText(String.format("%d\n%d\n%d\n%d\n%d", (int)out[0], (int)out[1], (int)out[2], (int)out[3], (int)out[4]));
                }
                else if (out.length==7) {
                    Constants.distOut.setText(String.format("%.02f\n%.02f\n%.02f\n%d\n%d\n%d\n%d\n",
                            out[0], out[1], out[2], (int)out[3], (int)out[4], (int)out[5], (int)out[6]));
                }

                Constants.stop=true;
//                stopit();
                NativeAudio.reset();
                Constants.task.cancel(true);
                Constants.timer.cancel();
                Constants.tv2.setText("0");
                Constants.toggleUI();

                Log.e("debug2","change bg color");
                (NativeAudio.av).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Constants.clayout.setBackgroundColor(Color.argb(255, 255, 255, 255));
                    }
                });
            }
        });

        Constants.clearButton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                Constants.debugPane.setText("");
                Constants.distOut.setText("");
                Constants.clayout.setBackgroundColor(Color.argb(255, 255, 255, 255));
            }
        });

        Constants.sw1.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                editor.putBoolean("water", isChecked);
                editor.commit();
                Constants.water  = isChecked;
                if (isChecked) {
                    Constants.sw1.setText("Water");
                }
                else {
                    Constants.sw1.setText("Air");
                }
            }
        });

        Constants.sw2.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                editor.putBoolean("reply", isChecked);
                editor.commit();
                Constants.reply  = isChecked;
            }
        });

        Constants.sw3.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                editor.putBoolean("naiser", isChecked);
                editor.commit();
                Constants.naiser  = isChecked;
            }
        });

        Constants.sw4.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(NativeAudio.this).edit();
                editor.putBoolean("runxcorr", isChecked);
                editor.commit();
                Constants.runxcorr  = isChecked;
            }
        });
//        Tests.testxcorr(this);
    }

    // Single out recording for run-permission needs
    private void recordAudio() {
        int status = ActivityCompat.checkSelfPermission(NativeAudio.this,
                Manifest.permission.RECORD_AUDIO);
        if (status != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    NativeAudio.this,
                    new String[]{Manifest.permission.RECORD_AUDIO},
                    AUDIO_ECHO_REQUEST);
            return;
        }

        Constants.task = new MyTask(NativeAudio.this,NativeAudio.this).execute();
    }

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onPause() {
        super.onPause();
    }

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy() {
        shutdown();
        super.onDestroy();
    }

    public void closeKeyboard() {
        View view = this.getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

    /** Native methods, implemented in jni folder */
    public static native void createEngine();
    public static native void createBufferQueueAudioPlayer(int sampleRate, int samplesPerBuf);
    public static native boolean createAudioRecorder(int micInterface);
    public static native void stopit();
    public static native void reset();
    public static native void forcewrite();
    public static native void shutdown();
    public static native long nowms2();
    public static native void calibrate(short[] data, short[] refData, int bufferSize_spk, int bufferSize_mic, int recordTime,
                                        String topfilename, String bottomfilename,
                                        String meta_filename, int initialOffset, int warmdown_len, int preamble_len,
                                        boolean water,
                                        boolean reply, boolean naiser, int sendDelay,float xcorrthresh, float minPeakDistance,
                                        int fs,
                                        double[] naiserTx1, double[] naiserTx2, int Ns, int N0, boolean CP, int N_FSK,
                                        float naiserThresh, float naiserShoulder,
                                        int win_size, int bias, int seekback, double pthresh, int round, int filenum, boolean runxcorr, int initialDelay,
                                        String mic_ts_fname, String speaker_ts_fname,int bigBufferSize,int bigBufferTimes, int numSym, int calibWait);

    public static native void testxcorr(double[] data, double[] refData, double[] refData2, int N0, boolean CP);
    public static native double[] getDistance(boolean reply);
    public static native double[] getVal();
    public static native boolean responderDone();
    public static native boolean replySet();
    public static native int getXcorrCount();
    public static native int[] getReplyIndexes();
    public static native int getQueuedSpeakerSegments();
    public static native int getNumChirps();

    /** Load jni .so on initialization */
    static {
        System.loadLibrary("native-audio-jni");
    }

    public void register_sensors() {
//        int speed = SensorManager.SENSOR_DELAY_FASTEST;
        int speed = SensorManager.SENSOR_DELAY_NORMAL;
        sensorManager.registerListener(this, accelerometer, speed);
//        sensorManager.registerListener(this, accelerometer_uncalib, speed);
        sensorManager.registerListener(this, gyroscope, speed);
//        sensorManager.registerListener(this, gyroscope_uncalib, speed);
        sensorManager.registerListener(this, magnetometer, speed);
        sensorManager.registerListener(this, magnetometer_uncalib, speed);
        sensorManager.registerListener(this, pressure, speed);
        sensorManager.registerListener(this, linear_acc, speed);
        sensorManager.registerListener(this, rotation_vec, speed);
    }

    public void deregister_sensors() {
        sensorManager.unregisterListener(this);
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
//        long timeInMillis = event.timestamp;
        long timeInMillis=NativeAudio.nowms2();
        if (event.sensor.equals(accelerometer)&&Constants.recordImu) {
            Constants.time_acc.add(timeInMillis);
            Constants.accx.add(event.values[0]);
            Constants.accy.add(event.values[1]);
            Constants.accz.add(event.values[2]);
        }
        else if (event.sensor.equals(accelerometer_uncalib)&&Constants.recordImu) {
            Constants.time_acc_uncalib.add(timeInMillis);
            Constants.accx_uncalib.add(event.values[0]);
            Constants.accy_uncalib.add(event.values[1]);
            Constants.accz_uncalib.add(event.values[2]);
        }
        else if (event.sensor.equals(gyroscope)&&Constants.recordImu) {
            Constants.time_gyro.add(timeInMillis);
            Constants.gyrox.add(event.values[0]);
            Constants.gyroy.add(event.values[1]);
            Constants.gyroz.add(event.values[2]);
        }
        else if (event.sensor.equals(gyroscope_uncalib)&&Constants.recordImu) {
            Constants.time_gyro_uncalib.add(timeInMillis);
            Constants.gyrox_uncalib.add(event.values[0]);
            Constants.gyroy_uncalib.add(event.values[1]);
            Constants.gyroz_uncalib.add(event.values[2]);
        }
        else if (event.sensor.equals(magnetometer)&&Constants.recordImu) {
            Constants.time_mag.add(timeInMillis);
            Constants.magx.add(event.values[0]);
            Constants.magy.add(event.values[1]);
            Constants.magz.add(event.values[2]);
        }
        else if (event.sensor.equals(magnetometer_uncalib)&&Constants.recordImu) {
            Constants.time_mag_uncalib.add(timeInMillis);
            Constants.magx_uncalib.add(event.values[0]);
            Constants.magy_uncalib.add(event.values[1]);
            Constants.magz_uncalib.add(event.values[2]);
        }
        else if (event.sensor.equals(pressure)&&Constants.recordImu) {
            Constants.time_pressure.add(timeInMillis);
            Constants.pressure_data.add(event.values[0]);
        }
        else if (event.sensor.equals(linear_acc)&&Constants.recordImu) {
            Constants.time_linear_acc.add(timeInMillis);
            Constants.linearaccx.add(event.values[0]);
            Constants.linearaccy.add(event.values[1]);
            Constants.linearaccz.add(event.values[2]);
        }
        else if (event.sensor.equals(rotation_vec)&&Constants.recordImu) {
            Constants.time_rot.add(timeInMillis);
            Constants.rotx.add(event.values[0]);
            Constants.roty.add(event.values[1]);
            Constants.rotz.add(event.values[2]);
            Constants.rotz.add(event.values[3]);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {

    }
}
