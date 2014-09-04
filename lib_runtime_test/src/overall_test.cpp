#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

#include <runtime/queue.h>
#include <runtime/string_stream.h>
#include <runtime/murmur_hash.h>
#include <runtime/hash.h>
#include <runtime/temp_allocator.h>
#include <runtime/array.h>
#include <runtime/memory.h>
#include <runtime/idlut.h>
#include <runtime/list.h>
#include <runtime/assert.h>
#include <runtime/pool_allocator.h>
#include <runtime/string_pool.h>
#include <runtime/json.h>
#include <runtime/tree.h>

namespace
{
  using namespace pge;

  void test_memory() {
    memory_globals::init();
    Allocator &a = memory_globals::default_allocator();

    void *p = a.allocate(100);
    ASSERT(a.allocated_size(p) >= 100);
    ASSERT(a.total_allocated() >= 100);
    void *q = a.allocate(100);
    ASSERT(a.allocated_size(q) >= 100);
    ASSERT(a.total_allocated() >= 200);

    a.deallocate(p);
    a.deallocate(q);

    memory_globals::shutdown();
  }

  void test_array() {
    memory_globals::init();
    Allocator &a = memory_globals::default_allocator();

    {
      Array<int> v(a);

      ASSERT(array::size(v) == 0);
      array::push_back(v, 3);
      ASSERT(array::size(v) == 1);
      ASSERT(v[0] == 3);

      Array<int> v2(v);
      ASSERT(v2[0] == 3);
      v2[0] = 5;
      ASSERT(v[0] == 3);
      ASSERT(v2[0] == 5);
      v2 = v;
      ASSERT(v2[0] == 3);

      ASSERT(array::end(v) - array::begin(v) == (i32)array::size(v));
      ASSERT(*array::begin(v) == 3);
      array::pop_back(v);
      ASSERT(array::empty(v));

      for (int i=0; i < 100; ++i)
        array::push_back(v, i);
      ASSERT(array::size(v) == 100);
    }

    memory_globals::shutdown();
  }

  void test_scratch() {
    memory_globals::init(256 * 1024);
    Allocator &a = memory_globals::default_scratch_allocator();

    char *p = (char *)a.allocate(10 * 1024);

    char *pointers[100];
    for (int i=0; i < 100; ++i)
      pointers[i] = (char *)a.allocate(1024);
    for (int i=0; i < 100; ++i)
      a.deallocate(pointers[i]);

    a.deallocate(p);

    for (int i=0; i < 100; ++i)
      pointers[i] = (char *)a.allocate(4 * 1024);
    for (int i=0; i < 100; ++i)
      a.deallocate(pointers[i]);

    memory_globals::shutdown();
  }

  void test_temp_allocator() {
    memory_globals::init();
    {
      TempAllocator128 ta;
      Array<int> a(ta);
      for (int i=0; i < 100; ++i)
        array::push_back(a, i);
      ta.allocate(2 * 1024);
    }
    memory_globals::shutdown();

    memory_globals::init();
    {
      TempAllocator<sizeof(u8)> ta;
      f32 *tmp = (f32*)ta.allocate(sizeof(f32));
      *tmp = 42.f;
      ta.deallocate(tmp);
    }
    memory_globals::shutdown();

  }

  void test_hash() {
    memory_globals::init();
    {
      TempAllocator128 ta;
      Hash<int> h(ta);
      ASSERT(hash::get(h, 0, 99) == 99);
      ASSERT(!hash::has(h, 0));
      hash::remove(h, 0);
      hash::set(h, 1000, 123);
      ASSERT(hash::get(h, 1000, 0) == 123);
      ASSERT(hash::get(h, 2000, 99) == 99);

      for (int i=0; i < 100; ++i)
        hash::set(h, i, i*i);
      for (int i=0; i < 100; ++i)
        ASSERT(hash::get(h, i, 0) == i*i);
      hash::remove(h, 1000);
      ASSERT(!hash::has(h, 1000));
      hash::remove(h, 2000);
      ASSERT(hash::get(h, 1000, 0) == 0);
      for (int i=0; i < 100; ++i)
        ASSERT(hash::get(h, i, 0) == i*i);
    }
    memory_globals::shutdown();
  }

