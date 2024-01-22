#include "UnitCpp/UnitCpp.h"
#include <kisCore/kisFixedHashMap.h>

uint32_t hashFuncQuarter(uint32_t c)
{
    return c / 4;
}

TEST(FixedHashMap, Simple)
{
    kisFixedHashMap<uint32_t, uint32_t, 100, hashFuncQuarter> map;

    // Fill the map.
    for(uint32_t i = 0; i < 100; ++i)
    {
        auto itr = map.add(i);
        (*itr) = i * 10;
    }

    bool keysFound[100];
    memset(keysFound, 0, sizeof(keysFound));

    // Test is the key and values are correct.
    for(auto itr = map.begin(); itr.isValid(); ++itr)
    {
        uint32_t key = itr.getKey();
        uint32_t value = itr.getValue();

        KIS_ASSERT(key < 100);

        TEST_EQUAL(value, key * 10);
        TEST_FALSE(keysFound[key]);
    
        keysFound[key] = true;
    }

    // Test if we have found all keys during the iteration.
    for(int i = 0; i < 100; i++)
    {
        TEST_TRUE(keysFound[i]);
    }
}


