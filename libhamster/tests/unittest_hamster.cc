#include <string>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/shm.h>

#include "gtest/gtest.h"

extern "C" {
#include "hamster.h"
#include "shm_error.h"
#include "shm_config.h"
#include "shm_segments.h"
}

using namespace std;

//////////////////////////////////////////////////////////////////////
// structs introduce from libhamster
struct h_value_t {
  /* pointer to value */
  void* ptr;
  /* size of this value */
  uint32_t size;
  /* max size of this value */
  uint32_t max_size; 
};

struct shm_data_header {
  int checksum;
  uint32_t total_size;
  uint32_t data_size;
  struct shmseg_ptr_base next;
};

struct seg_header {
  uint32_t off;          /* offset of used part */
  key_t    next_shm_key; /* next shm segment */
} __attribute__((aligned(16)));
//////////////////////////////////////////////////////////////////////

struct KeyValue {
  string key;
  h_value_t val;

  KeyValue() {
    memset(&val, 0, sizeof(val));
  }

  ~KeyValue() {
    delete [] (char*)val.ptr;
  }

  void Statistic() {
    printf(
        "key:  %s\n"
        "size: %d\n"
        "max_size: %d\n\n",
        key.c_str(), val.size, val.max_size
        );
  }

  void Generate(const string& k, uint32_t total_size) {
    if (val.ptr)
      delete [] (char*)val.ptr;

    key = k;
    val.size = val.max_size = total_size - sizeof(shm_data_header) - k.size() - 1;
    val.ptr = new char[val.size];
    for (uint32_t i = 0; i < val.size; i += 8)
      *(int64_t*)((char*)val.ptr + i) = ((int64_t)rand()) << 32 | rand(); 
  }

  void Set() {
    ASSERT_EQ(E_SHM_OK, hamster_set(key.c_str(), &val));
  }

  void Update() {
    if (val.ptr) delete [] (char*)val.ptr;
    val.size = val.max_size;
    val.ptr = new char[val.size];
    for (uint32_t i = 0; i < val.size; i += 8)
      *(int64_t*)((char*)val.ptr + i) = ((int64_t)rand()) << 32 | rand(); 

    ASSERT_EQ(E_SHM_OK, hamster_set(key.c_str(), &val));
  }

  void Check() {
    h_value_t get_val;
    ASSERT_EQ(E_SHM_OK, hamster_get(key.c_str(), &get_val));
    ASSERT_EQ(val.size, get_val.size);
    ASSERT_EQ(val.max_size, get_val.max_size);
    ASSERT_EQ(0, memcmp(val.ptr, get_val.ptr, val.size));
  }

  static uint32_t TotalSize2ValSize(const string& key, uint32_t total_size) {
    return total_size - sizeof(shm_data_header) - key.size() - 1;
  }
};

class hamster_test : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    system("ipcs -m|sed -n '4,$s/\\(\\S\\+\\).*/ipcrm -M \\1/p'|sh");
    seg_size = shm_pagesize * SHM_SIZE_IN_PAGES;
    uint32_t seg_avail_size = (seg_size - sizeof(seg_header));

    // t1[0]_ and t1[1]_ will occupy one segment exactly
    t1_[0].Generate("small_key0", ((seg_avail_size / 2) / 16) * 16);
    t1_[1].Generate("small_key1", seg_avail_size - ((seg_avail_size / 2) / 16) * 16);

    // t2[0]_ can fit in one segment, and the insertion of t2[1]_ will cause
    // another shm to be allocated
    t2_[0].Generate("large_key0", seg_avail_size / 2);
    t2_[1].Generate("large_key1", seg_avail_size / 2);

    // huge data that will occupy several pages  
    t3_.Generate("huge_key", shm_pagesize * (SHM_SIZE_IN_PAGES * 2));

    // new key, set after the crash_recovery_with_data_corruption test
    new_kv_.Generate("new_key", 1024);
  }

  static void TearDownTestCase() { 
    hamster_shutdown();
  }

  virtual void SetUp() {}
  virtual void TearDown() {}

 protected:
  static uint32_t seg_size;
  static KeyValue t1_[2];
  static KeyValue t2_[2];
  static KeyValue t3_;
  static KeyValue new_kv_;
};

