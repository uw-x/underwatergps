package com.example.nativeaudio;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

public class Tests {
    public static void naiser(Activity cxt) {
        short[] arr = FileOperations.readrawasset_binary(cxt,R.raw.test);
        double[] pre1=Utils.convert(FileOperations.readrawasset_binary(cxt,R.raw.train_sig1));
        double[] pre2=Utils.convert(FileOperations.readrawasset_binary(cxt,R.raw.train_sig2));

        double[] arr2=Utils.convert(arr);
        NativeAudio.naiserCorrTest(pre1,pre2,arr2,15956);
    }

    public static void testxcorr(Activity cxt) {
//        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.d10m_5_2_bottom);
//        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.d5m_5_4_bottom);
//        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.d26m_4_3_bottom);
//        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.d26m_4_4_bottom);
//        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.d26m_5_2_bottom);
//        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.d5m_5_3_bottom);
        short[] temp = FileOperations.readrawasset_binary(cxt,R.raw.test);
        double[] arr = Utils.convert(temp);
//        arr = Utils.segment(arr,1,48000);

        double[] pre1=Utils.convert(FileOperations.readrawasset_binary(cxt,R.raw.n_1260_480_half_train_sig1));
        double[] pre2=Utils.convert(FileOperations.readrawasset_binary(cxt,R.raw.n_1260_480_half_train_sig2));
//        double[] pre1=Utils.convert(FileOperations.readrawasset_binary(cxt,R.raw.n_1080_360_half_signal_train_sig1));
//        double[] pre2=Utils.convert(FileOperations.readrawasset_binary(cxt,R.raw.n_1080_360_half_signal_train_sig2));
        NativeAudio.testxcorr(arr,pre1,pre2,480,true);
    }
}
