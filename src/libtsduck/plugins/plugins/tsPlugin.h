//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of the API of a tsp plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsTSP.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsNames.h"
#include "tsDuckContext.h"

namespace ts {
    //!
    //! Each plugin has one of the following types
    //! @ingroup plugin
    //!
    enum class PluginType {
        INPUT,     //!< Input plugin.
        OUTPUT,    //!< Output plugin.
        PROCESSOR  //!< Packet processor plugin.
    };

    //!
    //! Displayable names of plugin types.
    //! @ingroup plugin
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& PluginTypeNames();

    //!
    //! Base class of all @c tsp plugins.
    //! @ingroup libtsduck plugin
    //!
    //! Plugin is a subclass of Args; each constructor is expected to define
    //! the syntax, help and option definitions for the command line.
    //!
    //! A shared library is invoked into a multi-threaded environment.
    //! It must be thread-safe. It may define its maximum stack usage.
    //!
    class TSDUCKDLL Plugin: public Args
    {
        TS_NOBUILD_NOCOPY(Plugin);
    public:
        //!
        //! Default stack usage in bytes for the thread executing a plugin.
        //!
        static constexpr size_t DEFAULT_STACK_USAGE = 128 * 1024;

        //!
        //! Define the maximum stack usage for the thread executing the plugin.
        //! If the method is not implemented by a subclass, the default value
        //! is @link DEFAULT_STACK_USAGE @endlink (128 kB).
        //! @return The maximum stack usage in bytes for the thread executing the plugin.
        //!
        virtual size_t stackUsage() const;

        //!
        //! The main application invokes getOptions() only once, at application startup.
        //! Optionally implemented by subclasses to analyze the command line options.
        //! A plugin may ignore getOptions() and analyzes the command line options as
        //! part of the start() method. However, if a plugin is started later, command
        //! line errors may be reported too late.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool getOptions();

        //!
        //! The main application invokes start() to start the plugin.
        //! Optionally implemented by subclasses.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool start();

        //!
        //! The main application invokes stop() to terminate the plugin.
        //! Optionally implemented by subclasses.
        //! @return True on success, false on error (ie. not started).
        //!
        virtual bool stop();

        //!
        //! Get the plugin bitrate.
        //!
        //! The main application may invoke getBitrate() at any time.
        //! The semantics depends on the capability.
        //! - Input plugin: Returns the current input bitrate of the device.
        //!   Useful for real-time devices only.
        //! - Output plugin: Returns the current output bitrate which is used
        //!   by the device.
        //! - Packet processing plugin: Return the current bitrate at the output
        //!   of the packet processor. This can be used by packet processors
        //!   which influence the bitrate by removing packets or introducing
        //!   delays.
        //!
        //! Optionally implemented by subclasses. By default, return that the
        //! plugin is not aware of the bitrate.
        //!
        //! @return Plugin bitrate in bits/second. Shall return 0 on error or unknown bitrate.
        //!
        virtual BitRate getBitrate();

        //!
        //! Get the plugin bitrate confidence.
        //!
        //! When a subclass overrides getBitrate(), it should also override getBitrateConfidence().
        //!
        //! @return The level of confidence of the bitrate value as returned by the previous call
        //! to getBitrate(). If the previous returned bitrate was zero, this confidence level
        //! shall be ignored.
        //!
        virtual BitRateConfidence getBitrateConfidence();

        //!
        //! Tell if the plugin is a real time one.
        //!
        //! Some plugin behave more accurately when the responsiveness of the
        //! environment is more accurate. Typically, input and output on tuners,
        //! modulators or ASI devices are real-time plugins. On the opposite,
        //! working on offline disk files is not.
        //!
        //! This method shall be implemented by real-time plugins and shall return
        //! true. The default implementation returns false.
        //!
        //! A plugin should be defined as real-time by design, not based on the
        //! interpretation of command-line parameters. Typically, the method
        //! isRealTime() is invoked before starting the plugin and consequently
        //! before the plugin has the opportunity to analyze its command-line
        //! parameters.
        //!
        //! @return True if the plugin usually requires real-time responsiveness.
        //!
        virtual bool isRealTime();

        //!
        //! Get the plugin type.
        //! @return The plugin type.
        //!
        virtual PluginType type() const = 0;

        //!
        //! Invoked when no packet could be retrieved within the specified timeout.
        //!
        //! For input plugins, this method is called when no space in input buffer
        //! can be found within the specified timeout.
        //!
        //! @return True if the application should continue to wait, false to abort.
        //! The default implementation aborts (but the default timeout is infinite).
        //!
        virtual bool handlePacketTimeout();

        //!
        //! Reset the internal TSDuck execution context of this plugin.
        //! This can be done to set default option values before getOptions() and start().
        //! This can also be done between stop() and start() to enforce a clean restart.
        //! @param [in] state Initial state to set or restore.
        //!
        void resetContext(const DuckContext::SavedArgs& state);

    protected:
        TSP* const  tsp;   //!< The TSP callback structure can be directly accessed by subclasses.
        DuckContext duck;  //!< The TSDuck context with various MPEG/DVB features.

        //!
        //! Constructor.
        //!
        //! @param [in] to_tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        Plugin(TSP* to_tsp, const UString& description = UString(), const UString& syntax = UString());
    };
}

//!
//! A convenience macro to declare constructors for plugin classes.
//! The default, copy and move constructors and assignments are explicitly deleted.
//! The only allowed constructor uses a TSP*.
//! @param classname Name of the enclosing plugin class.
//!
#define TS_PLUGIN_CONSTRUCTORS(classname)                                \
    public:                                                              \
        /** Constructor. */                                              \
        /** @param [in] tsp Associated callback to @c tsp executable. */ \
        classname(TSP* tsp);                                             \
    private:                                                             \
        classname() = delete;                                            \
        classname(classname&&) = delete;                                 \
        classname(const classname&) = delete;                            \
        classname& operator=(classname&&) = delete;                      \
        classname& operator=(const classname&) = delete
