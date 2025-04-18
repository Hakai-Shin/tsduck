//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface to receive invalid MPEG section from a SectionDemux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSection.h"

namespace ts {

    class SectionDemux;

    //!
    //! Abstract interface to receive an invalid MPEG section from a SectionDemux.
    //! @ingroup libtsduck mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of the content of invalid sections using a SectionDemux.
    //!
    class TSDUCKDLL InvalidSectionHandlerInterface
    {
        TS_INTERFACE(InvalidSectionHandlerInterface);
    public:
        //!
        //! This hook is invoked when an invalid section is detected.
        //! @param [in,out] demux The demux which sends the section.
        //! @param [in] data The invalid section from the demux.
        //! @param [in] status Reason for the section to be invalid.
        //!
        virtual void handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status) = 0;
    };
}
