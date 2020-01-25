//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/json
//

#ifndef BOOST_JSON_STRING_HPP
#define BOOST_JSON_STRING_HPP

#include <boost/json/config.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/detail/string_impl.hpp>
#include <boost/pilfer.hpp>
#include <algorithm>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace boost {
namespace json {

class value;

/** The native type of string values.

    Instances of string store and manipulate sequences
    of `char` using the UTF-8 encoding. The elements of
    a string are stored contiguously. A pointer to any
    character in a string may be passed to functions
    that expect a pointer to the first element of a
    null-terminated `char[]` array.

    String iterators are regular `char` pointers.

    @par Thread Safety

    Non-const member functions may not be called
    concurrently with any other member functions.

    @par Satisfies
    Meets the requirements of
        <em>Container</em>,
        <em>ContiguousContainer</em>,
        <em>ReversibleContainer</em>, and
        <em>SequenceContainer</em>.
*/
class string
{
    friend class value;

public:
    /// The traits used to perform character operations
    using traits_type       = std::char_traits<char>;

    /// The type of a character
    using value_type        = char;

    /// The type used to represent unsigned integers
    using size_type         = std::size_t;

    /// The type used to represent signed integers
    using difference_type   = std::ptrdiff_t;

    /// A pointer to an element
    using pointer           = char*;

    /// A const pointer to an element
    using const_pointer     = char const*;

    /// A reference to an element
    using reference         = char&;

    /// A const reference to an element
    using const_reference   = const char&;

    /// A random access iterator to an element
    using iterator          = char*;

    /// A random access const iterator  to an element
    using const_iterator    = char const*;

    /// A reverse random access iterator to an element
    using reverse_iterator =
        std::reverse_iterator<iterator>;

    /// A reverse random access const iterator to an element
    using const_reverse_iterator =
        std::reverse_iterator<const_iterator>;

    /// A special index
    static constexpr std::size_t npos =
        string_view::npos;

private:
    template<class T>
    using is_inputit = typename std::enable_if<
        std::is_convertible<typename
            std::iterator_traits<T>::value_type,
            char>::value>::type;

    static constexpr std::size_t mask_ = 0x0f;

    storage_ptr sp_; // must come first
    detail::string_impl impl_;

public:
    /** Destructor.

        Any dynamically allocated internal storage
        is freed.

        @par Complexity

        Constant.
    */
    ~string()
    {
        impl_.destroy(sp_);
    }

    //
    // Construction
    //

    /** Default constructor.

        The string will have a zero size and a non-zero,
        unspecified capacity, using the default storage.

        @par Complexity

        Constant.
    */
    string() = default;

    /** Pilfer constructor.

        Constructs the string with the contents of `other`
        using pilfer semantics.
        Ownership of the @ref storage is transferred.

        @note

        After construction, the pilfered-from string may
        only be destroyed.
        
        @par Complexity

        Constant.

        @param other The string to pilfer

        @see @ref pilfer
        
        Pilfering constructors are described in
        <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0308r0.html">Valueless Variants Considered Harmful</a>, by Peter Dimov.
    */
    string(pilfered<string> other) noexcept
        : sp_(std::move(other.get().sp_))
        , impl_(other.get().impl_)
    {
        ::new(&other.get().impl_) detail::string_impl();
    }

    /** Constructor.

        The string will have zero size and a non-zero,
        unspecified capacity, obtained from the specified storage.

        @par Complexity

        Constant.

        @param sp A pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
    */
    explicit
    string(storage_ptr sp)
        : sp_(std::move(sp))
    {
    }

    /** Constructor.

        Construct the contents with `count` copies of
        character `ch`.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param count The size of the resulting string.

        @param ch THe value to initialize characters
        of the string with.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::length_error `count > max_size()`.
    */
    string(
        std::size_t count,
        char ch,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(count, ch);
    }

    /** Constructor.

        Construct the contents with a copy of the
        substring `{pos, pos+count)` of `other`. If
        the requested substring lasts past the
        end of the string, or if `count == npos`,
        the resulting substring is `{pos, other.size())`.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param other The string to use as a source
        to copy from.

        @param pos The starting character position to
        copy from.

        @param count The number of characters to copy.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::out_of_range `pos >= other.size()`.
    */
    string(
        string const& other,
        std::size_t pos,
        std::size_t count = npos,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(other, pos, count);
    }

    /** Constructor.

        Construct the contents with those of the null
        terminated string pointed to by `s`. The length
        of the string is determined by the first null
        character.

        @par Complexity

        Linear in `strlen(s)`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param s A pointer to a character string used to
        copy from.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::length_error `strlen(s) > max_size()`.
    */
    string(
        char const* s,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(s);
    }

    /** Constructor.

        Construct the contents with copies of the characters
        in the range `{s, s+count)`. This range can contain
        null characters.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param count The number of characters to copy.

        @param s A pointer to a character string used to
        copy from.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::length_error `count > max_size()`.
    */
    string(
        char const* s,
        std::size_t count,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(s, count);
    }

    /** Constructor.

        Construct the contents with copies of characters
        in the range `{first, last)`.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @par Complexity

        Linear in `std::distance(first, last)`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param first An input iterator pointing to the
        first character to insert, or pointing to the
        end of the range.

        @param last An input iterator pointing to the end
        of the range.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @tparam InputIt The type of the iterators.

        @throw std::length_error `std::distance(first, last) > max_size()`.
    */
    template<class InputIt
    #ifndef GENERATING_DOCUMENTATION
        ,class = is_inputit<InputIt>
    #endif
    >
    string(
        InputIt first,
        InputIt last,
        storage_ptr sp = {});

    /** Copy constructor.

        Construct the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param other The string to use as a source
        to copy from.
    */
    string(string const& other)
        : sp_(other.sp_)
    {
        assign(other);
    }

    /** Constructor.

        Construct the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param other The string to use as a source
        to copy from.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.
    */
    string(
        string const& other,
        storage_ptr sp)
        : sp_(std::move(sp))
    {
        assign(other);
    }

