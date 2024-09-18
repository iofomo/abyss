package com.iofomo.opensrc.abyss.sdk;

import android.util.Log;

public class Logger {
    private static  String sDefTag = "undefine";

    public static final void setDefTag(String defTag){
        sDefTag = defTag;
    }


    public static void d(String TAG,String message){
        Log.e(TAG,message);
    }

    public static void e(String TAG,String message){
        Log.e(TAG,message);
    }

    public static void d(String message){
        Log.e(sDefTag,message);
    }

    public static void e(String message){
        Log.e(sDefTag,message);
    }
}
