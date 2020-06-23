// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <chrono>
#include <optional>

#include <QFileDialog>
#include <QGraphicsItem>
#include <QMessageBox>
#include "common/assert.h"
#include "common/file_util.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_system.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_system.h"

ConfigureSystem::ConfigureSystem(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureSystem) {
    ui->setupUi(this);
    connect(ui->button_regenerate_console_id, &QPushButton::clicked, this,
            &ConfigureSystem::RefreshConsoleID);

    connect(ui->rng_seed_checkbox, &QCheckBox::stateChanged, this, [this](int state) {
        ui->rng_seed_edit->setEnabled(state == Qt::Checked);
        if (state != Qt::Checked) {
            ui->rng_seed_edit->setText(QStringLiteral("00000000"));
        }
    });

    connect(ui->custom_rtc_checkbox, &QCheckBox::stateChanged, this, [this](int state) {
        ui->custom_rtc_edit->setEnabled(state == Qt::Checked);
        if (state != Qt::Checked) {
            ui->custom_rtc_edit->setDateTime(QDateTime::currentDateTime());
        }
    });

    ui->label_console_id->setVisible(Settings::configuring_global);
    ui->button_regenerate_console_id->setVisible(Settings::configuring_global);

    SetupPerGameUI();

    SetConfiguration();
}

ConfigureSystem::~ConfigureSystem() = default;

void ConfigureSystem::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureSystem::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureSystem::SetConfiguration() {
    enabled = !Core::System::GetInstance().IsPoweredOn();
    const auto rng_seed =
        QStringLiteral("%1")
            .arg(Settings::values.rng_seed.GetValue().value_or(0), 8, 16, QLatin1Char{'0'})
            .toUpper();
    const auto rtc_time = Settings::values.custom_rtc.GetValue().value_or(
        std::chrono::seconds(QDateTime::currentSecsSinceEpoch()));

    if (Settings::configuring_global) {
        ui->combo_language->setCurrentIndex(Settings::values.language_index);
        ui->combo_region->setCurrentIndex(Settings::values.region_index);
        ui->combo_time_zone->setCurrentIndex(Settings::values.time_zone_index);
        ui->combo_sound->setCurrentIndex(Settings::values.sound_index);

        ui->rng_seed_checkbox->setChecked(Settings::values.rng_seed.GetValue().has_value());
        ui->rng_seed_edit->setEnabled(Settings::values.rng_seed.GetValue().has_value());
        ui->rng_seed_edit->setText(rng_seed);

        ui->custom_rtc_checkbox->setChecked(Settings::values.custom_rtc.GetValue().has_value());
        ui->custom_rtc_edit->setEnabled(Settings::values.custom_rtc.GetValue().has_value());
        ui->custom_rtc_edit->setDateTime(QDateTime::fromSecsSinceEpoch(rtc_time.count()));
    } else {
        ConfigurationShared::SetPerGameSetting(ui->combo_language,
                                               &Settings::values.language_index);
        ConfigurationShared::SetPerGameSetting(ui->combo_region, &Settings::values.region_index);
        ConfigurationShared::SetPerGameSetting(ui->combo_time_zone,
                                               &Settings::values.time_zone_index);
        ConfigurationShared::SetPerGameSetting(ui->combo_sound, &Settings::values.sound_index);

        if (Settings::values.rng_seed.UsingGlobal()) {
            ui->rng_seed_checkbox->setCheckState(Qt::PartiallyChecked);
        } else {
            ui->rng_seed_checkbox->setCheckState(
                Settings::values.rng_seed.GetValue().has_value() ? Qt::Checked : Qt::Unchecked);
            if (Settings::values.rng_seed.GetValue().has_value()) {
                ui->rng_seed_edit->setText(rng_seed);
            }
        }

        if (Settings::values.custom_rtc.UsingGlobal()) {
            ui->custom_rtc_checkbox->setCheckState(Qt::PartiallyChecked);
        } else {
            ui->custom_rtc_checkbox->setCheckState(
                Settings::values.custom_rtc.GetValue().has_value() ? Qt::Checked : Qt::Unchecked);
            if (Settings::values.custom_rtc.GetValue().has_value()) {
                ui->custom_rtc_edit->setDateTime(QDateTime::fromSecsSinceEpoch(rtc_time.count()));
            }
        }
    }
}

