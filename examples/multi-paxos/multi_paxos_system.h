#define _HAS_ITERATOR_DEBUGGING 0

/*++
  Copyright (c) Microsoft Corporation

  This hash template is borrowed from Microsoft Z3
  (https://github.com/Z3Prover/z3).

  Simple implementation of bucket-list hash tables conforming roughly
  to SGI hash_map and hash_set interfaces, though not all members are
  implemented.

  These hash tables have the property that insert preserves iterators
  and references to elements.

  This package lives in namespace hash_space. Specializations of
  class "hash" should be made in this namespace.

  --*/

#pragma once

#ifndef HASH_H
#define HASH_H

#ifdef _WINDOWS
#pragma warning(disable:4267)
#endif

#include <string>
#include <vector>
#include <iterator>
#include <fstream>

namespace hash_space {

    unsigned string_hash(const char * str, unsigned length, unsigned init_value);

    template <typename T> class hash {
    public:
        size_t operator()(const T &s) const {
            return s.__hash();
        }
    };

    template <>
        class hash<int> {
    public:
        size_t operator()(const int &s) const {
            return s;
        }
    };

    template <>
        class hash<long long> {
    public:
        size_t operator()(const long long &s) const {
            return s;
        }
    };

    template <>
        class hash<unsigned> {
    public:
        size_t operator()(const unsigned &s) const {
            return s;
        }
    };

    template <>
        class hash<unsigned long long> {
    public:
        size_t operator()(const unsigned long long &s) const {
            return s;
        }
    };

    template <>
        class hash<bool> {
    public:
        size_t operator()(const bool &s) const {
            return s;
        }
    };

    template <>
        class hash<std::string> {
    public:
        size_t operator()(const std::string &s) const {
            return string_hash(s.c_str(), (unsigned)s.size(), 0);
        }
    };

    template <>
        class hash<std::pair<int,int> > {
    public:
        size_t operator()(const std::pair<int,int> &p) const {
            return p.first + p.second;
        }
    };

    template <typename T>
        class hash<std::vector<T> > {
    public:
        size_t operator()(const std::vector<T> &p) const {
            hash<T> h;
            size_t res = 0;
            for (unsigned i = 0; i < p.size(); i++)
                res += h(p[i]);
            return res;
        }
    };

    template <class T>
        class hash<std::pair<T *, T *> > {
    public:
        size_t operator()(const std::pair<T *,T *> &p) const {
            return (size_t)p.first + (size_t)p.second;
        }
    };

    template <class T>
        class hash<T *> {
    public:
        size_t operator()(T * const &p) const {
            return (size_t)p;
        }
    };

    enum { num_primes = 29 };

    static const unsigned long primes[num_primes] =
        {
            7ul,
            53ul,
            97ul,
            193ul,
            389ul,
            769ul,
            1543ul,
            3079ul,
            6151ul,
            12289ul,
            24593ul,
            49157ul,
            98317ul,
            196613ul,
            393241ul,
            786433ul,
            1572869ul,
            3145739ul,
            6291469ul,
            12582917ul,
            25165843ul,
            50331653ul,
            100663319ul,
            201326611ul,
            402653189ul,
            805306457ul,
            1610612741ul,
            3221225473ul,
            4294967291ul
        };

    inline unsigned long next_prime(unsigned long n) {
        const unsigned long* to = primes + (int)num_primes;
        for(const unsigned long* p = primes; p < to; p++)
            if(*p >= n) return *p;
        return primes[num_primes-1];
    }

    template<class Value, class Key, class HashFun, class GetKey, class KeyEqFun>
        class hashtable
    {
    public:

        typedef Value &reference;
        typedef const Value &const_reference;
    
        struct Entry
        {
            Entry* next;
            Value val;
      
        Entry(const Value &_val) : val(_val) {next = 0;}
        };
    

        struct iterator
        {      
            Entry* ent;
            hashtable* tab;

            typedef std::forward_iterator_tag iterator_category;
            typedef Value value_type;
            typedef std::ptrdiff_t difference_type;
            typedef size_t size_type;
            typedef Value& reference;
            typedef Value* pointer;

        iterator(Entry* _ent, hashtable* _tab) : ent(_ent), tab(_tab) { }

            iterator() { }

            Value &operator*() const { return ent->val; }

            Value *operator->() const { return &(operator*()); }

