//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for ARIB character sets.
//
//----------------------------------------------------------------------------

#include "tsARIBCharset.h"
#include "tsByteBlock.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ARIBCharsetTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testDecode();
    void testEncode();
    void testCanEncode();

    TSUNIT_TEST_BEGIN(ARIBCharsetTest);
    TSUNIT_TEST(testDecode);
    TSUNIT_TEST(testEncode);
    TSUNIT_TEST(testCanEncode);
    TSUNIT_TEST_END();

private:
    void dump(const ts::UString& title, const ts::ByteBlock& bb);
    void dump(const ts::UString& title, const ts::UString& str);
};

TSUNIT_REGISTER(ARIBCharsetTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ARIBCharsetTest::beforeTest()
{
}

// Test suite cleanup method.
void ARIBCharsetTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Dump routines in debug mode.
//----------------------------------------------------------------------------

void ARIBCharsetTest::dump(const ts::UString& title, const ts::ByteBlock& bb)
{
    if (debugMode()) {
        debug() << title.toJustifiedLeft(9) << ts::UString::Dump(bb, ts::UString::SINGLE_LINE) << std::endl;
    }
}

void ARIBCharsetTest::dump(const ts::UString& title, const ts::UString& str)
{
    if (debugMode()) {
        debug() << title.toJustifiedLeft(9) << '"' << str << '"' << std::endl << "UTF-16: ";
        for (size_t i = 0; i < str.size(); ++i) {
            debug() << ts::UString::Format(u" %X", {uint16_t(str[i])});
        }
        debug() << std::endl;
    }
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void ARIBCharsetTest::testCanEncode()
{
    const ts::Charset& cset(ts::ARIBCharset::B24);

    TSUNIT_ASSERT(cset.canEncode(u""));
    TSUNIT_ASSERT(cset.canEncode(u"alpha num 09"));
    TSUNIT_ASSERT(cset.canEncode(ts::UString({u'a', u'b'})));
    TSUNIT_ASSERT(!cset.canEncode(ts::UString({u'a', ts::LATIN_SMALL_LETTER_A_WITH_ACUTE, u'b'})));
    TSUNIT_ASSERT(cset.canEncode(ts::UString({0x004E, 0x0048, 0x004B, 0x7DCF, 0x5408, 0x0031, 0x30FB, 0x79CB, 0x7530})));
    TSUNIT_ASSERT(cset.canEncode(ts::UString({0x004E, 0x30E5, 0x30FC, 0xD83C, 0xDE14, 0xD83C, 0xDE11, 0x7DCF})));
    TSUNIT_ASSERT(!cset.canEncode(ts::UString({0x004E, 0x30E5, 0x30FC, 0xDBFF, 0xDFFF, 0xD83C, 0xDE11, 0x7DCF})));
}

void ARIBCharsetTest::testDecode()
{
    const ts::Charset& cset(ts::ARIBCharset::B24);

#define B(N,...) const ts::ByteBlock b##N({__VA_ARGS__})
#define U(N,...) const ts::UString   u##N({__VA_ARGS__})
#define T(N, success)                                                \
    dump(u"Input:", b##N);                                           \
    ts::UString dec##N;                                              \
    const bool s##N = cset.decode(dec##N, b##N.data(), b##N.size()); \
    dump(u"Decoded:", dec##N);                                       \
    dump(u"Expected:", u##N);                                        \
    debug() << std::endl;                                            \
    TSUNIT_EQUAL(success, s##N);                                     \
    TSUNIT_EQUAL(u##N, dec##N)

    debug() << std::endl;

    B(1, 0x0E, 0x4E, 0x48, 0x4B, 0x0F, 0x41, 0x6D, 0x39, 0x67, 0x0E, 0x31, 0xFE, 0x0F, 0x3D, 0x29, 0x45, 0x44);
    U(1, 0x004E, 0x0048, 0x004B, 0x7DCF, 0x5408, 0x0031, 0x30FB, 0x79CB, 0x7530);
    T(1, true);

    B(2, 0x0E, 0x4E, 0x48, 0x4B, 0x0F, 0x41, 0x6D, 0x39, 0x67, 0x0E, 0x32, 0xFE, 0x0F, 0x3D, 0x29, 0x45, 0x44);
    U(2, 0x004E, 0x0048, 0x004B, 0x7DCF, 0x5408, 0x0032, 0x30FB, 0x79CB, 0x7530);
    T(2, true);

    B(3, 0x0E, 0x4E, 0x48, 0x4B, 0x0F, 0x37, 0x48, 0x42, 0x53, 0x0E, 0x47, 0xFE, 0x0F, 0x3D, 0x29, 0x45, 0x44);
    U(3, 0x004E, 0x0048, 0x004B, 0x643A, 0x5E2F, 0x0047, 0x30FB, 0x79CB, 0x7530);
    T(3, true);

    B(4, 0x0E, 0x4E, 0x48, 0x4B, 0x45, 0x1D, 0x46, 0x1D, 0x6C, 0x31, 0x0F, 0x3D, 0x29, 0x45, 0x44);
    U(4, 0x004E, 0x0048, 0x004B, 0x0045, 0x30C6, 0x30EC, 0x0031, 0x79CB, 0x7530);
    T(4, true);

    B(5, 0x0E, 0x4E, 0x48, 0x4B, 0x45, 0x1D, 0x46, 0x1D, 0x6C, 0x32, 0x0F, 0x3D, 0x29, 0x45, 0x44);
    U(5, 0x004E, 0x0048, 0x004B, 0x0045, 0x30C6, 0x30EC, 0x0032, 0x79CB, 0x7530);
    T(5, true);

    B(6, 0x0E, 0x4E, 0x48, 0x4B, 0x45, 0x1D, 0x46, 0x1D, 0x6C, 0x33, 0x0F, 0x3D, 0x29, 0x45, 0x44);
    U(6, 0x004E, 0x0048, 0x004B, 0x0045, 0x30C6, 0x30EC, 0x0033, 0x79CB, 0x7530);
    T(6, true);

    B(7, 0x0E, 0x4E, 0x48, 0x4B, 0x0F, 0x37, 0x48, 0x42, 0x53, 0x0E, 0x32);
    U(7, 0x004E, 0x0048, 0x004B, 0x643A, 0x5E2F, 0x0032);
    T(7, true);

    B(8, 0x3D, 0x29, 0x45, 0x44, 0x44, 0x2B, 0x46, 0x7C, 0x4A, 0x7C, 0x41, 0x77);
    U(8, 0x79CB, 0x7530, 0x671D, 0x65E5, 0x653E, 0x9001);
    T(8, true);

    B(9, 0x0E, 0x41, 0x42, 0x53, 0x0F, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0x0E, 0x31);
    U(9, 0x0041, 0x0042, 0x0053, 0x79CB, 0x7530, 0x653E, 0x9001, 0x0031);
    T(9, true);

    B(10, 0x0E, 0x41, 0x42, 0x53, 0x0F, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0x0E, 0x32);
    U(10, 0x0041, 0x0042, 0x0053, 0x79CB, 0x7530, 0x653E, 0x9001, 0x0032);
    T(10, true);

    B(11, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0x37, 0x48, 0x42, 0x53);
    U(11, 0x79CB, 0x7530, 0x653E, 0x9001, 0x643A, 0x5E2F);
    T(11, true);

    B(12, 0x0E, 0x41, 0x42, 0x53, 0x0F, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0x4E, 0x57, 0x3B, 0x7E);
    U(12, 0x0041, 0x0042, 0x0053, 0x79CB, 0x7530, 0x653E, 0x9001, 0x81E8, 0x6642);
    T(12, true);

    B(13, 0x0E, 0x41, 0x4B, 0x54, 0x0F, 0x3D, 0x29, 0x45, 0x44, 0x1B, 0x7C, 0xC6, 0xEC, 0xD3, 0x0E, 0x31);
    U(13, 0x0041, 0x004B, 0x0054, 0x79CB, 0x7530, 0x30C6, 0x30EC, 0x30D3, 0x0031);
    T(13, true);

    B(14, 0x0E, 0x41, 0x4B, 0x54, 0x0F, 0x3D, 0x29, 0x45, 0x44, 0x1B, 0x7C, 0xC6, 0xEC, 0xD3, 0x0E, 0x32);
    U(14, 0x0041, 0x004B, 0x0054, 0x79CB, 0x7530, 0x30C6, 0x30EC, 0x30D3, 0x0032);
    T(14, true);

    B(15, 0x0E, 0x41, 0x4B, 0x54, 0x0F, 0x4E, 0x57, 0x3B, 0x7E);
    U(15, 0x0041, 0x004B, 0x0054, 0x81E8, 0x6642);
    T(15, true);

    B(16, 0x0E, 0x47, 0x1B, 0x7C, 0xAC, 0xA4, 0xC9);
    U(16, 0x0047, 0x30AC, 0x30A4, 0x30C9);
    T(16, true);

    B(17, 0x0E, 0x41, 0x4B, 0x54, 0x0F, 0x3D, 0x29, 0x45, 0x44, 0x1B, 0x7C, 0xC6, 0xEC, 0xD3);
    U(17, 0x0041, 0x004B, 0x0054, 0x79CB, 0x7530, 0x30C6, 0x30EC, 0x30D3);
    T(17, true);

    B(18, 0x0E, 0x4E, 0x48, 0x4B, 0x1D, 0x4B, 0x1D, 0x65, 0xF9, 0x1D, 0x39, 0x37, 0x0F, 0x1B, 0x24, 0x3B, 0x7A, 0x5A, 0x7A, 0x56);
    U(18, 0x004E, 0x0048, 0x004B, 0x30CB, 0x30E5, 0x30FC, 0x30B9, 0x0037, 0xD83C, 0xDE14, 0xD83C, 0xDE11);
    T(18, true);

    B(19, 0x4C, 0x6B, 0x0E, 0x37, 0x0F, 0x3B, 0x7E, 0xFD, 0xFB, 0x30, 0x6C, 0x4A, 0x62, 0x40, 0x68, 0xD8,
          0xFD, 0x30, 0x6C, 0x4A, 0x62, 0x3F, 0x3C, 0xAF, 0xFC, 0x21, 0x21, 0x3A, 0x23, 0xFD, 0xB3, 0xCE,
          0x1D, 0x4B, 0x1D, 0x65, 0xF9, 0x1D, 0x39, 0xF2, 0x46, 0x4F, 0xB1, 0xBF, 0xA4, 0x21, 0x21, 0x21,
          0x5A, 0x1B, 0x7C, 0xAD, 0xE3, 0xB9, 0xBF, 0xF9, 0x21, 0x5B, 0x40, 0x44, 0x30, 0x66, 0x3C, 0x42,
          0x21, 0x24, 0x21, 0x5A, 0xB5, 0xD6, 0xAD, 0xE3, 0xB9, 0xBF, 0xF9, 0x21, 0x5B, 0x43, 0x53, 0x45,
          0x44, 0x3F, 0x2D, 0x3B, 0x52, 0x21, 0x24, 0x30, 0x4B, 0x46, 0x23, 0x33, 0x24, 0x49, 0x27, 0x21,
          0x24, 0x21, 0x5A, 0x35, 0x24, 0x3E, 0x5D, 0xAD, 0xE3, 0xB9, 0xBF, 0xF9, 0x21, 0x5B, 0x43, 0x66,
          0x42, 0x3C, 0x48, 0x7E, 0x38, 0x78);
    U(19, 0x591C, 0x0037, 0x6642, 0x3001, 0x300C, 0x4E00, 0x6B69, 0x5148, 0x3078, 0x3001, 0x4E00, 0x6B69,
          0x6DF1, 0x304F, 0x300D, 0x3000, 0x4ECA, 0x3001, 0x3053, 0x306E, 0x30CB, 0x30E5, 0x30FC, 0x30B9,
          0x3092, 0x5C4A, 0x3051, 0x305F, 0x3044, 0x3000, 0x3010, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC,
          0x3011, 0x9752, 0x4E95, 0x5B9F, 0xFF0C, 0x3010, 0x30B5, 0x30D6, 0x30AD, 0x30E3, 0x30B9, 0x30BF,
          0x30FC, 0x3011, 0x6C60, 0x7530, 0x4F38, 0x5B50, 0xFF0C, 0x4F0A, 0x85E4, 0x6D77, 0x5F66, 0xFF0C,
          0x3010, 0x6C17, 0x8C61, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC, 0x3011, 0x4E2D, 0x6751, 0x7F8E,
          0x516C);
    T(19, true);

    B(20, 0x3D, 0x50, 0x31, 0x69, 0x3C, 0x54);
    U(20, 0x51FA, 0x6F14, 0x8005);
    T(20, true);

    B(21, 0x21, 0x5A, 0x1B, 0x7C, 0xAD, 0xE3, 0xB9, 0xBF, 0xF9, 0x21, 0x5B, 0x40, 0x44, 0x30, 0x66, 0x3C,
          0x42, 0x21, 0x24, 0x21, 0x5A, 0xB5, 0xD6, 0xAD, 0xE3, 0xB9, 0xBF, 0xF9, 0x21, 0x5B, 0x43, 0x53,
          0x45, 0x44, 0x3F, 0x2D, 0x3B, 0x52, 0x21, 0x24, 0x30, 0x4B, 0x46, 0x23, 0x33, 0x24, 0x49, 0x27,
          0x21, 0x24, 0x21, 0x5A, 0x35, 0x24, 0x3E, 0x5D, 0xAD, 0xE3, 0xB9, 0xBF, 0xF9, 0x21, 0x5B, 0x43,
          0x66, 0x42, 0x3C, 0x48, 0x7E, 0x38, 0x78);
    U(21, 0x3010, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC, 0x3011, 0x9752, 0x4E95, 0x5B9F, 0xFF0C, 0x3010,
          0x30B5, 0x30D6, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC, 0x3011, 0x6C60, 0x7530, 0x4F38, 0x5B50,
          0xFF0C, 0x4F0A, 0x85E4, 0x6D77, 0x5F66, 0xFF0C, 0x3010, 0x6C17, 0x8C61, 0x30AD, 0x30E3, 0x30B9,
          0x30BF, 0x30FC, 0x3011, 0x4E2D, 0x6751, 0x7F8E, 0x516C);
    T(21, true);

    B(22, 0x1D, 0x40, 0xF9, 0x1B, 0x6F, 0x26, 0x23, 0x73, 0xAC, 0x0F, 0x4D, 0x68, 0xBF, 0x21, 0x2A, 0xFB,
          0x47, 0x48, 0x4D, 0x70, 0xCE, 0x1B, 0x7C, 0xE9, 0xA4, 0xAA, 0xF3, 0x33, 0x58, 0x31, 0x60, 0x19,
          0x4B, 0x40, 0x78, 0x46, 0x7E, 0x21, 0x2A, 0x49, 0x34, 0x3D, 0x43, 0x19, 0x4E, 0x32, 0x26, 0x19,
          0x72, 0x4D, 0x5C, 0x40, 0x2E, 0x21, 0x2A, 0x21, 0x2A, 0xFC, 0x1B, 0x24, 0x3B, 0x7A, 0x5C, 0x7A,
          0x56);
    U(22, 0x30C0, 0x30FC, 0x30A6, 0x30A3, 0x30F3, 0x304C, 0x6765, 0x305F, 0xFF01, 0x300C, 0x6CE2, 0x4E71,
          0x306E, 0x30E9, 0x30A4, 0x30AA, 0x30F3, 0x5B66, 0x5712, 0x306B, 0x6F5C, 0x5165, 0xFF01, 0x767E,
          0x7363, 0x306E, 0x738B, 0x3092, 0x990A, 0x6210, 0xFF01, 0xFF01, 0x300D, 0xD83C, 0xDE16, 0xD83C,
          0xDE11);
    T(22, true);

    B(23, 0x40, 0x2E, 0x44, 0x39, 0xDE, 0xC3, 0xBF, 0xC0, 0x43, 0x66, 0xCE, 0x1B, 0x6F, 0x69, 0x24, 0x2A,
          0x73, 0xCE, 0x0F, 0x3B, 0x52, 0xC9, 0xE2, 0xBF, 0xC1, 0xAC, 0xFD, 0x37, 0x32, 0xEC, 0xCE, 0x43,
          0x66, 0xC7, 0x40, 0x68, 0x40, 0x38, 0x4C, 0x72, 0xCE, 0x42, 0x67, 0x3F, 0x4D, 0xAB, 0xE9, 0x3C,
          0x6D, 0xEA, 0xCE, 0x35, 0x3B, 0xE4, 0x3B, 0x52, 0x30, 0x69, 0xC6, 0x3D, 0x51, 0xF2, 0x33, 0x58,
          0xD6, 0xFA, 0x49, 0x54, 0x3F, 0x3F, 0x4C, 0x4C, 0x4C, 0x5C, 0xCA, 0x40, 0x38, 0x45, 0x4C, 0xCF,
          0x42, 0x60, 0x33, 0x58, 0x3D, 0x68, 0x4A, 0x2C, 0xCB, 0x21, 0x2A, 0x21, 0x29, 0x33, 0x58, 0x31,
          0x60, 0x1B, 0x7C, 0xC9, 0xE9, 0xDE, 0x34, 0x69, 0x49, 0x69, 0x19, 0x31, 0x19, 0x4E, 0x47, 0x48,
          0x4D, 0x70, 0x19, 0x4E, 0x46, 0x7C, 0x21, 0x39, 0x19, 0x4B, 0x4C, 0x29, 0x43, 0x65, 0x21, 0x2A);
    U(23, 0x6210, 0x9577, 0x307E, 0x3063, 0x305F, 0x3060, 0x4E2D, 0x306E, 0x30E9, 0x30A4, 0x30AA, 0x30F3,
          0x306E, 0x5B50, 0x3069, 0x3082, 0x305F, 0x3061, 0x304C, 0x3001, 0x7FA4, 0x308C, 0x306E, 0x4E2D,
          0x3067, 0x5148, 0x751F, 0x5F79, 0x306E, 0x5927, 0x4EBA, 0x304B, 0x3089, 0x72E9, 0x308A, 0x306E,
          0x6280, 0x3084, 0x5B50, 0x80B2, 0x3066, 0x8853, 0x3092, 0x5B66, 0x3076, 0x3002, 0x4E0D, 0x771F,
          0x9762, 0x76EE, 0x306A, 0x751F, 0x5F92, 0x306F, 0x9000, 0x5B66, 0x51E6, 0x5206, 0x306B, 0xFF01,
          0xFF1F, 0x5B66, 0x5712, 0x30C9, 0x30E9, 0x30DE, 0x9854, 0x8CA0, 0x3051, 0x306E, 0x6CE2, 0x4E71,
          0x306E, 0x65E5, 0x3005, 0x306B, 0x5BC6, 0x7740, 0xFF01);
    T(23, true);

    B(24, 0x21, 0x5A, 0x45, 0x5A, 0x4D, 0x4B, 0x3B, 0x7E, 0x42, 0x65, 0x1B, 0x7C, 0xC9, 0xE9, 0xDE, 0x21,
          0x5B, 0x31, 0x40, 0x4C, 0x38, 0x3F, 0x4E, 0x3A, 0x38, 0x31, 0x52, 0x4C, 0x67, 0x0E, 0x33, 0x0F,
          0x21, 0x21, 0x1B, 0x24, 0x3B, 0x7A, 0x6B, 0x1B, 0x24, 0x39, 0x21, 0x4A, 0x0E, 0x31, 0x0F, 0x21,
          0x4B, 0xFB, 0x4D, 0x72, 0x19, 0x4E, 0x4D, 0x3D, 0x34, 0x36, 0xFC, 0x1B, 0x24, 0x3B, 0x7A, 0x5C,
          0x7A, 0x56);
    U(24, 0x3010, 0x571F, 0x66DC, 0x6642, 0x4EE3, 0x30C9, 0x30E9, 0x30DE, 0x3011, 0x96F2, 0x9727, 0x4EC1,
          0x5DE6, 0x885B, 0x9580, 0x0033, 0x3000, 0xD83C, 0xDE1F, 0xFF08, 0x0031, 0xFF09, 0x300C, 0x5D50,
          0x306E, 0x4E88, 0x611F, 0x300D, 0xD83C, 0xDE16, 0xD83C, 0xDE11);
    T(24, true);

    B(25, 0x1B, 0x7C, 0xCA, 0xCB, 0xB3, 0xEC, 0x44, 0x41, 0x49, 0x34, 0x37, 0x4A, 0x20, 0x40, 0x56, 0x1B,
          0x7D, 0xC1, 0xE3, 0xF3, 0xAC, 0x25, 0x4A, 0x25, 0x5E, 0x25, 0x3A, 0xF2, 0x3F, 0x29, 0xD9, 0xEB,
          0x21, 0x29, 0x44, 0x41, 0x49, 0x77, 0x3D, 0x2C, 0x42, 0x33, 0x21, 0x39, 0x21, 0x75, 0x46, 0x30,
          0x4A, 0x2A, 0xC8, 0x3F, 0x4D, 0xCE, 0x36, 0x43, 0xAD, 0xCE, 0x3B, 0x36, 0x4A, 0x62, 0x0E, 0x53,
          0x50, 0x1B, 0x24, 0x3B, 0x0F, 0x7A, 0x56);
    U(25, 0x30CA, 0x30CB, 0x30B3, 0x30EC, 0x73CD, 0x767E, 0x666F, 0x3000, 0x8D64, 0x3061, 0x3083, 0x3093,
          0x304C, 0x30CA, 0x30DE, 0x30BA, 0x3092, 0x98DF, 0x3079, 0x308B, 0xFF1F, 0x73CD, 0x98A8, 0x7FD2,
          0x7D9A, 0x3005, 0xFF06, 0x52D5, 0x7269, 0x3068, 0x4EBA, 0x306E, 0x9A5A, 0x304D, 0x306E, 0x6563,
          0x6B69, 0x0053, 0x0050, 0xD83C, 0xDE11);
    T(25, true);

    B(26, 0x21, 0x7A, 0x46, 0x7C, 0x4B, 0x5C, 0x41, 0x34, 0x39, 0x71, 0xC7, 0x36, 0x43, 0xAD, 0xCE, 0x38,
          0x77, 0x37, 0x4A, 0xF2, 0x48, 0x2F, 0x38, 0x2B, 0x22, 0x27, 0x35, 0x5C, 0x3A, 0x6A, 0x21, 0x44,
          0xAA, 0x3F, 0x29, 0xA4, 0x3D, 0x69, 0xE1, 0xCF, 0x25, 0x4A, 0x25, 0x5E, 0x25, 0x3A, 0x4E, 0x41,
          0x4D, 0x7D, 0x22, 0x27, 0x4A, 0x21, 0x32, 0x2C, 0x21, 0x44, 0x38, 0x24, 0xCE, 0x42, 0x67, 0x37,
          0x33, 0x43, 0x44, 0xAC, 0x3B, 0x36, 0x4A, 0x62, 0x22, 0x27, 0x35, 0x7E, 0x45, 0x54, 0x21, 0x44,
          0x25, 0x2A, 0x25, 0x26, 0x25, 0x60, 0xC8, 0x3B, 0x36, 0x4A, 0x62, 0xB9, 0xEB, 0x1B, 0x7E, 0x89,
          0xB7, 0xB7, 0x8A, 0x3A, 0x50, 0x22, 0x27, 0x40, 0x45, 0x32, 0x2C, 0x21, 0x44, 0x33, 0x24, 0x34,
          0x5F, 0x1B, 0x7D, 0xCB, 0x34, 0x71, 0x40, 0x57, 0xCE, 0x35, 0x65, 0x42, 0x4E, 0x34, 0x64, 0x22,
          0x27, 0x40, 0x69, 0x4D, 0x55, 0x21, 0x44, 0x4C, 0x5A, 0xCE, 0x3B, 0x5E, 0xC7, 0x38, 0x24, 0x36,
          0x21, 0x4D, 0x5C);
    U(26, 0x2605, 0x65E5, 0x672C, 0x5168, 0x56FD, 0x3067, 0x9A5A, 0x304D, 0x306E, 0x5149, 0x666F, 0x3092,
          0x767A, 0x898B, 0x25BC, 0x5BAE, 0x5D0E, 0x2026, 0x304A, 0x98DF, 0x3044, 0x521D, 0x3081, 0x306F,
          0x30CA, 0x30DE, 0x30BA, 0x6599, 0x7406, 0x25BC, 0x798F, 0x5CA1, 0x2026, 0x72AC, 0x306E, 0x5927,
          0x8ECD, 0x56E3, 0x304C, 0x6563, 0x6B69, 0x25BC, 0x4EAC, 0x90FD, 0x2026, 0x30AA, 0x30A6, 0x30E0,
          0x3068, 0x6563, 0x6B69, 0x3059, 0x308B, 0x0037, 0x0037, 0x6B73, 0x25BC, 0x9759, 0x5CA1, 0x2026,
          0x6D77, 0x5CB8, 0x306B, 0x5947, 0x8DE1, 0x306E, 0x7403, 0x4F53, 0x5CA9, 0x25BC, 0x5343, 0x8449,
          0x2026, 0x6728, 0x306E, 0x679D, 0x3067, 0x72AC, 0x4F9B, 0x990A);
    T(26, false);

#undef B
#undef U
#undef T
}

void ARIBCharsetTest::testEncode()
{
    const ts::Charset& cset(ts::ARIBCharset::B24);

#define U(N,...) const ts::UString   u##N({__VA_ARGS__})
#define B(N,...) const ts::ByteBlock b##N({__VA_ARGS__})
#define T(N, success)                                                    \
    dump(u"Input:", u##N);                                               \
    dump(u"Expected:", b##N);                                            \
    const ts::ByteBlock enc##N(cset.encoded(u##N));                      \
    dump(u"Encoded:", enc##N);                                           \
    ts::UString dec##N;                                                  \
    const bool s##N = cset.decode(dec##N, enc##N.data(), enc##N.size()); \
    dump(u"Decoded:", dec##N);                                           \
    debug() << std::endl;                                                \
    TSUNIT_ASSERT(b##N == enc##N);                                       \
    TSUNIT_EQUAL(success, s##N);                                         \
    TSUNIT_EQUAL(u##N, dec##N)

    debug() << std::endl;

    U(1, 0x004E, 0x0048, 0x004B, 0x7DCF, 0x5408, 0x0031, 0x30FB, 0x79CB, 0x7530);
    B(1, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0x41, 0x6D, 0x39, 0x67, 0xB1, 0x21, 0x26, 0x3D, 0x29, 0x45, 0x44);
    T(1, true);

    U(2, 0x004E, 0x0048, 0x004B, 0x7DCF, 0x5408, 0x0032, 0x30FB, 0x79CB, 0x7530);
    B(2, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0x41, 0x6D, 0x39, 0x67, 0xB2, 0x21, 0x26, 0x3D, 0x29, 0x45, 0x44);
    T(2, true);

    U(3, 0x004E, 0x0048, 0x004B, 0x643A, 0x5E2F, 0x0047, 0x30FB, 0x79CB, 0x7530);
    B(3, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0x37, 0x48, 0x42, 0x53, 0xC7, 0x21, 0x26, 0x3D, 0x29, 0x45, 0x44);
    T(3, true);

    U(4, 0x004E, 0x0048, 0x004B, 0x0045, 0x30C6, 0x30EC, 0x0031, 0x79CB, 0x7530);
    B(4, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0xC5, 0x25, 0x46, 0x25, 0x6C, 0xB1, 0x3D, 0x29, 0x45, 0x44);
    T(4, true);

    U(5, 0x004E, 0x0048, 0x004B, 0x0045, 0x30C6, 0x30EC, 0x0032, 0x79CB, 0x7530);
    B(5, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0xC5, 0x25, 0x46, 0x25, 0x6C, 0xB2, 0x3D, 0x29, 0x45, 0x44);
    T(5, true);

    U(6, 0x004E, 0x0048, 0x004B, 0x0045, 0x30C6, 0x30EC, 0x0033, 0x79CB, 0x7530);
    B(6, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0xC5, 0x25, 0x46, 0x25, 0x6C, 0xB3, 0x3D, 0x29, 0x45, 0x44);
    T(6, true);

    U(7, 0x004E, 0x0048, 0x004B, 0x643A, 0x5E2F, 0x0032);
    B(7, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0x37, 0x48, 0x42, 0x53, 0xB2);
    T(7, true);

    U(8, 0x79CB, 0x7530, 0x671D, 0x65E5, 0x653E, 0x9001);
    B(8, 0x3D, 0x29, 0x45, 0x44, 0x44, 0x2B, 0x46, 0x7C, 0x4A, 0x7C, 0x41, 0x77);
    T(8, true);

    U(9, 0x0041, 0x0042, 0x0053, 0x79CB, 0x7530, 0x653E, 0x9001, 0x0031);
    B(9, 0x1B, 0x7E, 0xC1, 0xC2, 0xD3, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0xB1);
    T(9, true);

    U(10, 0x0041, 0x0042, 0x0053, 0x79CB, 0x7530, 0x653E, 0x9001, 0x0032);
    B(10, 0x1B, 0x7E, 0xC1, 0xC2, 0xD3, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0xB2);
    T(10, true);

    U(11, 0x79CB, 0x7530, 0x653E, 0x9001, 0x643A, 0x5E2F);
    B(11, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0x37, 0x48, 0x42, 0x53);
    T(11, true);

    U(12, 0x0041, 0x0042, 0x0053, 0x79CB, 0x7530, 0x653E, 0x9001, 0x81E8, 0x6642);
    B(12, 0x1B, 0x7E, 0xC1, 0xC2, 0xD3, 0x3D, 0x29, 0x45, 0x44, 0x4A, 0x7C, 0x41, 0x77, 0x4E, 0x57, 0x3B, 0x7E);
    T(12, true);

    U(13, 0x0041, 0x004B, 0x0054, 0x79CB, 0x7530, 0x30C6, 0x30EC, 0x30D3, 0x0031);
    B(13, 0x1B, 0x7E, 0xC1, 0xCB, 0xD4, 0x3D, 0x29, 0x45, 0x44, 0x25, 0x46, 0x25, 0x6C, 0x25, 0x53, 0xB1);
    T(13, true);

    U(14, 0x0041, 0x004B, 0x0054, 0x79CB, 0x7530, 0x30C6, 0x30EC, 0x30D3, 0x0032);
    B(14, 0x1B, 0x7E, 0xC1, 0xCB, 0xD4, 0x3D, 0x29, 0x45, 0x44, 0x25, 0x46, 0x25, 0x6C, 0x25, 0x53, 0xB2);
    T(14, true);

    U(15, 0x0041, 0x004B, 0x0054, 0x81E8, 0x6642);
    B(15, 0x1B, 0x7E, 0xC1, 0xCB, 0xD4, 0x4E, 0x57, 0x3B, 0x7E);
    T(15, true);

    U(16, 0x0047, 0x30AC, 0x30A4, 0x30C9);
    B(16, 0x1B, 0x7E, 0xC7, 0x25, 0x2C, 0x25, 0x24, 0x25, 0x49);
    T(16, true);

    U(17, 0x0041, 0x004B, 0x0054, 0x79CB, 0x7530, 0x30C6, 0x30EC, 0x30D3);
    B(17, 0x1B, 0x7E, 0xC1, 0xCB, 0xD4, 0x3D, 0x29, 0x45, 0x44, 0x25, 0x46, 0x25, 0x6C, 0x25, 0x53);
    T(17, true);

    U(18, 0x004E, 0x0048, 0x004B, 0x30CB, 0x30E5, 0x30FC, 0x30B9, 0x0037, 0xD83C, 0xDE14, 0xD83C, 0xDE11);
    B(18, 0x1B, 0x7E, 0xCE, 0xC8, 0xCB, 0x25, 0x4B, 0x25, 0x65, 0x21, 0x3C, 0x25, 0x39, 0xB7, 0x7A, 0x5A, 0x7A, 0x56);
    T(18, true);

    U(19, 0x591C, 0x0037, 0x6642, 0x3001, 0x300C, 0x4E00, 0x6B69, 0x5148, 0x3078, 0x3001, 0x4E00, 0x6B69,
          0x6DF1, 0x304F, 0x300D, 0x3000, 0x4ECA, 0x3001, 0x3053, 0x306E, 0x30CB, 0x30E5, 0x30FC, 0x30B9,
          0x3092, 0x5C4A, 0x3051, 0x305F, 0x3044, 0x3000, 0x3010, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC,
          0x3011, 0x9752, 0x4E95, 0x5B9F, 0xFF0C, 0x3010, 0x30B5, 0x30D6, 0x30AD, 0x30E3, 0x30B9, 0x30BF,
          0x30FC, 0x3011, 0x6C60, 0x7530, 0x4F38, 0x5B50, 0xFF0C, 0x4F0A, 0x85E4, 0x6D77, 0x5F66, 0xFF0C,
          0x3010, 0x6C17, 0x8C61, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC, 0x3011, 0x4E2D, 0x6751, 0x7F8E,
          0x516C);
    B(19, 0x4C, 0x6B, 0x1B, 0x7E, 0xB7, 0x3B, 0x7E, 0x21, 0x22, 0x21, 0x56, 0x30, 0x6C, 0x4A, 0x62, 0x40,
          0x68, 0x24, 0x58, 0x21, 0x22, 0x30, 0x6C, 0x4A, 0x62, 0x3F, 0x3C, 0x24, 0x2F, 0x21, 0x57, 0x21,
          0x21, 0x3A, 0x23, 0x21, 0x22, 0x24, 0x33, 0x24, 0x4E, 0x25, 0x4B, 0x25, 0x65, 0x21, 0x3C, 0x25,
          0x39, 0x24, 0x72, 0x46, 0x4F, 0x24, 0x31, 0x24, 0x3F, 0x24, 0x24, 0x21, 0x21, 0x21, 0x5A, 0x25,
          0x2D, 0x25, 0x63, 0x25, 0x39, 0x25, 0x3F, 0x21, 0x3C, 0x21, 0x5B, 0x40, 0x44, 0x30, 0x66, 0x3C,
          0x42, 0x21, 0x24, 0x21, 0x5A, 0x25, 0x35, 0x25, 0x56, 0x25, 0x2D, 0x25, 0x63, 0x25, 0x39, 0x25,
          0x3F, 0x21, 0x3C, 0x21, 0x5B, 0x43, 0x53, 0x45, 0x44, 0x3F, 0x2D, 0x3B, 0x52, 0x21, 0x24, 0x30,
          0x4B, 0x46, 0x23, 0x33, 0x24, 0x49, 0x27, 0x21, 0x24, 0x21, 0x5A, 0x35, 0x24, 0x3E, 0x5D, 0x25,
          0x2D, 0x25, 0x63, 0x25, 0x39, 0x25, 0x3F, 0x21, 0x3C, 0x21, 0x5B, 0x43, 0x66, 0x42, 0x3C, 0x48,
          0x7E, 0x38, 0x78);
    T(19, true);

    U(20, 0x51FA, 0x6F14, 0x8005);
    B(20, 0x3D, 0x50, 0x31, 0x69, 0x3C, 0x54);
    T(20, true);

    U(21, 0x3010, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC, 0x3011, 0x9752, 0x4E95, 0x5B9F, 0xFF0C, 0x3010,
          0x30B5, 0x30D6, 0x30AD, 0x30E3, 0x30B9, 0x30BF, 0x30FC, 0x3011, 0x6C60, 0x7530, 0x4F38, 0x5B50,
          0xFF0C, 0x4F0A, 0x85E4, 0x6D77, 0x5F66, 0xFF0C, 0x3010, 0x6C17, 0x8C61, 0x30AD, 0x30E3, 0x30B9,
          0x30BF, 0x30FC, 0x3011, 0x4E2D, 0x6751, 0x7F8E, 0x516C);
    B(21, 0x21, 0x5A, 0x25, 0x2D, 0x25, 0x63, 0x25, 0x39, 0x25, 0x3F, 0x21, 0x3C, 0x21, 0x5B, 0x40, 0x44,
          0x30, 0x66, 0x3C, 0x42, 0x21, 0x24, 0x21, 0x5A, 0x25, 0x35, 0x25, 0x56, 0x25, 0x2D, 0x25, 0x63,
          0x25, 0x39, 0x25, 0x3F, 0x21, 0x3C, 0x21, 0x5B, 0x43, 0x53, 0x45, 0x44, 0x3F, 0x2D, 0x3B, 0x52,
          0x21, 0x24, 0x30, 0x4B, 0x46, 0x23, 0x33, 0x24, 0x49, 0x27, 0x21, 0x24, 0x21, 0x5A, 0x35, 0x24,
          0x3E, 0x5D, 0x25, 0x2D, 0x25, 0x63, 0x25, 0x39, 0x25, 0x3F, 0x21, 0x3C, 0x21, 0x5B, 0x43, 0x66,
          0x42, 0x3C, 0x48, 0x7E, 0x38, 0x78);
    T(21, true);

    U(22, 0x30C0, 0x30FC, 0x30A6, 0x30A3, 0x30F3, 0x304C, 0x6765, 0x305F, 0xFF01, 0x300C, 0x6CE2, 0x4E71,
          0x306E, 0x30E9, 0x30A4, 0x30AA, 0x30F3, 0x5B66, 0x5712, 0x306B, 0x6F5C, 0x5165, 0xFF01, 0x767E,
          0x7363, 0x306E, 0x738B, 0x3092, 0x990A, 0x6210, 0xFF01, 0xFF01, 0x300D, 0xD83C, 0xDE16, 0xD83C,
          0xDE11);
    B(22, 0x25, 0x40, 0x21, 0x3C, 0x25, 0x26, 0x25, 0x23, 0x25, 0x73, 0x24, 0x2C, 0x4D, 0x68, 0x24, 0x3F,
          0x21, 0x2A, 0x21, 0x56, 0x47, 0x48, 0x4D, 0x70, 0x24, 0x4E, 0x25, 0x69, 0x25, 0x24, 0x25, 0x2A,
          0x25, 0x73, 0x33, 0x58, 0x31, 0x60, 0x24, 0x4B, 0x40, 0x78, 0x46, 0x7E, 0x21, 0x2A, 0x49, 0x34,
          0x3D, 0x43, 0x24, 0x4E, 0x32, 0x26, 0x24, 0x72, 0x4D, 0x5C, 0x40, 0x2E, 0x21, 0x2A, 0x21, 0x2A,
          0x21, 0x57, 0x7A, 0x5C, 0x7A, 0x56);
    T(22, true);

    U(23, 0x6210, 0x9577, 0x307E, 0x3063, 0x305F, 0x3060, 0x4E2D, 0x306E, 0x30E9, 0x30A4, 0x30AA, 0x30F3,
          0x306E, 0x5B50, 0x3069, 0x3082, 0x305F, 0x3061, 0x304C, 0x3001, 0x7FA4, 0x308C, 0x306E, 0x4E2D,
          0x3067, 0x5148, 0x751F, 0x5F79, 0x306E, 0x5927, 0x4EBA, 0x304B, 0x3089, 0x72E9, 0x308A, 0x306E,
          0x6280, 0x3084, 0x5B50, 0x80B2, 0x3066, 0x8853, 0x3092, 0x5B66, 0x3076, 0x3002, 0x4E0D, 0x771F,
          0x9762, 0x76EE, 0x306A, 0x751F, 0x5F92, 0x306F, 0x9000, 0x5B66, 0x51E6, 0x5206, 0x306B, 0xFF01,
          0xFF1F, 0x5B66, 0x5712, 0x30C9, 0x30E9, 0x30DE, 0x9854, 0x8CA0, 0x3051, 0x306E, 0x6CE2, 0x4E71,
          0x306E, 0x65E5, 0x3005, 0x306B, 0x5BC6, 0x7740, 0xFF01);
    B(23, 0x40, 0x2E, 0x44, 0x39, 0x24, 0x5E, 0x24, 0x43, 0x24, 0x3F, 0x24, 0x40, 0x43, 0x66, 0x24, 0x4E,
          0x25, 0x69, 0x25, 0x24, 0x25, 0x2A, 0x25, 0x73, 0x24, 0x4E, 0x3B, 0x52, 0x24, 0x49, 0x24, 0x62,
          0x24, 0x3F, 0x24, 0x41, 0x24, 0x2C, 0x21, 0x22, 0x37, 0x32, 0x24, 0x6C, 0x24, 0x4E, 0x43, 0x66,
          0x24, 0x47, 0x40, 0x68, 0x40, 0x38, 0x4C, 0x72, 0x24, 0x4E, 0x42, 0x67, 0x3F, 0x4D, 0x24, 0x2B,
          0x24, 0x69, 0x3C, 0x6D, 0x24, 0x6A, 0x24, 0x4E, 0x35, 0x3B, 0x24, 0x64, 0x3B, 0x52, 0x30, 0x69,
          0x24, 0x46, 0x3D, 0x51, 0x24, 0x72, 0x33, 0x58, 0x24, 0x56, 0x21, 0x23, 0x49, 0x54, 0x3F, 0x3F,
          0x4C, 0x4C, 0x4C, 0x5C, 0x24, 0x4A, 0x40, 0x38, 0x45, 0x4C, 0x24, 0x4F, 0x42, 0x60, 0x33, 0x58,
          0x3D, 0x68, 0x4A, 0x2C, 0x24, 0x4B, 0x21, 0x2A, 0x21, 0x29, 0x33, 0x58, 0x31, 0x60, 0x25, 0x49,
          0x25, 0x69, 0x25, 0x5E, 0x34, 0x69, 0x49, 0x69, 0x24, 0x31, 0x24, 0x4E, 0x47, 0x48, 0x4D, 0x70,
          0x24, 0x4E, 0x46, 0x7C, 0x21, 0x39, 0x24, 0x4B, 0x4C, 0x29, 0x43, 0x65, 0x21, 0x2A);
    T(23, true);

    U(24, 0x3010, 0x571F, 0x66DC, 0x6642, 0x4EE3, 0x30C9, 0x30E9, 0x30DE, 0x3011, 0x96F2, 0x9727, 0x4EC1,
          0x5DE6, 0x885B, 0x9580, 0x0033, 0x3000, 0xD83C, 0xDE1F, 0xFF08, 0x0031, 0xFF09, 0x300C, 0x5D50,
          0x306E, 0x4E88, 0x611F, 0x300D, 0xD83C, 0xDE16, 0xD83C, 0xDE11);
    B(24, 0x21, 0x5A, 0x45, 0x5A, 0x4D, 0x4B, 0x3B, 0x7E, 0x42, 0x65, 0x25, 0x49, 0x25, 0x69, 0x25, 0x5E,
          0x21, 0x5B, 0x31, 0x40, 0x4C, 0x38, 0x3F, 0x4E, 0x3A, 0x38, 0x31, 0x52, 0x4C, 0x67, 0x1B, 0x7E,
          0xB3, 0x21, 0x21, 0x7A, 0x6B, 0x21, 0x4A, 0xB1, 0x21, 0x4B, 0x21, 0x56, 0x4D, 0x72, 0x24, 0x4E,
          0x4D, 0x3D, 0x34, 0x36, 0x21, 0x57, 0x7A, 0x5C, 0x7A, 0x56);
    T(24, true);

    U(25, 0x0061, 0x0062, 0x0020, 0x0063, 0x9A5A, 0x304D, 0x3000, 0x306E, 0x6563);
    B(25, 0x1B, 0x7E, 0xE1, 0xE2, 0xA0, 0xE3, 0x36, 0x43, 0x24, 0x2D, 0x21, 0x21, 0x24, 0x4E, 0x3B, 0x36);
    T(25, true);

#undef B
#undef U
#undef T
}
