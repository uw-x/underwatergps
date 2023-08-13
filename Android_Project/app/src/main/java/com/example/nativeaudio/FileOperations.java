package com.example.nativeaudio;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.LinkedList;

public class FileOperations {

    public static void writeSensorsToDisk(Activity av, String filename) {
        new Thread() {
            public void run() {
                try {
                    String dir = av.getExternalFilesDir(null).toString();
                    File path = new File(dir);
                    if (!path.exists()) {
                        path.mkdirs();
                    }

                    File file = new File(dir, filename+"/"+filename+"-acc.txt");
                    BufferedWriter outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.accx.size(); i++) {
                        outfile.append(Constants.time_acc.get(i)+","+Constants.accx.get(i)+","+Constants.accy.get(i)+","+Constants.accz.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();

//                    file = new File(dir, filename+"/"+filename+"-acc-uncalib.txt");
//                    outfile = new BufferedWriter(new FileWriter(file,false));
//                    for (int i = 0; i < Constants.accx_uncalib.size(); i++) {
//                        outfile.append(Constants.time_acc_uncalib.get(i)+","+Constants.accx_uncalib.get(i)+","+Constants.accy_uncalib.get(i)+","+Constants.accz_uncalib.get(i));
//                        outfile.newLine();
//                    }
//                    outfile.flush();
//                    outfile.close();

                    file = new File(dir, filename+"/"+filename+"-gyro.txt");
                    outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.gyrox.size(); i++) {
                        outfile.append(Constants.time_gyro.get(i)+","+Constants.gyrox.get(i)+","+Constants.gyroy.get(i)+","+Constants.gyroz.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();

//                    file = new File(dir, filename+"/"+filename+"-gyro-uncalib.txt");
//                    outfile = new BufferedWriter(new FileWriter(file,false));
//                    for (int i = 0; i < Constants.gyrox_uncalib.size(); i++) {
//                        outfile.append(Constants.time_gyro_uncalib.get(i)+","+Constants.gyrox_uncalib.get(i)+","+Constants.gyroy_uncalib.get(i)+","+Constants.gyroz_uncalib.get(i));
//                        outfile.newLine();
//                    }
//                    outfile.flush();
//                    outfile.close();

                    file = new File(dir, filename+"/"+filename+"-mag.txt");
                    outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.magx.size(); i++) {
                        outfile.append(Constants.time_mag.get(i)+","+Constants.magx.get(i)+","+Constants.magy.get(i)+","+Constants.magz.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();

                    file = new File(dir, filename+"/"+filename+"-mag-uncalib.txt");
                    outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.magx_uncalib.size(); i++) {
                        outfile.append(Constants.time_mag_uncalib.get(i)+","+Constants.magx_uncalib.get(i)+","+Constants.magy_uncalib.get(i)+","+Constants.magz_uncalib.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();

                    file = new File(dir, filename+"/"+filename+"-pressure.txt");
                    outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.pressure_data.size(); i++) {
                        outfile.append(Constants.time_pressure.get(i)+","+Constants.pressure_data.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();

                    file = new File(dir, filename+"/"+filename+"-linear_acc.txt");
                    outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.linearaccx.size(); i++) {
                        outfile.append(Constants.time_linear_acc.get(i)+","+Constants.linearaccx.get(i)+","+Constants.linearaccy.get(i)+","+Constants.linearaccz.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();

                    file = new File(dir, filename+"/"+filename+"-rot.txt");
                    outfile = new BufferedWriter(new FileWriter(file,false));
                    for (int i = 0; i < Constants.rotx.size(); i++) {
                        outfile.append(Constants.time_rot.get(i)+","+Constants.rotx.get(i)+","+Constants.roty.get(i)+","+Constants.rotz.get(i)+","+Constants.rot4.get(i));
                        outfile.newLine();
                    }
                    outfile.flush();
                    outfile.close();


                } catch(Exception e) {
                    Log.e("ex", "writeRecToDisk");
                    Log.e("ex", e.getMessage());
                }
            }
        }.run();
    }

    public static double[] readfromfile(Activity av, String filename) {
        LinkedList<Double> ll = new LinkedList<Double>();

        try {
            String dir = av.getExternalFilesDir(null).toString();
            File file = new File(dir + File.separator + filename);
            Log.e("asdf",file.getAbsolutePath());
            Log.e("asdf",file.exists()+"");
            BufferedReader buf = new BufferedReader(new FileReader(file));

            String line;
            while ((line = buf.readLine()) != null && line.length() != 0) {
//                Log.e("asdf",line);
                ll.add(Double.parseDouble(line));
            }

            buf.close();
        } catch (Exception e) {
            Log.e("ble",e.getMessage());
        }

        double[] ar = new double[ll.size()];
        int counter = 0;
        for (Double d : ll) {
            ar[counter++] = d;
        }
        ll.clear();
        return ar;
    }

    public static short[] readrawasset_binary(Context context, int id) {
        InputStream inp = context.getResources().openRawResource(id);
        ArrayList<Integer> ll = new ArrayList<>();
        int counter=0;
        int byteRead=0;
        try {
            while ((byteRead = inp.read()) != -1) {
                ll.add(byteRead);
                counter += 1;
//                if (counter % 1000 == 0) {
//                    Log.e("asdf", counter + "");
//                }
            }
            inp.close();
        }
        catch(Exception e) {
            Log.e("asdf",e.getMessage());
        }
        short[] ar = new short[ll.size()/2];

        counter=0;
        for (int i = 0; i < ll.size(); i+=2) {
            int out=ll.get(i)+ll.get(i+1)*256;
            if (out > 32767) {
                out=out-65536;
            }
            ar[counter++]=(short)out;
        }

        return ar;
    }

    public static void writetofile(Activity av, short[] buff, String filename) {
        new Thread() {
            public void run() {
                try {
                    String dir = av.getExternalFilesDir(null).toString();
                    writetofile(dir, buff, filename);
                } catch (Exception e) {
                    Log.e("asdf", e.toString());
                }
            }
        }.run();
    }

    public static void writetofile_str(Activity av, String buff, String filename) {
        new Thread() {
            public void run() {
                try {
                    String dir = av.getExternalFilesDir(null).toString();
                    writetofile_str(dir, buff, filename);
                } catch (Exception e) {
                    Log.e("asdf", e.toString());
                }
            }
        }.run();
    }

    public static void writetofile_str(String _ExternalFilesDir, String buff, String filename) {
        Log.e("asdf","writetofile " + filename + " "+(buff==null));
        long ts = System.currentTimeMillis();

        try {
            String dir = _ExternalFilesDir;
            File path = new File(dir);
            if (!path.exists()) {
                path.mkdirs();
            }

            File file = new File(dir, filename);

            BufferedWriter buf = new BufferedWriter(new FileWriter(file,false));
            buf.write(buff);
//            for (int i = 0; i < buff.length; i++) {
//                buf.append(""+buff[i]);
//                buf.newLine();
//            }
            buf.flush();
            buf.close();
        } catch(Exception e) {
            Log.e("asdf","write to file exception " +e.toString());
        }
        Log.e("asdf","finish writing "+filename + (System.currentTimeMillis()-ts));
    }

    public static void writetofile(String _ExternalFilesDir, short[] buff, String filename) {
        Log.e("asdf","writetofile " + filename + " "+(buff==null));
        long ts = System.currentTimeMillis();

        try {
            String dir = _ExternalFilesDir;
            File path = new File(dir);
            if (!path.exists()) {
                path.mkdirs();
            }

            File file = new File(dir, filename);

            BufferedWriter buf = new BufferedWriter(new FileWriter(file,false));
            for (int i = 0; i < buff.length; i++) {
                buf.append(""+buff[i]);
                buf.newLine();
            }
            buf.flush();
            buf.close();
        } catch(Exception e) {
            Log.e("asdf","write to file exception " +e.toString());
        }
        Log.e("asdf","finish writing "+filename + (System.currentTimeMillis()-ts));
    }
}
