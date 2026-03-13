using System.Runtime.InteropServices;
using Android.App;
using Android.OS;

// Required for "adb shell run-as" to access the app's data directory in Release builds
[assembly: Application(Debuggable = true)]

namespace dotnet_signal;

[Activity(Name = "dotnet_signal.MainActivity", MainLauncher = true)]
public class MainActivity : Activity
{
    [DllImport("libc", EntryPoint = "abort")]
    static extern void abort();

    protected override void OnResume()
    {
        base.OnResume();

        var arg = Intent?.GetStringExtra("arg");
        if (!string.IsNullOrEmpty(arg))
        {
            var databasePath = FilesDir?.AbsolutePath + "/.sentry-native";

            // Post the test to the main thread's message queue so it runs
            // after OnResume returns and the activity is fully started.
            new Handler(Looper.MainLooper!).Post(() =>
            {
                try
                {
                    Program.RunTest(new[] { arg }, databasePath);
                    FinishAndRemoveTask();
                    Java.Lang.JavaSystem.Exit(0);
                }
                catch (Exception e)
                {
                    // Emulate what MAUI does: call abort() for unhandled
                    // exceptions so that the native crash handler can
                    // capture them. Without this, the main thread's
                    // UncaughtExceptionHandler would kill with SIGKILL.
                    Console.Error.WriteLine(e);
                    abort();
                }
            });
        }
    }
}
