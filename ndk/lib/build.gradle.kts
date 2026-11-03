import com.android.build.gradle.internal.lint.AndroidLintAnalysisTask
import org.objectweb.asm.ClassReader
import org.objectweb.asm.ClassVisitor
import org.objectweb.asm.ClassWriter
import org.objectweb.asm.MethodVisitor
import org.objectweb.asm.Opcodes

plugins {
    id("com.android.library")
    kotlin("android")
}

var sentryNativeSrc: String = "${project.projectDir}/../.."
val sanitizer =
    System
        .getenv("RUN_ANALYZER")
        .orEmpty()
        .split(',')
        .firstOrNull { it == "asan" || it == "tsan" }

android {
    compileSdk = 37
    // retain AGP 8.7.3's default NDK to avoid changing the compiler and libc++ in a hotfix
    ndkVersion = System.getenv("ANDROID_NDK")?.let { File(it).name } ?: "27.0.12077973"
    namespace = "io.sentry.ndk"

    testBuildType = "debug"

    defaultConfig {
        minSdk = 21

        aarMetadata {
            // avoid requiring consumers to compile against API 37
            minCompileSdk = 1
        }

        externalNativeBuild {
            cmake {
                arguments.add(0, "-DANDROID_STL=${if (sanitizer != null) "c++_shared" else "c++_static"}")
                arguments.add(0, "-DSENTRY_NATIVE_SRC=$sentryNativeSrc")
                if (sanitizer == "asan") {
                    arguments.add(0, "-DWITH_ASAN_OPTION=ON")
                }
                if (sanitizer == "tsan") {
                    arguments.add(0, "-DWITH_TSAN_OPTION=ON")
                }
            }
        }

        ndk {
            abiFilters.addAll(
                if (sanitizer == "tsan") {
                    listOf("x86_64", "arm64-v8a")
                } else {
                    listOf("x86", "armeabi-v7a", "x86_64", "arm64-v8a")
                },
            )
        }

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    if (sanitizer != null) {
        sourceSets {
            getByName("androidTest") {
                jniLibs.srcDir("build/$sanitizer/jniLibs")
                resources.srcDir("build/$sanitizer/resources")
            }
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
        getByName("debug") {
            externalNativeBuild {
                cmake {
                    arguments.add(0, "-DENABLE_TESTS=ON")
                }
            }
        }
        getByName("release") {
            consumerProguardFiles("proguard-rules.pro")
        }
    }

    buildFeatures {
        prefabPublishing = true
        buildConfig = true
    }

    // creates
    // lib.aar/prefab/modules/sentry-android/libs/<arch>/<lib>.so
    // lib.aar/prefab/modules/sentry-android/include/sentry.h
    prefab {
        create("sentry-android") {}
        create("sentry") {
            headers = "../../include"
        }
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
        checkReleaseBuilds = true
        disable.addAll(listOf("NewerVersionAvailable", "GradleDependency", "AndroidGradlePluginVersion"))
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}

// legacy pre-prefab support
// creates lib.aar/jni/include/sentry.h alongside AGP's lib.aar/jni/<arch>/<lib>.so
tasks.withType<Zip>().configureEach {
    if (name.startsWith("bundle") && name.endsWith("Aar")) {
        from("../../include") {
            into("jni/include")
        }
    }
}

dependencies {
    compileOnly("org.jetbrains:annotations:23.0.0")

    testImplementation("androidx.test.ext:junit:1.3.0")

    androidTestImplementation("androidx.test:runner:1.7.0")
    androidTestImplementation("androidx.test.ext:junit:1.3.0")
    androidTestImplementation("androidx.test:rules:1.7.0")
}

/*
 * Prefab doesn't support c++_static, so we need to change it to none.
 * This should be fine, as we don't expose any conflicting symbols.
 * Based on: https://github.com/bugsnag/bugsnag-android/blob/59460018551750dfcce4fd4e9f612eae7826559e/bugsnag-plugin-android-ndk/build.gradle.kts
 *
 * Copyright (c) 2012 Bugsnag

 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
afterEvaluate {
    tasks.getByName("prefabReleasePackage") {
        doLast {
            project
                .fileTree("build/intermediates/prefab_package/") {
                    include("**/abi.json")
                }.forEach { file ->
                    file.writeText(file.readText().replace("c++_static", "none"))
                }
        }
    }
}

// always rerun lint analysis
// cached results can hide errors when dependency metadata changes
tasks.withType<AndroidLintAnalysisTask>().configureEach {
    doNotTrackState("Dependency metadata can change without code changes")
}

// keep the classes dexable by D8/R8 of AGP 8.0 and older when compiled with JDK 21+
tasks.withType<JavaCompile>().configureEach {
    doLast(StripMethodParameters())
}

/**
 * Removes the MethodParameters attribute from methods that have a parameter without a name.
 *
 * Since JDK 21 (JDK-8292275) javac writes MethodParameters for synthetic or mandated parameters,
 * e.g. of enums, even without `-parameters`. Those entries have no name, and the D8/R8 of AGP 8.0
 * and older crash on them. Same as getsentry/sentry-java#6356.
 */
class StripMethodParameters : Action<Task> {
    override fun execute(task: Task) {
        (task as JavaCompile)
            .destinationDirectory
            .get()
            .asFile
            .walkTopDown()
            .filter { it.isFile && it.extension == "class" }
            .forEach { file ->
                val reader = ClassReader(file.readBytes())
                val methods = reader.methodsWithUnnamedParameters()
                if (methods.isNotEmpty()) {
                    val writer = ClassWriter(reader, 0)
                    reader.accept(
                        object : ClassVisitor(Opcodes.ASM9, writer) {
                            override fun visitMethod(
                                access: Int,
                                name: String?,
                                descriptor: String?,
                                signature: String?,
                                exceptions: Array<out String>?,
                            ): MethodVisitor? {
                                val next = super.visitMethod(access, name, descriptor, signature, exceptions)
                                if (name + descriptor !in methods) {
                                    return next
                                }
                                return object : MethodVisitor(Opcodes.ASM9, next) {
                                    override fun visitParameter(
                                        name: String?,
                                        access: Int,
                                    ) = Unit
                                }
                            }
                        },
                        0,
                    )
                    file.writeBytes(writer.toByteArray())
                }
            }
    }

    private fun ClassReader.methodsWithUnnamedParameters(): Set<String> {
        val methods = mutableSetOf<String>()
        accept(
            object : ClassVisitor(Opcodes.ASM9) {
                override fun visitMethod(
                    access: Int,
                    name: String?,
                    descriptor: String?,
                    signature: String?,
                    exceptions: Array<out String>?,
                ): MethodVisitor =
                    object : MethodVisitor(Opcodes.ASM9) {
                        override fun visitParameter(
                            parameterName: String?,
                            access: Int,
                        ) {
                            if (parameterName == null) {
                                methods.add(name + descriptor)
                            }
                        }
                    }
            },
            // no SKIP_DEBUG, ASM counts MethodParameters as debug info
            ClassReader.SKIP_CODE,
        )
        return methods
    }
}
