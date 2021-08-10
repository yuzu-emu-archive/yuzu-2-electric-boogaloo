﻿// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QDialog>
#include <QList>

#include "core/file_sys/vfs_types.h"
#include "yuzu/configuration/config.h"

class QGraphicsScene;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QVBoxLayout;

namespace Ui {
class ConfigurePerGame;
}

class ConfigurePerGame : public QDialog {
    Q_OBJECT

public:
    // Cannot use std::filesystem::path due to https://bugreports.qt.io/browse/QTBUG-73263
    explicit ConfigurePerGame(QWidget* parent, u64 title_id, const std::string& file_name);
    ~ConfigurePerGame() override;

    /// Save all button configurations to settings file
    void ApplyConfiguration();

    void LoadFromFile(FileSys::VirtualFile file);

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void HandleApplyButtonClicked();

    void LoadConfiguration();

    std::unique_ptr<Ui::ConfigurePerGame> ui;
    FileSys::VirtualFile file;
    u64 title_id;

    QGraphicsScene* scene;

    std::unique_ptr<Config> game_config;
};
