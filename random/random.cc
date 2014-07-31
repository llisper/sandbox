#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <algorithm>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <functional>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>

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

struct Player {
  Player(int range, int num, Rand *dice, bool own_dice = false) {
    this->dice = dice;
    this->own_dice = own_dice;
    this->num = num;
    this->range = range;
    val_cnt.resize(range);
  }

  ~Player(void) { if (own_dice) delete dice; }

  void Random(void) {
    --num;
    int v = (*dice)(range);
    vals.push_back(v);
    ++(val_cnt.at(v));
  }

  Rand *dice;
  bool own_dice;
  int num;
  int range;
  std::vector<int> vals;
  std::vector<int> val_cnt;
};

static int range;
static int num;
static int num_players;
static int shuffle;
static int global_dice;
static double disturb;
static int disturb_factor;
static int split;

static double statistics(std::ostream& os, const std::vector<int>& cnt, int split);

int main(int argc, char *argv[]) {
  if (argc < 6) {
    printf("%s range num num_players shuffle global_dice disturb disturb_factor split\n", argv[0]);
    return 0;
  }

  range = atoi(argv[1]);
  num = atoi(argv[2]);
  num_players = atoi(argv[3]);
  shuffle = atoi(argv[4]);
  global_dice = atoi(argv[5]);
  disturb = atof(argv[6]);
  disturb_factor = atoi(argv[7]);
  split = atoi(argv[8]);

  int total = num * num_players;
  int num_disturb = 0;
  Rand aux_dice((uint32_t)time(NULL));
  Rand g_dice((uint32_t)(time(NULL) + aux_dice()));
  std::vector<Player*> players;

  for (int i = 0; i < num_players; ++i) {
    Rand *dice = global_dice ? &g_dice : new Rand((uint32_t)(time(NULL) + aux_dice()));
    players.push_back(new Player(range, num, dice, !global_dice));
  }

  for (int i = 0; i < total; ++i) {
    std::vector<Player*> players_left;
    for (size_t k = 0; k < players.size(); ++k)
      if (players[k]->num)
        players_left.push_back(players[k]);

    Player *p = NULL;
    while (!p) {
      int index = shuffle ? aux_dice((int)players_left.size()) : 0;
      if (players_left[index]->num)
        p = players_left[index];
    }

    p->Random();

    if (global_dice && aux_dice.DRand() < disturb) {
      for (int i = 0; i < disturb_factor; ++i) {
        ++num_disturb;
        g_dice();
      }
    }
  }

  std::vector<int> g_count(range, 0);
  double avg_variance = 0.0;
  assert(disturb != 0 || num_disturb == 0);
  for (size_t i = 0; i < players.size(); ++i) {
    assert(players[i]->num == 0);
    assert(num == std::accumulate(players[i]->val_cnt.begin(), players[i]->val_cnt.end(), 0));
    avg_variance += statistics(std::cerr, players[i]->val_cnt, split);
    std::transform(players[i]->val_cnt.begin(), players[i]->val_cnt.end(),
                   g_count.begin(), g_count.begin(),
                   std::plus<int>());
  }
  avg_variance /= players.size();
  std::cout << '[' << avg_variance << "]\n";

  assert(total == std::accumulate(g_count.begin(), g_count.end(), 0));
  statistics(std::cout, g_count, split);
  return 0;
}

double statistics(std::ostream& os, const std::vector<int>& cnt, int split) {
  int total = std::accumulate(cnt.begin(), cnt.end(), 0);
  int part = cnt.size() / split;
  double expected = 1.0 / split;
  double variance = 0.0;

  std::vector<int> split_cnt(part, 0);
  for (size_t i = 0; i < cnt.size(); ++i) {
    int index = i / part;
    split_cnt[index] += cnt[i];
  }

  os << '[' << total << ',' << std::setprecision(3) << expected << "]: ";
  // [total, 1/split]: c1(p1), c2(p2), ... 
  for (int i = 0; i < split; ++i) {
    double prob = (double)split_cnt[i] / total;
    os << split_cnt[i] << '(' << std::setprecision(3) << prob << ") ";
    double diff = prob - expected;
    variance += diff * diff;
  }

  variance /= split;
  variance = sqrt(variance);
  os << '[' << variance << "]\n";
  return variance;
}

/*
static uint32_t s_seed = 0;
static drand48_data buffer;
static uint16_t xsubi[3] = {0};

static int my_rand(int algo, int range) {
  if (!s_seed) {
    s_seed = (uint32_t)time(NULL);
    memcpy(xsubi, &s_seed, sizeof(s_seed));
    seed48_r(xsubi, &buffer);
  }

  if (algo == 0) {
    return int(range * (rand_r(&s_seed) / (RAND_MAX + 1.0)));
  } else if (algo == 1) {
    return rand_r(&s_seed) % range;
  } else if (algo == 2) {
    double result;
    drand48_r(&buffer, &result);
    return int(result * range);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  int algo = atoi(argv[1]);   // 0: refine 1: origin
  int total = atoi(argv[2]);  // eg. 1000 times
  int range = atoi(argv[3]);  // eg. 100

  // [0, range)
  int *count = new int[range];
  memset(count, 0, sizeof(int) * range);
  for (int i = 0; i < total; ++i) {
     int val = my_rand(algo, range);
     assert(val >= 0 && val < range);
     ++(count[val]);
  }

  int part = range / 4;
  int *part_count = new int[part];
  memset(part_count, 0, sizeof(int) * part);
  for (int i = 0; i < range; ++i) {
    part_count[i / part] += count[i];
  }

  for (int i = 0; i < 4; ++i) {
    printf("[%d,%d): %d, %.3f%%\n", i * part, (i + 1) * part, part_count[i], (part_count[i] * 100.0) / total);
  }

  return 0;
}
*/