            iterator &operator++() {
                Entry *old = ent;
                ent = ent->next;
                if (!ent) {
                    size_t bucket = tab->get_bucket(old->val);
                    while (!ent && ++bucket < tab->buckets.size())
                        ent = tab->buckets[bucket];
                }
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                operator++();
                return tmp;
            }


            bool operator==(const iterator& it) const { 
                return ent == it.ent;
            }

            bool operator!=(const iterator& it) const {
                return ent != it.ent;
            }
        };

        struct const_iterator
        {      
            const Entry* ent;
            const hashtable* tab;

            typedef std::forward_iterator_tag iterator_category;
            typedef Value value_type;
            typedef std::ptrdiff_t difference_type;
            typedef size_t size_type;
            typedef const Value& reference;
            typedef const Value* pointer;

        const_iterator(const Entry* _ent, const hashtable* _tab) : ent(_ent), tab(_tab) { }

            const_iterator() { }

            const Value &operator*() const { return ent->val; }

            const Value *operator->() const { return &(operator*()); }

            const_iterator &operator++() {
                const Entry *old = ent;
                ent = ent->next;
                if (!ent) {
                    size_t bucket = tab->get_bucket(old->val);
                    while (!ent && ++bucket < tab->buckets.size())
                        ent = tab->buckets[bucket];
                }
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                operator++();
                return tmp;
            }


            bool operator==(const const_iterator& it) const { 
                return ent == it.ent;
            }

            bool operator!=(const const_iterator& it) const {
                return ent != it.ent;
            }
        };

    private:

        typedef std::vector<Entry*> Table;

        Table buckets;
        size_t entries;
        HashFun hash_fun ;
        GetKey get_key;
        KeyEqFun key_eq_fun;
    
    public:

    hashtable(size_t init_size) : buckets(init_size,(Entry *)0) {
            entries = 0;
        }
    
        hashtable(const hashtable& other) {
            dup(other);
        }

        hashtable& operator= (const hashtable& other) {
            if (&other != this)
                dup(other);
            return *this;
        }

        ~hashtable() {
            clear();
        }

        size_t size() const { 
            return entries;
        }

        bool empty() const { 
            return size() == 0;
        }

        void swap(hashtable& other) {
            buckets.swap(other.buckets);
            std::swap(entries, other.entries);
        }
    
        iterator begin() {
            for (size_t i = 0; i < buckets.size(); ++i)
                if (buckets[i])
                    return iterator(buckets[i], this);
            return end();
        }
    
        iterator end() { 
            return iterator(0, this);
        }

        const_iterator begin() const {
            for (size_t i = 0; i < buckets.size(); ++i)
                if (buckets[i])
                    return const_iterator(buckets[i], this);
            return end();
        }
    
        const_iterator end() const { 
            return const_iterator(0, this);
        }
    
        size_t get_bucket(const Value& val, size_t n) const {
            return hash_fun(get_key(val)) % n;
        }
    
        size_t get_key_bucket(const Key& key) const {
            return hash_fun(key) % buckets.size();
        }

        size_t get_bucket(const Value& val) const {
            return get_bucket(val,buckets.size());
        }

        Entry *lookup(const Value& val, bool ins = false)
        {
            resize(entries + 1);

            size_t n = get_bucket(val);
            Entry* from = buckets[n];
      
            for (Entry* ent = from; ent; ent = ent->next)
                if (key_eq_fun(get_key(ent->val), get_key(val)))
                    return ent;
      
            if(!ins) return 0;

            Entry* tmp = new Entry(val);
            tmp->next = from;
            buckets[n] = tmp;
            ++entries;
            return tmp;
        }

        Entry *lookup_key(const Key& key) const
        {
            size_t n = get_key_bucket(key);
            Entry* from = buckets[n];
      
            for (Entry* ent = from; ent; ent = ent->next)
                if (key_eq_fun(get_key(ent->val), key))
                    return ent;
      
            return 0;
        }

        const_iterator find(const Key& key) const {
            return const_iterator(lookup_key(key),this);
        }

        iterator find(const Key& key) {
            return iterator(lookup_key(key),this);
        }

        std::pair<iterator,bool> insert(const Value& val){
            size_t old_entries = entries;
            Entry *ent = lookup(val,true);
            return std::pair<iterator,bool>(iterator(ent,this),entries > old_entries);
        }
    
        iterator insert(const iterator &it, const Value& val){
            Entry *ent = lookup(val,true);
            return iterator(ent,this);
        }

