//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/json
//

// Test that header file is self-contained.
#include <boost/json/monotonic_resource.hpp>

#include <boost/json/parser.hpp>

#include "test_suite.hpp"

namespace boost {
namespace json {

class monotonic_resource_test
{
private:
    bool
    in_buffer(
        void* ptr,
        void* buffer,
        std::size_t buffer_size)
    {
        using ptr_t = const volatile unsigned char*;
        return reinterpret_cast<ptr_t>(ptr) >= reinterpret_cast<ptr_t>(buffer) &&
            reinterpret_cast<ptr_t>(ptr) < reinterpret_cast<ptr_t>(buffer) + buffer_size;
    }

    bool
    all_alloc_in_same_block(
        monotonic_resource& mr,
        std::size_t bytes,
        std::size_t align)
    {
        // base of the block
        auto first = mr.allocate(align, align);
        bool result = true;
        for (auto allocs = (bytes - align) / align; allocs; --allocs)
            result &= in_buffer(mr.allocate(align, align), first, bytes);
        return result;
    }

public:
    void
    testGeneral()
    {
        // test that each block gets filled to capacity
        // and if the growth factor is correct
        {
            monotonic_resource mr;
            BOOST_TEST(all_alloc_in_same_block(mr, 1024, 1));
            BOOST_TEST(all_alloc_in_same_block(mr, 2048, 2));
            BOOST_TEST(all_alloc_in_same_block(mr, 4096, 1));
            BOOST_TEST(all_alloc_in_same_block(mr, 8192, 4));
            BOOST_TEST(all_alloc_in_same_block(mr, 16384, 1));
            BOOST_TEST(all_alloc_in_same_block(mr, 32768, 8));
            BOOST_TEST(all_alloc_in_same_block(mr, 65536, 1));
        }
        // test if each allocation is aligned correctly
        {
            monotonic_resource mr;
            for (std::size_t i = 0; i < 4096; ++i)
            {
                const auto size = ((i * 3) % 32) + 1;
                std::size_t next = 1;
                for (auto mod = i % alignof(long double);
                    mod; mod >>= 1, next <<= 1);
                const auto align = (std::max)(next,
                    std::size_t(1));
                BOOST_TEST(!(reinterpret_cast<std::uintptr_t>(mr.allocate(size, align)) % align));
            }
        }
        // test if user provided sizes are correctly rounded
        {
            {
                monotonic_resource mr(10);
                BOOST_TEST(all_alloc_in_same_block(mr, 1024, 1));
            }
            {
                monotonic_resource mr(1025);
                BOOST_TEST(all_alloc_in_same_block(mr, 2048, 1));
            }
            {
                monotonic_resource mr(4000);
                BOOST_TEST(all_alloc_in_same_block(mr, 4096, 1));
            }
        }
        // test if sizes are correctly determined from initial buffers
        {
            {   
                unsigned char buf[512];
                monotonic_resource mr(buf, 512);
                BOOST_TEST(all_alloc_in_same_block(mr, 512, 1));
                BOOST_TEST(all_alloc_in_same_block(mr, 1024, 1));
            }
            {   
                unsigned char buf[2048];
                monotonic_resource mr(buf, 2048);
                BOOST_TEST(all_alloc_in_same_block(mr, 2048, 1));
                BOOST_TEST(all_alloc_in_same_block(mr, 4096, 1));
            }
            {   
                unsigned char buf[4000];
                monotonic_resource mr(buf, 4000);
                BOOST_TEST(all_alloc_in_same_block(mr, 4000, 1));
                BOOST_TEST(all_alloc_in_same_block(mr, 4096, 1));
            }
        }
        // test if allocations that exceed the block size cause rounding to occur
        {
            {
                monotonic_resource mr;
                mr.allocate(2048);
                BOOST_TEST(all_alloc_in_same_block(mr, 4096, 1));
            }
            {
                monotonic_resource mr;
                mr.allocate(2000, 1);
                mr.allocate(48, 1);
                BOOST_TEST(all_alloc_in_same_block(mr, 4096, 1));
            }
        }
    }

    void
    testStorage()
    {
        auto jv = parse(
R"xx({
    "glossary": {
        "title": "example glossary",
		"GlossDiv": {
            "title": "S",
			"GlossList": {
                "GlossEntry": {
                    "ID": "SGML",
					"SortAs": "SGML",
					"GlossTerm": "Standard Generalized Markup Language",
					"Acronym": "SGML",
					"Abbrev": "ISO 8879:1986",
					"GlossDef": {
                        "para": "A meta-markup language, used to create markup languages such as DocBook.",
						"GlossSeeAlso": ["GML", "XML"]
                    },
					"GlossSee": "markup"
                }
            }
        }
    }
})xx"
        , make_counted_resource<monotonic_resource>());
        BOOST_TEST_PASS();
    }

    void
    run()
    {
        testStorage();
        testGeneral();
    }
};

TEST_SUITE(monotonic_resource_test, "boost.json.monotonic_resource");

} // json
} // boost
