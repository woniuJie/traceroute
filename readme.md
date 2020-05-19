# TraceRoute

`Author:王竞` `Date:2018/8/23`

---

由于Android系统本身并没有traceroute的实现，本项目将[traceroute for linux](http://traceroute.sourceforge.net/)
移植到Android平台，通过jni方式可实现调用traceroute，也可以将traceroute for linux编译为可执行的文件，通过shell
调用traceroute。

编译过程遇到的问题：

1. 目前只能使用ndk编译，使用cmake编译不了
2. jni回调java，必须保证com.chuchujie.core.traceroute.TraceRoute中jni调用的方法不被混淆
3. exit()导致应用程序退出
4. 如何打印错误信息
5. C编译参数