plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("kotlin-parcelize")
}

/**
 * Use the number of seconds/10 since Jan 1 2016 as the versionCode.
 * This lets us upload a new build at most every 10 seconds for the
 * next 680 years.
 */
val autoVersion = (((System.currentTimeMillis() / 1000) - 1451606400) / 10).toInt()
var buildType = ""

@Suppress("UnstableApiUsage")
android {
    namespace = "org.yuzu.yuzu_emu"

    compileSdkVersion = "android-33"
    ndkVersion = "25.2.9519653"

    buildFeatures {
        viewBinding = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    kotlinOptions {
        jvmTarget = "11"
    }

    lint {
        // This is important as it will run lint but not abort on error
        // Lint has some overly obnoxious "errors" that should really be warnings
        abortOnError = false

        //Uncomment disable lines for test builds...
        //disable 'MissingTranslation'bin
        //disable 'ExtraTranslation'
    }

    defaultConfig {
        // TODO If this is ever modified, change application_id in strings.xml
        applicationId = "org.yuzu.yuzu_emu"
        minSdk = 28
        targetSdk = 33
        versionCode = autoVersion
        versionName = getVersion()

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        buildConfigField("String", "GIT_HASH", "\"${getGitHash()}\"")
        buildConfigField("String", "BRANCH", "\"${getBranch()}\"")
    }

    signingConfigs {
        //release {
        //    storeFile file('')
        //    storePassword System.getenv('ANDROID_KEYPASS')
        //    keyAlias = 'key0'
        //    keyPassword System.getenv('ANDROID_KEYPASS')
        //}
    }

    applicationVariants.all { variant ->
        buildType = variant.buildType.name // sets the current build type
        true
    }

    // Define build types, which are orthogonal to product flavors.
    buildTypes {

        // Signed by release key, allowing for upload to Play Store.
        release {
            signingConfig = signingConfigs.getByName("debug")
        }

        // builds a release build that doesn't need signing
        // Attaches 'debug' suffix to version and package name, allowing installation alongside the release build.
        register("relWithDebInfo") {
            initWith(getByName("release"))
            versionNameSuffix = "-debug"
            signingConfig = signingConfigs.getByName("debug")
            isMinifyEnabled = false
            enableAndroidTestCoverage = false
            isDebuggable = true
            isJniDebuggable = true
        }

        // Signed by debug key disallowing distribution on Play Store.
        // Attaches 'debug' suffix to version and package name, allowing installation alongside the release build.
        debug {
            // TODO If this is ever modified, change application_id in debug/strings.xml
            versionNameSuffix = "-debug"
            isDebuggable = true
            isJniDebuggable = true
        }
    }

    flavorDimensions.add("version")
    productFlavors {
        create("mainline") {
            dimension = "version"
        }
    }

    externalNativeBuild {
        cmake {
            version = "3.22.1"
            path = file("../../../CMakeLists.txt")
        }
    }

    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments(
                    "-DENABLE_QT=0", // Don't use QT
                    "-DENABLE_SDL2=0", // Don't use SDL
                    "-DENABLE_WEB_SERVICE=0", // Don't use telemetry
                    "-DBUNDLE_SPEEX=ON",
                    "-DANDROID_ARM_NEON=true", // cryptopp requires Neon to work
                    "-DYUZU_USE_BUNDLED_VCPKG=ON",
                    "-DYUZU_USE_BUNDLED_FFMPEG=ON"
                )

                abiFilters("arm64-v8a", "x86_64")
            }
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.10.0")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("androidx.exifinterface:exifinterface:1.3.6")
    implementation("androidx.cardview:cardview:1.0.0")
    implementation("androidx.recyclerview:recyclerview:1.3.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.lifecycle:lifecycle-viewmodel:2.6.1")
    implementation("androidx.fragment:fragment:1.5.6")
    implementation("androidx.slidingpanelayout:slidingpanelayout:1.2.0")
    implementation("androidx.documentfile:documentfile:1.0.1")
    implementation("com.google.android.material:material:1.8.0")
    implementation("androidx.preference:preference:1.2.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.6.1")
    implementation("io.coil-kt:coil:2.2.2")
    implementation("androidx.core:core-splashscreen:1.0.0")
    implementation("androidx.window:window:1.0.0")
    implementation("org.ini4j:ini4j:0.5.4")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.swiperefreshlayout:swiperefreshlayout:1.1.0")
}

fun getVersion(): String {
    var versionName = "0.0"

    try {
        versionName = ProcessBuilder("git", "describe", "--always", "--long")
            .directory(project.rootDir)
            .redirectOutput(ProcessBuilder.Redirect.PIPE)
            .redirectError(ProcessBuilder.Redirect.PIPE)
            .start().inputStream.bufferedReader().use { it.readText() }
            .trim()
            .replace(Regex("(-0)?-[^-]+$"), "")
    } catch (e: Exception) {
        logger.error("Cannot find git, defaulting to dummy version number")
    }

    if (System.getenv("GITHUB_ACTIONS") != null) {
        val gitTag = System.getenv("GIT_TAG_NAME")
        versionName = gitTag ?: versionName
    }

    return versionName
}

fun getGitHash(): String {
    try {
        val processBuilder = ProcessBuilder("git", "rev-parse", "HEAD")
        processBuilder.directory(project.rootDir)
        val process = processBuilder.start()
        val inputStream = process.inputStream
        val errorStream = process.errorStream
        process.waitFor()

        return if (process.exitValue() == 0) {
            inputStream.bufferedReader()
                .use { it.readText().trim() } // return the value of gitHash
        } else {
            val errorMessage = errorStream.bufferedReader().use { it.readText().trim() }
            logger.error("Error running git command: $errorMessage")
            "dummy-hash" // return a dummy hash value in case of an error
        }
    } catch (e: Exception) {
        logger.error("$e: Cannot find git, defaulting to dummy build hash")
        return "dummy-hash" // return a dummy hash value in case of an error
    }
}

fun getBranch(): String {
    try {
        val processBuilder = ProcessBuilder("git", "rev-parse", "--abbrev-ref", "HEAD")
        processBuilder.directory(project.rootDir)
        val process = processBuilder.start()
        val inputStream = process.inputStream
        val errorStream = process.errorStream
        process.waitFor()

        return if (process.exitValue() == 0) {
            inputStream.bufferedReader()
                .use { it.readText().trim() } // return the value of gitHash
        } else {
            val errorMessage = errorStream.bufferedReader().use { it.readText().trim() }
            logger.error("Error running git command: $errorMessage")
            "dummy-hash" // return a dummy hash value in case of an error
        }
    } catch (e: Exception) {
        logger.error("$e: Cannot find git, defaulting to dummy build hash")
        return "dummy-hash" // return a dummy hash value in case of an error
    }
}