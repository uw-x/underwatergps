package com.example.nativeaudio;

import android.util.Log;

public class Utils {

    public static double[] segment(double[] data, int i, int j) {
        double[] out = new double[j-i+1];
        int counter=0;
        for (int k = i; k <= j; k++) {
            out[counter++] = data[k];
        }
        return out;
    }

    public static void div(double[] data, double val) {
        for (int i = 0; i < data.length; i++) {
            data[i] /= val;
        }
    }

    public static double[] convert(short[] s) {
        double[] out = new double[s.length];
        for (int i = 0; i < s.length; i++) {
            out[i] = s[i];
        }
        return out;
    }

    public static String trim(String s) {
        return s.substring(1,s.length()-2);
    }

    public static void log(String s) {
        (NativeAudio.av).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Constants.debugPane.setText(Constants.debugPane.getText()+s);
                scrollToBottom();
            }
        });
    }
    public static void clear() {
        (NativeAudio.av).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Constants.debugPane.setText("");
                scrollToBottom();
            }
        });
    }

    public static void scrollToBottom() {
        Constants.sview.post(new Runnable() {
            public void run() {
                Constants.sview.smoothScrollTo(0, Constants.debugPane.getBottom());
//                Log.e("bottom",Constants.debugPane.getBottom()+"");
            }
        });
    }

    public static boolean isInteger(String s) {
        try {
            Integer.parseInt(s);
        } catch(NumberFormatException e) {
            return false;
        } catch(NullPointerException e) {
            return false;
        }
        // only got here if we didn't return false
        return true;
    }
    public static boolean isFloat(String s) {
        try {
            Float.parseFloat(s);
        } catch(NumberFormatException e) {
            return false;
        } catch(NullPointerException e) {
            return false;
        }
        // only got here if we didn't return false
        return true;
    }

    public static short[] segment(short[] data, int i, int j) {
        short[] out = new short[j-i+1];
        int counter=0;
        for (int k = i; k <= j; k++) {
            out[counter++] = data[k];
        }
        return out;
    }

    public static double max(double[] sig) {
        double x = 0;
        for (int i = 0; i < sig.length; i++) {
            if (sig[i] > x) {
                x=sig[i];
            }
        }
        return x;
    }
}
