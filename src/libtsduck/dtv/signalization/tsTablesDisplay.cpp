//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsBinaryTable.h"
#include "tsPSIBuffer.h"
#include "tsSection.h"
#include "tsArgs.h"
#include "tsDescriptor.h"
#include "tsDescriptorList.h"
#include "tsATSCMultipleString.h"
#include "tsIntegerUtils.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesDisplay::defineArgs(Args& args)
{
    args.option(u"c-style", 'c');
    args.help(u"c-style",
              u"Same as --raw-dump (no interpretation of section) but dump the "
              u"bytes in C-language style.");

    args.option(u"nested-tlv", 0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.help(u"nested-tlv", u"min-size",
              u"With option --tlv, try to interpret the value field of each TLV record as "
              u"another TLV area. If the min-size value is specified, the nested TLV "
              u"interpretation is performed only on value fields larger than this size. "
              u"The syntax of the nested TLV is the same as the enclosing TLV.");

    args.option(u"raw-dump", 'r');
    args.help(u"raw-dump", u"Raw dump of section, no interpretation.");

    args.option(u"tlv", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"tlv", u"For sections of unknown types, this option specifies how to interpret "
              u"some parts of the section payload as TLV records. Several --tlv options "
              u"are allowed, each one describes a part of the section payload.\n\n"
              u"Each syntax string has the form \"start,size,tagSize,lengthSize,order\". "
              u"The start and size fields define the offset and size of the TLV area "
              u"in the section payload. If the size field is \"auto\", the TLV extends up "
              u"to the end of the section. If the start field is \"auto\", the longest "
              u"TLV area in the section payload will be used. The fields tagSize and "
              u"lengthSize indicate the size in bytes of the Tag and Length fields in "
              u"the TLV structure. The field order must be either \"msb\" or \"lsb\" and "
              u"indicates the byte order of the Tag and Length fields.\n\n"
              u"All fields are optional. The default values are \"auto,auto,1,1,msb\".");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TablesDisplay::loadArgs(DuckContext& duck, Args &args)
{
    _raw_dump = args.present(u"raw-dump");
    _raw_flags = UString::HEXA;
    if (args.present(u"c-style")) {
        _raw_dump = true;
        _raw_flags |= UString::C_STYLE;
    }

    // The --nested-tlv has an optional value.
    // If present without value, use 1, meaning all non-empty TLV records.
    // If not present, we use 0, which means no nested TLV.
    _min_nested_tlv = args.present(u"nested-tlv") ? args.intValue<size_t>(u"nested-tlv", 1) : 0;

    // Get all TLV syntax specifications.
    _tlv_syntax.clear();
    const size_t count = args.count(u"tlv");
    for (size_t i = 0; i < count; ++i) {
        TLVSyntax tlv;
        tlv.fromString(args.value(u"tlv", u"", i), args);
        _tlv_syntax.push_back(tlv);
    }
    std::sort(_tlv_syntax.begin(), _tlv_syntax.end());
    return true;
}


//----------------------------------------------------------------------------
// A utility method to dump extraneous bytes after expected data.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayExtraData(PSIBuffer& buf, const UString& margin)
{
    // Reset read error to restart at last read point.
    buf.clearReadError();
    displayExtraData(buf.currentReadAddress(), buf.remainingReadBytes(), margin);
    buf.skipBytes(buf.remainingReadBytes());
}

void ts::TablesDisplay::displayExtraData(const void* data, size_t size, const UString& margin)
{
    std::ostream& strm(_duck.out());
    if (size > 0) {
        strm << margin << "Extraneous " << size << " bytes:" << std::endl;
        strm << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET, margin.size());
    }
}


//----------------------------------------------------------------------------
// A utility method to dump private binary data in a descriptor or section.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayPrivateData(const UString& title, const void* data, size_t size, const UString& margin, size_t single_line_max)
{
    std::ostream& strm(_duck.out());

    if (size > single_line_max) {
        strm << margin << title << " (" << size << " bytes):" << std::endl;
        strm << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, margin.size() + 2, 16);
    }
    else if (size > 0) {
        strm << margin << title << " (" << size << " bytes): " << UString::Dump(data, size, UString::SINGLE_LINE) << std::endl;
    }
}

