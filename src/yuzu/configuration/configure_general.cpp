// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_general.h"
#include "yuzu/configuration/configure_general.h"
#include "yuzu/ui_settings.h"

ConfigureGeneral::ConfigureGeneral(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureGeneral) {

    ui->setupUi(this);

    connect(ui->toggle_deepscan, &QCheckBox::stateChanged, this,
            [] { UISettings::values.is_game_list_reload_pending.exchange(true); });

    ui->use_cpu_jit->setEnabled(!Core::System::GetInstance().IsPoweredOn());
}

ConfigureGeneral::~ConfigureGeneral() = default;

void ConfigureGeneral::SetConfiguration() {
    ui->toggle_deepscan->setChecked(UISettings::values.game_directory_deepscan);
    ui->toggle_check_exit->setChecked(UISettings::values.confirm_before_closing);
    ui->toggle_user_on_boot->setChecked(UISettings::values.select_user_on_boot);
    ui->use_cpu_jit->setChecked(Settings::values.use_cpu_jit);
}

void ConfigureGeneral::ApplyConfiguration() {
    UISettings::values.game_directory_deepscan = ui->toggle_deepscan->isChecked();
    UISettings::values.confirm_before_closing = ui->toggle_check_exit->isChecked();
    UISettings::values.select_user_on_boot = ui->toggle_user_on_boot->isChecked();

    Settings::values.use_cpu_jit = ui->use_cpu_jit->isChecked();
}

void ConfigureGeneral::RetranslateUI() {
    ui->retranslateUi(this);
}
