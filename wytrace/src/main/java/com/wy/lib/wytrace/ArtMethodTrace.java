package com.wy.lib.wytrace;

import com.bytedance.shadowhook.ShadowHook;

/**
 * Description:
 *
 * @author zhou.junyou
 * Create by:Android Studio
 * Date:2022/9/22
 */
public class ArtMethodTrace {
    static {
        System.loadLibrary("wytrace");
    }

    /**
     *
     * @param methodName 方法名
     * @param tid   需要抓取trace线程tid   -1表示抓取全部线程
     * @param depth 方法内部抓取层级
     * @param debug 是否打印方法耗时日志
     */

    public static void methodHookStart(String methodName, int tid, int depth, boolean debug) {
        ShadowHook.init(new ShadowHook.ConfigBuilder()
                .setMode(ShadowHook.Mode.UNIQUE)
                .build());
        methodHook(methodName, tid, depth, debug);
    }

    private static native void methodHook(String methodName, int tid, int depth, boolean debug);

    public static native void methodUnHook();
}