void ts::TablesDisplay::displayPrivateData(const UString& title, PSIBuffer& buf, size_t size, const UString& margin, size_t single_line_max)
{
    size = std::min(size, buf.remainingReadBytes());
    displayPrivateData(title, buf.currentReadAddress(), size, margin, single_line_max);
    buf.skipBytes(size);
}


//----------------------------------------------------------------------------
// A utility method to display and integer and optional ASCII interpretation.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayIntAndASCII(const UString& format, PSIBuffer& buf, size_t size, const UString& margin)
{
    // Filter input errors.
    size = std::min(size, buf.remainingReadBytes());
    if (buf.error()) {
        return;
    }

    // Try to interpret the data as ASCII.
    std::string ascii;
    const uint8_t* const data = buf.currentReadAddress();
    for (size_t i = 0; i < size; ++i) {
        if (data[i] >= 0x20 && data[i] <= 0x7E) {
            // This is an ASCII character.
            if (i == ascii.size()) {
                ascii.push_back(char(data[i]));
            }
            else {
                // But come after trailing zero.
                ascii.clear();
                break;
            }
        }
        else if (data[i] != 0) {
            // Not ASCII, not trailing zero, unusable string.
            ascii.clear();
            break;
        }
    }

    // Now display the data.
    _duck.out() << margin << UString::Format(format, buf.getBits<uint64_t>(8 * size));
    if (!ascii.empty()) {
        _duck.out() << " (\"" << ascii << "\")";
    }
    _duck.out() << std::endl;
}


//----------------------------------------------------------------------------
// Display a table on the output stream.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayTable(const BinaryTable& table, const UString& margin, CASID cas)
{
    std::ostream& strm(_duck.out());

    // Filter invalid tables
    if (!table.isValid()) {
        return;
    }

    // Accumulate standards.
    _duck.addStandards(table.definingStandards());

    // Display hexa dump of each section in the table
    if (_raw_dump) {
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            const Section& section(*table.sectionAt(i));
            strm << UString::Dump(section.content(), section.size(), _raw_flags | UString::BPL, margin.size(), 16) << std::endl;
        }
        return;
    }

    const TID tid = table.tableId();
    cas = _duck.casId(cas);

    // Compute total size of table
    size_t total_size = 0;
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        total_size += table.sectionAt(i)->size();
    }

    // Display common header lines.
    strm << margin << UString::Format(u"* %s, TID %n", TIDName(_duck, tid, cas), table.tableId());
    if (table.sourcePID() != PID_NULL) {
        // If PID is the null PID, this means "unknown PID"
        strm << UString::Format(u", PID %n", table.sourcePID());
    }
    strm << std::endl;
    if (table.sectionCount() == 1 && table.sectionAt(0)->isShortSection()) {
        strm << margin << "  Short section";
    }
    else {
        strm << margin << "  Version: " << int(table.version()) << ", sections: " << table.sectionCount();
    }
    strm << ", total size: " << total_size << " bytes" << std::endl;

    // Loop across all sections.
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        const SectionPtr section(table.sectionAt(i));
        strm << margin << "  - Section " << i;
        if (section->isNext()) {
            strm << ", next (not yet applicable)";
        }
        strm << ":" << std::endl;
        displaySection(*section, margin + u"    ", cas, true);
    }
}


