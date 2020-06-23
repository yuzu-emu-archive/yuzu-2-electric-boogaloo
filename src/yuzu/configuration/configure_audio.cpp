// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include <QSignalBlocker>

#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_audio.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_audio.h"

ConfigureAudio::ConfigureAudio(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureAudio>()) {
    ui->setupUi(this);

    InitializeAudioOutputSinkComboBox();

    connect(ui->volume_slider, &QSlider::valueChanged, this,
            &ConfigureAudio::SetVolumeIndicatorText);
    connect(ui->output_sink_combo_box, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureAudio::UpdateAudioDevices);

    ui->volume_label->setVisible(Settings::configuring_global);
    ui->volume_combo_box->setVisible(!Settings::configuring_global);

    SetupPerGameUI();

    SetConfiguration();

    const bool is_powered_on = Core::System::GetInstance().IsPoweredOn();
    ui->output_sink_combo_box->setEnabled(!is_powered_on);
    ui->audio_device_combo_box->setEnabled(!is_powered_on);
}

ConfigureAudio::~ConfigureAudio() = default;

void ConfigureAudio::SetConfiguration() {
    SetOutputSinkFromSinkID();

    // The device list cannot be pre-populated (nor listed) until the output sink is known.
    UpdateAudioDevices(ui->output_sink_combo_box->currentIndex());

    SetAudioDeviceFromDeviceID();

    ui->volume_slider->setValue(Settings::values.volume * ui->volume_slider->maximum());

    if (Settings::configuring_global) {
        ui->toggle_audio_stretching->setChecked(Settings::values.enable_audio_stretching);
    } else {
        ConfigurationShared::SetPerGameSetting(ui->toggle_audio_stretching,
                                               &Settings::values.enable_audio_stretching);
        if (Settings::values.volume.UsingGlobal()) {
            ui->volume_combo_box->setCurrentIndex(0);
            ui->volume_slider->setEnabled(false);
        } else {
            ui->volume_combo_box->setCurrentIndex(1);
            ui->volume_slider->setEnabled(true);
        }
    }
    SetVolumeIndicatorText(ui->volume_slider->sliderPosition());
}

void ConfigureAudio::SetOutputSinkFromSinkID() {
    [[maybe_unused]] const QSignalBlocker blocker(ui->output_sink_combo_box);

    int new_sink_index = 0;
    const QString sink_id = QString::fromStdString(Settings::values.sink_id);
    for (int index = 0; index < ui->output_sink_combo_box->count(); index++) {
        if (ui->output_sink_combo_box->itemText(index) == sink_id) {
            new_sink_index = index;
            break;
        }
    }

    ui->output_sink_combo_box->setCurrentIndex(new_sink_index);
}

void ConfigureAudio::SetAudioDeviceFromDeviceID() {
    int new_device_index = -1;

    const QString device_id = QString::fromStdString(Settings::values.audio_device_id);
    for (int index = 0; index < ui->audio_device_combo_box->count(); index++) {
        if (ui->audio_device_combo_box->itemText(index) == device_id) {
            new_device_index = index;
            break;
        }
    }

    ui->audio_device_combo_box->setCurrentIndex(new_device_index);
}

void ConfigureAudio::SetVolumeIndicatorText(int percentage) {
    ui->volume_indicator->setText(tr("%1%", "Volume percentage (e.g. 50%)").arg(percentage));
}

void ConfigureAudio::ApplyConfiguration() {
    if (Settings::configuring_global) {
        Settings::values.sink_id =
            ui->output_sink_combo_box->itemText(ui->output_sink_combo_box->currentIndex())
                .toStdString();
        Settings::values.enable_audio_stretching = ui->toggle_audio_stretching->isChecked();
        Settings::values.audio_device_id =
            ui->audio_device_combo_box->itemText(ui->audio_device_combo_box->currentIndex())
                .toStdString();
        Settings::values.volume =
            static_cast<float>(ui->volume_slider->sliderPosition()) / ui->volume_slider->maximum();
    } else {
        ConfigurationShared::ApplyPerGameSetting(&Settings::values.enable_audio_stretching,
                                                 ui->toggle_audio_stretching);
        if (ui->volume_combo_box->currentIndex() == 0) {
            Settings::values.volume.SetGlobal(true);
        } else {
            Settings::values.volume.SetGlobal(false);
            Settings::values.volume = static_cast<float>(ui->volume_slider->sliderPosition()) /
                                      ui->volume_slider->maximum();
        }
    }
}

void ConfigureAudio::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureAudio::UpdateAudioDevices(int sink_index) {
    ui->audio_device_combo_box->clear();
    ui->audio_device_combo_box->addItem(QString::fromUtf8(AudioCore::auto_device_name));

    const std::string sink_id = ui->output_sink_combo_box->itemText(sink_index).toStdString();
    for (const auto& device : AudioCore::GetDeviceListForSink(sink_id)) {
        ui->audio_device_combo_box->addItem(QString::fromStdString(device));
    }
}

void ConfigureAudio::InitializeAudioOutputSinkComboBox() {
    ui->output_sink_combo_box->clear();
    ui->output_sink_combo_box->addItem(QString::fromUtf8(AudioCore::auto_device_name));

    for (const char* id : AudioCore::GetSinkIDs()) {
        ui->output_sink_combo_box->addItem(QString::fromUtf8(id));
    }
}

void ConfigureAudio::RetranslateUI() {
    ui->retranslateUi(this);
    SetVolumeIndicatorText(ui->volume_slider->sliderPosition());
}

void ConfigureAudio::SetupPerGameUI() {
    if (Settings::configuring_global)
        return;

    connect(ui->volume_combo_box, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int index) { ui->volume_slider->setEnabled(index == 1); });

    ui->output_sink_combo_box->setVisible(false);
    ui->output_sink_label->setVisible(false);
    ui->audio_device_combo_box->setVisible(false);
    ui->audio_device_label->setVisible(false);
}
