package com.iofomo.opensrc.abyss.sdk;

/**
 * tracer(ptrace所在进程)
 */
public class Native {
    static {
        System.loadLibrary("abyss");
    }
    public static native void init();
    public static native int trace_pid(int pid);
}
