package com.iofomo.opensrc.abyss.sdk.component;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;

import com.iofomo.opensrc.abyss.sdk.Logger;
import com.iofomo.opensrc.abyss.sdk.Native;

public class MTSTCProvider extends ContentProvider {
    private static final String TAG = MTSTCProvider.class.getSimpleName();


    public static final String METHOD_ATTACH_NEW_COMPONENT = "1";
    public static final String PARAMS_NEW_COMPONENT_PID = "params_pid";
    public static final String RET_PARAMS_CODE = "ret_params_code";
    public MTSTCProvider() {

    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        // Implement this to handle requests to delete one or more rows.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public String getType(Uri uri) {
        // at the given URI.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public boolean onCreate() {
        MTSTCService.start(getContext());
        Native.init();
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
                      String[] selectionArgs) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public Bundle call(String method, String arg, Bundle extras) {
        if (METHOD_ATTACH_NEW_COMPONENT.equals(method)){
            int pid = extras.getInt(PARAMS_NEW_COMPONENT_PID,-1);
            if (pid == -1) return null;
//            Logger.d(TAG,"waitForDebugger --------------------");
//            Debug.waitForDebugger();
            Logger.e(TAG,"pid:"+pid);

            int retCode = Native.trace_pid(pid);
            Logger.d(TAG,"retCode:"+retCode);
            Bundle ret = new Bundle();
            ret.putInt(RET_PARAMS_CODE,retCode);
            return ret;
        }
        return super.call(method, arg, extras);
    }


}