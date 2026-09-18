plugins {
    id("com.android.library")
    kotlin("android")
    id("com.ydq.android.gradle.native-aar.export")
}

extra["POM_ARTIFACT_ID"] = project.name