    /** Move constructor.

        Constructs the string with the contents of `other`
        using move semantics. Ownership of the underlying
        memory is transferred.
        The container acquires shared ownership of the
        @ref storage used by `other`. After construction, 
        the moved-from string behaves as if newly constructed 
        with its current storage pointer.
        
        @par Complexity

        Constant.

        @param other The string to move
    */
    string(string&& other) noexcept
        : sp_(other.sp_)
        , impl_(other.impl_)
    {
        ::new(&other.impl_) detail::string_impl();
    }

    /** Constructor.

        Construct the contents with those of `other`
        using move semantics.

        @li If `*other.storage() == *sp`,
        ownership of the underlying memory is transferred
        in constant time, with no possibility of exceptions.
        After construction, the moved-from string behaves
        as if newly constructed with its current @ref storage.

        @li if `*other.storage() != *sp`,
        a copy of the characters in `other` is made. In this
        case, the moved-from container is not changed.

        @par Complexity

        Constant or linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param other The string to assign from.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.
    */
    string(
        string&& other,
        storage_ptr sp)
        : sp_(std::move(sp))
    {
        assign(std::move(other));
    }

    /** Constructor.

        Construct the contents with those of the
        initializer list `init`.

        @par Complexity

        Linear in `init.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param init The initializer list to copy from.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::length_error `init.size() > max_size()`.
    */
    string(
        std::initializer_list<char> init,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(init);
    }

    /** Constructor.

        Construct the contents with those of a
        string view. This view can contain
        null characters.

        @par Complexity

        Linear in `s.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param s The string view to copy from.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::length_error `s.size() > max_size()`.
    */
    string(
        string_view s,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(s);
    }

    /** Constructor.

        Constructs the contents with the characters
        from the substring `{pos, pos+count)` of `s`.
        If `count == npos`, if `count` is not specified,
        or if the requested substring lasts past the end
        of the string, the resulting substring is `{pos, s.size())`.
        The substring can contain null characters.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param s The string view to copy from.

        @param pos The starting character position to
        copy from.

        @param count The number of characters to copy.

        @param sp An optional pointer to the @ref storage
        to use. The container will acquire shared
        ownership of the storage object.
        The default argument for this parameter is `{}`.

        @throw std::out_of_range `pos >= s.size()`

        @throw std::length_error `count > max_size()`.
    */
    string(
        string_view s,
        std::size_t pos,
        std::size_t count,
        storage_ptr sp = {})
        : sp_(std::move(sp))
    {
        assign(s.substr(pos, count));
    }

    //------------------------------------------------------
    //
    // Assignment
    //
    //------------------------------------------------------

    /** Copy assignment.

        Replace the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param other The string to use as a source
        to copy from.
    */
    string&
    operator=(string const& other)
    {
        return assign(other);
    }

    /** Move assignment.

        Replace the contents with those of `other`
        using move semantics.

        @li If `*other.storage() == *this->storage()`,
        ownership of the underlying memory is transferred
        in constant time, with no possibility of exceptions.
        After construction, the moved-from string behaves
        as if newly constructed with its current @ref storage.

        @li if `*other.storage() != *this->storage()`,
        a copy of the characters in `other` is made. In this
        case, the moved-from container is not changed.

        @par Complexity

        Constant or linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param other The string to use as a source
        to move from.
    */
    string&
    operator=(string&& other)
    {
        return assign(std::move(other));
    }

    /** Assign a value to the string.

        Replaces the contents with those of the null
        terminated string pointed to by `s`. The length
        of the string is determined by the first null
        character.

        @par Complexity

        Linear in `strlen(s)`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param s A pointer to a character string used to
        copy from.

        @throw std::length_error `strlen(s) > max_size()`.
    */
    string&
    operator=(char const* s)
    {
        return assign(s);
    }

    /** Assign a value to the string.

        Replaces the contents with those of the
        initializer list `init`.

        @par Complexity

        Linear in `init.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param init The initializer list to copy from.

        @throw std::length_error `init.size() > max_size()`.
    */
    string&
    operator=(std::initializer_list<char> init)
    {
        return assign(init);
    }

    /** Assign a value to the string.

        Replaces the contents with those of a
        string view. This view can contain
        null characters.

        @par Complexity

        Linear in `s.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param s The string view to copy from.

        @throw std::length_error `s.size() > max_size()`.
    */
    string&
    operator=(string_view s)
    {
        return assign(s);
    }

    //------------------------------------------------------

    /** Assign characters to a string.

        Replace the contents with `count` copies of
        character `ch`.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param count The size of the resulting string.

        @param ch THe value to initialize characters
        of the string with.

        @throw std::length_error `count > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    assign(
        std::size_t count,
        char ch);

    /** Assign characters to a string.

        Replace the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param other The string to use as a source
        to copy from.
    */
    BOOST_JSON_DECL
    string&
    assign(
        string const& other);

    /** Assign characters to a string.

        Replace the contents with a copy of the
        substring `{pos, pos+count)` of `other`. If
        the requested substring lasts past the
        end of the string, or if `count == npos`,
        the resulting substring is `{pos, other.size())`.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param other The string to use as a source
        to copy from.

        @param pos The starting character position to
        copy from.

        @param count The number of characters to copy.

        @throw std::out_of_range `pos >= other.size()`.
    */
    string&
    assign(
        string const& other,
        std::size_t pos,
        std::size_t count)
    {
        return assign(
            other.substr(pos, count));
    }

    /** Assign characters to a string.

        Replace the contents with those of `other`
        using move semantics.

        @li If `*other.storage() == *this->storage()`,
        ownership of the underlying memory is transferred
        in constant time, with no possibility of exceptions.
        After construction, the moved-from string behaves
        as if newly constructed with its current @ref storage.

        @li if `*other.storage() != *this->storage()`,
        a copy of the characters in `other` is made. In this
        case, the moved-from container is not changed.

        @par Complexity

        Constant or linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param other The string to assign from.
    */
    BOOST_JSON_DECL
    string&
    assign(string&& other);

