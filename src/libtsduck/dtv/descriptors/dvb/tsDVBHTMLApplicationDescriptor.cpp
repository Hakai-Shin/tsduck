//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBHTMLApplicationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dvb_html_application_descriptor"
#define MY_CLASS    ts::DVBHTMLApplicationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_HTML_APP, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationDescriptor::DVBHTMLApplicationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::DVBHTMLApplicationDescriptor::clearContent()
{
    application_ids.clear();
    parameter.clear();
}

ts::DVBHTMLApplicationDescriptor::DVBHTMLApplicationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBHTMLApplicationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.pushWriteSequenceWithLeadingLength(8); // appid_set_length
    for (auto it : application_ids) {
        buf.putUInt16(it);
    }
    buf.popState(); // update appid_set_length
    buf.putString(parameter);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.pushReadSizeFromLength(8); // appid_set_length
    while (buf.canRead()) {
        application_ids.push_back(buf.getUInt16());
    }
    buf.popState(); // end of appid_set_length
    buf.getString(parameter);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    buf.pushReadSizeFromLength(8); // appid_set_length
    while (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Application id: %n", buf.getUInt16()) << std::endl;
    }
    buf.popState(); // end of appid_set_length
    disp << margin << "Parameter: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"parameter", parameter);
    for (auto it : application_ids) {
        root->addElement(u"application")->setIntAttribute(u"id", it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBHTMLApplicationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getAttribute(parameter, u"parameter", false) && element->getChildren(children, u"application");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint16_t id = 0;
        ok = children[i]->getIntAttribute(id, u"id", true);
        application_ids.push_back(id);
    }
    return ok;
}
