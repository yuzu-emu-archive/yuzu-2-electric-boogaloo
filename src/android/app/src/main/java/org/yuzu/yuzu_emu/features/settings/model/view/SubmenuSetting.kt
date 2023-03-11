package org.yuzu.yuzu_emu.features.settings.model.view

import org.yuzu.yuzu_emu.features.settings.model.Setting

class SubmenuSetting(
    key: String?,
    setting: Setting?,
    titleId: Int,
    descriptionId: Int,
    val menuKey: String
) : SettingsItem(
    key, null, setting, titleId, descriptionId
) {
    override val type = TYPE_SUBMENU
}