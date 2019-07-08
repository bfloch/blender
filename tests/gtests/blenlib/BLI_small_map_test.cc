#include "testing/testing.h"
#include "BLI_small_map.hpp"
#include "BLI_small_set.hpp"

using IntFloatMap = BLI::SmallMap<int, float>;

TEST(small_map, DefaultConstructor)
{
  IntFloatMap map;
  EXPECT_EQ(map.size(), 0);
}

TEST(small_map, AddIncreasesSize)
{
  IntFloatMap map;
  EXPECT_EQ(map.size(), 0);
  map.add(2, 5.0f);
  EXPECT_EQ(map.size(), 1);
  map.add(6, 2.0f);
  EXPECT_EQ(map.size(), 2);
}

TEST(small_map, Contains)
{
  IntFloatMap map;
  EXPECT_FALSE(map.contains(4));
  map.add(5, 6.0f);
  EXPECT_FALSE(map.contains(4));
  map.add(4, 2.0f);
  EXPECT_TRUE(map.contains(4));
}

TEST(small_map, LookupExisting)
{
  IntFloatMap map;
  map.add(2, 6.0f);
  map.add(4, 1.0f);
  EXPECT_EQ(map.lookup(2), 6.0f);
  EXPECT_EQ(map.lookup(4), 1.0f);
}

TEST(small_map, LookupNotExisting)
{
  IntFloatMap map;
  map.add(2, 4.0f);
  map.add(1, 1.0f);
  EXPECT_EQ(map.lookup_ptr(0), nullptr);
  EXPECT_EQ(map.lookup_ptr(5), nullptr);
}

TEST(small_map, AddMany)
{
  IntFloatMap map;
  for (int i = 0; i < 100; i++) {
    map.add(i, i);
  }
}

TEST(small_map, PopItem)
{
  IntFloatMap map;
  map.add(2, 3.0f);
  map.add(1, 9.0f);
  EXPECT_TRUE(map.contains(2));
  EXPECT_TRUE(map.contains(1));

  EXPECT_EQ(map.pop(1), 9.0f);
  EXPECT_TRUE(map.contains(2));
  EXPECT_FALSE(map.contains(1));

  EXPECT_EQ(map.pop(2), 3.0f);
  EXPECT_FALSE(map.contains(2));
  EXPECT_FALSE(map.contains(1));
}

TEST(small_map, PopItemMany)
{
  IntFloatMap map;
  for (uint i = 0; i < 100; i++) {
    map.add_new(i, i);
  }
  for (uint i = 25; i < 80; i++) {
    EXPECT_EQ(map.pop(i), i);
  }
  for (uint i = 0; i < 100; i++) {
    EXPECT_EQ(map.contains(i), i < 25 || i >= 80);
  }
}

TEST(small_map, LookupRefOrInsert)
{
  IntFloatMap map;
  float &value = map.lookup_ref_or_insert(3, 5.0f);
  EXPECT_EQ(value, 5.0f);
  value += 1;
  value = map.lookup_ref_or_insert(3, 5.0f);
  EXPECT_EQ(value, 6.0f);
}

TEST(small_map, ValueIterator)
{
  IntFloatMap map;
  map.add(3, 5.0f);
  map.add(1, 2.0f);
  map.add(7, -2.0f);

  BLI::SmallSet<float> values;

  uint iterations = 0;
  for (float value : map.values()) {
    values.add(value);
    iterations++;
  }

  EXPECT_EQ(iterations, 3);
  EXPECT_TRUE(values.contains(5.0f));
  EXPECT_TRUE(values.contains(-2.0f));
  EXPECT_TRUE(values.contains(2.0f));
}

TEST(small_map, KeyIterator)
{
  IntFloatMap map;
  map.add(6, 3.0f);
  map.add(2, 4.0f);
  map.add(1, 3.0f);

  BLI::SmallSet<int> keys;

  uint iterations = 0;
  for (int key : map.keys()) {
    keys.add(key);
    iterations++;
  }

  EXPECT_EQ(iterations, 3);
  EXPECT_TRUE(keys.contains(1));
  EXPECT_TRUE(keys.contains(2));
  EXPECT_TRUE(keys.contains(6));
}

TEST(small_map, ItemIterator)
{
  IntFloatMap map;
  map.add(5, 3.0f);
  map.add(2, 9.0f);
  map.add(1, 0.0f);

  BLI::SmallSet<int> keys;
  BLI::SmallSet<float> values;

  uint iterations = 0;
  for (auto item : map.items()) {
    keys.add(item.key);
    values.add(item.value);
    iterations++;
  }

  EXPECT_EQ(iterations, 3);
  EXPECT_TRUE(keys.contains(5));
  EXPECT_TRUE(keys.contains(2));
  EXPECT_TRUE(keys.contains(1));
  EXPECT_TRUE(values.contains(3.0f));
  EXPECT_TRUE(values.contains(9.0f));
  EXPECT_TRUE(values.contains(0.0f));
}

float return_42()
{
  return 42.0f;
}

TEST(small_map, LookupOrInsertFunc_SeparateFunction)
{
  IntFloatMap map;
  EXPECT_EQ(map.lookup_ref_or_insert_func(0, return_42), 42.0f);
  EXPECT_EQ(map.lookup(0), 42);
}

TEST(small_map, LookupOrInsertFunc_Lambdas)
{
  IntFloatMap map;
  auto lambda1 = []() { return 11.0f; };
  EXPECT_EQ(map.lookup_ref_or_insert_func(0, lambda1), 11.0f);
  auto lambda2 = []() { return 20.0f; };
  EXPECT_EQ(map.lookup_ref_or_insert_func(1, lambda2), 20.0f);

  EXPECT_EQ(map.lookup_ref_or_insert_func(0, lambda2), 11.0f);
  EXPECT_EQ(map.lookup_ref_or_insert_func(1, lambda1), 20.0f);
}

TEST(small_map, InsertOrModify)
{
  IntFloatMap map;
  auto create_func = []() { return 10.0f; };
  auto modify_func = [](float &value) { value += 5; };
  EXPECT_TRUE(map.add_or_modify(1, create_func, modify_func));
  EXPECT_EQ(map.lookup(1), 10.0f);
  EXPECT_FALSE(map.add_or_modify(1, create_func, modify_func));
  EXPECT_EQ(map.lookup(1), 15.0f);
}