#include <cstdio>
#include <vector>
#include <cmath>
#include <stdint.h>

struct Rand {
  Rand(uint32_t s) {
    seed = s;
  }

  int operator()(int range = RAND_MAX) {
    return int(range * (rand_r(&seed) / (RAND_MAX + 1.0)));
  }

  double DRand(void) {
    return rand_r(&seed) / (RAND_MAX + 1.0);
  }

  uint32_t seed; 
};

static int range;
static int total;
static double disturb;
static int disturb_factor;
static int split;


int main(int argc, char *argv[]) {
  if (argc != 6) {
    printf("%s range total disturb disturb_factor split\n", argv[0]);
    return 0;
  }

  range = atoi(argv[1]);
  total = atoi(argv[2]);
  disturb = atof(argv[3]);
  disturb_factor = atoi(argv[4]);
  split = atoi(argv[5]);

  std::vector<int> cnt(range, 0);
  Rand aux_dice((uint32_t)time(NULL));
  Rand dice((uint32_t)(time(NULL) + aux_dice()));

  for (int i = 0; i < total; ++i) {
    ++(cnt[dice(range)]);
    if (aux_dice.DRand() < disturb) 
      for (int i = 0; i < disturb_factor; ++i)
        dice();
  }

  // [total, E=total/split(1/split)]: v1(p1) ... [D=srqt(variance)]
  int En = total / split;
  int part = range / split;
  double E = 1.0 / split;
  double variance = 0.0;

  printf("[%d, E=%d(%.3f)]: ", total, En, E);

  std::vector<int> split_cnt(split, 0);
  for (size_t i = 0; i < cnt.size(); ++i) {
    split_cnt[i / part] += cnt[i];
  }

  for (int i = 0; i < split; ++i) {
    double prob = (double)split_cnt[i] / total;
    double d = prob - E;
    variance += d * d;
    printf("%d(%.3f) ", split_cnt[i], prob);
  }

  variance = sqrt(variance / split);
  printf("[D=%f]\n", variance);

  return 0;
}

