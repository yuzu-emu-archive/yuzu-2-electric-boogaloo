// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QDialog>

class QDialogButtonBox;
class QLabel;
class QLineEdit;

/// A QDialog that functions similarly to QInputDialog, however, it allows
/// restricting the minimum and total number of characters that can be entered.
class LimitableInputDialog final : public QDialog {
    Q_OBJECT
public:
    explicit LimitableInputDialog(QWidget* parent = nullptr);
    ~LimitableInputDialog() override;

    enum class InputLimiter {
        None,
        Filesystem,
    };

    static QString GetText(QWidget* parent, const QString& title, const QString& text,
                           int min_character_limit, int max_character_limit,
                           InputLimiter limit_type = InputLimiter::None);

private:
    void CreateUI();
    void ConnectEvents();

    void RemoveInvalidCharacters();
    QString invalid_characters;

    QLabel* text_label;
    QLineEdit* text_entry;
    QLabel* text_label_invalid;
    QDialogButtonBox* buttons;
};
