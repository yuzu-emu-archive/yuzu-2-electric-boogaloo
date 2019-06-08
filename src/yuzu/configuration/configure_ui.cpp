// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <utility>

#include <QDirIterator>

#include "common/common_types.h"
#include "core/settings.h"
#include "ui_configure_ui.h"
#include "yuzu/configuration/configure_ui.h"
#include "yuzu/ui_settings.h"

namespace {
constexpr std::array default_icon_sizes{
    std::make_pair(0, QT_TR_NOOP("None")),
    std::make_pair(32, QT_TR_NOOP("Small (32x32)")),
    std::make_pair(64, QT_TR_NOOP("Standard (64x64)")),
    std::make_pair(128, QT_TR_NOOP("Large (128x128)")),
    std::make_pair(256, QT_TR_NOOP("Full Size (256x256)")),
};

constexpr std::array row_text_names{
    QT_TR_NOOP("Filename"),
    QT_TR_NOOP("Filetype"),
    QT_TR_NOOP("Title ID"),
    QT_TR_NOOP("Title Name"),
};
} // Anonymous namespace

ConfigureUi::ConfigureUi(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureUi) {
    ui->setupUi(this);

    InitializeLanguageComboBox();

    for (const auto& theme : UISettings::themes) {
        ui->theme_combobox->addItem(QString::fromStdString(theme.first),
                                    QString::fromStdString(theme.second));
    }

    InitializeIconSizeComboBox();
    InitializeRowComboBoxes();

    SetConfiguration();

    // Force game list reload if any of the relevant settings are changed.
    connect(ui->show_unknown, &QCheckBox::stateChanged, this, &ConfigureUi::RequestGameListUpdate);
    connect(ui->icon_size_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
    connect(ui->row_1_text_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
    connect(ui->row_2_text_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
}

ConfigureUi::~ConfigureUi() = default;

void ConfigureUi::ApplyConfiguration() {
    UISettings::values.theme =
        ui->theme_combobox->itemData(ui->theme_combobox->currentIndex()).toString();
    UISettings::values.show_unknown = ui->show_unknown->isChecked();
    UISettings::values.show_add_ons = ui->show_add_ons->isChecked();
    UISettings::values.icon_size = ui->icon_size_combobox->currentData().toUInt();
    UISettings::values.row_1_text_id = ui->row_1_text_combobox->currentData().toUInt();
    UISettings::values.row_2_text_id = ui->row_2_text_combobox->currentData().toUInt();
    Settings::Apply();
}

void ConfigureUi::RequestGameListUpdate() {
    UISettings::values.is_game_list_reload_pending.exchange(true);
}

void ConfigureUi::SetConfiguration() {
    ui->theme_combobox->setCurrentIndex(ui->theme_combobox->findData(UISettings::values.theme));
    ui->language_combobox->setCurrentIndex(
        ui->language_combobox->findData(UISettings::values.language));
    ui->show_unknown->setChecked(UISettings::values.show_unknown);
    ui->show_add_ons->setChecked(UISettings::values.show_add_ons);
    ui->icon_size_combobox->setCurrentIndex(
        ui->icon_size_combobox->findData(UISettings::values.icon_size));
    ui->row_1_text_combobox->setCurrentIndex(
        ui->row_1_text_combobox->findData(UISettings::values.row_1_text_id));
    ui->row_2_text_combobox->setCurrentIndex(
        ui->row_2_text_combobox->findData(UISettings::values.row_2_text_id));
}

void ConfigureUi::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
        return;
    }

    QWidget::changeEvent(event);
}

void ConfigureUi::RetranslateUI() {
    ui->retranslateUi(this);

    for (int i = 0; i < ui->icon_size_combobox->count(); i++) {
        ui->icon_size_combobox->setItemText(i, tr(default_icon_sizes[i].second));
    }

    for (int i = 0; i < ui->row_1_text_combobox->count(); i++) {
        const QString name = tr(row_text_names[i]);

        ui->row_1_text_combobox->setItemText(i, name);
        ui->row_2_text_combobox->setItemText(i, name);
    }
}

void ConfigureUi::InitializeLanguageComboBox() {
    ui->language_combobox->addItem(tr("<System>"), QString());
    ui->language_combobox->addItem(tr("English"), QStringLiteral("en"));
    QDirIterator it(QStringLiteral(":/languages"), QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString locale = it.next();
        locale.truncate(locale.lastIndexOf(QLatin1Char('.')));
        locale.remove(0, locale.lastIndexOf(QLatin1Char('/')) + 1);
        const QString lang = QLocale::languageToString(QLocale(locale).language());
        ui->language_combobox->addItem(lang, locale);
    }

    // Unlike other configuration changes, interface language changes need to be reflected on the
    // interface immediately. This is done by passing a signal to the main window, and then
    // retranslating when passing back.
    connect(ui->language_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::onLanguageChanged);
}

void ConfigureUi::InitializeIconSizeComboBox() {
    for (const auto& size : default_icon_sizes) {
        ui->icon_size_combobox->addItem(QString::fromUtf8(size.second), size.first);
    }
}

void ConfigureUi::InitializeRowComboBoxes() {
    for (std::size_t i = 0; i < row_text_names.size(); ++i) {
        const QString row_text_name = QString::fromUtf8(row_text_names[i]);

        ui->row_1_text_combobox->addItem(row_text_name, QVariant::fromValue(i));
        ui->row_2_text_combobox->addItem(row_text_name, QVariant::fromValue(i));
    }
}

void ConfigureUi::onLanguageChanged(int index) {
    if (index == -1)
        return;

    emit languageChanged(ui->language_combobox->itemData(index).toString());
}
