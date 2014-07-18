#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>

#include "gtest/gtest.h"

extern "C" {
#include "shm_error.h"
#include "shm_config.h"
#include "shm_segments.h"
}

struct seg_header {
  uint32_t off;          /* offset of used part */
  key_t    next_shm_key; /* next shm segment */
} __attribute__((aligned(16)));

struct test_data {
  char* ptr;
  uint32_t len;
};

class shm_segments_test : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    srand(time(NULL));
    generate_data(data1, SHM_SIZE_IN_PAGES * shm_pagesize - sizeof(seg_header));
    generate_data(data2, SHM_SIZE_IN_PAGES * shm_pagesize / 2);
    generate_data(data3, (SHM_SIZE_IN_PAGES + 1) * shm_pagesize);
  }

  static void TearDownTestCase() {
    delete [] data1.ptr;
    delete [] data2.ptr;
    delete [] data3.ptr;
  }

  virtual void SetUp() {}
  virtual void TearDown() {}

  static void generate_data(test_data& td, size_t len) {
    td.ptr = new char[len];
    td.len = len;
    for (size_t i = 0; i < len; i += 8)
      *(int64_t*)(td.ptr + i) = ((int64_t)rand()) << 32 | rand(); 
  }

 public:
  static test_data data1; 
  static test_data data2; 
  static test_data data3;
};

test_data shm_segments_test::data1; 
test_data shm_segments_test::data2; 
test_data shm_segments_test::data3; 

TEST_F(shm_segments_test, fresh_init) {
  // 1. clean up all existing shm
  system("ipcs -m|sed -n '4,$s/\\(\\S\\+\\).*/ipcrm -M \\1/p'|sh");
  // 2. check init result
  ASSERT_EQ(E_SHM_OK, shmseg_init(SHM_KEY));
  // 3. manually check the stat of this shm
  int shm_id = shmget(SHM_KEY, 0, 0600);
  ASSERT_NE(shm_id, -1);

  void* base_ptr = shmat(shm_id, 0, 0);
  ASSERT_NE(base_ptr, (void*)-1);

  seg_header* hdr = (seg_header*)base_ptr;
  ASSERT_EQ(hdr->off, sizeof(seg_header));
  ASSERT_EQ(hdr->next_shm_key, -1);

  ASSERT_EQ(shmdt(base_ptr), 0);
}

TEST_F(shm_segments_test, shmseg_get_within_one_segment) {
  // allocate a full segment size minus size of seg_header
  test_data data1 = shm_segments_test::data1;
  shmseg_ptr sptr;
  ASSERT_EQ(E_SHM_OK, shmseg_get(&data1.len, &sptr));
  ASSERT_EQ(sptr.base.off, sizeof(seg_header));

  void* ptr = shmseg_ptr_ptr(&sptr);
  ASSERT_NE(ptr, (void*)NULL);

  memcpy(ptr, data1.ptr, data1.len);

  seg_header* hdr = (seg_header*)((char*)ptr - sizeof(seg_header));
  ASSERT_EQ(hdr->off, (size_t)(SHM_SIZE_IN_PAGES * shm_pagesize));
  ASSERT_EQ(hdr->next_shm_key, -1);
}

TEST_F(shm_segments_test, shmseg_get_cross_segment) {
  // allocate half the the segment size
  test_data data2 = shm_segments_test::data2;
  // this shmget will trigger the new allocation
  shmseg_ptr sptr;
  ASSERT_EQ(E_SHM_OK, shmseg_get(&data2.len, &sptr));
  ASSERT_NE(sptr.base.shm_key, (key_t)SHM_KEY);
  ASSERT_EQ(sptr.base.off, sizeof(seg_header));

  void* ptr = shmseg_ptr_ptr(&sptr);
  ASSERT_NE(ptr, (void*)NULL);

  memcpy(ptr, data2.ptr, data2.len);

  seg_header* hdr = (seg_header*)((char*)ptr - sizeof(seg_header));
  ASSERT_EQ(hdr->off, sizeof(seg_header) + data2.len);
  ASSERT_EQ(hdr->next_shm_key, -1);

  key_t this_key = sptr.base.shm_key;
  shmseg_ptr_reset(&sptr);
  ASSERT_EQ(E_SHM_OK, shmseg_first_ptr(&sptr));
  hdr = (seg_header*)((char*)shmseg_ptr_ptr(&sptr) - sizeof(seg_header));
  ASSERT_EQ(hdr->next_shm_key, this_key);

}

TEST_F(shm_segments_test, shmseg_get_exceed_segment) {
  test_data data3 = shm_segments_test::data3;

  shmseg_ptr sptr;
  ASSERT_EQ(E_SHM_OK, shmseg_get(&data3.len, &sptr));
  ASSERT_NE(sptr.base.shm_key, (key_t)SHM_KEY);
  ASSERT_EQ(sptr.base.off, sizeof(seg_header));

  void* ptr = shmseg_ptr_ptr(&sptr);
  ASSERT_NE(ptr, (void*)NULL);

  memcpy(ptr, data3.ptr, data3.len);

  seg_header* hdr = (seg_header*)((char*)ptr - sizeof(seg_header));
  ASSERT_EQ(hdr->off, sizeof(seg_header) + data3.len);
  ASSERT_EQ(hdr->next_shm_key, -1);
}

// simulate a crash, init the segments again and check the data
extern "C" {
  struct seg_t;
  void unittest_shmseg_sim_crash();
  struct seg_t* unittest_seg_head();
  struct seg_t* unittest_seg_next(struct seg_t* s);
  void* unittest_seg_base(struct seg_t* s);
}

TEST_F(shm_segments_test, recovery_init) {
  unittest_shmseg_sim_crash();
  ASSERT_EQ(E_SHM_OK, shmseg_init(SHM_KEY));

  test_data datas[3] = {
    shm_segments_test::data1,
    shm_segments_test::data2,
    shm_segments_test::data3,
  };

  int k = 0;
  for (seg_t* s = unittest_seg_head();
       s != NULL;
       s = unittest_seg_next(s), ++k) {
    void* base_ptr = unittest_seg_base(s);
    ASSERT_NE(base_ptr, (void*)0);

    ASSERT_EQ(0, memcmp(base_ptr, datas[k].ptr, datas[k].len));
  }
}
