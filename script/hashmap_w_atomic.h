#pragma once
#include <atomic>
#include <utility>
#include <cassert>
#include "../object/object.h"

namespace verona::rt
{
  template<typename Entry>
  class ObjectMap
  {
    std::atomic<Entry*> slots;
    std::atomic<size_t> filled_slots;
    std::atomic<uint8_t> capacity_shift;
    std::atomic<uint8_t> longest_probe;

    static constexpr uintptr_t MARK_MASK = Object::ALIGNMENT >> 1;
    static constexpr uintptr_t PROBE_MASK = MARK_MASK - 1;

    static_assert((MARK_MASK & PROBE_MASK) == 0);
    static_assert(((MARK_MASK | PROBE_MASK) & ~Object::MASK) == 0);

    template<typename>
    struct inspect_entry_type : std::false_type
    {};
    template<typename K>
    struct inspect_entry_type<K*> : std::true_type
    {
      static_assert(std::is_base_of_v<Object, K>);
      using key_type = K;
      using value_type = key_type*;
      using entry_view = value_type;
      static constexpr bool is_set = true;
    };
    template<typename K, typename V>
    struct inspect_entry_type<std::pair<K*, V>> : std::true_type
    {
      static_assert(std::is_base_of_v<Object, K>);
      using key_type = K;
      using value_type = V;
      using entry_view = std::pair<key_type*, V*>;
      static constexpr bool is_set = false;
    };

    static_assert(
      inspect_entry_type<Entry>(),
      "Map Entry must be K* or std::pair<K*, V>"
      " where K is derived from Object");

    using KeyType = typename inspect_entry_type<Entry>::key_type;
    using ValueType = typename inspect_entry_type<Entry>::value_type;
    using EntryView = typename inspect_entry_type<Entry>::entry_view;
    static constexpr bool is_set = inspect_entry_type<Entry>::is_set;

    static uintptr_t& key_of(Entry& entry)
    {
      if constexpr (is_set)
        return (uintptr_t&)entry;
      else
        return (uintptr_t&)std::get<0>(entry);
    }

    static uintptr_t unmark_key(uintptr_t key)
    {
      return key & ~Object::MASK;
    }

    static uint8_t probe_index(uintptr_t key)
    {
      return (uint8_t)(key & PROBE_MASK);
    }

    void init_alloc(Alloc& alloc)
    {
      static constexpr size_t init_capacity = 8;
      capacity_shift.store((uint8_t)bits::ctz(init_capacity), std::memory_order_relaxed);
      slots.store((Entry*)alloc.alloc<init_capacity * sizeof(Entry), YesZero>(), std::memory_order_relaxed);
      filled_slots.store(0, std::memory_order_relaxed);
      longest_probe.store(0, std::memory_order_relaxed);
    }

    void resize(Alloc& alloc)
    {
      auto old_slots = slots.load(std::memory_order_relaxed);
      auto old_capacity = capacity();
      auto old_longest_probe = longest_probe.load(std::memory_order_relaxed);

      capacity_shift.fetch_add(1, std::memory_order_relaxed);
      auto new_capacity = capacity();
      auto new_slots = (Entry*)alloc.alloc<YesZero>(new_capacity * sizeof(Entry));
      slots.store(new_slots, std::memory_order_release);
      filled_slots.store(0, std::memory_order_relaxed);
      longest_probe.store(0, std::memory_order_relaxed);

      for (size_t i = 0; i < old_capacity; i++)
      {
        auto& entry = old_slots[i];
        if (key_of(entry) != 0)
        {
          if constexpr (is_set)
            insert(alloc, entry);
          else
            insert(alloc, std::make_pair(std::move(std::get<0>(entry)), std::move(std::get<1>(entry))));
          key_of(entry) = 0;
        }
      }

      alloc.dealloc(old_slots, old_capacity * sizeof(Entry));
    }

    template<typename E>
    void place_entry(E entry, size_t index, uint8_t probe_len)
    {
      auto& key = key_of(entry);
      assert(probe_len <= PROBE_MASK);
      key = (key & ~PROBE_MASK) | probe_len;
      slots.load(std::memory_order_acquire)[index] = std::forward<E>(entry);
      if (probe_len > longest_probe.load(std::memory_order_relaxed))
        longest_probe.store(probe_len, std::memory_order_relaxed);
    }

  public:
    class Iterator
    {
      template<typename _Entry>
      friend class ObjectMap;

      const ObjectMap* map;
      size_t index;

      Entry& entry()
      {
        return map->slots[index];
      }

      Iterator(const ObjectMap* m, size_t i) : map(m), index(i) {}

    public:
      KeyType* key()
      {
        return (KeyType*)unmark_key(key_of(entry()));
      }

      template<bool v = !is_set, typename = typename std::enable_if_t<v>>
      ValueType& value()
      {
        return entry().second;
      }

      bool is_marked()
      {
        return key_of(entry()) & MARK_MASK;
      }

      void mark()
      {
        key_of(entry()) |= MARK_MASK;
      }

      void unmark()
      {
        key_of(entry()) &= ~MARK_MASK;
      }

      EntryView operator*()
      {
        if constexpr (is_set)
          return key();
        else
          return std::make_pair(key(), &value());
      }

