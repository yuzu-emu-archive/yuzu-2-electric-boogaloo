// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_graphics_advanced.h"
#include "yuzu/configuration/configure_graphics_advanced.h"

ConfigureGraphicsAdvanced::ConfigureGraphicsAdvanced(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureGraphicsAdvanced) {

    ui->setupUi(this);

    SetConfiguration();
}

ConfigureGraphicsAdvanced::~ConfigureGraphicsAdvanced() = default;

void ConfigureGraphicsAdvanced::SetConfiguration() {
    const bool runtime_lock = !Core::System::GetInstance().IsPoweredOn();
    ui->gpu_accuracy->setCurrentIndex(static_cast<int>(Settings::config_values->gpu_accuracy));
    ui->use_vsync->setEnabled(runtime_lock);
    ui->use_vsync->setChecked(Settings::config_values->use_vsync);
    ui->use_assembly_shaders->setEnabled(runtime_lock);
    ui->use_assembly_shaders->setChecked(Settings::config_values->use_assembly_shaders);
    ui->use_fast_gpu_time->setChecked(Settings::config_values->use_fast_gpu_time);
    ui->force_30fps_mode->setEnabled(runtime_lock);
    ui->force_30fps_mode->setChecked(Settings::config_values->force_30fps_mode);
    ui->anisotropic_filtering_combobox->setEnabled(runtime_lock);
    ui->anisotropic_filtering_combobox->setCurrentIndex(Settings::config_values->max_anisotropy);
}

void ConfigureGraphicsAdvanced::ApplyConfiguration() {
    auto gpu_accuracy = static_cast<Settings::GPUAccuracy>(ui->gpu_accuracy->currentIndex());
    Settings::config_values->gpu_accuracy = gpu_accuracy;
    Settings::config_values->use_vsync = ui->use_vsync->isChecked();
    Settings::config_values->use_assembly_shaders = ui->use_assembly_shaders->isChecked();
    Settings::config_values->use_fast_gpu_time = ui->use_fast_gpu_time->isChecked();
    Settings::config_values->force_30fps_mode = ui->force_30fps_mode->isChecked();
    Settings::config_values->max_anisotropy = ui->anisotropic_filtering_combobox->currentIndex();
}

void ConfigureGraphicsAdvanced::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureGraphicsAdvanced::RetranslateUI() {
    ui->retranslateUi(this);
}
