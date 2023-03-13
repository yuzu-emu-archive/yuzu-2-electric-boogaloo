// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_graphics_advanced.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_graphics_advanced.h"

ConfigureGraphicsAdvanced::ConfigureGraphicsAdvanced(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureGraphicsAdvanced>()}, system{system_} {

    ui->setupUi(this);

    SetupPerGameUI();

    SetConfiguration();

    ui->enable_compute_pipelines_checkbox->setVisible(false);
}

ConfigureGraphicsAdvanced::~ConfigureGraphicsAdvanced() = default;

void ConfigureGraphicsAdvanced::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();
    ui->use_reactive_flushing->setEnabled(runtime_lock);
    ui->async_present->setEnabled(runtime_lock);
    ui->renderer_force_max_clock->setEnabled(runtime_lock);
    ui->async_astc->setEnabled(runtime_lock);
    ui->astc_recompression_combobox->setEnabled(runtime_lock);
    ui->use_asynchronous_shaders->setEnabled(runtime_lock);
    ui->anisotropic_filtering_combobox->setEnabled(runtime_lock);
    ui->enable_compute_pipelines_checkbox->setEnabled(runtime_lock);

    ui->async_present->setChecked(Settings::values.async_presentation.GetValue());
    ui->renderer_force_max_clock->setChecked(Settings::values.renderer_force_max_clock.GetValue());
    ui->use_reactive_flushing->setChecked(Settings::values.use_reactive_flushing.GetValue());
    ui->async_astc->setChecked(Settings::values.async_astc.GetValue());
    ui->use_asynchronous_shaders->setChecked(Settings::values.use_asynchronous_shaders.GetValue());
    ui->use_fast_gpu_time->setChecked(Settings::values.use_fast_gpu_time.GetValue());
    ui->use_vulkan_driver_pipeline_cache->setChecked(
        Settings::values.use_vulkan_driver_pipeline_cache.GetValue());
    ui->enable_compute_pipelines_checkbox->setChecked(
        Settings::values.enable_compute_pipelines.GetValue());
    ui->use_video_framerate_checkbox->setChecked(Settings::values.use_video_framerate.GetValue());
    ui->barrier_feedback_loops_checkbox->setChecked(
        Settings::values.barrier_feedback_loops.GetValue());
    ui->transform_feedback_query->setChecked(Settings::values.transform_feedback_query.GetValue());

    if (Settings::IsConfiguringGlobal()) {
        ui->gpu_accuracy->setCurrentIndex(
            static_cast<int>(Settings::values.gpu_accuracy.GetValue()));
        ui->anisotropic_filtering_combobox->setCurrentIndex(
            Settings::values.max_anisotropy.GetValue());
        ui->astc_recompression_combobox->setCurrentIndex(
            static_cast<int>(Settings::values.astc_recompression.GetValue()));
    } else {
        ConfigurationShared::SetPerGameSetting(ui->gpu_accuracy, &Settings::values.gpu_accuracy);
        ConfigurationShared::SetPerGameSetting(ui->anisotropic_filtering_combobox,
                                               &Settings::values.max_anisotropy);
        ConfigurationShared::SetPerGameSetting(ui->astc_recompression_combobox,
                                               &Settings::values.astc_recompression);
        ConfigurationShared::SetHighlight(ui->label_gpu_accuracy,
                                          !Settings::values.gpu_accuracy.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->af_label,
                                          !Settings::values.max_anisotropy.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->label_astc_recompression,
                                          !Settings::values.astc_recompression.UsingGlobal());
    }
}

void ConfigureGraphicsAdvanced::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.gpu_accuracy, ui->gpu_accuracy);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.async_presentation,
                                             ui->async_present, async_present);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.renderer_force_max_clock,
                                             ui->renderer_force_max_clock,
                                             renderer_force_max_clock);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.max_anisotropy,
                                             ui->anisotropic_filtering_combobox);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_reactive_flushing,
                                             ui->use_reactive_flushing, use_reactive_flushing);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.async_astc, ui->async_astc,
                                             async_astc);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.astc_recompression,
                                             ui->astc_recompression_combobox);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_asynchronous_shaders,
                                             ui->use_asynchronous_shaders,
                                             use_asynchronous_shaders);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_fast_gpu_time,
                                             ui->use_fast_gpu_time, use_fast_gpu_time);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_vulkan_driver_pipeline_cache,
                                             ui->use_vulkan_driver_pipeline_cache,
                                             use_vulkan_driver_pipeline_cache);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.enable_compute_pipelines,
                                             ui->enable_compute_pipelines_checkbox,
                                             enable_compute_pipelines);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_video_framerate,
                                             ui->use_video_framerate_checkbox, use_video_framerate);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.barrier_feedback_loops,
                                             ui->barrier_feedback_loops_checkbox,
                                             barrier_feedback_loops);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.transform_feedback_query,
                                             ui->transform_feedback_query,
                                             transform_feedback_query);
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

