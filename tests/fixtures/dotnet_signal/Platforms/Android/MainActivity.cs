using Android.App;
using Android.OS;

// Required for "adb shell run-as" to access the app's data directory in Release builds
[assembly: Application(Debuggable = true)]

namespace dotnet_signal;

[Activity(Name = "dotnet_signal.MainActivity", MainLauncher = true)]
public class MainActivity : Activity
{
    protected override void OnCreate(Bundle? savedInstanceState)
    {
        base.OnCreate(savedInstanceState);

        var arg = Intent?.GetStringExtra("arg");
        if (!string.IsNullOrEmpty(arg))
        {
            var databasePath = FilesDir?.AbsolutePath + "/.sentry-native";

            new Thread(() =>
            {
                Program.RunTest(new[] { arg }, databasePath);
                RunOnUiThread(() =>
                {
                    FinishAndRemoveTask();
                    Java.Lang.JavaSystem.Exit(0);
                });
            }).Start();
        }
    }
}
