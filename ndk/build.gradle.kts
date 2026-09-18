import com.android.build.gradle.LibraryExtension
import com.diffplug.gradle.spotless.SpotlessPlugin
import com.diffplug.spotless.LineEnding
import com.vanniktech.maven.publish.MavenPublishBaseExtension
import com.vanniktech.maven.publish.MavenPublishPlugin
import com.vanniktech.maven.publish.MavenPublishPluginExtension
import com.yy.android.gradle.nativedepend.NativeBundleExportExtension
import groovy.util.Node
import io.gitlab.arturbosch.detekt.extensions.DetektExtension
import org.gradle.api.plugins.ExtensionAware
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.gradle.api.tasks.testing.logging.TestLogEvent
import org.jetbrains.kotlin.gradle.dsl.KotlinJvmOptions

plugins {
    `java-library`
    id("com.diffplug.spotless") version "6.25.0" apply true
    id("io.gitlab.arturbosch.detekt") version "1.19.0"
    `maven-publish`
    id("org.jetbrains.kotlinx.binary-compatibility-validator") version "0.13.0"
}

buildscript {
    repositories {
        google()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:8.7.3")
        classpath(kotlin("gradle-plugin", version = "1.8.0"))
        classpath("com.vanniktech:gradle-maven-publish-plugin:0.18.0")
        // dokka is required by gradle-maven-publish-plugin.
        classpath("org.jetbrains.dokka:dokka-gradle-plugin:1.7.10")
        classpath("net.ltgt.gradle:gradle-errorprone-plugin:3.0.1")

        // legacy pre-prefab support
        // https://github.com/howardpang/androidNativeBundle
        classpath("io.github.howardpang:androidNativeBundle:1.1.4")
    }
}

allprojects {
    repositories {
        google()
        mavenCentral()
    }
    group = "io.sentry"
    version = properties["versionName"].toString()
    description = "SDK for sentry.io"
    tasks {
        withType<Test> {
            testLogging.showStandardStreams = true
            testLogging.exceptionFormat = TestExceptionFormat.FULL
            testLogging.events =
                setOf(
                    TestLogEvent.SKIPPED,
                    TestLogEvent.PASSED,
                    TestLogEvent.FAILED,
                )
            maxParallelForks = Runtime.getRuntime().availableProcessors() / 2

            // Cap JVM args per test
            minHeapSize = "128m"
            maxHeapSize = "1g"
            dependsOn("cleanTest")
        }
        withType<JavaCompile> {
            options.compilerArgs.addAll(arrayOf("-Xlint:all", "-Werror", "-Xlint:-classfile", "-Xlint:-processing", "-Xlint:-options"))
        }
    }
}

subprojects {
    plugins.withId("com.ydq.android.gradle.native-aar.export") {
        configureNdkLibrary()
    }

    plugins.withId("io.gitlab.arturbosch.detekt") {
        configure<DetektExtension> {
            buildUponDefaultConfig = true
            allRules = true
            config.setFrom("${rootProject.rootDir}/detekt.yml")
        }
    }

    if (!name.contains("sample")) {
        apply<DistributionPlugin>()

        val sep = File.separator

        configure<DistributionContainer> {
            getByName("main").contents {
                // non android modules
                from("build${sep}libs")
                from("build${sep}publications${sep}maven")
                // android modules
                from("build${sep}outputs${sep}aar") {
                    include("*-release*")
                }
                from("build${sep}publications${sep}release")
            }

            // craft only uses zip archives
            forEach { dist ->
                if (dist.name == DistributionPlugin.MAIN_DISTRIBUTION_NAME) {
                    tasks.getByName("distTar").enabled = false
                } else {
                    tasks.getByName(dist.name + "DistTar").enabled = false
                }
            }
        }

        val distZipProvider =
            project.layout.buildDirectory
                .dir("distributions").map { it.file("${project.name}-${project.version}.zip") }

        tasks.named("distZip").configure {
            dependsOn("publishToMavenLocal")
            doLast {
                val distZip = distZipProvider.get().asFile
                require(distZip.exists()) { "Distribution file does not exist: ${distZip.absolutePath}" }
                require(distZip.length() > 0L) { "Distribution file is empty: ${distZip.absolutePath}" }
            }
        }

        afterEvaluate {
            apply<MavenPublishPlugin>()

            configure<MavenPublishPluginExtension> {
                // signing is done when uploading files to MC
                // via gpg:sign-and-deploy-file (release.kts)
                releaseSigningEnabled = false
            }

            @Suppress("UnstableApiUsage")
            configure<MavenPublishBaseExtension> {
                assignAarTypes()
            }
        }
    }

    apply<SpotlessPlugin>()
}

spotless {
    lineEndings = LineEnding.UNIX
    java {
        target("**/*.java")
        removeUnusedImports()
        googleJavaFormat()
        targetExclude("**/generated/**", "**/vendor/**")
    }
    kotlin {
        target("**/*.kt")
        targetExclude("**/generated/**")
        ktlint()
    }
    kotlinGradle {
        target("**/*.kts")
        targetExclude("**/generated/**")
        ktlint()
    }
}

private val androidLibs =
    setOf(
        "lib",
    )

private val androidXLibs =
    listOf(
        "androidx.core:core",
    )