        size_t erase(const Key& key)
        {
            Entry** p = &(buckets[get_key_bucket(key)]);
            size_t count = 0;
            while(*p){
                Entry *q = *p;
                if (key_eq_fun(get_key(q->val), key)) {
                    ++count;
                    *p = q->next;
                    delete q;
                }
                else
                    p = &(q->next);
            }
            entries -= count;
            return count;
        }

        void resize(size_t new_size) {
            const size_t old_n = buckets.size();
            if (new_size <= old_n) return;
            const size_t n = next_prime(new_size);
            if (n <= old_n) return;
            Table tmp(n, (Entry*)(0));
            for (size_t i = 0; i < old_n; ++i) {
                Entry* ent = buckets[i];
                while (ent) {
                    size_t new_bucket = get_bucket(ent->val, n);
                    buckets[i] = ent->next;
                    ent->next = tmp[new_bucket];
                    tmp[new_bucket] = ent;
                    ent = buckets[i];
                }
            }
            buckets.swap(tmp);
        }
    
        void clear()
        {
            for (size_t i = 0; i < buckets.size(); ++i) {
                for (Entry* ent = buckets[i]; ent != 0;) {
                    Entry* next = ent->next;
                    delete ent;
                    ent = next;
                }
                buckets[i] = 0;
            }
            entries = 0;
        }

        void dup(const hashtable& other)
        {
            clear();
            buckets.resize(other.buckets.size());
            for (size_t i = 0; i < other.buckets.size(); ++i) {
                Entry** to = &buckets[i];
                for (Entry* from = other.buckets[i]; from; from = from->next)
                    to = &((*to = new Entry(from->val))->next);
            }
            entries = other.entries;
        }
    };

    template <typename T> 
        class equal {
    public:
        bool operator()(const T& x, const T &y) const {
            return x == y;
        }
    };

    template <typename T>
        class identity {
    public:
        const T &operator()(const T &x) const {
            return x;
        }
    };

    template <typename T, typename U>
        class proj1 {
    public:
        const T &operator()(const std::pair<T,U> &x) const {
            return x.first;
        }
    };

    template <typename Element, class HashFun = hash<Element>, 
        class EqFun = equal<Element> >
        class hash_set
        : public hashtable<Element,Element,HashFun,identity<Element>,EqFun> {

    public:

    typedef Element value_type;

    hash_set()
    : hashtable<Element,Element,HashFun,identity<Element>,EqFun>(7) {}
    };

    template <typename Key, typename Value, class HashFun = hash<Key>, 
        class EqFun = equal<Key> >
        class hash_map
        : public hashtable<std::pair<Key,Value>,Key,HashFun,proj1<Key,Value>,EqFun> {

    public:

    hash_map()
    : hashtable<std::pair<Key,Value>,Key,HashFun,proj1<Key,Value>,EqFun>(7) {}

    Value &operator[](const Key& key) {
	std::pair<Key,Value> kvp(key,Value());
	return 
	hashtable<std::pair<Key,Value>,Key,HashFun,proj1<Key,Value>,EqFun>::
        lookup(kvp,true)->val.second;
    }
    };

    template <typename D,typename R>
        class hash<hash_map<D,R> > {
    public:
        size_t operator()(const hash_map<D,R> &p) const {
            hash<D > h1;
            hash<R > h2;
            size_t res = 0;
            
            for (typename hash_map<D,R>::const_iterator it=p.begin(), en=p.end(); it!=en; ++it)
                res += (h1(it->first)+h2(it->second));
            return res;
        }
    };

    template <typename D,typename R>
    inline bool operator ==(const hash_map<D,R> &s, const hash_map<D,R> &t){
        for (typename hash_map<D,R>::const_iterator it=s.begin(), en=s.end(); it!=en; ++it) {
            typename hash_map<D,R>::const_iterator it2 = t.find(it->first);
            if (it2 == t.end() || !(it->second == it2->second)) return false;
        }
        for (typename hash_map<D,R>::const_iterator it=t.begin(), en=t.end(); it!=en; ++it) {
            typename hash_map<D,R>::const_iterator it2 = s.find(it->first);
            if (it2 == t.end() || !(it->second == it2->second)) return false;
        }
        return true;
    }
}
#endif
typedef std::string __strlit;
extern std::ofstream __ivy_out;
void __ivy_exit(int);

