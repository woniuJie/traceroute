package com.chuchujie.core.traceroute;

/**
 * Created by wangjing on 2018/8/21.
 */
public interface TraceRouteCallback {

    void onSuccess(TraceRouteResult traceRouteResult);

    void onUpdate(String text);

    void onFailed(int code, String reason);

}