    /** Assign characters to a string.

        Replaces the contents with copies of the characters
        in the range `{s, s+count)`. This range can contain
        null characters.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param count The number of characters to copy.

        @param s A pointer to a character string used to
        copy from.

        @throw std::length_error `count > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    assign(
        char const* s,
        std::size_t count);

    /** Assign characters to a string.

        Replaces the contents with those of the null
        terminated string pointed to by `s`. The length
        of the string is determined by the first null
        character.

        @par Complexity

        Linear in `strlen(s)`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param s A pointer to a character string used to
        copy from.

        @throw std::length_error `strlen(s) > max_size()`.
    */
    string&
    assign(
        char const* s)
    {
        return assign(s,
            traits_type::length(s));
    }

    /** Assign characters to a string.

        Replaces the contents with copies of characters
        in the range `{first, last)`.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @par Complexity

        Linear in `std::distance(first, last)`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param first An input iterator pointing to the
        first character to insert, or pointing to the
        end of the range.

        @param last An input iterator pointing to the end
        of the range.

        @tparam InputIt The type of the iterators.

        @throw std::length_error `std::distance(first, last) > max_size()`.
    */
    template<class InputIt
    #ifndef GENERATING_DOCUMENTATION
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    assign(
        InputIt first,
        InputIt last);

    /** Assign characters to a string.

        Replaces the contents with those of the
        initializer list `init`.

        @par Complexity

        Linear in `init.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param init The initializer list to copy from.

        @throw std::length_error `init.size() > max_size()`.
    */
    string&
    assign(std::initializer_list<char> init)
    {
        return assign(init.begin(), init.size());
    }

    /** Assign characters to a string.

        Replaces the contents with those of a
        string view. This view can contain
        null characters.

        @par Complexity

        Linear in `s.size()`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param s The string view to copy from.

        @throw std::length_error `s.size() > max_size()`.
    */
    string&
    assign(string_view s)
    {
        return assign(s.data(), s.size());
    }

    /** Assign characters to a string.

        Replaces the contents with the characters
        from the substring `{pos, pos+count)` of `s`.
        If `count == npos`, if `count` is not specified,
        or if the requested substring lasts past the end
        of the string, the resulting substring is `{pos, s.size())`.
        The substring can contain null characters.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @return `*this`

        @param s The string view to copy from.

        @param pos The starting character position to
        copy from.

        @param count The number of characters to copy. 
        The default argument for this parameter is `npos`

        @throw std::out_of_range `pos >= s.size()`

        @throw std::length_error `count > max_size()`.
    */
    string&
    assign(
        string_view s,
        std::size_t pos,
        std::size_t count = npos)
    {
        return assign(s.substr(pos, count));
    }

    //------------------------------------------------------

    /** Access the storage associated with the container.

        Returns a pointer to the storage associated with the container

        @note 
        
        Shared ownership of the @ref storage is propagated by
        the container to all of its children recursively.

        @par Complexity

        Constant.
    */
    storage_ptr const&
    storage() const noexcept
    {
        return sp_;
    }

    //------------------------------------------------------
    //
    // Element Access
    //
    //------------------------------------------------------

    /** Access a character with bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Complexity

        Constant.

        @par Exception Safety

        Strong guarantee.

        @param pos A zero-based index to access

        @throw std::out_of_range `pos >= size()`
    */
    char&
    at(std::size_t pos)
    {
        if(pos >= size())
            BOOST_THROW_EXCEPTION(
                std::out_of_range(
                    "pos >= size()"));
        return impl_.data()[pos];
    }

    /** Access a character with bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Complexity

        Constant.

        @par Exception Safety

        Strong guarantee.

        @param pos A zero-based index to access

        @throw std::out_of_range `pos >= size()`
    */
    char const&
    at(std::size_t pos) const
    {
        if(pos >= size())
            BOOST_THROW_EXCEPTION(
                std::out_of_range(
                    "pos >= size()"));
        return impl_.data()[pos];
    }

    /** Access a character without bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Precondition
        
        `pos >= size`

        @par Complexity

        Constant.

        @param pos A zero-based index to access
    */
    char&
    operator[](std::size_t pos)
    {
        return impl_.data()[pos];
    }

   /**  Access a character without bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Precondition
        
        `pos >= size`

        @par Complexity

        Constant.

        @param pos A zero-based index to access
    */
    const char&
    operator[](std::size_t pos) const
    {
        return impl_.data()[pos];
    }

    /** Access the first character.

        Returns a reference to the first character.

        @par Precondition

        `not empty()`

        @par Complexity

        Constant.
    */
    char&
    front()
    {
        return impl_.data()[0];
    }

    /** Access the first character.

        Returns a reference to the first character.

        @par Precondition

        `not empty()`

        @par Complexity

        Constant.
    */
    char const&
    front() const
    {
        return impl_.data()[0];
    }

    /** Access the last character.

        Returns a reference to the last character.

        @par Precondition

        `not empty()`

        @par Complexity

        Constant.
    */
    char&
    back()
    {
        return impl_.data()[impl_.size() - 1];
    }

    /** Access the last character.

        Returns a reference to the last character.

        @par Precondition

        `not empty()`

        @par Complexity

        Constant.
    */
    char const&
    back() const
    {
        return impl_.data()[impl_.size() - 1];
    }

    /** Access the underlying character array directly.

        Returns a pointer to the underlying array
        serving as storage. The value returned is such that
        the range `{data(), data()+size())` is always a
        valid range, even if the container is empty.

        @par Complexity

        Constant.

        @note The value returned from
        this function is never equal to `nullptr`.
    */
    char*
    data() noexcept
    {
        return impl_.data();
    }

    /** Access the underlying character array directly.

        Returns a pointer to the underlying array
        serving as storage. 
        
        @note The value returned is such that
        the range `[data(), data()+size())` is always a
        valid range, even if the container is empty. 
        The value returned from
        this function is never equal to `nullptr`.

        @par Complexity

        Constant.
    */
    char const*
    data() const noexcept
    {
        return impl_.data();
    }

    /** Access the underlying character array directly.

        Returns a pointer to the underlying array
        serving as storage. The value returned is such that
        the range `[c_str(), c_str()+c_str())` is always a
        valid range, even if the container is empty.

        @par Complexity

        Constant.

        @note The value returned from
        this function is never equal to `nullptr`.
    */
    char const*
    c_str() const noexcept
    {
        return impl_.data();
    }

