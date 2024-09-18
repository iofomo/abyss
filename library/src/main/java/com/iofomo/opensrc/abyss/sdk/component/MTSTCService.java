package com.iofomo.opensrc.abyss.sdk.component;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;

import com.iofomo.opensrc.abyss.sdk.Logger;

public class MTSTCService extends Service {
    private static final String TAG = "PtService";

    public static void start(Context ctx){
        Intent intent = new Intent(ctx, MTSTCService.class);
        if (ctx.startService(intent) == null){
            Logger.e("startService error---");
        }
    }

    public MTSTCService() {
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Logger.d(TAG,"onCreate----");
    }

    @Override
    public IBinder onBind(Intent intent) {
        Logger.d(TAG,"onBind----");
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Logger.d(TAG,"onStartCommand----");
        return START_STICKY;
    }
}