uint32_t hamster_test::seg_size;
KeyValue hamster_test::t1_[2];
KeyValue hamster_test::t2_[2];
KeyValue hamster_test::t3_;
KeyValue hamster_test::new_kv_;

#define f_t1 hamster_test::t1_
#define f_t2 hamster_test::t2_
#define f_t3 hamster_test::t3_
#define f_new_kv hamster_test::new_kv_

TEST_F(hamster_test, init) {
  ASSERT_EQ(E_SHM_OK, hamster_init()); 
}

TEST_F(hamster_test, small_data) {
  f_t1[0].Set();
  f_t1[1].Set();

  f_t1[0].Check();
  f_t1[1].Check();

  f_t1[0].Update();
  f_t1[1].Update();

  f_t1[0].Check();
  f_t1[1].Check();

  ASSERT_EQ((size_t)2, hamster_count());
}

TEST_F(hamster_test, large_data) {
  f_t2[0].Set();
  f_t2[1].Set();

  f_t2[0].Check();
  f_t2[1].Check();

  f_t2[0].Update();
  f_t2[1].Update();

  f_t2[0].Check();
  f_t2[1].Check();

  ASSERT_EQ((size_t)4, hamster_count());
}

TEST_F(hamster_test, huge_data) {
  f_t3.Set();
  f_t3.Check();
  f_t3.Update();
  f_t3.Check();

  ASSERT_EQ((size_t)5, hamster_count());
}

/// crash recovery
/// crash recovery with data corruption
extern "C" void unittest_hamster_sim_crash();
TEST_F(hamster_test, crash_recovery) {
  unittest_hamster_sim_crash();
  ASSERT_EQ(E_SHM_OK, hamster_init()); 
  f_t1[0].Check();
  f_t1[1].Check();
  f_t2[0].Check();
  f_t2[1].Check();
  f_t3.Check();
  ASSERT_EQ((size_t)5, hamster_count());
}

TEST_F(hamster_test, crash_recovery_with_data_corruption) {
  unittest_hamster_sim_crash();
  // modify the data a little
  // corrupt some part of f_t2[0]
  int shm_id = shmget(SHM_KEY + 1, 0, 0600);
  ASSERT_NE(shm_id, -1);
  void* base_ptr = shmat(shm_id, 0, 0);
  ASSERT_NE(base_ptr, (void*)-1);
  base_ptr = (char*)base_ptr + sizeof(seg_header) + sizeof(shm_data_header);
  memset(base_ptr, 0xff, 32);

  int ec = hamster_init();
  ASSERT_EQ(E_SHM_DATA_CORRUPTED, ec); 
  f_t1[0].Check();
  f_t1[1].Check();

  ASSERT_EQ((size_t)2, hamster_count());

  h_value_t get_val;
  ASSERT_EQ(E_SHM_KEY_NOT_FOUND, hamster_get(f_t2[0].key.c_str(), &get_val));
  ASSERT_EQ(E_SHM_KEY_NOT_FOUND, hamster_get(f_t2[1].key.c_str(), &get_val));
  ASSERT_EQ(E_SHM_KEY_NOT_FOUND, hamster_get(f_t3.key.c_str(), &get_val));

  f_new_kv.Set();
  f_new_kv.Check();
  f_new_kv.Update();
  f_new_kv.Check();

  ASSERT_EQ((size_t)3, hamster_count());
}

TEST_F(hamster_test, recovery_after_corruption_happend) {
  unittest_hamster_sim_crash();
  ASSERT_EQ(E_SHM_OK, hamster_init()); 
  f_t1[0].Check();
  f_t1[1].Check();
  h_value_t get_val;
  ASSERT_EQ(E_SHM_KEY_NOT_FOUND, hamster_get(f_t2[0].key.c_str(), &get_val));
  ASSERT_EQ(E_SHM_KEY_NOT_FOUND, hamster_get(f_t2[1].key.c_str(), &get_val));
  ASSERT_EQ(E_SHM_KEY_NOT_FOUND, hamster_get(f_t3.key.c_str(), &get_val));
  f_new_kv.Check();
  ASSERT_EQ((size_t)3, hamster_count());
}

