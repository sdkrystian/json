//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/json
//

#ifndef BOOST_JSON_TO_VALUE_HPP
#define BOOST_JSON_TO_VALUE_HPP

#include <boost/json/config.hpp>
#include <boost/json/value.hpp>
#include <boost/json/traits.hpp>
#include <boost/json/detail/to_value.hpp>

namespace boost {
namespace json {

/** Construct a JSON value from T.

    This function attempts to convert the instance of
    type `T` to a @ref value. It handles these cases,
    in order of decreasing priority:

    @li If @ref value is constructible `T` then the
        function returns `value( t, sp )`.

    @li If `T::to_json` exists then the function invokes
        `t.to_json( value& jv )` with `jv` set to a
        null @ref value using storage `sp`, and returns
        `jv`.

    @li If @ref to_value_traits contains a specialization
        for `T`, then the function invokes
        `to_value_traits<T>:: assign( jv, t )` with
        `jv` set to a null @ref value constructed with
        storage `sp`, and returns `jv`.

    @li Otherwise if `T` matches any from a set of
        generic requirements implemented by the library,
        then the function returns a value generated
        from `t`.

    @par Constraints
    @code
    has_to_value< T >::value
    @endcode

    @par Exception Safety

    Strong guarantee.

    @param t The instance of `T` to convert.

    @param jv The value to assign to.

    @returns The JSON value representing `t`.

    @see @ref value_cast
*/
template<class T>
auto
to_value(
    T&& t,
    storage_ptr sp = {}) ->
#ifdef GENERATING_DOCUMENTATION
        value
#else
    typename std::enable_if<
        std::is_constructible<
            value, T&&, storage_ptr>::value ||
        detail::has_to_value_traits<
            detail::remove_cvref<T> >::value ||
        detail::has_to_json_mf<
            detail::remove_cvref<T> >::value ||
        detail::has_to_value_generic<
            detail::remove_cvref<T> >::value,
    value>::type
#endif
{
    return detail::to_value_impl_1(
        std::forward<T>(t),
        detail::move(sp),
        std::is_constructible<
            value, T&&, storage_ptr>{});
}

/** Determine if T can be converted to a JSON value.
*/
#ifdef GENERATING_DOCUMENTATION
template<class T>
using has_to_value = __see_below__;
#else

template<class T, class>
struct has_to_value : std::false_type
{
};

template<class T>
struct has_to_value<T,
    detail::void_t<decltype(
        std::declval<value&>() = to_value(
            std::declval<T&&>(),
            std::declval<storage_ptr const&>())
    ) > > : std::true_type
{
};

#endif

} // json
} // boost

#endif
