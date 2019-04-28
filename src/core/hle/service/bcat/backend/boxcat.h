// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <map>
#include <optional>
#include "core/hle/service/bcat/backend/backend.h"

namespace Service::BCAT {

struct EventStatus {
    std::optional<std::string> header;
    std::optional<std::string> footer;
    std::vector<std::string> events;
};

/// Boxcat is yuzu's custom backend implementation of Nintendo's BCAT service. It is free to use and
/// doesn't require a switch or nintendo account. The content is controlled by the yuzu team.
class Boxcat final : public Backend {
    friend void SynchronizeInternal(DirectoryGetter dir_getter, TitleIDVersion title,
                                    CompletionCallback callback,
                                    std::optional<std::string> dir_name);

public:
    explicit Boxcat(DirectoryGetter getter);
    ~Boxcat() override;

    bool Synchronize(TitleIDVersion title, CompletionCallback callback) override;
    bool SynchronizeDirectory(TitleIDVersion title, std::string name,
                              CompletionCallback callback) override;

    bool Clear(u64 title_id) override;

    void SetPassphrase(u64 title_id, const Passphrase& passphrase) override;

    enum class StatusResult {
        Success,
        Offline,
        ParseError,
        BadClientVersion,
    };

    static StatusResult GetStatus(std::optional<std::string>& global,
                                  std::map<std::string, EventStatus>& games);

private:
    std::atomic_bool is_syncing{false};

    class Client;
    std::unique_ptr<Client> client;
};

} // namespace Service::BCAT
