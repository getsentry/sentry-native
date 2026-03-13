using Android.App;
using Android.OS;

// Required for "adb shell run-as" to access the app's data directory in Release builds
[assembly: Application(Debuggable = true)]

namespace dotnet_signal;

[Activity(Name = "dotnet_signal.MainActivity", MainLauncher = true)]
public class MainActivity : Activity
{
    protected override void OnResume()
    {
        base.OnResume();

        var arg = Intent?.GetStringExtra("arg");
        if (!string.IsNullOrEmpty(arg))
        {
            var databasePath = FilesDir?.AbsolutePath + "/.sentry-native";
            Program.RunTest(new[] { arg }, databasePath);
            FinishAndRemoveTask();
            Java.Lang.JavaSystem.Exit(0);
        }
    }
}