template <typename D, typename R>
struct thunk {
    virtual R operator()(const D &) = 0;
    int ___ivy_choose(int rng,const char *name,int id) {
        return 0;
    }
};
template <typename D, typename R, class HashFun = hash_space::hash<D> >
struct hash_thunk {
    thunk<D,R> *fun;
    hash_space::hash_map<D,R,HashFun> memo;
    hash_thunk() : fun(0) {}
    hash_thunk(thunk<D,R> *fun) : fun(fun) {}
    ~hash_thunk() {
//        if (fun)
//            delete fun;
    }
    R &operator[](const D& arg){
        std::pair<typename hash_space::hash_map<D,R>::iterator,bool> foo = memo.insert(std::pair<D,R>(arg,R()));
        R &res = foo.first->second;
        if (foo.second && fun)
            res = (*fun)(arg);
        return res;
    }
};

    #include <netinet/tcp.h>
    #include <list>
    #include <semaphore.h>

    class tcp_listener;   // class of threads that listen for connections
    class tcp_callbacks;  // class holding callbacks to ivy

    // A tcp_config maps endpoint ids to IP addresses and ports.

    class tcp_config {
    public:
        // get the address and port from the endpoint id
        virtual void get(int id, unsigned long &inetaddr, unsigned long &inetport);

        // get the endpoint id from the address and port
        virtual int rev(unsigned long inetaddr, unsigned long inetport);
    };

    class tcp_queue;


	class sec_timer;
    

    class reader;
    class timer;

class multi_paxos_system {
  public:
    typedef multi_paxos_system ivy_class;

