plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("com.google.devtools.ksp") version "1.9.22-1.0.18"  // P0.6: Updated for Kotlin 1.9.22 compatibility
    id("org.jetbrains.kotlin.plugin.serialization") version "1.9.22"  // M8: Kotlin Serialization for JSON (fixed version match)
    id("io.gitlab.arturbosch.detekt")
}

detekt {
    buildUponDefaultConfig = true
    allRules = false
    config.setFrom(files("$rootDir/config/detekt/detekt.yml"))
    baseline = file("$projectDir/detekt-baseline.xml")
}

android {
    namespace = "com.tribex.groovebox"
    compileSdk = 34
    ndkVersion = "26.1.10909125"

    defaultConfig {
        applicationId = "com.tribex.groovebox"
        minSdk = 26  // M4.5: Updated from 24 to 26 for AAudio support (99%+ device coverage)
        targetSdk = 34
        versionCode = 1
        versionName = "0.2.0"  // M4.5: Version bump

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters.addAll(listOf("arm64-v8a"))
        }

        externalNativeBuild {
            cmake {
                cppFlags("-std=c++17")
                arguments("-DANDROID_STL=c++_shared")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    kotlinOptions {
        jvmTarget = "1.8"
    }

    buildFeatures {
        compose = true
        prefab = true
    }
    
    // Room schema export location (M8)
    ksp {
        arg("room.schemaLocation", "$projectDir/schemas")
    }

    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.10"  // M6: Updated for Kotlin 1.9.22 compatibility
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    // Core Android
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.7.0")  // M4.5: Updated
    implementation("androidx.lifecycle:lifecycle-process:2.7.0")  // M8: For ProcessLifecycleOwner
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.activity:activity-compose:1.8.2")  // M4.5: Updated

    // Compose
    implementation(platform("androidx.compose:compose-bom:2024.02.01"))  // M4.5: Updated from 2023.10.01
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")

    // Oboe Audio Library
    implementation("com.google.oboe:oboe:1.8.0")

    // Room Persistence (M8)
    implementation("androidx.room:room-runtime:2.6.1")
    implementation("androidx.room:room-ktx:2.6.1")
    ksp("androidx.room:room-compiler:2.6.1")
    
    // Kotlin Serialization (M8)
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.2")

    // Testing
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.7.3")
    testImplementation("androidx.arch.core:core-testing:2.2.0")
    testImplementation("org.mockito:mockito-core:5.11.0")
    testImplementation("org.mockito.kotlin:mockito-kotlin:5.3.1")
    
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    androidTestImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.7.3")
    androidTestImplementation(platform("androidx.compose:compose-bom:2023.10.01"))
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")

    // Debug
    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}

val nativeTestBuildDir = layout.buildDirectory.dir("native-tests")

tasks.register("nativeTest") {
    group = "verification"
    description = "Build and run native Sequencer tests via CMake/CTest."
    doLast {
        val buildDirFile = nativeTestBuildDir.get().asFile
        project.exec {
            commandLine("cmake", "-S", "src/test/cpp", "-B", buildDirFile.absolutePath, "-G", "Ninja")
        }
        project.exec {
            commandLine("cmake", "--build", buildDirFile.absolutePath)
        }
        project.exec {
            commandLine("ctest", "--test-dir", buildDirFile.absolutePath, "--output-on-failure")
        }
    }
}

tasks.named("check") {
    dependsOn("nativeTest")
}