//----------------------------------------------------------------------------
// Display a section on the output stream.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displaySection(const Section& section, const UString& margin, CASID cas, bool no_header)
{
    std::ostream& strm(_duck.out());

    // Filter invalid section
    if (!section.isValid()) {
        return;
    }

    // Accumulate standards.
    _duck.addStandards(section.definingStandards());

    // Display hexa dump of the section
    if (_raw_dump) {
        strm << UString::Dump(section.content(), section.size(), _raw_flags | UString::BPL, margin.size(), 16) << std::endl;
        return;
    }

    const TID tid = section.tableId();
    cas = _duck.casId(cas);
    UString extra_margin;

    // Display common header lines.
    if (!no_header) {
        strm << margin << UString::Format(u"* %s, TID %n", TIDName(_duck, tid, cas), tid);
        if (section.sourcePID() != PID_NULL) {
            // If PID is the null PID, this means "unknown PID"
            strm << UString::Format(u", PID %n", section.sourcePID());
        }
        strm << std::endl;
        if (section.isShortSection()) {
            strm << margin << "  Short section";
        }
        else {
            strm << margin << "  Section: " << int(section.sectionNumber())
                 << " (last: " << int(section.lastSectionNumber())
                 << "), version: " << int(section.version());
            if (section.isNext()) {
                strm << ", next (not yet applicable)";
            }
        }
        strm << ", size: " << section.size() << " bytes" << std::endl;
        extra_margin = u"  ";
    }

    // Validate reserved bits in the section header.
    std::vector<size_t> errors;
    const uint8_t byte1 = section.content()[1];
    // The private_indicator must be zero in an MPEG-defined table.
    if (section.tableId() <= TID_MPEG_LAST && (byte1 & 0x40) != 0) {
        errors.push_back((1 << 4) | (1 << 1) | 0);
    }
    // The private_indicator must be set in a DVB-defined table.
    // Other standards do not always follow the MPEG rules.
    if (bool(section.definingStandards() & Standards::DVB) && (byte1 & 0x40) == 0) {
        errors.push_back((1 << 4) | (1 << 1) | 1);
    }
    // Two reserved bits.
    if ((byte1 & 0x20) == 0) {
        errors.push_back((1 << 4) | (2 << 1) | 1);
    }
    if ((byte1 & 0x10) == 0) {
        errors.push_back((1 << 4) | (3 << 1) | 1);
    }
    if (section.isLongSection()) {
        const uint8_t byte5 = section.content()[5];
        // Two reserved bits.
        if ((byte5 & 0x80) == 0) {
            errors.push_back((5 << 4) | (0 << 1) | 1);
        }
        if ((byte5 & 0x40) == 0) {
            errors.push_back((5 << 4) | (1 << 1) | 1);
        }
    }
    if (!errors.empty()) {
        strm << margin << extra_margin << "Reserved bits incorrectly set in section header:" << std::endl;
        strm << Buffer::ReservedBitsErrorString(errors, 0, margin + extra_margin + u"  ") << std::endl;
    }

    // Display section body
    displaySectionData(section, margin + extra_margin, cas);
}


//----------------------------------------------------------------------------
// Display a section on the output stream.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displaySectionData(const Section& section, const UString& margin, CASID cas)
{
    // Find the display handler for this table id (and maybe CAS).
    const SectionContext context(section.sourcePID(), _duck.standards(), _duck.casId(cas));
    DisplaySectionFunction handler = PSIRepository::Instance().getTable(section.tableId(), context).display;

    if (handler != nullptr) {
        PSIBuffer buf(_duck, section.payload(), section.payloadSize());
        handler(*this, section, buf, margin);
        displayExtraData(buf, margin);
        if (buf.reservedBitsError()) {
            std::ostream& strm(_duck.out());
            strm << margin << "Reserved bits incorrectly set:" << std::endl;
            strm << buf.reservedBitsErrorString(section.headerSize(), margin + u"  ") << std::endl;
        }
    }
    else {
        displayUnkownSectionData(section, margin);
    }
}


//----------------------------------------------------------------------------
// Log a line, either on redirected output or on report if output was not redirected.
//----------------------------------------------------------------------------

void ts::TablesDisplay::logLine(const UString& line)
{
    if (_duck.redirectedOutput()) {
        // The output has been redirected, use it.
        _duck.out() << line << std::endl;
    }
    else {
        // Use the report object for logging.
        _duck.report().info(line);
    }
}


//----------------------------------------------------------------------------
// Display the payload of a section on the output stream as a one-line "log" message.
//----------------------------------------------------------------------------

