//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB AC-3_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DVB AC-3_descriptor.
    //! @see ETSI EN 300 468, D.3.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL DVBAC3Descriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        std::optional<uint8_t> component_type {};   //!< See ETSI EN 300 468, D.3.
        std::optional<uint8_t> bsid {};             //!< See ETSI EN 300 468, D.3.
        std::optional<uint8_t> mainid {};           //!< See ETSI EN 300 468, D.3.
        std::optional<uint8_t> asvc {};             //!< See ETSI EN 300 468, D.3.
        ByteBlock              additional_info {};  //!< See ETSI EN 300 468, D.3.

        //!
        //! Default constructor.
        //!
        DVBAC3Descriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBAC3Descriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;
        virtual bool merge(const AbstractDescriptor& desc) override;

        //!
        //! Name of an AC-3 Component Type.
        //! @param [in] type AC-3 Component Type.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        static UString ComponentTypeName(uint8_t type, NamesFlags flags = NamesFlags::NAME);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };

    //!
    //! Legacy name for a DVB AC-3_descriptor.
    //!
    using AC3Descriptor = DVBAC3Descriptor;
}