    /** Convert to a `string view` referring to the string.

        Returns a string view to the
        underlying character string. The size of the view does
        not include the null terminator.

        @par Complexity

        Constant.
    */
    operator string_view() const noexcept
    {
        return {data(), size()};
    }

    //------------------------------------------------------
    //
    // Iterators
    //
    //------------------------------------------------------

    /** Return an iterator to the first character.

        If the container is empty, the returned iterator
        will be equal to @ref end().

        @par Complexity

        Constant.
    */
    iterator
    begin() noexcept
    {
        return impl_.data();
    }

    /** Return an iterator to the first character.

        If the container is empty, the returned iterator
        will be equal to @ref end().

        @par Complexity

        Constant.
    */
    const_iterator
    begin() const noexcept
    {
        return impl_.data();
    }

    /** Return an iterator to the first character.

        If the container is empty, the returned iterator
        will be equal to @ref end().

        @par Complexity

        Constant.
    */
    const_iterator
    cbegin() const noexcept
    {
        return impl_.data();
    }

    /** Return an iterator to the character following the last character.

        The character acts as a placeholder; attempting to
        access it results in undefined behavior.

        @par Complexity

        Constant.
    */
    iterator
    end() noexcept
    {
        return impl_.end();
    }

    /** Return an iterator to the character following the last character.

        The character acts as a placeholder; attempting to
        access it results in undefined behavior.

        @par Complexity

        Constant.
    */
    const_iterator
    end() const noexcept
    {
        return impl_.end();
    }

    /** Return an iterator to the character following the last character.

        The character acts as a placeholder; attempting to
        access it results in undefined behavior.

        @par Complexity

        Constant.
    */
    const_iterator
    cend() const noexcept
    {
        return impl_.end();
    }

    /** Return a reverse iterator to the first character of the reversed container.

        Returns the pointed-to character that corresponds to the last character
        of the non-reversed container. 
        
        @note If the container is empty,
        the returned iterator is equal to @ref rend()

        @par Complexity

        Constant.
    */
    reverse_iterator
    rbegin() noexcept
    {
        return reverse_iterator(impl_.end());
    }

    /** Return a reverse iterator to the first character of the reversed container.

        Returns the pointed-to character that corresponds to the last character
        of the non-reversed container.
        
        @note If the container is empty,
        the returned iterator is equal to @ref rend()

        @par Complexity

        Constant.
    */
    const_reverse_iterator
    rbegin() const noexcept
    {
        return const_reverse_iterator(impl_.end());
    }

    /** Return a reverse iterator to the first character of the reversed container.

        Returns the pointed-to character that corresponds to the last character
        of the non-reversed container.
        
        @note If the container is empty,
        the returned iterator is equal to @ref rend()

        @par Complexity

        Constant.
    */
    const_reverse_iterator
    crbegin() const noexcept
    {
        return const_reverse_iterator(impl_.end());
    }

    /** Return a reverse iterator to the character following the last character of the reversed container.

        Returns the pointed-to character that corresponds to the character
        preceding the first character of the non-reversed container. 
        
        @note This character acts as a placeholder, attempting to access
        it results in undefined behavior.

        @par Complexity

        Constant.
    */
    reverse_iterator
    rend() noexcept
    {
        return reverse_iterator(begin());
    }

    /** Return a reverse iterator to the character following the last character of the reversed container.

        Returns the pointed-to character that corresponds to the character
        preceding the first character of the non-reversed container. 

        @note This character acts as a placeholder, attempting to access
        it results in undefined behavior.

        @par Complexity

        Constant.
    */
    const_reverse_iterator
    rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    /** Return a reverse iterator to the character following the last character of the reversed container.

        Returns the pointed-to character that corresponds to the character
        preceding the first character of the non-reversed container. 
        
        @note This character acts as a placeholder, attempting to access
        it results in undefined behavior.

        @par Complexity

        Constant.
    */
    const_reverse_iterator
    crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    //------------------------------------------------------
    //
    // Capacity
    //
    //------------------------------------------------------

    /** Check if the string has no characters.

        Returns `true` if there are no characters in
        the string, i.e. @ref size() returns 0.

        @par Complexity

        Constant.
    */
    bool
    empty() const noexcept
    {
        return impl_.size() == 0;
    }

    /** Return the number of characters in the string.

        The value returned does not include the
        null terminator, which is always present.

        @par Complexity

        Constant.
    */
    std::size_t
    size() const noexcept
    {
        return impl_.size();
    }

    /** Return the maximum number of characters the string can hold.

        The maximum is an implementation-defined number.
        This value is a theoretical limit; at runtime,
        the actual maximum size may be less due to
        resource limits.

        @par Complexity

        Constant.
    */
    static
    constexpr
    std::size_t
    max_size() noexcept
    {
        return BOOST_JSON_MAX_STRING_SIZE;
    }

    /** Return the number of characters that can be held without a reallocation.

        This number represents the largest number of
        characters that may be currently held without
        reallocating the existing storage.
        This number may be larger than the value returned
        by @ref size().

        @par Complexity

        Constant.
    */
    std::size_t
    capacity() const noexcept
    {
        return impl_.capacity();
    }

    /** Increase the capacity to at least a certain amount.

        This increases the capacity of the array to a value
        that is greater than or equal to `new_capacity`. If
        `new_capacity > capacity()`, new memory is allocated.
        Otherwise, the call has no effect. The number of
        elements and therefore the @ref size() of the container
        is not changed.

        @note

        If new memory is allocated, all iterators including any
        past-the-end iterators, and all references to the
        elements are invalidated. Otherwise, no iterators or
        references are invalidated.

        @par Complexity

        At most, linear in @ref size().

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to @ref storage::allocate may throw.

        @param new_capacity The new capacity of the array.

        @throw std::length_error `new_capacity > max_size()`
    */
    void
    reserve(std::size_t new_capacity)
    {
        if(new_capacity <= capacity())
            return;
        reserve_impl(new_capacity);
    }

    /** Request the removal of unused capacity.

        This performs a non-binding request to reduce
        @ref capacity() to @ref size(). The request may
        or may not be fulfilled. 
        
        @note If reallocation occurs, all iterators including 
        any past-the-end iterators, and all 
        references to characters are invalidated.
        Otherwise, no iterators or references are
        invalidated.

        @par Complexity

        At most, linear in @ref size().
    */
    BOOST_JSON_DECL
    void
    shrink_to_fit();