  void test_multi_hash()
  {
    memory_globals::init();
    {
      TempAllocator128 ta;
      Hash<int> h(ta);

      ASSERT(multi_hash::count(h, 0) == 0);
      multi_hash::insert(h, 0, 1);
      multi_hash::insert(h, 0, 2);
      multi_hash::insert(h, 0, 3);
      ASSERT(multi_hash::count(h, 0) == 3);

      Array<int> a(ta);
      multi_hash::get(h, 0, a);
      ASSERT(array::size(a) == 3);
      std::sort(array::begin(a), array::end(a));
      ASSERT(a[0] == 1 && a[1] == 2 && a[2] == 3);

      multi_hash::remove(h, multi_hash::find_first(h, 0));
      ASSERT(multi_hash::count(h, 0) == 2);
      multi_hash::remove_all(h, 0);
      ASSERT(multi_hash::count(h, 0) == 0);
    }

    {
      TempAllocator128 ta;
      Hash<int> h(ta);

      multi_hash::insert(h, 1, 1);
      multi_hash::insert(h, 2, 2);
      multi_hash::insert(h, 3, 3);
      multi_hash::insert(h, 4, 4);
      multi_hash::insert(h, 5, 5);
      multi_hash::insert(h, 6, 6);
      multi_hash::insert(h, 9, 666);
      multi_hash::insert(h, 9, 777);
      multi_hash::insert(h, 9, 888);

      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 1);
      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 2);
      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 3);
      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 4);
      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 5);
      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 6);
      ASSERT(multi_hash::count(h, 9) == 3);
      multi_hash::remove_all(h, 9);
      ASSERT(multi_hash::count(h, 2) == 0);
    }

    memory_globals::shutdown();
  }

  void test_murmur_hash()
  {
    const char *s = "test_string";
    u64 h = murmur_hash_64(s, 0);
    ASSERT(h == 0xe604acc23b568f83ull);
  }

  void test_pointer_arithmetic()
  {
    const char check = -2;
    const unsigned test_size = 128;

    TempAllocator512 ta;
    Array<char> buffer(ta);
    array::set_capacity(buffer, test_size);
    memset(array::begin(buffer), 0, array::size(buffer));

    void* data = array::begin(buffer);
    for (unsigned i = 0; i != test_size; ++i) {
      buffer[i] = check;
      char* value = (char*)memory::pointer_add(data, i);
      ASSERT(*value == buffer[i]);
    }
  }

  void test_string_stream()
  {
    memory_globals::init();
    {
      using namespace string_stream;

      TempAllocator1024 ta;
      Buffer ss(ta);

      ss << "Name"; 			 tab(ss, 20);	ss << "Score\n";
      repeat(ss, 10, '-'); tab(ss, 20);	repeat(ss, 10, '-'); ss << "\n";
      ss << "Niklas";      tab(ss, 20);	printf(ss, "%.2f", 2.7182818284f); ss << "\n";
      ss << "Jim";         tab(ss, 20);	printf(ss, "%.2f", 3.14159265f);   ss << "\n";

      ASSERT(
        0 == strcmp(c_str(ss),
        "Name                Score\n"
        "----------          ----------\n"
        "Niklas              2.72\n"
        "Jim                 3.14\n"
        )
        );
    }
    memory_globals::shutdown();
  }

  void test_queue()
  {
    memory_globals::init();
    {
      TempAllocator1024 ta;
      Queue<int> q(ta);

      queue::reserve(q, 10);
      ASSERT(queue::space(q) == 10);
      queue::push_back(q, 11);
      queue::push_front(q, 22);
      ASSERT(queue::size(q) == 2);
      ASSERT(q[0] == 22);
      ASSERT(q[1] == 11);
      queue::consume(q, 2);
      ASSERT(queue::size(q) == 0);
      int items[] ={ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
      queue::push(q, items, 10);
      ASSERT(queue::size(q) == 10);
      for (int i=0; i < 10; ++i)
        ASSERT(q[i] == i + 1);
      queue::consume(q, queue::end_front(q) - queue::begin_front(q));
      queue::consume(q, queue::end_front(q) - queue::begin_front(q));
      ASSERT(queue::size(q) == 0);
    }
  }

  void test_idlut()
  {
    memory_globals::init();
    {
      Allocator &a = memory_globals::default_allocator();
      u64 id1, id2, id3;
      IdLookupTable<i32> t(a);

      const u32 NUM_INSERT = 100;
      u64 ids[NUM_INSERT];

      //idlut::reserve(t, 2);

      id1 = idlut::add(t, 1);
      ASSERT(idlut::has(t, id1));
      ASSERT(idlut::lookup(t, id1, 0) == 1);

      id2 = idlut::add(t, 2);
      ASSERT(idlut::has(t, id2));
      ASSERT(idlut::lookup(t, id2, 0) == 2);

      idlut::remove(t, id1);
      ASSERT(!idlut::has(t, id1));

      id3 = idlut::add(t, 3);
      ASSERT(idlut::has(t, id3));
      ASSERT(idlut::lookup(t, id3, 0) == 3);

      for (int i=0; i < NUM_INSERT; ++i)
        ids[i] = idlut::add(t, i);

      ASSERT(idlut::lookup(t, id2, 0) == 2);
      idlut::remove(t, id2);
      ASSERT(idlut::lookup(t, id3, 0) == 3);
      idlut::remove(t, id3);

      ASSERT(idlut::size(t) == NUM_INSERT);
      ASSERT(idlut::end(t) - idlut::begin(t) == NUM_INSERT);
      idlut::remove(t, ids[0]);
      idlut::remove(t, ids[NUM_INSERT - 1]);

      for (int i=1; i < NUM_INSERT - 1; ++i)
        ASSERT(idlut::lookup(t, ids[i], 0) == i);


      {
        IdLookupTable<i32> t2(a);
        const i32 NUM_ITEMS = 8;
        u64 ids[NUM_ITEMS];

        for (i32 i = 0; i < 100; i++) {
          for (i32 j = 0; j < NUM_ITEMS; j++)
            ids[j] = idlut::add(t2, j);

          for (i32 j = 0; j < NUM_ITEMS; j++)
            ASSERT(*idlut::lookup(t2, ids[j]) == j);

          for (i32 j = NUM_ITEMS - 1; j >= 0; j--)
            idlut::remove(t2, ids[j]);
        }
      }
    }
    memory_globals::shutdown();
  }

  void test_pool_allocator()
  {
    memory_globals::init();
    {
      PoolAllocator pa(4, 100);

      char *pointers[150];
      for (int i=0; i < 150; ++i)
        pointers[i] = (char *)pa.allocate(sizeof(int), alignof(int));
      for (int i=0; i < 150; ++i)
        pa.deallocate(pointers[i]);

      void *p = pa.allocate(2 * 1024, pa.DEFAULT_ALIGN);
      pa.deallocate(p);
    }
    memory_globals::shutdown();
  }

  void test_string_pool()
  {
    memory_globals::init();
    {
      PoolAllocator pa(16, 2);
      StringPool sp(pa);

      const char *s1 = string_pool::acquire(sp, "hello world!");
      const char *s2 = string_pool::acquire(sp, "hello world!");

      ASSERT(s1 == s2);
      string_pool::release(sp, s1);
      ASSERT(strcmp(s2, "hello world!") == 0);
      string_pool::release(sp, s2);
      ASSERT(!string_pool::has(sp, "hello world!"));
    }
    memory_globals::shutdown();
  }

  void test_list() {
    memory_globals::init();
    Allocator &a = memory_globals::default_allocator();

    {
      List<u32> l(a);
      u32 value;

      value = 1111;
      list::push_front(l, value);
      ASSERT(list::size(l) == 1);
      ASSERT(l[0] == 1111);

      value = 2222;
      list::push_front(l, value);
      ASSERT(list::size(l) == 2);
      ASSERT(l[0] == 2222 && l[1] == 1111);

      value = 3333;
      list::push_front(l, value);
      ASSERT(list::size(l) == 3);
      ASSERT(l[0] == 3333 && l[1] == 2222 && l[2] == 1111);

      list::clear(l);

      value = 1111; list::push_back(l, value);
      value = 2222; list::push_back(l, value);
      value = 3333; list::push_back(l, value);
      value = 4444; list::push_back(l, value);
      ASSERT(list::size(l) == 4);
      ASSERT(l[0] == 1111 && l[1] == 2222 && l[2] == 3333 && l[3] == 4444);

      list::swap(l, 1, 3);
      list::swap(l, 1, 2);
      ASSERT(l[0] == 1111 && l[1] == 3333 && l[2] == 4444 && l[3] == 2222);

      list::clear(l);
      value = 1111; list::push_back(l, value);
      value = 2222; list::push_back(l, value);
      list::remove(l, 0);
      value = 5555; list::insert(l, value, 1);
      value = 6666; list::push_back(l, value);
      ASSERT(l[0] == 2222 && l[1] == 5555 && l[2] == 6666);

      list::clear(l);
      value = 1111; list::push_back(l, value);
      value = 2222; list::push_back(l, value);
      value = 3333; list::push_front(l, value);
      value = 4444; list::push_back(l, value);
      list::remove(l, 0);
      list::remove(l, 1);
      value = 5555; list::insert(l, value, 1);
      value = 6666; list::push_back(l, value);
      ASSERT(l[0] == 1111 && l[1] == 5555 && l[2] == 4444 && l[3] == 6666);
    }

    memory_globals::shutdown();
  }

  void test_tree()
  {
    memory_globals::init();
    {
      Tree<int> t(memory_globals::default_allocator(), 0);
      const u64 root = tree::root(t);
      const u64 n1   = tree::add(t, root, 1);
      const u64 n2   = tree::add(t, root, 2);
      const u64 n21  = tree::add(t, n2, 21);
      const u64 n22  = tree::add(t, n2, 22);
      const u64 n3   = tree::add(t, root, 3);

      ASSERT(tree::get(t, n2) == 2);
      ASSERT(tree::get(t, n21) == 21);
      ASSERT(tree::get(t, n22) == 22);

      ASSERT(tree::has_child(t, n2));
      ASSERT(!tree::has_child(t, n3));
      tree::move(t, n2, n3);
      ASSERT(tree::has_child(t, n2));
      ASSERT(tree::has_child(t, n3));
      tree::remove(t, n2);

      ASSERT(tree::has(t, n1));
      ASSERT(tree::has(t, n3));

      ASSERT(!tree::has(t, n2));
      ASSERT(!tree::has(t, n21));
      ASSERT(!tree::has(t, n22));
    }
    memory_globals::shutdown();
  }

  void test_json()
  {
    const char *input = "{\"menu\": { \"id\": \"file\", \"value\": \"File\", \"popup\": { \"menuitem\": [ {\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"}, {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"}, {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"} ]}}}";
    const char *object1 = "{\"apple\": 0, \"banana\": {\"weight\": 52, \"price\": 100}, \"cherry\": 97}";
    const char *object2 = "{\"banana\": {\"price\": 200}, \"durian\": 100}";
    memory_globals::init();
    {
      PoolAllocator pa(16, 2);
      StringPool sp(pa);

      Json jsn(memory_globals::default_allocator(), sp);
      json::parse_from_string(jsn, json::root(jsn), input);

      ASSERT(json::has(jsn, json::root(jsn), "menu"));
      const u64 menu = json::get_id(jsn, json::root(jsn), "menu");

      ASSERT(json::has(jsn, menu, "id"));
      ASSERT(strcmp(json::get_string(jsn, menu, "id"), "file") == 0);
      ASSERT(json::has(jsn, menu, "value"));
      ASSERT(strcmp(json::get_string(jsn, menu, "value"), "File") == 0);
      ASSERT(strcmp(json::get_string(jsn, menu, "JUNK", "junk"), "junk") == 0);

      ASSERT(json::has(jsn, menu, "popup"));
      const u64 popup = json::get_id(jsn, menu, "popup");
      ASSERT(json::has(jsn, popup, "menuitem"));
      const u64 menu_items = json::get_id(jsn, popup, "menuitem");

      ASSERT(json::size(jsn, menu_items) == 3);
      ASSERT(strcmp(json::get_string(jsn, json::get_id(jsn, menu_items, 0), "value"), "New") == 0);
      ASSERT(strcmp(json::get_string(jsn, json::get_id(jsn, menu_items, 1), "value"), "Open") == 0);
      ASSERT(strcmp(json::get_string(jsn, json::get_id(jsn, menu_items, 2), "value"), "Close") == 0);

      Json jsn1(memory_globals::default_allocator(), sp);
      json::parse_from_string(jsn1, json::root(jsn1), object1);

      Json jsn2(memory_globals::default_allocator(), sp);
      json::parse_from_string(jsn2, json::root(jsn2), object2);

      json::merge(jsn1, jsn2, json::root(jsn1), json::root(jsn2));

      ASSERT(json::get_integer(jsn1, json::root(jsn1), "apple") == 0);
      ASSERT(json::get_integer(jsn1, json::root(jsn1), "cherry") == 97);
      ASSERT(json::get_integer(jsn1, json::root(jsn1), "durian") == 100);
      u64 banana = json::get_id(jsn1, json::root(jsn1), "banana");
      ASSERT(json::get_integer(jsn1, banana, "weight") == 52);
      ASSERT(json::get_integer(jsn1, banana, "price") == 200);
      ASSERT(json::get_integer(jsn1, banana, "JUNK", 666) == 666);

      {
        using namespace string_stream;
        TempAllocator1024 ta;
        Buffer buf(ta);
        json::write(jsn1, json::get_id(jsn1, json::root(jsn1), "banana"), buf, true);
        printf("%s", c_str(buf));
      }
    }
    memory_globals::shutdown();
  }
}

namespace pge
{
  void overall_test()
  {
    //test_memory();
    //test_array();
    //test_scratch();
    //test_temp_allocator();
    //test_hash();
    //test_multi_hash();
    //test_murmur_hash();
    //test_pointer_arithmetic();
    //test_string_stream();
    //test_queue();
    test_idlut();
    //test_pool_allocator();
    //test_string_pool();
    //test_list();
    //test_tree();
    //test_json();
  }
}