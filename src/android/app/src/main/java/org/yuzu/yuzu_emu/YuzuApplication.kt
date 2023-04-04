// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.DocumentsTree
import org.yuzu.yuzu_emu.utils.GpuDriverHelper
import java.io.File

fun Context.getPublicFilesDir() : File = getExternalFilesDir(null) ?: filesDir

class YuzuApplication : Application() {
    private fun createNotificationChannel() {
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        val name: CharSequence = getString(R.string.app_notification_channel_name)
        val description = getString(R.string.app_notification_channel_description)
        val channel = NotificationChannel(
            getString(R.string.app_notification_channel_id),
            name,
            NotificationManager.IMPORTANCE_LOW
        )
        channel.description = description
        channel.setSound(null, null)
        channel.vibrationPattern = null
        // Register the channel with the system; you can't change the importance
        // or other notification behaviors after this
        val notificationManager = getSystemService(NotificationManager::class.java)
        notificationManager.createNotificationChannel(channel)
    }

    override fun onCreate() {
        super.onCreate()
        application = this
        documentsTree = DocumentsTree()
        DirectoryInitialization.start(applicationContext)
        GpuDriverHelper.initializeDriverParameters(applicationContext)
        NativeLibrary.logDeviceInfo()

        // TODO(bunnei): Disable notifications until we support app suspension.
        //createNotificationChannel();
    }

    companion object {
        @JvmField
        var documentsTree: DocumentsTree? = null
        lateinit var application: YuzuApplication

        @JvmStatic
        val appContext: Context
            get() = application.applicationContext
    }
}