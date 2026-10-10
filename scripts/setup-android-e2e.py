#!/usr/bin/env python3

import argparse
import os
import re
import shutil
import subprocess
import tomllib
import uuid
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(
        description="Build the Android native E2E fixture APK"
    )
    parser.add_argument(
        "sentry_java",
        type=Path,
        help="sentry-java checkout with the native crash service",
    )
    parser.add_argument(
        "--build-dir", type=Path, default=PROJECT_DIR / "build/android-e2e"
    )
    args = parser.parse_args()
    java = args.sentry_java.expanduser().resolve()
    if not (java / "sentry-android-ndk-native").is_dir():
        parser.error(
            "sentry-java must contain the native crash service (feat/native-crash-service)"
        )
    for name in ["ANDROID_HOME", "ANDROID_NDK", "ANDROID_ARCH"]:
        if not os.environ.get(name):
            parser.error(f"{name} must be set")

    build_dir = args.build_dir.expanduser().resolve()
    app = build_dir / "app"
    shutil.copytree(PROJECT_DIR / "tests/fixtures/android_e2e", app, dirs_exist_ok=True)
    repo = build_dir / "maven"
    version = f"0.0.0-e2e-{uuid.uuid4().hex}"
    init = build_dir / "ndk.gradle"
    # override the downstream SDK's pinned NDK version without editing its catalog
    init.write_text(f"""
gradle.settingsEvaluated {{ settings ->
    settings.dependencyResolutionManagement.repositories {{
        maven {{ url = uri('{repo.as_uri()}') }}
    }}
}}
allprojects {{
    configurations.configureEach {{
        resolutionStrategy.force('io.sentry:sentry-native-ndk-native:{version}')
    }}
}}
""")

    def gradle(project, *args):
        command = [
            str(project / "gradlew"),
            "--console=plain",
            f"-PversionName={version}",
            f"-Dmaven.repo.local={repo}",
            *args,
        ]
        print("+ " + " ".join(command), flush=True)
        subprocess.run(command, cwd=project, check=True)

    gradle(
        PROJECT_DIR / "ndk",
        "-PPOM_ARTIFACT_ID=sentry-native-ndk-native",
        ":sentry-native-ndk-native:publishToMavenLocal",
    )
    gradle(
        java,
        "--init-script",
        str(init),
        ":sentry:publishToMavenLocal",
        ":sentry-android-core:publishToMavenLocal",
        ":sentry-android-ndk-native:publishToMavenLocal",
    )

    versions = tomllib.loads((java / "gradle/libs.versions.toml").read_text())[
        "versions"
    ]
    agp = re.search(
        r'id\("com.android.application"\) version "([^"]+)"',
        (PROJECT_DIR / "ndk/build.gradle.kts").read_text(),
    ).group(1)
    subprocess.run(
        [
            str(java / "gradlew"),
            "--console=plain",
            f"-PagpVersion={agp}",
            f"-PcompileSdk={versions['compileSdk']}",
            f"-PcompileSdkMinor={versions.get('compileSdkMinor', '0')}",
            f"-PtargetSdk={versions['targetSdk']}",
            f"-PsentryVersion={version}",
            f"-PlocalRepo={repo}",
            "assembleDebug",
        ],
        cwd=app,
        check=True,
    )
    print(f"Prepared APK: {app / 'build/outputs/apk/debug/app-debug.apk'}")


if __name__ == "__main__":
    main()
