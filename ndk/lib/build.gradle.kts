plugins {
    id("com.android.library")
    kotlin("android")
    jacoco
    id("com.mxalbert.gradle.jacoco-android")
    id("com.ydq.android.gradle.native-aar.export")
    id("com.github.ben-manes.versions")
}

var sentryNativeSrc: String = "${project.projectDir}/../.."

android {
    compileSdk = 34
    namespace = "io.sentry.ndk"

    defaultConfig {
        minSdk = 19 // NDK requires a higher API level than core.

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                arguments.add(0, "-DANDROID_STL=c++_static")
                arguments.add(0, "-DSENTRY_NATIVE_SRC=$sentryNativeSrc")
            }
        }

        ndk {
            abiFilters.addAll(listOf("x86", "armeabi-v7a", "x86_64", "arm64-v8a"))
        }
    }

    // we use the default NDK and CMake versions based on the AGP's version
    // https://developer.android.com/studio/projects/install-ndk#apply-specific-version

    externalNativeBuild {
        cmake {
            path("CMakeLists.txt")
        }
    }

    buildTypes {
        getByName("debug")
        getByName("release") {
            consumerProguardFiles("proguard-rules.pro")
        }
    }

    kotlinOptions {
        jvmTarget = JavaVersion.VERSION_1_8.toString()
    }

    testOptions {
        animationsDisabled = true
        unitTests.apply {
            isReturnDefaultValues = true
            isIncludeAndroidResources = true
        }
    }

    lint {
        warningsAsErrors = true
        checkDependencies = true

        // We run a full lint analysis as build part in CI, so skip vital checks for assemble tasks.
        checkReleaseBuilds = false
    }

    nativeBundleExport {
        headerDir = "${project.projectDir}/../include"
    }

    // needed because of Kotlin 1.4.x
    configurations.all {
        resolutionStrategy.force("org.jetbrains:annotations:23.0.0")
    }

    variantFilter {
        if (System.getenv("CI")?.toBoolean() ?: false && buildType.name == "debug") {
            ignore = true
        }
    }
}

dependencies {
    compileOnly("org.jetbrains:annotations:23.0.0")
}
