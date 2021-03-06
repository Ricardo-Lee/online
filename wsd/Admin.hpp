/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_ADMIN_HPP
#define INCLUDED_ADMIN_HPP

#include <mutex>

#include "AdminModel.hpp"
#include "Log.hpp"

#include "net/WebSocketHandler.hpp"

class Admin;

/// Handle admin client's Websocket requests & replies.
class AdminSocketHandler : public WebSocketHandler
{
public:
    AdminSocketHandler(Admin* adminManager,
                       const std::weak_ptr<StreamSocket>& socket,
                       const Poco::Net::HTTPRequest& request);

    /// Handle the initial Admin WS upgrade request.
    /// @returns true if we should give this socket to the Admin poll.
    static bool handleInitialRequest(const std::weak_ptr<StreamSocket> &socket,
                                     const Poco::Net::HTTPRequest& request);

private:
    /// Sends text frames simply to authenticated clients.
    void sendTextFrame(const std::string& message);

    /// Process incoming websocket messages
    void handleMessage(bool fin, WSOpCode code, std::vector<char> &data);

private:
    Admin* _admin;
    int _sessionId;
    bool _isAuthenticated;
};

class MemoryStatsTask;

/// An admin command processor.
class Admin : public SocketPoll
{
    Admin();
public:
    virtual ~Admin();

    static Admin& instance()
    {
        static Admin admin;
        return admin;
    }

    void start()
    {
        // FIXME: not if admin console is not enabled ?
        startThread();
    }

    /// Custom poll thread function
    void pollingThread() override;

    size_t getTotalMemoryUsage();
    size_t getTotalCpuUsage();

    void modificationAlert(const std::string& dockey, Poco::Process::PID pid, bool value);
    /// Update the Admin Model.
    void update(const std::string& message);

    /// Calls with same pid will increment view count, if pid already exists
    void addDoc(const std::string& docKey, Poco::Process::PID pid, const std::string& filename, const std::string& sessionId, const std::string& userName);

    /// Decrement view count till becomes zero after which doc is removed
    void rmDoc(const std::string& docKey, const std::string& sessionId);

    /// Remove the document with all views. Used on termination or catastrophic failure.
    void rmDoc(const std::string& docKey);

    void setForKitPid(const int forKitPid) { _forKitPid = forKitPid; }
    void setForKitWritePipe(const int forKitWritePipe) { _forKitWritePipe = forKitWritePipe; }

    /// Callers must ensure that modelMutex is acquired
    AdminModel& getModel();

    unsigned getMemStatsInterval();

    unsigned getCpuStatsInterval();

    void rescheduleMemTimer(unsigned interval);

    void rescheduleCpuTimer(unsigned interval);

    void updateLastActivityTime(const std::string& docKey);
    void updateMemoryDirty(const std::string& docKey, int dirty);
    void addBytes(const std::string& docKey, uint64_t sent, uint64_t recv);

    void dumpState(std::ostream& os) override;

    const DocProcSettings& getDefDocProcSettings() const { return _defDocProcSettings; }
    void setDefDocProcSettings(const DocProcSettings& docProcSettings)
    {
        _defDocProcSettings = docProcSettings;
        notifyForkit();
    }

private:
    /// Notify Forkit of changed settings.
    void notifyForkit();

private:
    /// The model is accessed only during startup & in
    /// the Admin Poll thread.
    AdminModel _model;
    int _forKitPid;
    int _forKitWritePipe;
    size_t _lastTotalMemory;
    size_t _lastJiffies;

    std::atomic<int> _memStatsTaskIntervalMs;
    std::atomic<int> _cpuStatsTaskIntervalMs;

    DocProcSettings _defDocProcSettings;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
