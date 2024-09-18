package com.iofomo.opensrc.abyss.sdk;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;

import com.iofomo.opensrc.abyss.sdk.component.MTSTCProvider;

/**
 * tracee (应用业务所在进程)
 */
public class Nativee {
    private static final int ERRNO_MAX = 133;

    private static final int FILTERED_FUNC_OPEN = 0x1;
    private static final int FILTERED_FUNC_CLOSE = 0x2;

    static {
        System.loadLibrary("abyss");
    }

    /**
     *  初始化
     */
    private static native boolean before_attach();
    private static native boolean tracee_init_native(int flags);


    public static  void tracee_init(){
        tracee_init_native(FILTERED_FUNC_OPEN|FILTERED_FUNC_CLOSE);
    }

    public static  void tracee_init(int flags){
        tracee_init_native(flags);
    }

    /**
     *
     * @param appCtx
     * @return 0 成功,非0 失败
     */
    public static int attachMe(Context appCtx){
        Nativee.before_attach();
        Bundle extras = new Bundle();
        extras.putInt(MTSTCProvider.PARAMS_NEW_COMPONENT_PID, Process.myPid());
        int code = 0;
        Log.e("zzz","attachMe:" + "content://"+appCtx.getPackageName()+".component.MTSTCProvider/");
        Bundle ret = appCtx.getContentResolver().call(Uri.parse("content://"+appCtx.getPackageName()+".component.MTSTCProvider/"), MTSTCProvider.METHOD_ATTACH_NEW_COMPONENT,null,extras);
        if (ret != null){
            code = ret.getInt(MTSTCProvider.RET_PARAMS_CODE,-1);
            if (code == 0){
                Logger.d("trace success,pid:"+Process.myPid());
            }else{
                Logger.e("trace error,pid:"+Process.myPid()+",code:"+code);
            }
        }else {
            Logger.e("trace error bundle is null,pid:"+Process.myPid());
            return ERRNO_MAX + 1;
        }
        Logger.d("tracee init");
        return code;
    }

}
