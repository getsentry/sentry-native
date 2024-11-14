namespace dotnet_signal;

using System;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("crash", EntryPoint = "native_crash")]
    static extern void native_crash();

    [DllImport("sentry", EntryPoint = "sentry_options_new")]
    static extern IntPtr sentry_options_new();

    [DllImport("sentry", EntryPoint = "sentry_options_set_handler_strategy")]
    static extern IntPtr sentry_options_set_handler_strategy(IntPtr options, int strategy);

    [DllImport("sentry", EntryPoint = "sentry_options_set_debug")]
    static extern IntPtr sentry_options_set_debug(IntPtr options, int debug);

    [DllImport("sentry", EntryPoint = "sentry_init")]
    static extern int sentry_init(IntPtr options);

    static void Main(string[] args)
    {
        // setup minimal sentry-native
        var options = sentry_options_new();
        sentry_options_set_handler_strategy(options, 1);
        sentry_options_set_debug(options, 1);
        sentry_init(options);

        var doNativeCrash = args is ["native-crash"];
        if (doNativeCrash)
        {
            native_crash();
        }
        else
        {
            try
            {
                Console.WriteLine("dereference a NULL object from managed code");
                var s = default(string);
                var c = s.Length;
            }
            catch (NullReferenceException exception)
            {
                Console.WriteLine("dereference another NULL object from managed code");
                var s = default(string);
                var c = s.Length;
            }
        }
    }
}