void ts::TablesDisplay::logSectionData(const Section& section, const UString& header, size_t max_bytes, CASID cas)
{
    // Find the log handler for this table id (and maybe CAS).
    const SectionContext context(section.sourcePID(), _duck.standards(), _duck.casId(cas));
    LogSectionFunction handler = PSIRepository::Instance().getTable(section.tableId(), context).log;
    if (handler == nullptr) {
        handler = LogUnknownSectionData;
    }

    // Output exactly one line.
    logLine(header + handler(section, max_bytes));
}


//----------------------------------------------------------------------------
// Log the content of an unknown section.
//----------------------------------------------------------------------------

ts::UString ts::TablesDisplay::LogUnknownSectionData(const Section& section, size_t max_bytes)
{
    // Number of bytes to log.
    size_t log_size = section.payloadSize();
    if (max_bytes > 0 && max_bytes < log_size) {
        log_size = max_bytes;
    }

    // Build log line.
    return UString::Dump(section.payload(), log_size, UString::SINGLE_LINE) + (section.payloadSize() > log_size ? u" ..." : u"");
}


//----------------------------------------------------------------------------
// Display an invalid section on the output stream.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayInvalidSection(const DemuxedData& data, const UString& reason, const UString& margin, CASID cas, bool no_header)
{
    std::ostream& strm(_duck.out());

    // Display hexa dump of the section
    if (_raw_dump) {
        strm << UString::Dump(data.content(), data.size(), _raw_flags | UString::BPL, margin.size(), 16) << std::endl;
        return;
    }

    const TID tid = data.size() > 0 ? data.content()[0] : TID(TID_NULL);
    cas = _duck.casId(cas);

    // Display common header lines.
    if (!no_header) {
        strm << margin << "* Invalid section";
        if (!reason.empty()) {
            strm << ", " << reason;
        }
        strm << std::endl << margin << "  ";
        if (tid != TID_NULL) {
            strm << UString::Format(u"%s, TID %n, ", TIDName(_duck, tid, cas), tid);
        }
        if (data.sourcePID() != PID_NULL) {
            strm << UString::Format(u"PID %n, ", data.sourcePID());
        }
        strm << UString::Format(u"%'d bytes:", data.size()) << std::endl;
    }

    // Display invalid section data
    strm << UString::Dump(data.content(), data.size(), UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, margin.size() + 4, 16);
}


//----------------------------------------------------------------------------
// Display the content of an unknown descriptor.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownDescriptor(DID did, const uint8_t * payload, size_t size, const UString& margin)
{
    _duck.out() << UString::Dump(payload, size, UString::HEXA | UString::ASCII | UString::OFFSET, margin.size());
}


//----------------------------------------------------------------------------
// Display an unknown section
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownSectionData(const ts::Section& section, const UString& margin)
{
    std::ostream& strm(_duck.out());

    // The table id extension was not yet displayed since it depends on the table id.
    if (section.isLongSection()) {
        strm << margin << UString::Format(u"TIDext: %n", section.tableIdExtension()) << std::endl;
    }

    // Section payload.
    const uint8_t* const payload = section.payload();
    const size_t payloadSize = section.payloadSize();

    // Current index to display in payload.
    size_t index = 0;

    // Loop on all possible TLV syntaxen.
    for (auto it = _tlv_syntax.begin(); it != _tlv_syntax.end() && index < payloadSize; ++it) {

        // Can we locate a TLV area after current index?
        size_t tlvStart = 0;
        size_t tlvSize = 0;
        if (it->locateTLV(payload, payloadSize, tlvStart, tlvSize) && tlvStart >= index && tlvSize > 0) {

            // Display TLV fields, from index to end of TLV area.
            const size_t endIndex = index + tlvStart + tlvSize;
            displayTLV(payload + index,    // start of area to display
                       tlvStart - index,   // offset of TLV records in area to display
                       tlvSize,            // total size of TLV records
                       index,              // offset to display for start of area
                       margin.size(),      // left margin
                       0,                  // inner margin
                       *it);               // TLV syntax
            index = endIndex;

            // Display a separator after TLV area.
            if (index < payloadSize) {
                strm << margin << UString::Format(u"%04X:  End of TLV area", index) << std::endl;
            }
        }
    }

    // Display remaining binary data.
    strm << UString::Dump(payload + index, payloadSize - index, UString::HEXA | UString::ASCII | UString::OFFSET, margin.size(), UString::DEFAULT_HEXA_LINE_WIDTH, index);
}


