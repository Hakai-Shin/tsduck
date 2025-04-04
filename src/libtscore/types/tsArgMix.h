//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Element of an argument list with mixed integer and string types.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"
#include "tsIntegerUtils.h"
#include "tsAbstractNumber.h"
#include "tsStringifyInterface.h"

//
// There is a problem here with Microsoft C/C++.
//
// In some cases, typically in std::initializer_list of ArgMixIn, the lifetime
// of temporary string objects is not conformant with the C++11 standard.
// The result is incorrect generated code which leads to incorrect behaviour.
// This is a very dangerous class of bug. Not only for TSDuck but for all
// applications. This is the kind of latent bug which stays dormant in your
// code, until it blows up your application after modifying some innocent
// part of your application.
//
// A fix was first introduced with Visual Studio 2019 16.4.0 Preview 1.0
// (_MSC_FULL_VER = 192428117, _MSC_VER = 1924, _MSC_BUILD = 0).
//
// Until TSDuck version 3.28-2463, it was possible to use older versions
// of MSC, with conditional compilation, at the expense of some horrible hack
// and a significant performance penalty. After that version of TSDuck, new
// code was added and the previous (horrible) workaround was no longer possible.
// As a consequence, we now relunctantly no longer support older versions of MSC.
// If you think that you are obliged to use such an old version of MSC, you are
// left with two choices only: either stay with an old version of TSDuck or
// upgrade MSC. Please consider that the MSC bug is potentially dangerous for
// all applications, including yours. Consequently, you should _really_ upgrade
// MSC, even for your own application. You have been warned...
//
#if defined(_MSC_VER) && (_MSC_VER < 1924)
#error "unsupported version of MSC compiler, use Visual Studio 2019 16.4.0 or higher"
#endif

