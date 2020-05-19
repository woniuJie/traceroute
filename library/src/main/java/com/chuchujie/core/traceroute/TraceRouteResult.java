package com.chuchujie.core.traceroute;

/**
 * Created by wangjing on 2018/8/21.
 */
public class TraceRouteResult {

    private int code;

    private String result;

    public TraceRouteResult() {
    }

    public TraceRouteResult(int code, String result) {
        this.code = code;
        this.result = result;
    }

    public int getCode() {
        return code;
    }

    public void setCode(int code) {
        this.code = code;
    }

    public String getResult() {
        return result;
    }

    public void setResult(String result) {
        this.result = result;
    }

    public boolean isSuccess() {
        return code == 0;
    }

    @Override
    public String toString() {
        return "TraceRouteResult{" +
                "code=" + code +
                ", result='" + result + '\'' +
                '}';
    }
}