//----------------------------------------------------------------------------
// Display a memory area containing a list of TLV records.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayTLV(const uint8_t* data,
                                   size_t tlvStart,
                                   size_t tlvSize,
                                   size_t dataOffset,
                                   size_t indent,
                                   size_t innerIndent,
                                   const TLVSyntax& tlv)
{
    std::ostream& strm(_duck.out());

    // We use the same syntax for the optional embedded TLV, except that it is automatically located.
    TLVSyntax tlvInner(tlv);
    tlvInner.setAutoLocation();

    // Display binary data preceding TLV, from data to data + tlvStart.
    strm << UString::Dump(data, tlvStart, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, dataOffset, innerIndent);

    // Display TLV fields, from data + tlvStart to data + tlvStart + tlvSize.
    size_t index = tlvStart;
    const size_t endIndex = tlvStart + tlvSize;
    while (index < endIndex) {

        // Get TLV header (tag, length)
        uint32_t tag = 0;
        size_t valueSize = 0;
        const size_t headerSize = tlv.getTagAndLength(data + index, endIndex - index, tag, valueSize);
        if (headerSize == 0 || index + headerSize + valueSize > endIndex) {
            break; // no more TLV record
        }

        // Location of value area.
        const uint8_t* const value = data + index + headerSize;
        const size_t valueOffset = dataOffset + index + headerSize;

        // Description of the TLV record.
        strm << UString::Format(u"%*s%04X:  %*sTag: %*d (0x%0*X), length: %*d bytes, value: ",
                                indent, u"",
                                dataOffset + index,
                                innerIndent, u"",
                                MaxDecimalWidth(tlv.getTagSize()), tag,
                                MaxHexaWidth(tlv.getTagSize()), tag,
                                MaxDecimalWidth(tlv.getLengthSize()), valueSize);

        // Display the value field.
        size_t tlvInnerStart = 0;
        size_t tlvInnerSize = 0;
        if (_min_nested_tlv > 0 && valueSize >= _min_nested_tlv && tlvInner.locateTLV(value, valueSize, tlvInnerStart, tlvInnerSize)) {
            // Found a nested TLV area.
            strm << std::endl;
            displayTLV(value, tlvInnerStart, tlvInnerSize, valueOffset, indent, innerIndent + 2, tlvInner);
        }
        else if (valueSize <= 8) {
            // If value is short, display it on the same line.
            strm << UString::Dump(value, valueSize, UString::HEXA | UString::SINGLE_LINE) << std::endl;
        }
        else {
            strm << std::endl
                 << UString::Dump(value, valueSize, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, valueOffset, innerIndent + 2);
        }

        // Point after current TLV record.
        index += headerSize + valueSize;
    }

    // Display a separator after TLV area.
    if (index > tlvStart && index < endIndex) {
        strm << UString::Format(u"%*s%04X:  %*sEnd of TLV area", indent, u"", index, innerIndent, u"") << std::endl;
    }

    // Display remaining binary data.
    strm << UString::Dump(data + index, endIndex - index, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, dataOffset + index, innerIndent);
}


//----------------------------------------------------------------------------
// Display a descriptor on the output stream.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayDescriptor(const Descriptor& desc, DescriptorContext& context, const UString& margin)
{
    if (desc.isValid()) {
        displayDescriptorData(desc, context, margin);
    }
}


