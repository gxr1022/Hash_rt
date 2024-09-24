// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT
#include "verona-rt/src/rt/ds/hashmap.h"

#include "debug/harness.h"
#include "test/opt.h"
#include "test/xoroshiro.h"
#include "verona.h"
#include "test/opt.h"
#include <unordered_map>
#include <vector>

using namespace snmalloc;
using namespace verona::rt;

struct Key : public VCown<Key>
{
  
};

auto& alloc = ThreadAlloc::get();
ObjectMap<std::pair<Cown*, int32_t>> map(alloc);
std::unordered_map<Key*, int32_t> model;

std::stringstream err;

template<typename Entry, typename Model>
bool model_check(const ObjectMap<Entry>& map, const Model& model, std::stringstream& err)
{
  map.debug_layout(err) << "\n";
  if (map.size() != model.size())
  {
    err << "map size (" << map.size() << ") is not expected (" << model.size()
        << ")"
        << "\n";
    return false;
  }
  for (const auto& e : model)
  {
    auto it = map.find(e.first);
    if (it == map.end())
    {
      err << "not found: " << e.first << "\n";
      return false;
    }
    else if (it.is_marked())
    {
      err << "marked: " << e.first << "\n";
      return false;
    }
  }
  auto iter_model = model;
  for (auto it = map.begin(); it != map.end(); ++it)
    iter_model.erase(it.key());

  if (!iter_model.empty())
  {
    for (const auto& e : iter_model)
      err << "not found: " << e.first << "\n";

    return false;
  }

  return true;
}

void erase(Cown* key)
{
  auto erased = map.erase(key);
  if (!erased)
  {
    std::cout << err.str() << "not erased: " << key << std::endl;
    return;
  }
  std::cout << "Erase successfully: " << key->id() <<std::endl;
  Cown::release(alloc, key);
}

void update(Cown* key, int i)
{
  auto entry = std::make_pair(key, (int32_t)i);
  auto insertion = map.insert(alloc, entry);
    
    if ((insertion.first != true) || (insertion.second.key() != key))
    {
      map.debug_layout(err)
        << "\n"
        << "incorrect return from update: " << insertion.first << ", "
        << insertion.second.key() << "\n";
      std::cout << err.str() << std::flush;
      return;
    }
    
    std::cout << "update " << key->id() <<std::endl; 
}

int find(Cown* key)
{
  auto it = map.find(key);
  if (it == map.end())
  {
    std::cout << "not found: " << key->id() << std::endl;
    return -1;
  }
  else if (it.is_marked())
  {
    std::cout<< "marked: " << key->id() << std::endl;
    return -1;
  }
  std::cout<< "found: " << key->id() <<std::endl;
  return it.value();
}

void insert(Cown* key, int i)
{ 
    auto entry = std::make_pair(key, (int32_t)i);
    auto insertion = map.insert(alloc, entry);
    
    if ((insertion.first != true) || (insertion.second.key() != key))
    {
      std::cout << "incorrect return from insert: " << insertion.first << ", "
        << insertion.second.key() << std::endl;
      return;
    }
    std::cout << "insert " << key->id() <<std::endl; 
}

