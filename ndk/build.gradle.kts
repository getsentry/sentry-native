import com.diffplug.gradle.spotless.SpotlessPlugin
import com.diffplug.spotless.LineEnding
import com.vanniktech.maven.publish.MavenPublishBaseExtension
import com.vanniktech.maven.publish.MavenPublishPlugin
import groovy.util.Node
import io.gitlab.arturbosch.detekt.extensions.DetektExtension
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.gradle.api.tasks.testing.logging.TestLogEvent

plugins {
    `java-library`
    id("com.android.application") version "9.4.0" apply false
    id("com.android.library") version "9.4.0" apply false
    id("com.diffplug.spotless") version "8.8.0" apply true
    id("io.gitlab.arturbosch.detekt") version "1.23.8"
    id("com.vanniktech.maven.publish") version "0.30.0" apply false
    id("net.ltgt.errorprone") version "3.0.1" apply false
    // dokka is required by gradle-maven-publish-plugin.
    id("org.jetbrains.dokka") version "2.0.0" apply false
    kotlin("android") version "2.3.21" apply false
    `maven-publish`
    id("org.jetbrains.kotlinx.binary-compatibility-validator") version "0.13.0"
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
    // keep Java and Kotlin bytecode compatible with existing consumers
    val javaVersion = JavaVersion.VERSION_1_8
    plugins.withId("com.android.base") {
        configure<com.android.build.gradle.BaseExtension> {
            compileOptions {
                sourceCompatibility = javaVersion
                targetCompatibility = javaVersion
            }
        }
    }
    plugins.withId("org.jetbrains.kotlin.android") {
        configure<org.jetbrains.kotlin.gradle.dsl.KotlinAndroidProjectExtension> {
            compilerOptions {
                jvmTarget =
                    org.jetbrains.kotlin.gradle.dsl.JvmTarget
                        .fromTarget(javaVersion.toString())
            }
        }
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
        apply<MavenPublishPlugin>()

        @Suppress("UnstableApiUsage")
        configure<MavenPublishBaseExtension> {
            assignAarTypes()
        }

        configure<DistributionContainer> {
            getByName("main").contents {
                // non android modules
                from("build/libs")
                from("build/publications/maven")
                // android modules
                from("build/outputs/aar") {
                    include("*-release*")
                }
                from("build/publications/release")
                from("build/intermediates/java_doc_jar/release") {
                    include("*javadoc*")
                    rename { it.replace("release", "${project.name}-${project.version}") }
                }
                from("build/intermediates/source_jar/release") {
                    include("*sources*")
                    rename { it.replace("release", "${project.name}-${project.version}") }
                }
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
                .dir("distributions")
                .map { it.file("${project.name}-${project.version}.zip") }

        tasks.named("distZip").configure {
            dependsOn("publishToMavenLocal")
            doLast {
                val distZip = distZipProvider.get().asFile
                require(distZip.exists()) { "Distribution file does not exist: ${distZip.absolutePath}" }
                require(distZip.length() > 0L) { "Distribution file is empty: ${distZip.absolutePath}" }
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
    ignoredProjects.addAll(listOf("sample"))
}