namespace ts {
    //!
    //! Base class for elements of an argument list with mixed types.
    //!
    //! This class is typically used as element in an std::initializer_list
    //! to build type-safe variable argument lists. Instances of ArgMix are
    //! directly built in the initializer list and cannot be copied or assigned.
    //!
    //! This is a base class. It can be used only through the two derived
    //! classes ArgMixIn and ArgMixOut.
    //!
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL ArgMix
    {
    public:
        //!
        //! Check if the argument value is an integer, either input or output.
        //! @return True if the argument value is an integer.
        //!
        bool isInteger() const { return (_type & INTEGER) == INTEGER; }
        //!
        //! Check if the argument value is an output integer.
        //! @return True if the argument value is an output integer.
        //!
        bool isOutputInteger() const { return (_type & (INTEGER | POINTER)) == (INTEGER | POINTER); }
        //!
        //! Check if the argument value is an output floating point.
        //! @return True if the argument value is an output integer.
        //!
        bool isOutputFloat() const { return (_type & (DOUBLE | POINTER)) == (DOUBLE | POINTER); }
        //!
        //! Check if the argument value is a signed integer, either input or output.
        //! @return True if the argument value is a signed integer.
        //!
        bool isSigned() const { return (_type & (SIGNED | INTEGER)) == (SIGNED | INTEGER); }
        //!
        //! Check if the argument value is an unsigned integer, either input or output.
        //! @return True if the argument value is an unsigned integer.
        //!
        bool isUnsigned() const { return (_type & (SIGNED | INTEGER)) == INTEGER; }
        //!
        //! Check if the argument value is a bool.
        //! @return True if the argument value is a bool.
        //!
        bool isBool() const { return (_type & (BIT1 | INTEGER)) == (BIT1 | INTEGER); }
        //!
        //! Check if the argument value is a string of any type.
        //! @return True if the argument value is a string.
        //!
        bool isAnyString() const { return (_type & STRING) == STRING; }
        //!
        //! Check if the argument value is a string of 8-bit characters.
        //! @return True if the argument value is a string of 8-bit characters (char* or std::string).
        //!
        bool isAnyString8() const { return (_type & (STRING | BIT8)) == (STRING | BIT8); }
        //!
        //! Check if the argument value is a string of 16-bit characters.
        //! @return True if the argument value is a string of 16-bit characters (UChar* or ts::UString).
        //!
        bool isAnyString16() const { return (_type & (STRING | BIT16)) == (STRING | BIT16); }
        //!
        //! Check if the argument value is a char* string.
        //! @return True if the argument value is a char* string.
        //!
        bool isCharPtr() const { return (_type & (STRING | BIT8 | CLASS)) == (STRING | BIT8); }
        //!
        //! Check if the argument value is a std::string.
        //! @return True if the argument value is a std::string.
        //!
        bool isString() const { return (_type & (STRING | BIT8 | CLASS)) == (STRING | BIT8 | CLASS); }
        //!
        //! Check if the argument value is a UChar* string.
        //! @return True if the argument value is a UChar* string.
        //!
        bool isUCharPtr() const { return (_type & (STRING | BIT16 | CLASS)) == (STRING | BIT16); }
        //!
        //! Check if the argument value is a ts::UString.
        //! @return True if the argument value is a ts::UString.
        //!
        bool isUString() const { return (_type & (STRING | BIT16 | CLASS)) == (STRING | BIT16 | CLASS); }
        //!
        //! Check if the argument value is a double floating point value, either input or output.
        //! @return True if the argument value is a double.
        //!
        bool isDouble() const { return (_type & DOUBLE) == DOUBLE; }
        //!
        //! Check if the argument value is an AbstractNumber value, either input or output.
        //! @return True if the argument value is an AbstractNumber.
        //!
        bool isAbstractNumber() const { return (_type & ANUMBER) == ANUMBER; }
        //!
        //! Check if the argument value is a std::chrono::duration value.
        //! @return True if the argument value is a std::chrono::duration value.
        //!
        bool isChrono() const { return (_type & CHRONO) == CHRONO; }
        //!
        //! Get the original integer size in bytes of the argument data.
        //! @return The original integer size in bytes of the argument data or zero for a string or double.
        //!
        size_t size() const { return _size; }

        //!
        //! Get the argument data value as an integer.
        //! @tparam INT An integer type.
        //! @param [in] raw In the case of fixed-point value, return the integral part when false (the default)
        //! and the raw value if true. Ignored for plain integer types.
        //! @return The argument data as an integer value of type @a INT or zero for a string or double.
        //!
        template <typename INT> requires std::integral<INT>
        INT toInteger(bool raw = false) const;
        //!
        //! Get the argument data value as a 32-bit signed integer.
        //! @return The argument data as an integer value or zero for a string or double.
        //!
        int32_t toInt32() const { return toInteger<int32_t>(); }
        //!
        //! Get the argument data value as a 32-bit unsigned integer.
        //! @return The argument data as an integer value or zero for a string or double.
        //!
        uint32_t toUInt32() const { return toInteger<uint32_t>(); }
        //!
        //! Get the argument data value as a 64-bit signed integer.
        //! @return The argument data as an integer value or zero for a string or double.
        //!
        int64_t toInt64() const { return toInteger<int64_t>(); }
        //!
        //! Get the argument data value as a 64-bit unsigned integer.
        //! @return The argument data as an integer value or zero for a string or double.
        //!
        uint64_t toUInt64() const { return toInteger<uint64_t>(); }
        //!
        //! Get the argument data value as a bool.
        //! @return The argument data as a bool value or false for a string or double.
        //!
        bool toBool() const { return toInteger<uint32_t>() != 0; }
        //!
        //! Get the argument data value as a nul-terminated string of 8-bit characters.
        //! @return Address of the nul-terminated string for CHARPTR or STRING, an empty string for other data types.
        //!
        const char* toCharPtr() const;
        //!
        //! Get the argument data value as a nul-terminated string of 16-bit characters.
        //! @return Address of the nul-terminated string for UCHARPTR or USTRING, an empty string for other data types.
        //!
        const UChar* toUCharPtr() const;
        //!
        //! Get the argument data value as constant reference to a C++ string of 8-bit characters.
        //! @return Reference to the string for STRING, to an empty string for other data types.
        //!
        const std::string& toString() const;
        //!
        //! Get the argument data value as constant reference to a C++ string of 16-bit characters.
        //! @return Reference to the string for USTRING, to an empty string for other data types.
        //!
        const UString& toUString() const;
        //!
        //! Get the argument data value as a double floating point value.
        //! @return The argument data as a double or zero for a string. Integers are converted to double.
        //!
        double toDouble() const;
        //!
        //! Get the argument data value as constant reference to an AbstractNumber instance.
        //! @return Reference to the AbstractNumber.
        //!
        const AbstractNumber& toAbstractNumber() const;
        //!
        //! In the case of a std::chrono::duration value, return the numerator of the ratio.
        //! The chrono count is obtained using tsInt64().
        //! @return The numerator of the ratio for a std::chrono::duration value, zero otherwise.
        //!
        std::intmax_t num() const { return _num; }
        //!
        //! In the case of a std::chrono::duration value, return the denominator of the ratio.
        //! The chrono count is obtained using tsInt64().
        //! @return The denominator of the ratio for a std::chrono::duration value, zero otherwise.
        //!
        std::intmax_t den() const { return _den; }
        //!
        //! Store an integer value in the argument data, for pointers to integer.
        //! @tparam INT An integer type.
        //! @param [in] i The integer value to store in the argument data.
        //! @return True on success, false if the argument data is not a pointer to integer.
        //!
        template <typename INT> requires std::integral<INT>
        bool storeInteger(INT i) const;

        //!
        //! Store a floating point value in the argument data, for pointers to double.
        //! @param [in] f The floating point value to store in the argument data.
        //! @return True on success, false if the argument data is not a pointer to double.
        //!
        template <typename FLOAT> requires std::floating_point<FLOAT>
        bool storeFloat(FLOAT f) const;

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ArgMix(const ArgMix& other);

        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move.
        //!
        ArgMix(ArgMix&& other);

        //!
        //! Destructor.
        //!
        ~ArgMix();

    protected:
        //!
        //! Type of an argument, used as bitmask.
        //!
        using TypeFlags = uint16_t;

        //!
        //! Anonymous enum, used as bitmask.
        //!
        enum : TypeFlags {
            INTEGER   = 0x0001,  //!< Integer type.
            SIGNED    = 0x0002,  //!< With INTEGER, 1 means signed, 0 means unsigned.
            STRING    = 0x0004,  //!< String of characters.
            CLASS     = 0x0008,  //!< With STRING, 1 means std::string or ts::UString, O means const char* or const UChar*.
            BIT1      = 0x0010,  //!< 1-bit integer, ie. bool.
            BIT8      = 0x0020,  //!< 8-bit integer or string of 8-bit characters (char).
            BIT16     = 0x0040,  //!< 16-bit integer or string of 16-bit characters (UChar).
            BIT32     = 0x0080,  //!< 32-bit integer.
            BIT64     = 0x0100,  //!< 64-bit integer.
            POINTER   = 0x0200,  //!< A pointer to a writeable data (data type is given by other bits).
            STRINGIFY = 0x0400,  //!< A pointer to a StringifyInterface object.
            PATH      = 0x0800,  //!< A pointer to a std::filesystem::path object.
            DOUBLE    = 0x1000,  //!< Double floating point type.
            ANUMBER   = 0x2000,  //!< A pointer to an AbstractNumber object.
            CHRONO    = 0x4000,  //!< A instance of std::chrono::duration.
        };

        // Get the TypeFlags value of an integer size.
        //! @cond nodoxygen
        template<size_t SIZE> struct SizeFlags { static constexpr TypeFlags value = 0; };
        //! @endcond

#if !defined(DOXYGEN)
        //
        // Storage of an argument.
        //
        union TSCOREDLL Value {
            int32_t                   int32;
            uint32_t                  uint32;
            int64_t                   int64;
            uint64_t                  uint64;
            double                    dbl;
            const char*               charptr;
            const UChar*              ucharptr;
            void*                     vptr;  // output
            const std::string*        string;
            const UString*            ustring;
            const StringifyInterface* stringify;
            const fs::path*           path;
            const AbstractNumber*     anumber;

            Value(void* p)              : vptr(p) {}
            Value(bool b)               : uint32(b) {}
            Value(int32_t i)            : int32(i) {}
            Value(uint32_t i)           : uint32(i) {}
            Value(int64_t i)            : int64(i) {}
            Value(uint64_t i)           : uint64(i) {}
            Value(double d)             : dbl(d) {}
            Value(const char* s)        : charptr(s) {}
            Value(const UChar* s)       : ucharptr(s) {}
            Value(const std::string& s) : string(&s) {}
            Value(const UString& s)     : ustring(&s) {}
            Value(const fs::path& s)    : path(&s) {}
            Value(const AbstractNumber& s) : anumber(&s) {}
            Value(const StringifyInterface& s) : stringify(&s) {}
        };
#endif // DOXYGEN

        //!
        //! Default constructor.
        //! The argument does not represent anything.
        //!
        ArgMix();

        //!
        //! Constructor for subclasses.
        //! @param [in] type Indicate which overlay to use in _value.
        //! @param [in] size Original size for integer type.
        //! @param [in] value Actual value of the argument.
        //! @param [in] num Numerator of the ratio in the case of a std::chrono::duration value.
        //! @param [in] den Denominator of the ratio in the case of a std::chrono::duration value.
        //!
        ArgMix(TypeFlags type, size_t size, const Value value, std::intmax_t num = 0, std::intmax_t den = 0);

#if !defined(DOXYGEN)
        // Warning: The rest of this class is carefully crafted template meta-programming (aka. Black Magic).
        // It is correct, it works, but it is not immediately easy to understand. So, do not modify it if you
        // are not 100% sure to have understood it and you know what you are doing. You have been warned...

        // The meta-type "storage_type" defines the characteristics of the type which is
        // used to store an integer or enum type in an ArgMixIn.
        template <typename T>
        struct storage_type {

            // The meta-type "type" is the storage type, namely one of int32_t, uint32_t, int64_t, uint64_t.
            using type = typename std::conditional<
                std::is_signed<typename ts::underlying_type<T>::type>::value,
                typename std::conditional<(sizeof(T) > 4), int64_t, int32_t>::type,
                typename std::conditional<(sizeof(T) > 4), uint64_t, uint32_t>::type
            >::type;

            // The meta-type "type_constant" defines the ArgMix type flags value for the type T.
            using type_constant = typename std::conditional<
                std::is_signed<typename ts::underlying_type<T>::type>::value,
                typename std::conditional<
                    (sizeof(T) > 4),
                    std::integral_constant<TypeFlags, INTEGER | SIGNED | BIT64>,
                    std::integral_constant<TypeFlags, INTEGER | SIGNED | BIT32>
                >::type,
                typename std::conditional<
                    (sizeof(T) > 4),
                    std::integral_constant<TypeFlags, INTEGER | BIT64>,
                    std::integral_constant<TypeFlags, INTEGER | BIT32>
                >::type
            >::type;

            // The "value" is the ArgMix type flags value for the type T.
            static constexpr TypeFlags value = type_constant::value;
        };

        // The meta-type "reference_type" defines the characteristics of the type which is
        // used to reference an integer or enum type in an ArgMixOut.
        template <typename T>
        struct reference_type {

            // The meta-type "type" is the storage type.
            using type = typename std::conditional<
                std::is_signed<typename ts::underlying_type<T>::type>::value,
                typename std::conditional<(sizeof(T) > 4), int64_t, std::conditional<(sizeof(T) > 2), int32_t, std::conditional<(sizeof(T) > 1), int16_t, int8_t>>>::type,
                typename std::conditional<(sizeof(T) > 4), uint64_t, std::conditional<(sizeof(T) > 2), uint32_t, std::conditional<(sizeof(T) > 1), uint16_t, uint8_t>>>::type
            >::type;

            // The meta-type "type_constant" defines the ArgMix type flags value for the type T.
            using type_constant = typename std::conditional<
                std::is_signed<typename ts::underlying_type<T>::type>::value,
                typename std::conditional<
                    (sizeof(T) > 4),
                    std::integral_constant<TypeFlags, POINTER | INTEGER | SIGNED | BIT64>,
                    typename std::conditional<
                        (sizeof(T) > 2),
                        std::integral_constant<TypeFlags, POINTER | INTEGER | SIGNED | BIT32>,
                        typename std::conditional<
                            (sizeof(T) > 1),
                            std::integral_constant<TypeFlags, POINTER | INTEGER | SIGNED | BIT16>,
                            std::integral_constant<TypeFlags, POINTER | INTEGER | SIGNED | BIT8>
                        >::type
                    >::type
                >::type,
                typename std::conditional<
                    (sizeof(T) > 4),
                    std::integral_constant<TypeFlags, POINTER | INTEGER | BIT64>,
                    typename std::conditional<
                        (sizeof(T) > 2),
                        std::integral_constant<TypeFlags, POINTER | INTEGER | BIT32>,
                        typename std::conditional<
                            (sizeof(T) > 1),
                            std::integral_constant<TypeFlags, POINTER | INTEGER | BIT16>,
                            std::integral_constant<TypeFlags, POINTER | INTEGER | BIT8>
                        >::type
                    >::type
                >::type
            >::type;

            // The "value" is the ArgMix type flags value for the type T.
            static constexpr TypeFlags value = type_constant::value;
        };
#endif

    private:
        // Implementation of an ArgMix.
        const TypeFlags     _type;      //!< Indicate which overlay to use in _value.
        const uint8_t       _size;      //!< Original size for integer type (not greater than 8).
        const Value         _value;     //!< Actual value of the argument.
        const std::intmax_t _num;       //!< Ratio numerator for chrono types.
        const std::intmax_t _den;       //!< Ratio denominator for chrono types.
        mutable UString*    _aux;       //!< Auxiliary string (for StringifyInterface and character pointers).

        // Instances are directly built in initializer lists and cannot be assigned.
        ArgMix& operator=(ArgMix&&) = delete;
        ArgMix& operator=(const ArgMix&) = delete;
    };