void ConfigureSystem::ReadSystemSettings() {}

void ConfigureSystem::ApplyConfiguration() {
    if (!enabled) {
        return;
    }

    if (Settings::configuring_global) {
        Settings::values.language_index = ui->combo_language->currentIndex();
        Settings::values.region_index = ui->combo_region->currentIndex();
        Settings::values.time_zone_index = ui->combo_time_zone->currentIndex();
        Settings::values.sound_index = ui->combo_sound->currentIndex();

        if (ui->rng_seed_checkbox->isChecked()) {
            Settings::values.rng_seed = ui->rng_seed_edit->text().toULongLong(nullptr, 16);
        } else {
            Settings::values.rng_seed = std::nullopt;
        }

        if (ui->custom_rtc_checkbox->isChecked()) {
            Settings::values.custom_rtc =
                std::chrono::seconds(ui->custom_rtc_edit->dateTime().toSecsSinceEpoch());
        } else {
            Settings::values.custom_rtc = std::nullopt;
        }
    } else {
        ConfigurationShared::ApplyPerGameSetting(&Settings::values.language_index,
                                                 ui->combo_language);
        ConfigurationShared::ApplyPerGameSetting(&Settings::values.region_index, ui->combo_region);
        ConfigurationShared::ApplyPerGameSetting(&Settings::values.time_zone_index,
                                                 ui->combo_time_zone);
        ConfigurationShared::ApplyPerGameSetting(&Settings::values.sound_index, ui->combo_sound);

        switch (ui->rng_seed_checkbox->checkState()) {
        case Qt::Checked:
            Settings::values.rng_seed.SetGlobal(false);
            Settings::values.rng_seed = ui->rng_seed_edit->text().toULongLong(nullptr, 16);
            break;
        case Qt::Unchecked:
            Settings::values.rng_seed.SetGlobal(false);
            Settings::values.rng_seed = std::nullopt;
            break;
        case Qt::PartiallyChecked:
            Settings::values.rng_seed.SetGlobal(true);
        }

        switch (ui->custom_rtc_checkbox->checkState()) {
        case Qt::Checked:
            Settings::values.custom_rtc.SetGlobal(false);
            Settings::values.custom_rtc =
                std::chrono::seconds(ui->custom_rtc_edit->dateTime().toSecsSinceEpoch());
            break;
        case Qt::Unchecked:
            Settings::values.custom_rtc.SetGlobal(false);
            Settings::values.custom_rtc = std::nullopt;
            break;
        case Qt::PartiallyChecked:
            Settings::values.custom_rtc.SetGlobal(true);
        }
    }

    Settings::Apply();
}

void ConfigureSystem::RefreshConsoleID() {
    QMessageBox::StandardButton reply;
    QString warning_text = tr("This will replace your current virtual Switch with a new one. "
                              "Your current virtual Switch will not be recoverable. "
                              "This might have unexpected effects in games. This might fail, "
                              "if you use an outdated config savegame. Continue?");
    reply = QMessageBox::critical(this, tr("Warning"), warning_text,
                                  QMessageBox::No | QMessageBox::Yes);
    if (reply == QMessageBox::No) {
        return;
    }

    u64 console_id{};
    ui->label_console_id->setText(
        tr("Console ID: 0x%1").arg(QString::number(console_id, 16).toUpper()));
}

void ConfigureSystem::SetupPerGameUI() {
    if (Settings::configuring_global) {
        return;
    }

    ConfigurationShared::InsertGlobalItem(ui->combo_language);
    ConfigurationShared::InsertGlobalItem(ui->combo_region);
    ConfigurationShared::InsertGlobalItem(ui->combo_time_zone);
    ConfigurationShared::InsertGlobalItem(ui->combo_sound);
    ui->rng_seed_checkbox->setTristate(true);
    ui->custom_rtc_checkbox->setTristate(true);
}