//----------------------------------------------------------------------------
// Display a list of descriptors from a PSI Buffer
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayDescriptorList(const Section& section,
                                              DescriptorContext& context,
                                              bool top_level,
                                              PSIBuffer& buf,
                                              const UString& margin,
                                              const UString& title,
                                              const UString& empty_text,
                                              size_t length)
{
    if (length == NPOS) {
        length = buf.remainingReadBytes();
    }
    if (!buf.readIsByteAligned() || length > buf.remainingReadBytes()) {
        buf.setUserError();
    }
    else if (!buf.error()) {
        if (!title.empty() && (length > 0 || !empty_text.empty())) {
            _duck.out() << margin << title << std::endl;
        }
        if (length > 0) {
            displayDescriptorList(section, context, top_level, buf.currentReadAddress(), length, margin);
            buf.skipBytes(length);
        }
        else if (!empty_text.empty()) {
            _duck.out() << margin << "- " << empty_text << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display a list of descriptors (with preceding length) from a PSI buffer
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayDescriptorListWithLength(const Section& section,
                                                        DescriptorContext& context,
                                                        bool top_level,
                                                        PSIBuffer& buf,
                                                        const UString& margin,
                                                        const UString& title,
                                                        const UString& empty_text,
                                                        size_t length_bits)
{
    const size_t length = buf.getUnalignedLength(length_bits);
    displayDescriptorList(section, context, top_level, buf, margin, title, empty_text, length);
}


//----------------------------------------------------------------------------
// Display a list of descriptors from a memory area
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayDescriptorList(const Section& section, DescriptorContext& context, bool top_level, const void* data, size_t size, const UString& margin)
{
    std::ostream& strm(_duck.out());
    const uint8_t* const initial_start = reinterpret_cast<const uint8_t*>(data);
    const size_t initial_size = size;
    const uint8_t* desc_start = initial_start;
    size_t desc_index = 0;

    // Loop across all descriptors
    while (size >= 2) {  // descriptor header size

        // Set context analysis up to the current descriptor.
        context.setCurrentRawDescriptorList(initial_start, desc_start - initial_start);

        // Get descriptor header
        const uint8_t desc_tag = desc_start[0];
        const size_t desc_length = desc_start[1];
        desc_start += 2;
        size -= 2;

        // Check that the descriptor is complete. Abort descriptor list if not.
        if (size < desc_length) {
            strm << margin << "- Invalid descriptor length: " << desc_length << " (" << size << " bytes allocated)" << std::endl;
            break;
        }

        // Build a descriptor object.
        Descriptor desc(desc_start - 2, 2 + desc_length);

        // Display descriptor header
        strm << margin << "- Descriptor " << desc_index++ << ": "
             << DIDName(desc_tag, context, NamesFlags::NAME_VALUE | NamesFlags::HEX_DEC) << ", "
             << desc_length << " bytes" << std::endl;

        // Display descriptor.
        displayDescriptorData(desc, context, margin + u"  ");

        // Move to next descriptor for next iteration
        desc_start += desc_length;
        size -= desc_length;
    }

    // Report extraneous bytes
    displayExtraData(desc_start, size, margin);

    // If the descriptor list is the top level list of its table, declare it now in the context.
    // It will be used to track REGID's in lower-level descriptor lists.
    if (top_level) {
        context.setTopLevelRawDescriptorList(initial_start, initial_size);
        context.setCurrentRawDescriptorList(nullptr, 0);
    }
}


//----------------------------------------------------------------------------
// Display a dlist of descriptors.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayDescriptorList(const DescriptorList& dlist, DescriptorContext& context, const UString& margin)
{
    std::ostream& strm(_duck.out());

    for (size_t i = 0; i < dlist.count(); ++i) {
        const Descriptor& desc(dlist[i]);
        context.setCurrentDescriptorList(&dlist, i);
        strm << margin << "- Descriptor " << i << ": "
             << DIDName(desc.tag(), context, NamesFlags::NAME_VALUE | NamesFlags::HEX_DEC) << ", "
             << desc.size() << " bytes" << std::endl;
        displayDescriptor(desc, context, margin + u"  ");
    }
}


//----------------------------------------------------------------------------
// Display a descriptor on the output stream.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayDescriptorData(const Descriptor& desc, DescriptorContext& context, const UString& margin)
{
    std::ostream& strm(_duck.out());

    const uint8_t* payload = desc.payload();
    size_t size = desc.payloadSize();
    size_t header_size = 2;

    if (desc.tag() == DID_MPEG_EXTENSION && size >= 1) {
        // MPEG extension descriptor, the extension id is in the first byte of the payload.
        const uint8_t ext = *payload++;
        header_size++;
        size--;
        // Display extended descriptor header
        strm << margin << "MPEG extended descriptor: " << XDIDNameMPEG(ext, NamesFlags::NAME_VALUE | NamesFlags::HEX_DEC) << std::endl;
    }
    else if (desc.tag() == DID_DVB_EXTENSION && size >= 1) {
        // Extension descriptor, the extension id is in the first byte of the payload.
        const uint8_t ext = *payload++;
        header_size++;
        size--;
        // Display extended descriptor header
        strm << margin << "Extended descriptor: " << XDIDNameDVB(ext, NamesFlags::NAME_VALUE | NamesFlags::HEX_DEC) << std::endl;
    }

    // Locate the display handler for this descriptor payload.
    DisplayDescriptorFunction handler = PSIRepository::Instance().getDescriptor(desc.xdid(), context).display;
    if (handler != nullptr) {
        PSIBuffer buf(_duck, payload, size);
        handler(*this, desc, buf, margin, context);
        displayExtraData(buf, margin);
        if (buf.reservedBitsError()) {
            strm << margin << "Reserved bits incorrectly set:" << std::endl;
            strm << buf.reservedBitsErrorString(header_size, margin + u"  ") << std::endl;
        }
    }
    else {
        displayUnkownDescriptor(desc.tag(), payload, size, margin);
    }
}


//----------------------------------------------------------------------------
// Display a CRC32 from a section.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayCRC32(const Section& section, const UString& margin)
{
    std::ostream& strm(_duck.out());
    const uint32_t sect_crc32 = GetUInt32(section.content() + section.size() - 4);
    const CRC32 comp_crc32(section.content(), section.size() - 4);

    strm << margin << UString::Format(u"CRC32: 0x%X ", sect_crc32);
    if (sect_crc32 == comp_crc32) {
        strm << "(OK)";
    }
    else {
        strm << UString::Format(u"(WRONG, expected 0x%X)", comp_crc32.value());
    }
    strm << std::endl;
}

void ts::TablesDisplay::displayCRC32(const Section& section, PSIBuffer& buf, const UString& margin)
{
    if (!buf.error() && buf.remainingReadBytes() == 4) {
        displayCRC32(section, margin);
        buf.skipBytes(4);
    }
}


//----------------------------------------------------------------------------
// Display an ATSC multiple_string_structure() from a PSI buffer.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayATSCMultipleString(PSIBuffer& buf, size_t length_bytes, const UString& margin, const UString& title)
{
    if (buf.error() || !buf.readIsByteAligned() || length_bytes > 8) {
        buf.setUserError();
        return;
    }

    // Get maximum size of structure.
    size_t mss_size = NPOS;
    if (length_bytes > 0) {
        mss_size = buf.getBits<size_t>(8 * length_bytes);
        if (buf.error()) {
            return;
        }
    }

    // These pointers will be updated by Display().
    const uint8_t* data = buf.currentReadAddress();
    const size_t initial_size = buf.remainingReadBytes();
    size_t size = initial_size;
    ATSCMultipleString::Display(*this, title, margin, data, size, mss_size);

    // Adjust read pointer after the structure.
    assert(size <= initial_size);
    buf.skipBytes(initial_size - size);
}


//----------------------------------------------------------------------------
// Display the 32-bit values in a structured manner with specified number of
// items on each line
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayVector(const UString& title, const std::vector<uint32_t>& values, const UString& margin, bool space_first, size_t num_per_line)
{
    if (!values.empty()) {
        std::ostream& strm(_duck.out());
        UString myMargin(margin.length() + title.length(), ' ');
        strm << margin << title;
        for (size_t j = 0; j < values.size(); j++) {
            strm << (space_first ? " " : "") << UString::Format(u"%08X",  values[j] );
            if ((j + 1) % num_per_line == 0) {
                strm << std::endl;
                if (j != (values.size() - 1)) {
                    strm << myMargin;
                }
            }
        }
        if (values.size() % num_per_line != 0) {
            strm << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display the 16-bit values in a structured manner with specified number of
// items on each line
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayVector(const UString& title, const std::vector<uint16_t>& values, const UString& margin, bool space_first, size_t num_per_line)
{
    if (!values.empty()) {
        std::ostream& strm(_duck.out());
        UString myMargin(margin.length() + title.length(), ' ');
        strm << margin << title;
        for (size_t j = 0; j < values.size(); j++) {
            strm << (space_first ? " " : "") << UString::Format(u"%04X",  values[j] );
            if ((j + 1) % num_per_line == 0) {
                strm << std::endl;
                if (j != (values.size() - 1)) {
                    strm << myMargin;
                }
            }
        }
        if (values.size() % num_per_line != 0) {
            strm << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display the 8-bit values in a structured manner with specified number of
// items on each line
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayVector(const UString& title, const std::vector<uint8_t>& values, const UString& margin, bool space_first, size_t num_per_line)
{
    if (!values.empty()) {
        std::ostream& strm(_duck.out());
        UString myMargin(margin.length() + title.length(), ' ');
        strm << margin << title;
        for (size_t j = 0; j < values.size(); j++) {
            strm << (space_first ? " " : "") << UString::Format(u"%02X",  values[j] );
            if ((j + 1) % num_per_line == 0) {
                strm << std::endl;
                if (j != (values.size() - 1)) {
                    strm << myMargin;
                }
            }
        }
        if (values.size() % num_per_line != 0) {
            strm << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display the signed 8-bit values in a structured manner with specified
// number of items on each line
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayVector(const UString& title, const std::vector<int8_t>& values, const UString& margin, bool space_first, size_t num_per_line)
{
    if (!values.empty()) {
        std::ostream& strm(_duck.out());
        UString myMargin(margin.length() + title.length(), ' ');
        bool hasNegative = false;
        for (size_t i = 0; !hasNegative && i < values.size(); i++) {
            if (values[i] < 0) { hasNegative = true; }
        }
        strm << margin << title;
        for (size_t j = 0; j < values.size(); j++) {
            strm << (space_first ? " " : "") << UString::Format(u"%d",  values[j] ).toJustifiedRight(hasNegative?4:3);
            if ((j + 1) % num_per_line == 0) {
                strm << std::endl;
                if (j != (values.size() - 1)) {
                    strm << myMargin;
                }
            }
        }
        if (values.size() % num_per_line != 0) {
            strm << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display boolean values in a structured manner using the characters
// specified and with the specified number of items on each line
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayVector(const UString& title, const std::vector<bool>& values, const UString& margin, bool space_first, size_t num_per_line, char true_val,
    char false_val)
{
    if (!values.empty()) {
        std::ostream& strm(_duck.out());
        UString myMargin(margin.length() + title.length(), ' ');
        strm << margin << title;
        for (size_t j = 0; j < values.size(); j++) {
            strm << (space_first ? " " : "") << (values[j] ? true_val : false_val);
            if ((j + 1) % num_per_line == 0) {
                strm << std::endl;
                if (j != (values.size() - 1)) {
                    strm << myMargin;
                }
            }
        }
        if (values.size() % num_per_line != 0) {
            strm << std::endl;
        }
    }
}

//----------------------------------------------------------------------------
// Display string values in a tabular manner with the values being left
// aligned and with the specified number of items on each line
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayVector(const UString& title, const UStringVector& values, const UString& margin, bool space_first, size_t num_per_line)
{
    if (!values.empty()) {
        size_t _maxlen = 0;
        for (const auto& i : values) {
            if (i.length() > _maxlen) {
                _maxlen = i.length();
            }
        }

        std::ostream& strm(_duck.out());
        UString myMargin(margin.length() + title.length(), ' ');
        strm << margin << title;
        for (size_t j = 0; j < values.size(); j++) {
            strm << (space_first ? " " : "") << values[j].toJustifiedLeft(_maxlen);
            if ((j + 1) % num_per_line == 0) {
                strm << std::endl;
                if (j != (values.size() - 1)) {
                    strm << myMargin;
                }
            }
        }
        if (values.size() % num_per_line != 0) {
            strm << std::endl;
        }
    }
}