bool test(int argc, char** argv){
  Logging::enable_logging();
  opt::Opt opt(argc, argv);

  size_t num_of_ops;

  map.debug_layout(err) << "\n";

  const auto cores = opt.is<size_t>("--cores", 4); // the number of threads
  // const auto cowns = (size_t)1 << opt.is<size_t>("--cowns", 8); // the number of conwn
  // const auto loops = opt.is<size_t>("--loops", 100); // the operation number of each thread
  num_of_ops = opt.is<size_t>("--number of ops", 100);

  //Allocating memory for global thread pool
  auto& sched = verona::rt::Scheduler::get();
  sched.set_fair(true);

  //Initialized the number of threads in the runtime system.
  sched.init(cores);

  uint64_t id=0;
  std::vector<Key*> inserted_keys;

  // insert operations
  for(size_t i=0;i<num_of_ops;i++)
  {
    auto *key = new (alloc)Key();
    if (key == nullptr) {
      std::cerr << "Failed to allocate Key object." << std::endl;
      continue;
    }
    inserted_keys.push_back(key);
    id++;
    schedule_lambda<YesTransfer>(
        key,
        [key, i]() { insert(key, i);});
  }

  //Query operations
  for(size_t i=0;i<num_of_ops;i++)
  {
    auto *key = inserted_keys[i];
    schedule_lambda<YesTransfer>(
        key,
        [key]() { 
          int ret_value=find(key);
          // std::cout<<"return value: "<<ret_value<<std::endl;
    });
  }
  //Update operations
  for(size_t i=0;i<num_of_ops;i++)
  {
    auto *key = inserted_keys[i];
    schedule_lambda<YesTransfer>(key,[key, i]() { insert(key, i);});
  }
  
  //Delete operations
  for(size_t i=0;i<num_of_ops;i++)
  {
    auto *key = inserted_keys[i];
    schedule_lambda<YesTransfer>(
        key,
        [key]() { erase(key);});
  }
  // schedule threads to run tasks
  auto start = snmalloc::Aal::tick();
  sched.run();
  auto end = snmalloc::Aal::tick();
  std::cout << "Time:" << (end - start) / (num_of_ops) << std::endl;

  map.clear(alloc);
  if (map.size() != 0)
  {
    map.debug_layout(std::cout) << "not empty" << std::endl;
    return false;
  }

  return true;

}

void insert1(std::vector<Cown*>& keys, size_t & num_of_ops)
{
    for(size_t i=0; i<num_of_ops & !keys.empty();i++)
    {
      auto* key = keys.back();
      keys.pop_back();

      auto entry = std::make_pair(key, (int32_t)i);
      auto insertion = map.insert(alloc, entry);
      std::cout << "insert " << key->id() <<std::endl;
    }
    
}

void schedule_insert1(std::vector<Cown*>& keys, int &i, size_t num_of_ops)
{
    // Behaviour f only run once, but it can consider all cowns at the same time, so no matter how much the num_of_ops is, it wil only insert the last cown.
    // all cowns are available, but only run f once.
    schedule_lambda(
        num_of_ops,
        keys.data(),
        [&keys, &num_of_ops]() { insert1(keys, num_of_ops); }
    );
}

bool test1(int argc, char** argv)
{ 
  size_t num_of_ops;

  // input options
  opt::Opt opt(argc, argv);

  const auto cores = opt.is<size_t>("--cores", 1); // the number of threads
  // const auto cowns = (size_t)1 << opt.is<size_t>("--cowns", 8); // the number of conwn
  // const auto loops = opt.is<size_t>("--loops", 100); // the operation number of each thread
  num_of_ops = opt.is<size_t>("--number of ops", 10);

  //Allocating memory for global thread pool
  auto& sched = verona::rt::Scheduler::get();
  sched.set_fair(true);

  //Initialized the number of threads in the runtime system.
  sched.init(cores);

  std::vector<Cown*> keys;
  uint64_t id=0;
  for(size_t i=0;i<num_of_ops;i++)
  {
    auto *key = new (alloc)Key();
    keys.push_back(key);
    id++;
  }
  int i=0;
  schedule_insert1(keys, i, num_of_ops);

  // schedule threads to run tasks
  auto start = snmalloc::Aal::tick();
  sched.run();
  auto end = snmalloc::Aal::tick();
  std::cout << "Time:" << (end - start) / (num_of_ops) << std::endl;

  map.clear(alloc);
  if (map.size() != 0)
  {
    map.debug_layout(std::cout) << "not empty" << std::endl;
    return false;
  }

  for (auto e : model)
    Cown::release(alloc, e.first);

  return true;
}

int main(int argc, char** argv)
{
  
  test(argc,argv);
  test1(argc,argv);

  return 0;
}