void ConfigureGraphicsAdvanced::SetupPerGameUI() {
    // Disable if not global (only happens during game)
    if (Settings::IsConfiguringGlobal()) {
        ui->gpu_accuracy->setEnabled(Settings::values.gpu_accuracy.UsingGlobal());
        ui->async_present->setEnabled(Settings::values.async_presentation.UsingGlobal());
        ui->renderer_force_max_clock->setEnabled(
            Settings::values.renderer_force_max_clock.UsingGlobal());
        ui->use_reactive_flushing->setEnabled(Settings::values.use_reactive_flushing.UsingGlobal());
        ui->async_astc->setEnabled(Settings::values.async_astc.UsingGlobal());
        ui->astc_recompression_combobox->setEnabled(
            Settings::values.astc_recompression.UsingGlobal());
        ui->use_asynchronous_shaders->setEnabled(
            Settings::values.use_asynchronous_shaders.UsingGlobal());
        ui->use_fast_gpu_time->setEnabled(Settings::values.use_fast_gpu_time.UsingGlobal());
        ui->use_vulkan_driver_pipeline_cache->setEnabled(
            Settings::values.use_vulkan_driver_pipeline_cache.UsingGlobal());
        ui->anisotropic_filtering_combobox->setEnabled(
            Settings::values.max_anisotropy.UsingGlobal());
        ui->enable_compute_pipelines_checkbox->setEnabled(
            Settings::values.enable_compute_pipelines.UsingGlobal());
        ui->use_video_framerate_checkbox->setEnabled(
            Settings::values.use_video_framerate.UsingGlobal());
        ui->barrier_feedback_loops_checkbox->setEnabled(
            Settings::values.barrier_feedback_loops.UsingGlobal());
        ui->transform_feedback_query->setEnabled(
            Settings::values.transform_feedback_query.UsingGlobal());

        return;
    }

    ConfigurationShared::SetColoredTristate(ui->async_present, Settings::values.async_presentation,
                                            async_present);
    ConfigurationShared::SetColoredTristate(ui->renderer_force_max_clock,
                                            Settings::values.renderer_force_max_clock,
                                            renderer_force_max_clock);
    ConfigurationShared::SetColoredTristate(
        ui->use_reactive_flushing, Settings::values.use_reactive_flushing, use_reactive_flushing);
    ConfigurationShared::SetColoredTristate(ui->async_astc, Settings::values.async_astc,
                                            async_astc);
    ConfigurationShared::SetColoredTristate(ui->use_asynchronous_shaders,
                                            Settings::values.use_asynchronous_shaders,
                                            use_asynchronous_shaders);
    ConfigurationShared::SetColoredTristate(ui->use_fast_gpu_time,
                                            Settings::values.use_fast_gpu_time, use_fast_gpu_time);
    ConfigurationShared::SetColoredTristate(ui->use_vulkan_driver_pipeline_cache,
                                            Settings::values.use_vulkan_driver_pipeline_cache,
                                            use_vulkan_driver_pipeline_cache);
    ConfigurationShared::SetColoredTristate(ui->enable_compute_pipelines_checkbox,
                                            Settings::values.enable_compute_pipelines,
                                            enable_compute_pipelines);
    ConfigurationShared::SetColoredTristate(ui->use_video_framerate_checkbox,
                                            Settings::values.use_video_framerate,
                                            use_video_framerate);
    ConfigurationShared::SetColoredTristate(ui->barrier_feedback_loops_checkbox,
                                            Settings::values.barrier_feedback_loops,
                                            barrier_feedback_loops);
    ConfigurationShared::SetColoredTristate(ui->transform_feedback_query,
                                            Settings::values.transform_feedback_query,
                                            transform_feedback_query);

    ConfigurationShared::SetColoredComboBox(
        ui->gpu_accuracy, ui->label_gpu_accuracy,
        static_cast<int>(Settings::values.gpu_accuracy.GetValue(true)));
    ConfigurationShared::SetColoredComboBox(
        ui->anisotropic_filtering_combobox, ui->af_label,
        static_cast<int>(Settings::values.max_anisotropy.GetValue(true)));
    ConfigurationShared::SetColoredComboBox(
        ui->astc_recompression_combobox, ui->label_astc_recompression,
        static_cast<int>(Settings::values.astc_recompression.GetValue(true)));
}

void ConfigureGraphicsAdvanced::ExposeComputeOption() {
    ui->enable_compute_pipelines_checkbox->setVisible(true);
}
