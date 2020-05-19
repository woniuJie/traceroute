package com.chuchujie.core.traceroute;

import android.os.Handler;
import android.os.Looper;

/**
 * Created by wangjing on 2018/8/19.
 */
public class TraceRoute {

    private StringBuilder result;

    private TraceRouteCallback mCallback;

    private Handler mHandler;

    static {
        System.loadLibrary("trace-route");
    }

    private TraceRoute() {
    }

    public static TraceRoute getInstance() {
        return Holder.INSTANCE;
    }

    private static class Holder {
        static final TraceRoute INSTANCE = new TraceRoute();
    }

    public void setCallback(TraceRouteCallback callback) {
        mCallback = callback;
    }

    private Handler getHandler() {
        if (mHandler == null) {
            mHandler = new Handler(Looper.getMainLooper());
        }
        return mHandler;
    }

    /**
     * 清除以前的trace route结果
     */
    void clearResult() {
        result = null;
    }

    /**
     * jni回调结果
     *
     * @param text
     */
    void appendResult(final String text) {
        if (result == null) {
            result = new StringBuilder();
        }
        result.append(text);
        if (mCallback != null) {
            getHandler().post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onUpdate(text);
                }
            });
        }
    }

    public void traceRouteAsync(final String hostname) {
        String[] args = new String[2];
        args[0] = "traceroute";
        args[1] = hostname;
        traceRouteAsync(args);
    }

    public synchronized void traceRouteAsync(final String[] args) {
        new Thread("trace_route_thread") {
            @Override
            public void run() {
                super.run();
                traceRoute(args);
            }
        }.start();
    }

    public TraceRouteResult traceRoute(String hostname) {
        String[] args = new String[2];
        args[0] = "traceroute";
        args[1] = hostname;
        return traceRoute(args);
    }

    public synchronized TraceRouteResult traceRoute(String[] args) {
        int code = execute(args);
        final TraceRouteResult traceRouteResult = new TraceRouteResult();
        traceRouteResult.setCode(code);
        if (code == 0) {
            traceRouteResult.setResult(result.toString());
            if (mCallback != null) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        mCallback.onSuccess(traceRouteResult);
                    }
                });
            }
        } else {
            traceRouteResult.setResult("execute traceroute failed.");
            if (mCallback != null) {
                getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        mCallback.onFailed(traceRouteResult.getCode(),
                                traceRouteResult.getResult());
                    }
                });
            }
        }
        return traceRouteResult;
    }

    native int execute(String[] args);

}