    // Specializations for ArgMix::SizeFlags for integral types.
    //! @cond nodoxygen
    template<> struct ArgMix::SizeFlags<1> { static constexpr TypeFlags value = BIT8; };
    template<> struct ArgMix::SizeFlags<2> { static constexpr TypeFlags value = BIT16; };
    template<> struct ArgMix::SizeFlags<4> { static constexpr TypeFlags value = BIT32; };
    template<> struct ArgMix::SizeFlags<8> { static constexpr TypeFlags value = BIT64; };
    //! @endcond

    //!
    //! Define an element of an argument list with mixed integer and string input types.
    //!
    //! This class is typically used as element in an std::initializer_list.
    //! This mechanism is used by ts::UString::Format() for instance.
    //!
    //! An instance of ArgMixIn may reference external data. The lifetime of the
    //! pointed data must be longer than the ArgMixIn instance. This is the case
    //! for a std::initializer_list<ArgMixIn> which is used as parameter to Format().
    //! But this is not guaranteed in other usages.
    //!
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL ArgMixIn: public ArgMix
    {
    public:
        //!
        //! Default constructor.
        //!
        ArgMixIn() = default;
        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ArgMixIn(const ArgMixIn& other) : ArgMix(other) {}
        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move.
        //!
        ArgMixIn(ArgMixIn&& other) noexcept : ArgMix(std::move(other)) {}
        //!
        //! Constructor from a nul-terminated string of 8-bit characters.
        //! @param [in] s Address of nul-terminated string.
        //!
        ArgMixIn(const char* s) : ArgMix(STRING | BIT8, 0, Value(s)) {}
        //!
        //! Constructor from a nul-terminated string of 16-bit characters.
        //! @param [in] s Address of nul-terminated string.
        //!
        ArgMixIn(const UChar* s) : ArgMix(STRING | BIT16, 0, Value(s)) {}
        //!
        //! Constructor from a C++ string of 8-bit characters.
        //! @param [in] s Reference to a C++ string.
        //!
        ArgMixIn(const std::string& s) : ArgMix(STRING | BIT8 | CLASS, 0, s) {}
        //!
        //! Constructor from a C++ string of 16-bit characters.
        //! @param [in] s Reference to a C++ string.
        //!
        ArgMixIn(const UString& s) : ArgMix(STRING | BIT16 | CLASS, 0, s) {}
        //!
        //! Constructor from a stringifiable object.
        //! @param [in] s Reference to a stringifiable object.
        //!
        ArgMixIn(const StringifyInterface& s) : ArgMix(STRING | BIT16 | CLASS | STRINGIFY, 0, s) {}
        //!
        //! Constructor from a std::filesystem::path object.
        //! @param [in] s Reference to a path object.
        //!
        ArgMixIn(const fs::path& s) : ArgMix(STRING | SizeFlags<sizeof(fs::path::value_type)>::value | CLASS | PATH, 0, s) {}
        //!
        //! Constructor from an AbstractNumber object.
        //! @param [in] s Reference to an AbstractNumber object.
        //!
        ArgMixIn(const AbstractNumber& s) : ArgMix(ANUMBER, 0, s) {}
        //!
        //! Constructor from a bool.
        //! @param [in] b Boolean value.
        //!
        ArgMixIn(bool b) : ArgMix(INTEGER | BIT1, 1, Value(b)) {}
        //!
        //! Constructor from a double.
        //! @param [in] d double value.
        //!
        ArgMixIn(double d) : ArgMix(DOUBLE, 0, Value(d)) {}
        //!
        //! Constructor from an integer or enum type.
        //! @param [in] i Integer value of the ArgMix. Internally stored as a 32-bit or 64-bit integer.
        //!
        template<typename T> requires ts::int_enum<T>
        ArgMixIn(T i) : ArgMix(storage_type<T>::value, sizeof(i), static_cast<typename storage_type<T>::type>(i)) {}
        //!
        //! Constructor from a std::chrono::duration value.
        //! @param [in] d A std::chrono::duration value.
        //!
        template <class Rep, class Period>
        ArgMixIn(const cn::duration<Rep, Period>& d) :
            ArgMix(storage_type<int64_t>::value | CHRONO, sizeof(int64_t), int64_t(d.count()), Period::num, Period::den) {}

    private:
        // Instances are directly built in initializer lists and cannot be assigned.
        ArgMixIn& operator=(ArgMixIn&&) = delete;
        ArgMixIn& operator=(const ArgMixIn&) = delete;
    };

