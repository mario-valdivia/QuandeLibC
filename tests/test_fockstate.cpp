// MIT License
//
// Copyright (c) 2022 Quandela
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <random>
#include <algorithm>
#include <map>
#include <iterator>

#include <catch2/catch.hpp>
#include "../src/fockstate.h"

SCENARIO("C++ Testing FockState") {
    GIVEN("an empty fockstate") {
        WHEN("instantiating") {
            fockstate fs;
            THEN("build Succeed !") {
                REQUIRE(true);
                REQUIRE(fs.to_str() == "|>");
            }
        }

        WHEN("Instantiating with x modes") {
            auto modeNumber = GENERATE(-1, 0, 3, 9, 1000000);
            fockstate fs(modeNumber);

            THEN("instance has x modes") {
                REQUIRE(fs.get_m() == modeNumber);
            }
        }

        WHEN("Default value for m,n and iterator") {
            fockstate fs(3, 2);
            fockstate fs_copy(fs);
            THEN("all photons are in first mode") {
                REQUIRE(fs_copy.to_str() == "|2,0,0>"); // internally AA
            }
            ++fs_copy;
            THEN("photons moving...") {
                REQUIRE(fs_copy.to_str() == "|1,1,0>"); // internally AB
            }
            ++fs_copy;
            THEN("photons moving...") {
                REQUIRE(fs_copy.to_str() == "|1,0,1>"); // internally AC
            }
            fs_copy += 3;
            THEN("photons moving...") {
                REQUIRE(fs_copy.to_str() == "|0,0,2>"); // internally CC
            }
            ++fs_copy;
            THEN("reached the end of the iterator") {
                REQUIRE(fs_copy.to_str() == "|,,>");
            }
            REQUIRE_THROWS_AS(++fs_copy, std::invalid_argument);
        }

        WHEN("Instantiating with invalid strings") {
            const char *txt = GENERATE("", "|0à1>", "2", "|", "[0,1>", "{0,1}");

            THEN("instantiation throw error") {
                REQUIRE_THROWS_AS(fockstate(txt), std::invalid_argument);
            }
        }

        WHEN("Instantiating with valid string") {
            const char *txt = GENERATE("|>", "|0>", "|1>", "|0,0>",
                                       "|0,1>", "|0,2,0>", "|,>");
            fockstate fs(txt);

            THEN("instance has same txt representation") {
                REQUIRE(fs.to_str() == txt);
            }
        }
    }
    SECTION("multiple string constructors - space insensitive") {
        REQUIRE(fockstate("[0,1]").to_str() == "|0,1>");
        REQUIRE(fockstate("[0, 1]").to_str() == "|0,1>");
        REQUIRE(fockstate("[ 0,1] ").to_str() == "|0,1>");
        REQUIRE(fockstate("|0,1〉").to_str() == "|0,1>");
    }
    SECTION("from list, add/mul operation, equality") {
        fockstate fs1(std::vector<int>{0, 1, 0});
        fockstate fs2(std::vector<int>{1, 0, 0});
        fockstate fs3 = fs1 + fs2;
        REQUIRE(fs3 == fockstate(std::vector<int>{1, 1, 0}));
        REQUIRE(fs3.to_str() == "|1,1,0>");
        fs3 += fs1;
        REQUIRE(fs3.to_str() == "|1,2,0>");
        fs3 += fs3;
        REQUIRE(fs3.to_str() == "|2,4,0>");
        REQUIRE(fs3 == fs3);
        REQUIRE(fs3 != fs1);
        REQUIRE(fs3.get_n() == 6);
    }
    SECTION("test photon to mode") {
        fockstate fs1(std::vector<int>{0, 1, 0});
        REQUIRE(fs1.photon2mode(0) == 1);
        fockstate fs2(std::vector<int>{1, 2, 3});
        REQUIRE(fs2.photon2mode(0) == 0);
        REQUIRE(fs2.photon2mode(1) == 1);
        REQUIRE(fs2.photon2mode(2) == 1);
        REQUIRE(fs2.photon2mode(3) == 2);
        REQUIRE(fs2.photon2mode(4) == 2);
        REQUIRE(fs2.photon2mode(5) == 2);
        REQUIRE_THROWS_AS(fs2.photon2mode(7), std::out_of_range);
    }
    SECTION("cast to vector, get modes, iterators on mode") {
        std::vector<int> v{1, 4, 1, 0, 6};
        fockstate fs(v);
        REQUIRE(fs.to_vect() == v);
        REQUIRE(fs.get_m() == (int)v.size());
        WHEN("testing valid mode") {
            for(size_t i=0; i < v.size(); i++)
                REQUIRE(fs[i]==v[i]);
        }
        WHEN("testing invalid mode") {
            int mk = GENERATE(-1, 5);
            THEN("instance has same txt representation") {
                REQUIRE_THROWS_AS(fs[mk], std::out_of_range);
            }
        }
        std::vector<int> v2;
        fs.to_vect(v2);
        REQUIRE(v==v2);
    }
    SECTION("tensor product") {
        {
            fockstate fs1(std::vector<int>{1, 2});
            fockstate fs2(std::vector<int>{3, 4});
            fockstate fs3 = fs1 * fs2;
            REQUIRE(fs3 == fockstate(std::vector<int>{1, 2, 3, 4}));
        }
        {
            fockstate fs1(std::vector<int>{0, 1});
            fockstate fs2(std::vector<int>{1});
            fockstate fs3 = fs1 * fs2;
            REQUIRE(fs3 == fockstate(std::vector<int>{0, 1, 1}));
        }
    }
    SECTION("prodnfact") {
        REQUIRE(fockstate(std::vector<int>{1, 2, 3}).prodnfact()==12);
        REQUIRE(fockstate(std::vector<int>{0, 0}).prodnfact()==1);
    }
    SECTION("random states and hashing") {
        std::random_device rnd_device;
        std::mt19937 mersenne_engine {rnd_device()};  // Generates random integers
        std::uniform_int_distribution<int> dist {1, 6};

        auto gen = [&dist, &mersenne_engine](){
            return dist(mersenne_engine);
        };

        std::map<long long, std::vector<int>> test_hash;
        int nb_collisions = 0;
        for(int i=0; i<1000; i++) {
            std::vector<int> vec(10);
            generate(begin(vec), end(vec), gen);
            long long hash=fockstate(vec).hash();
            if (test_hash.find(hash) != test_hash.end()) {
                if (test_hash[hash] != vec)
                    nb_collisions++;
            }
            test_hash[hash] = vec;
        }
        /* we can get unlucky and get one collision, but odd of getting more than one are more than tiny */
        REQUIRE(nb_collisions <= 1);
    }
    SECTION("Fockstate get slice") {
        fockstate fs(std::vector<int>{0,1,0,2,1,1});
        REQUIRE(fs.slice(0,5) == fs);
        REQUIRE(fs.slice(-3,-1) == fockstate(std::vector<int>{2, 1, 1}));
        REQUIRE(fs.slice(1,3) == fockstate(std::vector<int>{1,0,2}));
        REQUIRE(fs.slice(2,2) == fockstate(1, 0));
        REQUIRE(fs.slice(1,5,2) == fockstate(std::vector<int>{1, 2, 1}));
        REQUIRE(fs.slice(1,5,3) == fockstate(std::vector<int>{1, 1}));
        REQUIRE_THROWS_AS(fs.slice(0,8), std::out_of_range);
        REQUIRE_THROWS_AS(fs.slice(2,1), std::invalid_argument);
    }
    SECTION("Fockstate set slice") {
        fockstate fs(std::vector<int>{0,1,0,2,1,1});
        REQUIRE(fs.set_slice(fockstate(std::vector<int>{2,0,3}),2,4) ==
                        fockstate(std::vector<int>{0,1,2,0,3,1}));
        REQUIRE_THROWS_AS(fs.set_slice(fockstate(std::vector<int>{2,0}),2,4),
                          std::invalid_argument);
    }
}