    //------------------------------------------------------
    //
    // Operations
    //
    //------------------------------------------------------

    /** Clear the contents.

        Erases all characters from the string. After this
        call, @ref size() returns zero but @ref capacity()
        is unchanged. 
        
        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Complexity

        Linear in @ref size().
    */
    BOOST_JSON_DECL
    void
    clear() noexcept;

    //------------------------------------------------------

    /** Insert a character.
        
        Inserts `count` copies of `ch` at the position `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param pos The index to insert at.
        @param count The number of characters to insert.
        @param ch The character to insert.

        @throw std::length_error `size() + count > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    insert(
        std::size_t pos,
        std::size_t count,
        char ch);

    /** Insert a string.

        Inserts the null-terminated character string pointed to by `s`
        at the position `pos`. The length of the string is determined
        by using `traits_type::length(s)`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param pos The index to insert at.
        @param s The string to insert.

        @throw std::length_error `size() + traits_type::length(s) > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    string&
    insert(
        std::size_t pos,
        char const* s)
    {
        return insert(pos, s,
            traits_type::length(s));
    }

    /** Insert a string.

        Inserts `count` characters of the string pointed to by `s`
        at the position `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param pos The index to insert at.
        @param s The string to insert.
        @param count The length of the string to insert.

        @throw std::length_error `size() + count > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    insert(
        std::size_t pos,
        char const* s,
        std::size_t count);

    /** Insert a string.

        Inserts the string pointed to by `s`
        at the position `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param pos The index to insert at.
        @param s The string to insert.

        @throw std::length_error `size() + s.size() > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    string&
    insert(
        std::size_t pos,
        string const& s)
    {
        return insert(pos, s.data(), s.size());
    }

    /** Insert a string.

        Inserts a string, obtained by `str.substr(pos_str, count)`
        at the position `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param pos The index to insert at.
        @param s The string from which to insert.
        @param pos_str The index in `s` to start inserting from.
        @param count The number of characters to insert. 
        The default argument for this parameter is `npos`.

        @throw std::length_error `size() + str.substr(pos_str, count).size() > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    string&
    insert(
        std::size_t pos,
        string const& s,
        std::size_t pos_str,
        std::size_t count = npos)
    {
        return insert(pos, s.substr(pos_str, count));
    }

    /** Insert a character.

        Inserts the character `ch` before the character pointed by `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return An iterator which refers to the first inserted character
        or `pos` if no characters were inserted

        @param pos The index to insert at.
        @param ch The character to insert.

        @par Precondition

        `pos` shall be vaild within `[data(), data() + size()]`

        @throw std::length_error `size() + 1 > max_size()`
    */
    iterator
    insert(
        const_iterator pos,
        char ch)
    {
        return insert(pos, 1, ch);
    }

    /** Insert characters.

        Inserts `count` copies of `ch` before the character pointed by `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return An iterator which refers to the first inserted character
        or `pos` if no characters were inserted

        @param pos The position to insert at.
        @param count The number of characters to insert.
        @param ch The character to insert.

        @par Precondition

        `pos` shall be valid within `[data(), data() + size()]`

        @throw std::length_error `size() + count > max_size()`
    */
    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        std::size_t count,
        char ch);

    /** Insert a range of characters.

        Inserts characters from the range `[first, last)` before the
        character pointed to by `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @par Exception Safety

        Strong guarantee.

        @par Precondition

        `pos` shall be valid within `[data(), data() + size()]`, 
        
        `[first, last)` shall be a valid range

        @return An iterator which refers to the first inserted character
        or `pos` if no characters were inserted

        @param pos The position to insert at.
        @param first An iterator representing the first character to insert.
        @param last An iterator representing one past the last character to insert.

        @tparam InputIt The type of the iterators.

        @throw std::length_error `size() + insert_count > max_size()`
    */
    template<class InputIt
    #ifndef GENERATING_DOCUMENTATION
        ,class = is_inputit<InputIt>
    #endif
    >
    iterator
    insert(
        const_iterator pos,
        InputIt first,
        InputIt last);