    //!
    //! Define an element of an argument list with integer output types of mixed sizes.
    //!
    //! This class is typically used as element in an std::initializer_list.
    //! This mechanism is used by ts::UString::Scan() for instance.
    //!
    //! An instance of ArgMixOut references external data. The lifetime of the
    //! pointed data must be longer than the ArgMixOut instance. This is the case
    //! for a std::initializer_list<ArgMixOut> which is used as parameter to Scan().
    //! But this is not guaranteed in other usages.
    //!
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL ArgMixOut: public ArgMix
    {
    public:
        //!
        //! Default constructor.
        //!
        ArgMixOut() = default;
        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ArgMixOut(const ArgMixOut& other) : ArgMix(other) {}
        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move.
        //!
        ArgMixOut(ArgMixOut&& other) noexcept : ArgMix(std::move(other)) {}
        //!
        //! Constructor from the address of an integer or enum data.
        //! @param [in] ptr Address of an integer or enum data.
        //!
        template<typename T> requires ts::int_enum<T>
        ArgMixOut(T* ptr) : ArgMix(reference_type<T>::value, sizeof(T), Value(ptr)) {}
        //!
        //! Constructor from the address of floating point data.
        //! @param [in] ptr Address of a double floating point.
        //!
        ArgMixOut(double* ptr) : ArgMix(POINTER | DOUBLE, 0, Value(ptr)) {}

    private:
        // Instances are directly built in initializer lists and cannot be assigned.
        ArgMixOut& operator=(ArgMixOut&&) = delete;
        ArgMixOut& operator=(const ArgMixOut&) = delete;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Return ArgMix value as an integer.
template <typename INT> requires std::integral<INT>
INT ts::ArgMix::toInteger(bool raw) const
{
    switch (_type) {
        case INTEGER | BIT32 | SIGNED:
            return static_cast<INT>(_value.int32);
        case INTEGER | BIT1: // bool
        case INTEGER | BIT32:
            return static_cast<INT>(_value.uint32);
        case INTEGER | BIT64 | SIGNED:
        case INTEGER | BIT64 | SIGNED | CHRONO:
            return static_cast<INT>(_value.int64);
        case INTEGER | BIT64:
            return static_cast<INT>(_value.uint64);
        case POINTER | INTEGER | BIT8  | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int8_t*>(_value.vptr));
        case POINTER | INTEGER | BIT8:
            return static_cast<INT>(*reinterpret_cast<uint8_t*>(_value.vptr));
        case POINTER | INTEGER | BIT16 | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int16_t*>(_value.vptr));
        case POINTER | INTEGER | BIT16:
            return static_cast<INT>(*reinterpret_cast<uint16_t*>(_value.vptr));
        case POINTER | INTEGER | BIT32 | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int32_t*>(_value.vptr));
        case POINTER | INTEGER | BIT32:
            return static_cast<INT>(*reinterpret_cast<uint32_t*>(_value.vptr));
        case POINTER | INTEGER | BIT64 | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int64_t*>(_value.vptr));
        case POINTER | INTEGER | BIT64:
            return static_cast<INT>(*reinterpret_cast<uint64_t*>(_value.vptr));
        case ANUMBER:
            return static_cast<INT>(_value.anumber->toInt64());
        default:
            return static_cast<INT>(0);
    }
}