      Iterator& operator++()
      {
        while (++index < map->capacity())
        {
          const auto key = key_of(map->slots[index]);
          if (key != 0)
            break;
        }
        return *this;
      }

      bool operator==(const Iterator& other) const
      {
        return (index == other.index) && (map == other.map);
      }

      bool operator!=(const Iterator& other) const
      {
        return !(*this == other);
      }
    }; //end of iterator

    ObjectMap(Alloc& alloc)
    {
      init_alloc(alloc);
    }

    ~ObjectMap()
    {
      dealloc(ThreadAlloc::get());
    }

    static ObjectMap<Entry>* create(Alloc& alloc)
    {
      return new (alloc.alloc<sizeof(ObjectMap<Entry>)>()) ObjectMap(alloc);
    }

    void dealloc(Alloc& alloc)
    {
      clear(alloc, true);
      alloc.dealloc(slots.load(std::memory_order_relaxed), capacity() * sizeof(Entry));
    }

    size_t size() const
    {
      return filled_slots.load(std::memory_order_acquire);
    }

    size_t capacity() const
    {
      return ((size_t)1 << capacity_shift.load(std::memory_order_relaxed));
    }

    Iterator begin() const
    {
      auto it = Iterator(this, 0);
      if (unmark_key(key_of(slots.load(std::memory_order_acquire)[0])) == 0)
        ++it;

      return it;
    }

    Iterator end() const
    {
      return Iterator(this, capacity());
    }

    Iterator find(const KeyType* key) const
    {
      if (key == nullptr)
        return end();

      const auto hash = bits::hash(key->id());
      auto index = hash & (capacity() - 1);
      for (size_t probe_len = 0; probe_len <= longest_probe.load(std::memory_order_acquire); probe_len++)
      {
        const auto k = unmark_key(key_of(slots.load(std::memory_order_acquire)[index]));
        if (k == (uintptr_t)key)
          return Iterator(this, index);

        if (++index == capacity())
          index = 0;
      }

      return end();
    }

    template<typename E>
    std::pair<bool, Iterator> insert(Alloc& alloc, E entry)
    {
      if (SNMALLOC_UNLIKELY(size() == capacity()))
        resize(alloc);

      assert(key_of(entry) != 0);
      const auto key = unmark_key(key_of(entry));
      const auto hash = bits::hash(((const Object*)key)->id());
      auto index = hash & (capacity() - 1);
      size_t iter_index = ~(size_t)0;

      for (uint8_t probe_len = 0; probe_len <= PROBE_MASK; probe_len++)
      {
        const auto k = key_of(slots.load(std::memory_order_acquire)[index]);

        if (unmark_key(k) == key)
        {
          if constexpr (!is_set)
            entry.second = std::forward<E>(entry).second;

          if (iter_index == ~(size_t)0)
            iter_index = index;

          return std::make_pair(false, Iterator(this, iter_index));
        }

        if (k == 0)
        {
          place_entry(std::forward<E>(entry), index, probe_len);
          filled_slots.fetch_add(1, std::memory_order_release);
          if (iter_index == ~(size_t)0)
            iter_index = index;

          return std::make_pair(true, Iterator(this, iter_index));
        }

        if (probe_index(k) < probe_len)
        {
          if (iter_index == ~(size_t)0)
            iter_index = index;

          Entry swap = std::move(slots.load(std::memory_order_acquire)[index]);
          place_entry(std::forward<E>(entry), index, probe_len);
          entry = swap;
          probe_len = probe_index(key_of(entry));
        }

        if (++index == capacity())
          index = 0;
      }

      resize(alloc);
      auto it = insert(alloc, std::forward<E>(entry)).second;
      if ((uintptr_t)it.key() != key)
        it = find((const KeyType*)key);

      return std::make_pair(true, std::move(it));
    }

    bool erase(const KeyType* key)
    {
      auto it = find(key);
      if (it == end())
        return false;

      erase(it);
      return true;
    }

    void erase(Iterator& it)
    {
      assert(key_of(it.entry()) != 0);
      it.entry().~Entry();
      key_of(it.entry()) = 0;
      filled_slots.fetch_sub(1, std::memory_order_release);
    }

    void clear(Alloc& alloc, bool skip_deallocate = false)
    {
      for (auto it = begin(); it != end(); ++it)
        erase(it);

      longest_probe.store(0, std::memory_order_relaxed);

      if (!skip_deallocate && (capacity() > 8))
      {
        alloc.dealloc(slots.load(std::memory_order_acquire), capacity() * sizeof(Entry));
        init_alloc(alloc);
      }
    }

    template<typename OutStream>
    OutStream& debug_layout(OutStream& out) const
    {
      out << "{";
      for (size_t i = 0; i < capacity(); i++)
      {
        const auto key = key_of(slots.load(std::memory_order_acquire)[i]);
        if (key == 0)
        {
          out << " ∅";
          continue;
        }
        out << " (" << ((const KeyType*)unmark_key(key_of(slots.load(std::memory_order_acquire)[i])))->id()
            << ", probe " << (size_t)probe_index(key) << ")";
      }
      out << " } cap: " << capacity();
      return out;
    }
  };
}
