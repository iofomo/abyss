package com.iofomo.opensrc.abyss;

import android.app.ActivityManager;
import android.app.Application;
import android.content.Context;

import com.iofomo.opensrc.abyss.sdk.Logger;
import com.iofomo.opensrc.abyss.sdk.Nativee;

import java.util.List;

public class AbyssApplication extends Application {

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        Logger.setDefTag("INTERCEPT");
        Logger.e("build Type:" + BuildConfig.BUILD_TYPE);

        /**
         * ------------------ 拦截集成逻辑 START----
         */
        initSvcSdk();
        /**
         * ------------------ 拦截集成逻辑 END----
         */
    }

    public void initSvcSdk() {
        if (isTraceeProcess()) {
            int ret = Nativee.attachMe(this);
            Logger.d("attachMe ret:" + ret);
            if (ret != 0) {
                Logger.e("attach error");
                return;
            }
            Nativee.tracee_init();
            test();
        } else {
            Logger.d("not tracee process,ignore");
        }
    }

    private void test() {
//        String content = FileIOUtils.readFile2String(new File("/proc/self/status"));
//        Logger.d("status:"+content);
    }

    /**
     *  是否是tracee进程(被ptrace控制的进程)
     */
    boolean isTraceeProcess() {
        //demo中,非MyContentProvider所在的:process进程
       String name = getCurrentProcessName();
       Logger.d("cur process:"+name);
       if (name == null || !name.endsWith(":MTSTCProc")) {
           return true;
       }
       return false;
    }

    String getCurrentProcessName() {
        ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> runningProcesses = am.getRunningAppProcesses();
        for (ActivityManager.RunningAppProcessInfo info : runningProcesses) {
            if (info.pid == android.os.Process.myPid()) {
                String processName = info.processName;
                return processName;
            }
        }
        return "unknown";
    }
}