// Store an integer value in the argument data, for pointers to integer.
template <typename INT> requires std::integral<INT>
bool ts::ArgMix::storeInteger(INT i) const
{
    switch (_type) {
        case POINTER | INTEGER | BIT8  | SIGNED:
            *reinterpret_cast<int8_t*>(_value.vptr) = static_cast<int8_t>(i);
            return true;
        case POINTER | INTEGER | BIT8:
            *reinterpret_cast<uint8_t*>(_value.vptr) = static_cast<uint8_t>(i);
            return true;
        case POINTER | INTEGER | BIT16 | SIGNED:
            *reinterpret_cast<int16_t*>(_value.vptr) = static_cast<int16_t>(i);
            return true;
        case POINTER | INTEGER | BIT16:
            *reinterpret_cast<uint16_t*>(_value.vptr) = static_cast<uint16_t>(i);
            return true;
        case POINTER | INTEGER | BIT32 | SIGNED:
            *reinterpret_cast<int32_t*>(_value.vptr) = static_cast<int32_t>(i);
            return true;
        case POINTER | INTEGER | BIT32:
            *reinterpret_cast<uint32_t*>(_value.vptr) = static_cast<uint32_t>(i);
            return true;
        case POINTER | INTEGER | BIT64 | SIGNED:
            *reinterpret_cast<int64_t*>(_value.vptr) = static_cast<int64_t>(i);
            return true;
        case POINTER | INTEGER | BIT64:
            *reinterpret_cast<uint64_t*>(_value.vptr) = static_cast<uint64_t>(i);
            return true;
        case POINTER | DOUBLE:
            *reinterpret_cast<double*>(_value.vptr) = static_cast<double>(i);
            return true;
        default:
            // Not a pointer to integer or float.
            return false;
    }
}

// Store a floating point value in the argument data, for pointers to double.
template <typename FLOAT> requires std::floating_point<FLOAT>
bool ts::ArgMix::storeFloat(FLOAT f) const
{
    if (_type == (POINTER | DOUBLE)) {
        *reinterpret_cast<double*>(_value.vptr) = static_cast<double>(f);
        return true;
    }
    else {
        // Not a pointer to double, try integer or error.
        return storeInteger(std::intmax_t(f));
    }
}