    std::vector<std::string> __argv;
#ifdef _WIN32
    void *mutex;  // forward reference to HANDLE
#else
    pthread_mutex_t mutex;
#endif
    void __lock();
    void __unlock();

#ifdef _WIN32
    std::vector<HANDLE> thread_ids;

#else
    std::vector<pthread_t> thread_ids;

#endif
    void install_reader(reader *);
    void install_thread(reader *);
    void install_timer(timer *);
    virtual ~multi_paxos_system();
    std::vector<int> ___ivy_stack;
    int ___ivy_choose(int rng,const char *name,int id);
    virtual void ivy_assert(bool,const char *){}
    virtual void ivy_assume(bool,const char *){}
    virtual void ivy_check_progress(int,int){}
    typedef int node;
    struct node__iter__t {
    bool is_end;
    node val;
        size_t __hash() const { return hash_space::hash<bool>()(is_end)+hash_space::hash<node>()(val);}
    };
    class nset__arr : public std::vector<node>{
        public: size_t __hash() const { return hash_space::hash<std::vector<node> >()(*this);};
    };
    struct nset {
    nset__arr repr;
        size_t __hash() const { return hash_space::hash<nset__arr>()(repr);}
    };
    struct vote_struct {
    unsigned long long maxr;
    __strlit maxv;
        size_t __hash() const { return hash_space::hash<unsigned long long>()(maxr)+hash_space::hash<__strlit>()(maxv);}
    };
    class votemap : public std::vector<vote_struct>{
        public: size_t __hash() const { return hash_space::hash<std::vector<vote_struct> >()(*this);};
    };
    struct votemap_seg {
    unsigned long long offset;
    votemap elems;
        size_t __hash() const { return hash_space::hash<unsigned long long>()(offset)+hash_space::hash<votemap>()(elems);}
    };
    enum msg_kind{msg_kind__one_a,msg_kind__one_b,msg_kind__two_a,msg_kind__two_b,msg_kind__keep_alive,msg_kind__missing_two_a,msg_kind__missing_decision,msg_kind__decide};
    struct msg {
    msg_kind m_kind;
    unsigned long long m_round;
    unsigned long long m_inst;
    node m_node;
    __strlit m_value;
    votemap_seg m_votemap;
        size_t __hash() const { return hash_space::hash<int>()(m_kind)+hash_space::hash<unsigned long long>()(m_round)+hash_space::hash<unsigned long long>()(m_inst)+hash_space::hash<node>()(m_node)+hash_space::hash<__strlit>()(m_value)+hash_space::hash<votemap_seg>()(m_votemap);}
    };
    struct system__ballot_status {
    bool active;
    nset voters;
    __strlit proposal;
    bool decided;
        size_t __hash() const { return hash_space::hash<bool>()(active)+hash_space::hash<nset>()(voters)+hash_space::hash<__strlit>()(proposal)+hash_space::hash<bool>()(decided);}
    };
    class system__ballot_status_array : public std::vector<system__ballot_status>{
        public: size_t __hash() const { return hash_space::hash<std::vector<system__ballot_status> >()(*this);};
    };
    class system__timearray : public std::vector<unsigned long long>{
        public: size_t __hash() const { return hash_space::hash<std::vector<unsigned long long> >()(*this);};
    };
    struct system__decision_struct {
    __strlit decision;
    bool present;
        size_t __hash() const { return hash_space::hash<__strlit>()(decision)+hash_space::hash<bool>()(present);}
    };
    class system__log : public std::vector<system__decision_struct>{
        public: size_t __hash() const { return hash_space::hash<std::vector<system__decision_struct> >()(*this);};
    };
    node node__size;
    unsigned long long system__server__two_a_retransmitter__current_time;
    unsigned long long system__server__leader_election__timeout;
    unsigned long long system__server__first_undecided;
    votemap system__server__my_votes;
    unsigned long long system__server__leader_election__my_time;
    unsigned long long system__server__leader_election__last_heard_from_leader;
    nset nset__all;
    unsigned long long system__server__catch_up__current_time;
    hash_thunk<node,bool> net__proc__isup;
    unsigned long long system__server__leader_election__last_start_round;
    hash_thunk<nset,int> nset__card;
    hash_thunk<node,bool> net__proc__pend;
    unsigned long long system__server__current_round;
    nset system__server__joined;
    hash_thunk<node,int> net__proc__sock;
    system__ballot_status system__init_status;
    votemap_seg system__server__joined_votes;
    bool system__server__round_active;
    system__log system__server__my_log;
    unsigned long long system__server__next_inst;
    bool system__server__current_leader__impl__full;
    system__decision_struct system__server__no_decision;
    system__timearray system__server__two_a_retransmitter__two_a_times;
    system__ballot_status_array system__server__inst_status;
    vote_struct not_a_vote;
    system__timearray system__server__catch_up__two_a_times;
    bool _generating;
    node system__server__current_leader__impl__val;
    node self;
    long long __CARD__value;
    long long __CARD__system__time;
    long long __CARD__nset__index;
    long long __CARD__inst;
    long long __CARD__net__tcp__socket;
    long long __CARD__round;
    virtual bool nset__member(const node& N, const nset& S);
    virtual bool nset__majority(const nset& S);
    virtual vote_struct votemap_seg__value(const votemap_seg& X, unsigned long long N);
    virtual unsigned long long votemap_seg__first(const votemap_seg& X);
    virtual unsigned long long votemap_seg__upper(const votemap_seg& x);
    virtual unsigned long long none();
    virtual vote_struct votemap_seg_ops__maxvote(const votemap_seg& M, unsigned long long I);
    virtual bool leader_of(const node& N, unsigned long long R);
    virtual bool system__server__is_decided(unsigned long long J);
    virtual __strlit no_op();
    virtual node __num0();
    virtual bool nset__arr__value(const nset__arr& a, int i, const node& v);
    virtual int nset__arr__end(const nset__arr& a);
    virtual vote_struct votemap__value(const votemap& a, unsigned long long i);
    virtual unsigned long long votemap__end(const votemap& a);
    virtual system__ballot_status system__ballot_status_array__value(const system__ballot_status_array& a, unsigned long long i);
    virtual unsigned long long system__ballot_status_array__end(const system__ballot_status_array& a);
    virtual unsigned long long system__timearray__value(const system__timearray& a, unsigned long long i);
    virtual unsigned long long system__timearray__end(const system__timearray& a);
    virtual system__decision_struct system__log__value(const system__log& a, unsigned long long i);
    virtual unsigned long long system__log__end(const system__log& a);
    virtual node round_leader__leader_fun(unsigned long long r);

    tcp_listener *net__tcp__impl__rdr;             // the listener task
    tcp_callbacks *net__tcp__impl__cb;             // the callbacks to ivy
    hash_space::hash_map<int,tcp_queue *> net__tcp__impl__send_queue;   // queues of blocked packets, per socket


    tcp_config *the_tcp_config;  // the current configurations

    // Get the current TCP configuration. If none, create a default one.

    tcp_config *get_tcp_config() {
        if (!the_tcp_config) 
            the_tcp_config = new tcp_config();
        return the_tcp_config; 
    }

    // Set the current TCP configuration. This is called by the runtime environment.

