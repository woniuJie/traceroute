package com.chuchujie.core.traceroute.traceroute_for_android

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.support.v7.widget.AppCompatButton
import android.support.v7.widget.AppCompatEditText
import android.support.v7.widget.AppCompatTextView
import com.chuchujie.core.traceroute.TraceRoute
import com.chuchujie.core.traceroute.TraceRouteCallback
import com.chuchujie.core.traceroute.TraceRouteResult

class MainActivity : AppCompatActivity() {

    lateinit var text: AppCompatEditText

    lateinit var result: AppCompatTextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

//        val argcs = arrayOf("traceroute","--help")
//        val traceRoute = TraceRoute.getInstance().traceRoute(argcs)

//        val traceRoute = TraceRoute.getInstance().traceRoute("api.chuchujie.com")
//        Log.e("xixihaha", traceRoute.toString())


        text = findViewById(R.id.sample_text)
        result = findViewById(R.id.result)

        findViewById<AppCompatButton>(R.id.sample_button).setOnClickListener {
            doTraceRoute()
        }

    }

    private fun doTraceRoute() {

        TraceRoute.getInstance().setCallback(object : TraceRouteCallback {
            override fun onSuccess(traceRouteResult: TraceRouteResult?) {
                result.append("\ntraceroute finish")
            }

            override fun onUpdate(text: String?) {
                result.append(text)
            }

            override fun onFailed(code: Int, reason: String?) {
                result.append("\ntraceroute failed")
            }

        })

        TraceRoute.getInstance().traceRouteAsync(text.text.toString())

    }

}
