//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Analyze the level of stuffing in tables.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsCASSelectionArgs.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class StuffAnalyzePlugin:
        public ProcessorPlugin,
        private TableHandlerInterface,
        private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(StuffAnalyzePlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Analysis context for a PID.
        class PIDContext
        {
        public:
            PIDContext() = default;          // Constructor.
            uint64_t total_sections = 0;     // Total number of sections.
            uint64_t stuffing_sections = 0;  // Number of stuffing sections.
            uint64_t total_bytes = 0;        // Total number of bytes in sections.
            uint64_t stuffing_bytes = 0;     // Total number of bytes in stuffing sections.

            // Format as a string.
            UString toString() const;
        };

        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID, PIDContextPtr>;

        // Plugin private fields.
        fs::path         _output_name {};    // Output file name
        std::ofstream    _output_stream {};  // Output file stream
        std::ostream*    _output = nullptr;  // Actual output stream
        CASSelectionArgs _cas_args {};       // CAS selection
        PIDSet           _analyze_pids {};   // List of PIDs to pass
        SectionDemux     _analyze_demux {duck, nullptr, this};  // Demux for sections to analyze for stuffing
        SectionDemux     _psi_demux {duck, this, nullptr};      // Demux for PSI tables parsing
        PIDContext       _total {};          // Global context.
        PIDContextMap    _pid_contexts {};   // Contexts of analyzed PID's.

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"stuffanalyze", ts::StuffAnalyzePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::StuffAnalyzePlugin::StuffAnalyzePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze the level of stuffing in tables", u"[options]")
{
    option(u"output-file", 'o', FILENAME);
    help(u"output-file",
         u"Specify the output text file for the analysis result. "
         u"By default, use the standard output.\n\n"
         u"Analyze the level of \"stuffing\" in sections in a list of selected PID's. "
         u"The PID's to analyze can be selected manually or using CAS criteria. "
         u"A section is considered as \"stuffing\" when its payload is filled with "
         u"the same byte value (all 0x00 or all 0xFF for instance).");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid",  u"pid1[-pid2]",
         u"Analyze all tables from these PID's. Several -p or --pid options may be specified.");

    // CAS filtering options.
    _cas_args.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::StuffAnalyzePlugin::start()
{
    // Get command line arguments
    _cas_args.loadArgs(duck, *this);
    getPathValue(_output_name, u"output-file");
    getIntValues(_analyze_pids, u"pid");

    // Initialize the PSI demux.
    _psi_demux.reset();
    if (_cas_args.pass_emm) {
        // To get the EMM PID's we need to analyze the CAT.
        _psi_demux.addPID(PID_CAT);
    }
    if (_cas_args.pass_ecm) {
        // To get the ECM PID's we need to analyze the PMT's.
        // To get the PMT PID's, we need to analyze the PAT.
        _psi_demux.addPID(PID_PAT);
    }

    // Initialize the demux which analyzes sections.
    _analyze_demux.setPIDFilter(_analyze_pids);

    // Create the output file.
    if (_output_name.empty()) {
        _output = &std::cout;
    }
    else {
        _output = &_output_stream;
        _output_stream.open(_output_name);
        if (!_output_stream) {
            error(u"cannot create file %s", _output_name);
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Format a PID context as a string.
//----------------------------------------------------------------------------

ts::UString ts::StuffAnalyzePlugin::PIDContext::toString() const
{
    return UString::Format(u"%10d %10d %10d %10d %9s",
                           total_sections, stuffing_sections, total_bytes, stuffing_bytes,
                           UString::Percentage(stuffing_bytes, total_bytes));
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::StuffAnalyzePlugin::stop()
{
    // Now it is time to produce the report.
    (*_output) << "Number of analyzed PID's: " << _analyze_pids.count() << std::endl
               << "PID's with sections:      " << _pid_contexts.size() << std::endl
               << std::endl
               << "PID             Sections (stuffing)      Bytes (stuffing) (percent)" << std::endl
               << "------------- ---------- ---------- ---------- ---------- ---------" << std::endl;

    for (const auto& it : _pid_contexts) {
        const PID pid = it.first;
        const PIDContextPtr& ctx(it.second);
        if (ctx != nullptr) {
            (*_output) << UString::Format(u"%4d (0x%04<X) ", pid) << ctx->toString() << std::endl;
        }
    }
    (*_output) << "Total         " << _total.toString() << std::endl;

    // Close output file
    if (!_output_name.empty()) {
        _output_stream.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete PSI table is available.
//----------------------------------------------------------------------------

void ts::StuffAnalyzePlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            // Add all PMT PID's to PSI demux.
            PAT pat(duck, table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                for (const auto& it : pat.pmts) {
                    _psi_demux.addPID(it.second);
                }
            }
            break;
        }

        case TID_CAT: {
            // Analyze stuffing on all required EMM PID's.
            CAT cat(duck, table);
            if (cat.isValid() && table.sourcePID() == PID_CAT) {
                PIDSet pids;
                _cas_args.addMatchingPIDs(pids, cat, *this);
                _analyze_demux.addPIDs(pids);
                _analyze_pids |= pids;
            }
            break;
        }

        case TID_PMT: {
            // Analyze stuffing on all required EMM PID's.
            PMT pmt(duck, table);
            if (pmt.isValid()) {
                PIDSet pids;
                _cas_args.addMatchingPIDs(pids, pmt, *this);
                _analyze_demux.addPIDs(pids);
                _analyze_pids |= pids;
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux for each section to analyze.
//----------------------------------------------------------------------------

void ts::StuffAnalyzePlugin::handleSection(SectionDemux& demux, const Section& section)
{
    // Locate the PID context.
    const PID pid = section.sourcePID();
    PIDContextPtr ctx(_pid_contexts[pid]);
    if (ctx == nullptr) {
        // First section on this PID, allocate a context.
        // Note that the new context becomes managed by the safe pointer (assignment magic).
        ctx = std::make_shared<PIDContext>();
        _pid_contexts[pid] = ctx;
    }

    // Count sizes.
    ctx->total_sections += 1;
    ctx->total_bytes += section.size();
    _total.total_sections += 1;
    _total.total_bytes += section.size();

    if (!section.hasDiversifiedPayload()) {
        // The section payload is full of identical values, all 00, all FF, whatever.
        // We consider this as a stuffing section.
        ctx->stuffing_sections += 1;
        ctx->stuffing_bytes += section.size();
        _total.stuffing_sections += 1;
        _total.stuffing_bytes += section.size();
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::StuffAnalyzePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _psi_demux.feedPacket(pkt);
    _analyze_demux.feedPacket(pkt);
    return TSP_OK;
}
