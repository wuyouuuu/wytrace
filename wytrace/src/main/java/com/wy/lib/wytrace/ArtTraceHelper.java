package com.wy.lib.wytrace;

import android.content.Context;
import android.os.Build;
import android.os.Debug;

import java.io.File;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Paths;

/**
 * Description: Andorid 13 利用jvmti 切换解释执行模式
 * @author zhou.junyou
 * Create by:Android Studio
 * Date:2023/7/12
 */
public class ArtTraceHelper {
    public static void useExecuteSwitchImplAsm(Context context) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                ClassLoader classLoader = context.getClassLoader();
                Method findLibrary = ClassLoader.class.getDeclaredMethod("findLibrary", String.class);
                String jvmtiAgentLibPath = (String) findLibrary.invoke(classLoader, "wytrace");
                File filesDir = context.getFilesDir();
                File jvmtiLibDir = new File(filesDir, "jvmti");
                if (!jvmtiLibDir.exists()) {
                    jvmtiLibDir.mkdirs();

                }
                File agentLibSo = new File(jvmtiLibDir, "libwytrace.so");
                if (agentLibSo.exists()) {
                    agentLibSo.delete();
                }

                Files.copy(Paths.get(new File(jvmtiAgentLibPath).getAbsolutePath()), Paths.get((agentLibSo).getAbsolutePath()));
                Debug.attachJvmtiAgent(agentLibSo.getAbsolutePath(), null, classLoader);
                System.load(agentLibSo.getAbsolutePath());
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}