/*
 * Adapted from https://github.com/androidx/androidx/blob/c799cba927a71f01ea6b421a8f83c181682633fb/buildSrc/private/src/main/kotlin/androidx/build/MavenUploadHelper.kt#L524-L549
 *
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Workaround for https://github.com/gradle/gradle/issues/3170
@Suppress("UnstableApiUsage")
fun MavenPublishBaseExtension.assignAarTypes() {
    pom {
        withXml {
            val dependencies =
                asNode().children().find {
                    it is Node && it.name().toString().endsWith("dependencies")
                } as Node?

            dependencies?.children()?.forEach { dep ->
                if (dep !is Node) {
                    return@forEach
                }
                val group =
                    dep.children().firstOrNull {
                        it is Node && it.name().toString().endsWith("groupId")
                    } as? Node
                val groupValue = group?.children()?.firstOrNull() as? String

                val artifactId =
                    dep.children().firstOrNull {
                        it is Node && it.name().toString().endsWith("artifactId")
                    } as? Node
                val artifactIdValue = artifactId?.children()?.firstOrNull() as? String

                if (artifactIdValue in androidLibs) {
                    dep.appendNode("type", "aar")
                } else if ("$groupValue:$artifactIdValue" in androidXLibs) {
                    dep.appendNode("type", "aar")
                }
            }
        }
    }
}

apiValidation {
    ignoredProjects.addAll(listOf("sample", "sentry-native-ndk-native"))
}

fun Project.configureNdkLibrary() {
    var sentryNativeSrc: String = "${project.projectDir}/../.."
    val nativeBackend = project.name == "sentry-native-ndk-native"
    val sentryBackend = if (nativeBackend) "native" else providers.gradleProperty("sentryBackend").orElse("inproc").get()
    val libDir = rootProject.file("lib")
    val nativeTransport = "io.github.vvb2060.ndk:curl:8.18.0"
    val sanitizer =
        System.getenv("RUN_ANALYZER").orEmpty().split(',')
            .firstOrNull { it == "asan" || it == "tsan" }

    configure<LibraryExtension> {
        compileSdk = 35
        namespace = "io.sentry.ndk"

        testBuildType = "debug"

        defaultConfig {
            minSdk = 21

            externalNativeBuild {
                cmake {
                    arguments.add(0, "-DSENTRY_BACKEND=$sentryBackend")
                    if (sentryBackend == "inproc") {
                        arguments.add(0, "-DSENTRY_TRANSPORT=none")
                    }
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

        if (nativeBackend) {
            sourceSets.all { setRoot("$libDir/src/$name") }
        }

        publishing {
            singleVariant("release")
        }

        if (sanitizer != null) {
            System.getenv("ANDROID_NDK")?.let { ndkVersion = it }
            sourceSets.getByName("androidTest") {
                jniLibs.srcDir("$libDir/build/$sanitizer/jniLibs")
                resources.srcDir("$libDir/build/$sanitizer/resources")
            }
        }

        // we use the default NDK and CMake versions based on the AGP's version
        // https://developer.android.com/studio/projects/install-ndk#apply-specific-version
        externalNativeBuild {
            cmake {
                path("$libDir/CMakeLists.txt")
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
                consumerProguardFiles("$libDir/proguard-rules.pro")
            }
        }

        buildFeatures {
            prefab = true
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

        // legacy pre-prefab support
        // https://github.com/howardpang/androidNativeBundle
        // creates
        // lib.aar/jni/<arch>/<lib>.so
        // lib.aar/jni/include/sentry.h
        project.extensions.configure<NativeBundleExportExtension> {
            headerDir = "../../include"
        }

        (this as ExtensionAware).extensions.configure<KotlinJvmOptions> {
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
            checkReleaseBuilds = true
        }

        packaging {
            jniLibs {
                useLegacyPackaging = true
            }
        }
    }

    dependencies {
        // TODO: this was the first match on maven central..
        if (sentryBackend == "native") {
            "implementation"(nativeTransport)
        }

        "compileOnly"("org.jetbrains:annotations:23.0.0")

        "testImplementation"("androidx.test.ext:junit:1.3.0")

        "androidTestImplementation"("androidx.test:runner:1.7.0")
        "androidTestImplementation"("androidx.test.ext:junit:1.3.0")
        "androidTestImplementation"("androidx.test:rules:1.7.0")
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
        configurations.matching {
            it.name.startsWith("releaseVariant") && it.name.endsWith("Publication")
        }.configureEach {
            outgoing.capability("$group:${project.name}:$version")
            outgoing.capability("$group:sentry-ndk-backend:$version")
        }

        tasks.matching { it.name.startsWith("prefab") && it.name.endsWith("Package") }.configureEach {
            doLast {
                project.fileTree("build/intermediates/") {
                    include("**/abi.json")
                    include("**/prefab.json")
                    include("**/prefab_publication.json/*")
                }.forEach { file ->
                    val contents = file.readText().replace("c++_static", "none")
                    file.writeText(
                        if (nativeBackend) {
                            contents.replace("\"name\": \"${project.name}\"", "\"name\": \"sentry-native-ndk\"")
                                .replace("\"packageName\": \"${project.name}\"", "\"packageName\": \"sentry-native-ndk\"")
                        } else {
                            contents
                        },
                    )
                }
            }
        }
    }
}