    void set_tcp_config(tcp_config *conf) {
        the_tcp_config = conf;
    }

	sec_timer *system__server__timer__sec__impl__tmr;
        multi_paxos_system(node node__size, node self);
    virtual void ext__system__server__catch_up__tick();
    virtual unsigned long long ext__system__ballot_status_array__size(const system__ballot_status_array& a);
    virtual void ext__nset__add(nset& s, const node& n);
    virtual void ext__node__iter__next(node__iter__t& x);
    virtual void ext__system__server__start_round();
    virtual unsigned long long ext__inst__next(unsigned long long x);
    virtual system__ballot_status_array ext__system__ballot_status_array__empty();
    virtual void ext__system__timearray__set(system__timearray& a, unsigned long long x, unsigned long long y);
    virtual void net__tcp__impl__handle_fail(int s);
    virtual node__iter__t ext__node__iter__end();
    virtual msg ext__system__server__build_decide_msg(unsigned long long i);
    virtual void ext__system__server__decide(unsigned long long i, __strlit v);
    virtual void ext__system__ballot_status_array__resize(system__ballot_status_array& a, unsigned long long s, const system__ballot_status& v);
    virtual void ext__protocol__join_round(const node& n, unsigned long long r);
    virtual void ext__system__server__catch_up__notify_two_a(unsigned long long i);
    virtual system__decision_struct ext__system__server__query(unsigned long long i);
    virtual unsigned long long ext__system__log__size(const system__log& a);
    virtual void ext__shim__two_a_handler__handle(const msg& m);
    virtual void ext__shim__two_b_handler__handle(const msg& m);
    virtual system__timearray ext__system__timearray__empty();
    virtual bool ext__net__tcp__send(int s, const msg& p);
    virtual unsigned long long ext__round__next(unsigned long long x);
    virtual nset ext__nset__emptyset();
    virtual void ext__protocol__propose(unsigned long long r, unsigned long long i, __strlit v);
    virtual void ext__system__server__ask_for_retransmission(unsigned long long i);
    virtual int ext__net__tcp__connect(const node& other);
    virtual void ext__shim__decide_handler__handle(const msg& m);
    virtual void ext__votemap_seg__resize(votemap_seg& seg, unsigned long long x, const vote_struct& y);
    virtual void ext__system__log__set(system__log& a, unsigned long long x, const system__decision_struct& y);
    virtual void ext__net__tcp__recv(int s, const msg& p);
    virtual votemap ext__votemap__empty();
    virtual void ext__shim__one_b_handler__handle(const msg& m);
    virtual void ext__votemap__set(votemap& a, unsigned long long x, const vote_struct& y);
    virtual msg ext__system__server__build_proposal(unsigned long long j, __strlit v);
    virtual bool ext__system__server__propose(__strlit v);
    virtual void ext__net__send(const node& dst, const msg& v);
    virtual void ext__system__server__two_a_retransmitter__tick();
    virtual msg ext__system__server__build_vote_msg(unsigned long long i, __strlit v);
    virtual node ext__node__next(const node& x);
    virtual system__decision_struct ext__system__log__get(const system__log& a, unsigned long long x);
    virtual void ext__system__log__resize(system__log& a, unsigned long long s, const system__decision_struct& v);
    virtual node ext__system__server__current_leader__get();
    virtual void ext__net__tcp__close(int s);
    virtual bool ext__system__server__leader_election__is_leader_too_quiet();
    virtual void imp__system__server__decide(unsigned long long i, __strlit v);
    virtual void imp__shim__debug_receiving(const msg& m);
    virtual nset__arr ext__nset__arr__empty();
    virtual void ext__nset__arr__append(nset__arr& a, const node& v);
    virtual void ext__shim__debug_receiving(const msg& m);
    virtual void ext__shim__missing_decision_handler__handle(const msg& m);
    virtual bool ext__node__is_max(const node& x);
    virtual void ext__system__server__two_a_retransmitter__notify_two_a(unsigned long long i);
    virtual votemap_seg ext__votemap_seg__make(const votemap& data, unsigned long long begin, unsigned long long end);
    virtual node__iter__t ext__node__iter__create(const node& x);
    virtual void ext__net__tcp__failed(int s);
    virtual void system__server__timer__sec__impl__handle_timeout();
    virtual void ext__shim__unicast(const node& dst, const msg& m);
    virtual void ext__net__recv(const msg& v);
    virtual unsigned long long ext__system__time__next(unsigned long long x);
    virtual void ext__votemap__append(votemap& a, const vote_struct& v);
    virtual void __init();
    virtual votemap_seg ext__votemap_seg_ops__zip_max(const votemap_seg& seg1, const votemap_seg& seg2);
    virtual void ext__votemap__resize(votemap& a, unsigned long long s, const vote_struct& v);
    virtual void ext__shim__bcast(const msg& m);
    virtual unsigned long long ext__system__server__next_self_leader_round(unsigned long long r);
    virtual bool ext__system__server__leader_election__start_round_timed_out();
    virtual void ext__shim__keep_alive_handler__handle(const msg& m);
    virtual void ext__net__tcp__accept(int s, const node& other);
    virtual void ext__votemap_seg__set(votemap_seg& seg, unsigned long long x, const vote_struct& y);
    virtual void ext__shim__debug_sending(const node& dst, const msg& m);
    virtual void ext__system__server__current_leader__set(const node& v);
    virtual void ext__system__server__catch_up__ask_missing(unsigned long long j);
    virtual void ext__shim__one_a_handler__handle(const msg& m);
    virtual void ext__shim__missing_two_a_handler__handle(const msg& m);
    virtual system__ballot_status ext__system__ballot_status_array__get(const system__ballot_status_array& a, unsigned long long x);
    virtual void ext__system__server__timer__sec__timeout();
    virtual void ext__system__server__vote(const node& leader, unsigned long long i, __strlit v);
    virtual void ext__protocol__receive_join_acks(unsigned long long r, const nset& q, const votemap_seg& m);
    virtual void ext__system__timearray__resize(system__timearray& a, unsigned long long s, unsigned long long v);
    virtual void imp__shim__debug_sending(const node& dst, const msg& m);
    virtual void ext__system__server__leader_election__tick();
    virtual void net__tcp__impl__handle_accept(int s, const node& other);
    virtual void ext__protocol__cast_vote(const node& n, unsigned long long i, unsigned long long r, __strlit v);
    virtual void net__tcp__impl__handle_recv(int s, const msg& x);
    virtual void net__tcp__impl__handle_connected(int s);
    virtual void ext__system__ballot_status_array__set(system__ballot_status_array& a, unsigned long long x, const system__ballot_status& y);
    virtual void ext__system__server__leader_election__notify_join_round(unsigned long long r);
    virtual void ext__protocol__decide(const node& n, unsigned long long i, unsigned long long r, __strlit v, const nset& q);
    virtual void ext__system__server__update_first_undecided(unsigned long long i);
    virtual system__log ext__system__log__empty();
    virtual void ext__system__server__change_round(unsigned long long r);
    virtual votemap_seg ext__votemap_seg__empty();
    virtual void ext__net__tcp__connected(int s);
    void __tick(int timeout);
};
inline bool operator ==(const multi_paxos_system::node__iter__t &s, const multi_paxos_system::node__iter__t &t){
    return ((s.is_end == t.is_end) && (s.val == t.val));
}
inline bool operator ==(const multi_paxos_system::nset &s, const multi_paxos_system::nset &t){
    return ((s.repr == t.repr));
}
inline bool operator ==(const multi_paxos_system::vote_struct &s, const multi_paxos_system::vote_struct &t){
    return ((s.maxr == t.maxr) && (s.maxv == t.maxv));
}
inline bool operator ==(const multi_paxos_system::votemap_seg &s, const multi_paxos_system::votemap_seg &t){
    return ((s.offset == t.offset) && (s.elems == t.elems));
}
inline bool operator ==(const multi_paxos_system::msg &s, const multi_paxos_system::msg &t){
    return ((s.m_kind == t.m_kind) && (s.m_round == t.m_round) && (s.m_inst == t.m_inst) && (s.m_node == t.m_node) && (s.m_value == t.m_value) && (s.m_votemap == t.m_votemap));
}
inline bool operator ==(const multi_paxos_system::system__ballot_status &s, const multi_paxos_system::system__ballot_status &t){
    return ((s.active == t.active) && (s.voters == t.voters) && (s.proposal == t.proposal) && (s.decided == t.decided));
}
inline bool operator ==(const multi_paxos_system::system__decision_struct &s, const multi_paxos_system::system__decision_struct &t){
    return ((s.decision == t.decision) && (s.present == t.present));
}