    /** Insert characters from an `initializer_list`.

        Inserts characters from `init` before `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return An iterator which refers to the first inserted character
        or `pos` if no characters were inserted

        @param pos The position to insert at.
        @param init The `initializer_list` from which to insert.

        @par Precondition

        `pos` shall be valid within `[data(), data() + size()]`

        @throw std::length_error `size() + init.size() > max_size()`
    */
    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        std::initializer_list<char> init);

    /** Insert characters from an object convertible to `string_view`.

        Constructs a temporary `string_view` object `sv` from `t` and
        inserts `[sv.begin(), sv.end())` at `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @par Constraints

        `T` satisfies __is_string_viewish__.

        @param pos The index to insert at.
        @param t The object to insert from.

        @tparam T The type of the object to convert.

        @throw std::length_error `size() + sv.size() > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    insert(
        std::size_t pos,
        T const& t)
    {
        string_view s(t);
        return insert(pos, s.data(), s.size());
    }

    /** Insert characters from an object convertible to `string_view`.

        Constructs a temporary `string_view` object `sv` from `t`
        and inserts `sv.substr(pos_str, count)` at `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @par Constraints

        `T` satisfies __is_string_viewish__.

        @return `*this`

        @param pos The index to insert at.
        @param t The object to insert from.
        @param pos_str The index in the temporary `string_view` object 
        to start the substring from.
        @param count The number of characters to insert.

        @tparam T The type of the object to convert.

        @throw std::length_error `size() + sv.size() > max_size()`
        @throw std::out_of_range `pos > size()`
    */
    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    insert(
        std::size_t pos,
        T const& t,
        std::size_t pos_str,
        std::size_t count = npos)
    {
        return insert(pos,
            string_view(t).substr(pos_str, count));
    }

    //------------------------------------------------------

    /** Erase characters from the string.
        
        Erases `num` characters from the string, starting at `pos`.
        `num` is determined as the the smaller of `count` and `size() - pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param pos The index to erase at.
        The default argument for this parameter is `0`.
        @param count The number of characters to erase.
        The default argument for this parameter is `npos`.
        
        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    erase(
        std::size_t pos = 0,
        std::size_t count = npos);

    /** Erase a character from the string.
        
        Erases the character at `pos`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @par Precondition

        `pos` shall be valid within `[data(), data() + size()]`

        @return An iterator referring to character immediately following
        the erased character, or @ref end() if one does not exist.

        @param pos An iterator referring to the character to erase.
    */
    BOOST_JSON_DECL
    iterator
    erase(const_iterator pos);

    /** Erase a range from the string.

        Erases the characters in the range `[first, last)`.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Exception Safety

        Strong guarantee.

        @par Precondition

        `[first, last]` shall be valid within `[data(), data() + size()]`

        @return An iterator referring to the character `last` previously referred to, or @ref end() if one does not exist.

        @param first An iterator representing the first character to erase.
        @param last An iterator representing one past the last character to erase.
    */
    BOOST_JSON_DECL
    iterator
    erase(
        const_iterator first,
        const_iterator last);

    //------------------------------------------------------

    /** Append a character.
        
        Appends a character to the end of the string.

        @par Exception Safety

        Strong guarantee.
        
        @param ch The character to append.
        
        @throw std::length_error `size() + 1 > max_size()`
    */
    BOOST_JSON_DECL
    void
    push_back(char ch);

    /** Remove a character.
        
        Removes a character from the end of the string.

        @par Precondition

        `not empty()`
    */
    BOOST_JSON_DECL
    void
    pop_back();

    //------------------------------------------------------

    /** Append characters to the string.
        
        Appends `count` copies of `ch` to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param count The number of characters to append.
        @param ch The character to append.

        @throw std::length_error `size() + count > max_size()`
    */
    BOOST_JSON_DECL
    string&
    append(
        std::size_t count,
        char ch);

    /** Append a another string to the string.

        Appends `s` the end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param s The string to append.

        @throw std::length_error `size() + s.size() > max_size()`
    */
    string&
    append(string const& s)
    {
        return append(
            s.data(), s.size());
    }

    /** Append a string to the string.

        Appends a string, obtained by `s.substr(pos, count)`
        to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param s The string to append.
        @param pos The position from which to begin the substring.
        @param count The number of characters to append.
        The default argument for this parameter is `npos`.

        @throw std::length_error `size() + s.substr(pos, count).size() > max_size()`
        @throw std::out_of_range `pos > s.size()`
    */
    string&
    append(
        string const& s,
        std::size_t pos,
        std::size_t count = npos)
    {
        return append(
            s.substr(pos, count));
    }

    /** Append a null terminated string to the string.

        Appends `count` characters from the null terminated
        string pointed to by `s` to the end of the string.
        `count` is obtained using `traits_type::length(s)`.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param s The string to append.

        @throw std::length_error `size() + count > max_size()`
    */
    string&
    append(char const* s)
    {
        return append(s,
            traits_type::length(s));
    }

    /** Append a null terminated string to the string.

        Appends `count` characters from the null terminated
        string pointed to by `s` to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param s The string to append.
        @param count The number of characters to append.

        @throw std::length_error `size() + count > max_size()`
    */
    BOOST_JSON_DECL
    string&
    append(
        char const* s,
        std::size_t count);

    /** Append a range of characters.

        Appends characters from the range `[first, last)`
        to the end of the string.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @par Exception Safety

        Strong guarantee.

        @par Precondition

        `[first, last)` shall be a valid range

        @return `*this`

        @param first An iterator representing the
        first character to append.
        @param last An iterator representing one past the
        last character to append.

        @tparam InputIt The type of the iterators.

        @throw std::length_error `size() + insert_count > max_size()`
    */
    template<class InputIt
    #ifndef GENERATING_DOCUMENTATION
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    append(InputIt first, InputIt last);

    /** Append characters from an `initializer_list`.

        Appends characters from `init` to the
        end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param init The `initializer_list` to append.

        @throw std::length_error `size() + init.size() > max_size()`
    */
    string&
    append(std::initializer_list<char> init)
    {
        return append(init.begin(), init.size());
    }

    /** Append characters from an object convertible to `string_view`.

        Constructs a temporary `string_view` object `sv` from `t` and
        appends `[sv.begin(), sv.end())` to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @par Constraints

        `T` satisfies __is_string_viewish__.

        @param t The object to append.

        @tparam T The type of the object to convert.

        @throw std::length_error `size() + sv.size() > max_size()`
    */
    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    append(T const& t)
    {
        string_view s(t);
        return append(s.data(), s.size());
    }

    /** Append characters from an object convertible to `string_view`.

        Constructs a temporary `string_view` object `sv` from `t`
        and appends `sv.substr(pos_str, count)` to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @par Constraints

        `T` satisfies __is_string_viewish__.

        @return `*this`

        @param t The object to append.
        @param pos The index to begin the substring from.
        @param count The number of characters to append.
        The default argument for this parameter is `npos`.

        @tparam T The type of the object to convert.

        @throw std::length_error `size() + sv.size() > max_size()`
        @throw std::out_of_range `pos > sv.size()`
    */
    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    append(
        T const& t,
        std::size_t pos,
        std::size_t count = npos)
    {
        auto s = string_view(t).substr(pos, count);
        return append(s.data(), s.size());
    }

    //------------------------------------------------------

    string&
    operator+=(string const& s)
    {
        return append(s);
    }

    string&
    operator+=(char ch)
    {
        push_back(ch);
        return *this;
    }

    string&
    operator+=(char const* s)
    {
        return append(s);
    }

    string&
    operator+=(std::initializer_list<char> init)
    {
        return append(init);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    operator+=(T const& t)
    {
        return append(t);
    }

    //------------------------------------------------------

    int
    compare(string const& s) const noexcept
    {
        return string_view(*this).compare(
            string_view(s));
    }

    int
    compare(
        std::size_t pos1,
        std::size_t count1,
        string const& s) const
    {
        return string_view(*this).compare(
            pos1, count1, string_view(s));
    }

    int
    compare(
        std::size_t pos1,
        std::size_t count1,
        string const& s,
        std::size_t pos2,
        std::size_t count2 = npos) const
    {
        return string_view(*this).compare(
            pos1, count1, string_view(s),
            pos2, count2);
    }

    int
    compare(char const* s) const
    {
        return string_view(*this).compare(s);
    }

    int
    compare(
        std::size_t pos1,
        std::size_t count1,
        char const* s) const
    {
        return string_view(*this).compare(
            pos1, count1, s);
    }

    int
    compare(
        std::size_t pos1,
        std::size_t count1,
        char const* s,
        std::size_t count2) const
    {
        return string_view(*this).compare(
            pos1, count1, s, count2);
    }

    int
    compare(string_view s) const noexcept
    {
        return string_view(*this).compare(s);
    }

    int
    compare(
        std::size_t pos1,
        std::size_t count1,
        string_view s) const
    {
        return string_view(*this).compare(
            pos1, count1, s);
    }

    int
    compare(
        std::size_t pos1,
        std::size_t count1,
        string_view s,
        std::size_t pos2,
        std::size_t count2 = npos) const
    {
        return string_view(*this).compare(
            pos1, count1, s, pos2, count2);
    }

    //------------------------------------------------------

    bool
    starts_with(string_view s) const noexcept
    {
        return substr(0, s.size()) == s;
    }

    bool
    starts_with(char ch) const noexcept
    {
        return ! empty() && front() == ch;
    }

    bool 
    starts_with(char const* s) const
    {
        return starts_with(string_view(s));
    }

    bool
    ends_with(string_view s) const noexcept
    {
        return size() >= s.size() &&
            substr(size() - s.size()) == s;
    }

    bool
    ends_with(char ch) const noexcept
    {
        return ! empty() && back() == ch;
    }

    bool 
    ends_with(char const* s) const
    {
        return ends_with(string_view(s));
    }

    //------------------------------------------------------

    string&
    replace(
        std::size_t pos,
        std::size_t count,
        string const& s);

    string&
    replace(
        const_iterator first,
        const_iterator last,
        string const& s);

    string&
    replace(
        std::size_t pos,
        std::size_t count,
        string const& s,
        std::size_t pos2,
        std::size_t count2 = npos);

    template<class InputIt
    #ifndef GENERATING_DOCUMENTATION
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    replace(
        const_iterator first,
        const_iterator last,
        InputIt first2,
        InputIt last2);

    string&
    replace(
        std::size_t pos,
        std::size_t count,
        char const* s,
        std::size_t count2);

    string&
    replace(
        const_iterator first,
        const_iterator last,
        char const* s,
        std::size_t count2);

    string&
    replace(
        std::size_t pos,
        std::size_t count,
        char const* s);

    string&
    replace(
        const_iterator first,
        const_iterator last,
        char const* s);

    string&
    replace(
        std::size_t pos,
        std::size_t count,
        std::size_t count2,
        char ch);

    string&
    replace(
        const_iterator first,
        const_iterator last,
        std::size_t count2,
        char ch);

    string&
    replace(
        const_iterator first,
        const_iterator last,
        std::initializer_list<char> init);

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    replace(
        std::size_t pos,
        std::size_t count,
        T const& t);

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    replace(
        const_iterator first,
        const_iterator last,
        T const& t);

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    string&
    replace(
        std::size_t pos,
        std::size_t count,
        T const& t,
        std::size_t pos2,
        std::size_t count2 = npos);

    //------------------------------------------------------

    string_view
    substr(
        std::size_t pos = 0,
        std::size_t count = npos) const
    {
        return string_view(*this).substr(pos, count);
    }

    //------------------------------------------------------

    std::size_t
    copy(
        char* dest,
        std::size_t count,
        std::size_t pos = 0) const
    {
        return string_view(*this).copy(dest, count, pos);
    }

    //------------------------------------------------------

    void
    resize(std::size_t count)
    {
        resize(count, 0);
    }

    BOOST_JSON_DECL
    void
    resize(std::size_t count, char ch);

    /** Increase size without changing capacity.

        This increases the size of the string by `n`
        characters, adjusting the position of the
        terminating null for the new size. The new
        characters remain uninitialized. This function
        may be used to append characters directly into
        the storage between `end()` and
        `data() + capacity()`.

        @par Precondition

        @code
        count <= capacity() - size()
        @endcode

        @par Exception Safety
        
        No-throw guarantee.

        @param n The amount to increase the size by.
    */
    void
    grow(std::size_t n) noexcept
    {
        BOOST_ASSERT(
            n <= impl_.capacity() - impl_.size());
        impl_.term(impl_.size() + n);
    }

    //------------------------------------------------------

    /** Swap the contents.

        Exchanges the contents of this string with another
        string. Ownership of the respective @ref storage
        objects is not transferred.

        @li If `*other.storage() == *sp`, ownership of the
        underlying memory is swapped in constant time, with
        no possibility of exceptions. All iterators and
        references remain valid.

        @li If `*other.storage() != *sp`, the contents are
        logically swapped by making a copy, which can throw.
        In this case all iterators and references are invalidated.

        @par Preconditions

        `&other != this`
        
        @par Complexity

        Constant or linear in @ref size() plus `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to @ref storage::allocate may throw.

        @param other The string to swap with
    */
    BOOST_JSON_DECL
    void
    swap(string& other);

    //------------------------------------------------------
    //
    // Search
    //
    //------------------------------------------------------

    std::size_t
    find(
        string const& s,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find(string_view(s), pos);
    }

    std::size_t
    find(
        char const* s,
        std::size_t pos,
        std::size_t count) const
    {
        return string_view(*this).find(s, pos, count);
    }

    std::size_t
    find(
        char const* s,
        std::size_t pos = 0) const
    {
        return string_view(*this).find(s, pos);
    }


    std::size_t
    find(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find(ch, pos);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    std::size_t
    find(
        T const &t,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find(t, pos);
    }

    //------------------------------------------------------

    std::size_t
    rfind(
        string const& s,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).rfind(string_view(s), pos);
    }

    std::size_t
    rfind(
        char const* s,
        std::size_t pos,
        std::size_t count) const
    {
        return string_view(*this).rfind(s, pos, count);
    }

    std::size_t
    rfind(
        char const* s,
        std::size_t pos = npos) const
    {
        return string_view(*this).rfind(s, pos);
    }

    std::size_t
    rfind(
        char ch,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).rfind(ch, pos);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    std::size_t
    rfind(
        T const& t,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).rfind(t, pos);
    }

    //------------------------------------------------------

    std::size_t
    find_first_of(
        string const& s,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(string_view(s), pos);
    }

    std::size_t
    find_first_of(
        char const* s,
        std::size_t pos,
        std::size_t count) const
    {
        return string_view(*this).find_first_of(s, pos, count);
    }

    std::size_t
    find_first_of(
        char const* s,
        std::size_t pos = 0) const
    {
        return string_view(*this).find_first_of(s, pos);
    }

    std::size_t
    find_first_of(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(ch, pos);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    std::size_t
    find_first_of(
        T const& t,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(t, pos);
    }

    //------------------------------------------------------

    std::size_t
    find_first_not_of(
        string const& s,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_not_of(string_view(s), pos);
    }

    std::size_t
    find_first_not_of(
        char const* s,
        std::size_t pos,
        std::size_t count) const
    {
        return string_view(*this).find_first_not_of(s, pos, count);
    }

    std::size_t
    find_first_not_of(
        char const* s,
        std::size_t pos = 0) const
    {
        return string_view(*this).find_first_not_of(s, pos);
    }

    std::size_t
    find_first_not_of(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_not_of(ch, pos);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    std::size_t
    find_first_not_of(
        T const& t,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_not_of(t, pos);
    }

    //------------------------------------------------------

    std::size_t
    find_last_of(
        string const& s,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_last_of(string_view(s), pos);
    }

    std::size_t
    find_last_of(
        char const* s,
        std::size_t pos,
        std::size_t count) const
    {
        return string_view(*this).find_last_of(s, pos, count);
    }

    std::size_t
    find_last_of(
        char const* s,
        std::size_t pos = 0) const
    {
        return string_view(*this).find_last_of(s, pos);
    }

    std::size_t
    find_last_of(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_last_of(ch, pos);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    std::size_t
    find_last_of(
        T const& t,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_last_of(t, pos);
    }

    //------------------------------------------------------

    std::size_t
    find_last_not_of(
        string const& s,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_last_not_of(string_view(s), pos);
    }

    std::size_t
    find_last_not_of(
        char const* s,
        std::size_t pos,
        std::size_t count) const
    {
        return string_view(*this).find_last_not_of(s, pos, count);
    }

    std::size_t
    find_last_not_of(
        char const* s,
        std::size_t pos = 0) const
    {
        return string_view(*this).find_last_not_of(s, pos);
    }

    std::size_t
    find_last_not_of(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_last_not_of(ch, pos);
    }

    template<class T
    #ifndef GENERATING_DOCUMENTATION
        ,class = detail::is_string_viewish<T>
    #endif
    >
    std::size_t
    find_last_not_of(
        T const& t,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_last_not_of(t, pos);
    }

private:
    class undo;

    template<class It>
    using iter_cat = typename
        std::iterator_traits<It>::iterator_category;

    template<class InputIt>
    void
    assign(InputIt first, InputIt last,
        std::random_access_iterator_tag);

    template<class InputIt>
    void
    assign(InputIt first, InputIt last,
        std::input_iterator_tag);

    template<class InputIt>
    void
    append(InputIt first, InputIt last,
        std::random_access_iterator_tag);

    template<class InputIt>
    void
    append(InputIt first, InputIt last,
        std::input_iterator_tag);

    BOOST_JSON_DECL
    void
    reserve_impl(std::size_t new_capacity);
};

//----------------------------------------------------------

/** Exchange the given values.

    Exchanges the contents of the string `lhs` with
    another string `rhs`. Ownership of the respective
    @ref storage objects is not transferred.

    @li If `*lhs.storage() == *rhs.storage()`,
    ownership of the underlying memory is swapped in
    constant time, with no possibility of exceptions.
    All iterators and references remain valid.

    @li If `*lhs.storage() != *rhs.storage()`,
    the contents are logically swapped by making a copy,
    which can throw. In this case all iterators and
    references are invalidated.

    @par Preconditions

    `&lhs != &rhs`
        
    @par Complexity

    Constant or linear in `lhs.size() + rhs.size()`.

    @par Exception Safety

    Strong guarantee.
    Calls to @ref storage::allocate may throw.

    @param lhs The string to exchange.

    @param rhs The string to exchange.
*/
inline
void
swap(string& lhs, string& rhs)
{
    lhs.swap(rhs);
}

//----------------------------------------------------------

/** Perform stream output.

    Behaves as a formatted output function.
*/
BOOST_JSON_DECL
std::ostream&
operator<<(std::ostream& os, string const& s);

/** Return true if lhs equals rhs.

    A lexicographical comparison is used.
*/
#ifdef GENERATING_DOCUMENTATION
bool
operator==(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator==(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) == string_view(rhs);
}

/** Return true if lhs does not equal rhs.

    A lexicographical comparison is used.
*/
#ifdef GENERATING_DOCUMENTATION
bool
operator!=(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator!=(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) != string_view(rhs);
}

/** Return true if lhs is less than rhs.

    A lexicographical comparison is used.
*/
#ifdef GENERATING_DOCUMENTATION
bool
operator<(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator<(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) < string_view(rhs);
}

/** Return true if lhs is less than or equal to rhs.

    A lexicographical comparison is used.
*/
#ifdef GENERATING_DOCUMENTATION
bool
operator<=(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator<=(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) <= string_view(rhs);
}

#ifdef GENERATING_DOCUMENTATION
bool
operator>=(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator>=(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) >= string_view(rhs);
}

/** Return true if lhs is greater than rhs.

    A lexicographical comparison is used.
*/
#ifdef GENERATING_DOCUMENTATION
bool
operator>(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator>(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) > string_view(rhs);
}

} // json
} // boost

#include <boost/json/impl/string.hpp>
#ifdef BOOST_JSON_HEADER_ONLY
#include <boost/json/impl/string.ipp>
#endif

#endif
