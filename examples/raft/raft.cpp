#include "raft.h"

#include <sstream>
#include <algorithm>

#include <iostream>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#include <io.h>
#define isatty _isatty
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <sys/select.h>
#include <unistd.h>
#define _open open
#define _dup2 dup2
#endif
#include <string.h>
#include <stdio.h>
#include <string>
#if __cplusplus < 201103L
#else
#include <cstdint>
#endif
typedef raft ivy_class;
std::ofstream __ivy_out;
std::ofstream __ivy_modelfile;
void __ivy_exit(int code){exit(code);}

class reader {
public:
    virtual int fdes() = 0;
    virtual void read() = 0;
    virtual void bind() {}
    virtual bool running() {return fdes() >= 0;}
    virtual ~reader() {}
};

class timer {
public:
    virtual int ms_delay() = 0;
    virtual void timeout(int) = 0;
    virtual ~timer() {}
};

#ifdef _WIN32
DWORD WINAPI ReaderThreadFunction( LPVOID lpParam ) 
{
    reader *cr = (reader *) lpParam;
    cr->bind();
    while (true)
        cr->read();
    return 0;
} 

DWORD WINAPI TimerThreadFunction( LPVOID lpParam ) 
{
    timer *cr = (timer *) lpParam;
    while (true) {
        int ms = cr->ms_delay();
        Sleep(ms);
        cr->timeout(ms);
    }
    return 0;
} 
#else
void * _thread_reader(void *rdr_void) {
    reader *rdr = (reader *) rdr_void;
    rdr->bind();
    while(rdr->running()) {
        rdr->read();
    }
    delete rdr;
    return 0; // just to stop warning
}

void * _thread_timer( void *tmr_void ) 
{
    timer *tmr = (timer *) tmr_void;
    while (true) {
        int ms = tmr->ms_delay();
        struct timespec ts;
        ts.tv_sec = ms/1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        nanosleep(&ts,NULL);
        tmr->timeout(ms);
    }
    return 0;
} 
#endif 

void raft::install_reader(reader *r) {
    #ifdef _WIN32

        DWORD dummy;
        HANDLE h = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            ReaderThreadFunction,   // thread function name
            r,                      // argument to thread function 
            0,                      // use default creation flags 
            &dummy);                // returns the thread identifier 
        if (h == NULL) {
            std::cerr << "failed to create thread" << std::endl;
            exit(1);
        }
        thread_ids.push_back(h);
    #else
        pthread_t thread;
        int res = pthread_create(&thread, NULL, _thread_reader, r);
        if (res) {
            std::cerr << "failed to create thread" << std::endl;
            exit(1);
        }
        thread_ids.push_back(thread);
    #endif
}      

void raft::install_thread(reader *r) {
    install_reader(r);
}

void raft::install_timer(timer *r) {
    #ifdef _WIN32

        DWORD dummy;
        HANDLE h = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            TimersThreadFunction,   // thread function name
            r,                      // argument to thread function 
            0,                      // use default creation flags 
            &dummy);                // returns the thread identifier 
        if (h == NULL) {
            std::cerr << "failed to create thread" << std::endl;
            exit(1);
        }
        thread_ids.push_back(h);
    #else
        pthread_t thread;
        int res = pthread_create(&thread, NULL, _thread_timer, r);
        if (res) {
            std::cerr << "failed to create thread" << std::endl;
            exit(1);
        }
        thread_ids.push_back(thread);
    #endif
}      


#ifdef _WIN32
    void raft::__lock() { WaitForSingleObject(mutex,INFINITE); }
    void raft::__unlock() { ReleaseMutex(mutex); }
#else
    void raft::__lock() { pthread_mutex_lock(&mutex); }
    void raft::__unlock() { pthread_mutex_unlock(&mutex); }
#endif
struct thunk__net__tcp__impl__handle_accept{
    raft *__ivy;
    thunk__net__tcp__impl__handle_accept(raft *__ivy): __ivy(__ivy){}
    void operator()(int s, raft::node other) const {
        __ivy->net__tcp__impl__handle_accept(s,other);
    }
};
struct thunk__net__tcp__impl__handle_connected{
    raft *__ivy;
    thunk__net__tcp__impl__handle_connected(raft *__ivy): __ivy(__ivy){}
    void operator()(int s) const {
        __ivy->net__tcp__impl__handle_connected(s);
    }
};
struct thunk__net__tcp__impl__handle_fail{
    raft *__ivy;
    thunk__net__tcp__impl__handle_fail(raft *__ivy): __ivy(__ivy){}
    void operator()(int s) const {
        __ivy->net__tcp__impl__handle_fail(s);
    }
};
struct thunk__net__tcp__impl__handle_recv{
    raft *__ivy;
    thunk__net__tcp__impl__handle_recv(raft *__ivy): __ivy(__ivy){}
    void operator()(int s, raft::msg x) const {
        __ivy->net__tcp__impl__handle_recv(s,x);
    }
};
struct thunk__sec__impl__handle_timeout{
    raft *__ivy;
    thunk__sec__impl__handle_timeout(raft *__ivy): __ivy(__ivy){}
    void operator()() const {
        __ivy->sec__impl__handle_timeout();
    }
};

/*++
Copyright (c) Microsoft Corporation

This string hash function is borrowed from Microsoft Z3
(https://github.com/Z3Prover/z3). 

--*/


#define mix(a,b,c)              \
{                               \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8);  \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5);  \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

#ifndef __fallthrough
#define __fallthrough
#endif

namespace hash_space {

// I'm using Bob Jenkin's hash function.
// http://burtleburtle.net/bob/hash/doobs.html
unsigned string_hash(const char * str, unsigned length, unsigned init_value) {
    register unsigned a, b, c, len;

    /* Set up the internal state */
    len = length;
    a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
    c = init_value;      /* the previous hash value */

    /*---------------------------------------- handle most of the key */
    while (len >= 12) {
        a += reinterpret_cast<const unsigned *>(str)[0];
        b += reinterpret_cast<const unsigned *>(str)[1];
        c += reinterpret_cast<const unsigned *>(str)[2];
        mix(a,b,c);
        str += 12; len -= 12;
    }

    /*------------------------------------- handle the last 11 bytes */
    c += length;
    switch(len) {        /* all the case statements fall through */
    case 11: 
        c+=((unsigned)str[10]<<24);
        __fallthrough;
    case 10: 
        c+=((unsigned)str[9]<<16);
        __fallthrough;
    case 9 : 
        c+=((unsigned)str[8]<<8);
        __fallthrough;
        /* the first byte of c is reserved for the length */
    case 8 : 
        b+=((unsigned)str[7]<<24);
        __fallthrough;
    case 7 : 
        b+=((unsigned)str[6]<<16);
        __fallthrough;
    case 6 : 
        b+=((unsigned)str[5]<<8);
        __fallthrough;
    case 5 : 
        b+=str[4];
        __fallthrough;
    case 4 : 
        a+=((unsigned)str[3]<<24);
        __fallthrough;
    case 3 : 
        a+=((unsigned)str[2]<<16);
        __fallthrough;
    case 2 : 
        a+=((unsigned)str[1]<<8);
        __fallthrough;
    case 1 : 
        a+=str[0];
        __fallthrough;
        /* case 0: nothing left to add */
    }
    mix(a,b,c);
    /*-------------------------------------------- report the result */
    return c;
}

}




struct ivy_value {
    int pos;
    std::string atom;
    std::vector<ivy_value> fields;
    bool is_member() const {
        return atom.size() && fields.size();
    }
};
struct deser_err {
};

struct ivy_ser {
    virtual void  set(long long) = 0;
    virtual void  set(bool) = 0;
    virtual void  setn(long long inp, int len) = 0;
    virtual void  set(const std::string &) = 0;
    virtual void  open_list(int len) = 0;
    virtual void  close_list() = 0;
    virtual void  open_list_elem() = 0;
    virtual void  close_list_elem() = 0;
    virtual void  open_struct() = 0;
    virtual void  close_struct() = 0;
    virtual void  open_field(const std::string &) = 0;
    virtual void  close_field() = 0;
    virtual void  open_tag(int, const std::string &) {throw deser_err();}
    virtual void  close_tag() {}
    virtual ~ivy_ser(){}
};
struct ivy_binary_ser : public ivy_ser {
    std::vector<char> res;
    void setn(long long inp, int len) {
        for (int i = len-1; i >= 0 ; i--)
            res.push_back((inp>>(8*i))&0xff);
    }
    void set(long long inp) {
        setn(inp,sizeof(long long));
    }
    void set(bool inp) {
        set((long long)inp);
    }
    void set(const std::string &inp) {
        for (unsigned i = 0; i < inp.size(); i++)
            res.push_back(inp[i]);
        res.push_back(0);
    }
    void open_list(int len) {
        set((long long)len);
    }
    void close_list() {}
    void open_list_elem() {}
    void close_list_elem() {}
    void open_struct() {}
    void close_struct() {}
    virtual void  open_field(const std::string &) {}
    void close_field() {}
    virtual void  open_tag(int tag, const std::string &) {
        set((long long)tag);
    }
    virtual void  close_tag() {}
};

struct ivy_deser {
    virtual void  get(long long&) = 0;
    virtual void  get(std::string &) = 0;
    virtual void  getn(long long &res, int bytes) = 0;
    virtual void  open_list() = 0;
    virtual void  close_list() = 0;
    virtual bool  open_list_elem() = 0;
    virtual void  close_list_elem() = 0;
    virtual void  open_struct() = 0;
    virtual void  close_struct() = 0;
    virtual void  open_field(const std::string &) = 0;
    virtual void  close_field() = 0;
    virtual int   open_tag(const std::vector<std::string> &) {throw deser_err();}
    virtual void  close_tag() {}
    virtual void  end() = 0;
    virtual ~ivy_deser(){}
};

struct ivy_binary_deser : public ivy_deser {
    std::vector<char> inp;
    int pos;
    std::vector<int> lenstack;
    ivy_binary_deser(const std::vector<char> &inp) : inp(inp),pos(0) {}
    virtual bool more(unsigned bytes) {return inp.size() >= pos + bytes;}
    virtual bool can_end() {return pos == inp.size();}
    void get(long long &res) {
       getn(res,8);
    }
    void getn(long long &res, int bytes) {
        if (!more(bytes))
            throw deser_err();
        res = 0;
        for (int i = 0; i < bytes; i++)
            res = (res << 8) | (((long long)inp[pos++]) & 0xff);
    }
    void get(std::string &res) {
        while (more(1) && inp[pos]) {
//            if (inp[pos] == '"')
//                throw deser_err();
            res.push_back(inp[pos++]);
        }
        if(!(more(1) && inp[pos] == 0))
            throw deser_err();
        pos++;
    }
    void open_list() {
        long long len;
        get(len);
        lenstack.push_back(len);
    }
    void close_list() {
        lenstack.pop_back();
    }
    bool open_list_elem() {
        return lenstack.back();
    }
    void close_list_elem() {
        lenstack.back()--;
    }
    void open_struct() {}
    void close_struct() {}
    virtual void  open_field(const std::string &) {}
    void close_field() {}
    int open_tag(const std::vector<std::string> &tags) {
        long long res;
        get(res);
        if (res >= tags.size())
            throw deser_err();
        return res;
    }
    void end() {
        if (!can_end())
            throw deser_err();
    }
};
struct ivy_socket_deser : public ivy_binary_deser {
      int sock;
    public:
      ivy_socket_deser(int sock, const std::vector<char> &inp)
          : ivy_binary_deser(inp), sock(sock) {}
    virtual bool more(unsigned bytes) {
        while (inp.size() < pos + bytes) {
            int oldsize = inp.size();
            int get = pos + bytes - oldsize;
            get = (get < 1024) ? 1024 : get;
            inp.resize(oldsize + get);
            int newbytes;
	    if ((newbytes = read(sock,&inp[oldsize],get)) < 0)
		 { std::cerr << "recvfrom failed\n"; exit(1); }
            inp.resize(oldsize + newbytes);
            if (newbytes == 0)
                 return false;
        }
        return true;
    }
    virtual bool can_end() {return true;}
};

struct out_of_bounds {
    std::string txt;
    int pos;
    out_of_bounds(int _idx, int pos = 0) : pos(pos){
        std::ostringstream os;
        os << "argument " << _idx+1;
        txt = os.str();
    }
    out_of_bounds(const std::string &s, int pos = 0) : txt(s), pos(pos) {}
};

template <class T> T _arg(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <class T> T __lit(const char *);

template <>
bool _arg<bool>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
    if (!(args[idx].atom == "true" || args[idx].atom == "false") || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return args[idx].atom == "true";
}

template <>
int _arg<int>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
    std::istringstream s(args[idx].atom.c_str());
    s.unsetf(std::ios::dec);
    s.unsetf(std::ios::hex);
    s.unsetf(std::ios::oct);
    long long res;
    s  >> res;
    // int res = atoi(args[idx].atom.c_str());
    if (bound && (res < 0 || res >= bound) || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return res;
}

template <>
long long _arg<long long>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
    std::istringstream s(args[idx].atom.c_str());
    s.unsetf(std::ios::dec);
    s.unsetf(std::ios::hex);
    s.unsetf(std::ios::oct);
    long long res;
    s  >> res;
//    long long res = atoll(args[idx].atom.c_str());
    if (bound && (res < 0 || res >= bound) || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return res;
}

template <>
unsigned long long _arg<unsigned long long>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
    std::istringstream s(args[idx].atom.c_str());
    s.unsetf(std::ios::dec);
    s.unsetf(std::ios::hex);
    s.unsetf(std::ios::oct);
    unsigned long long res;
    s  >> res;
//    unsigned long long res = atoll(args[idx].atom.c_str());
    if (bound && (res < 0 || res >= bound) || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return res;
}

template <>
unsigned _arg<unsigned>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
    std::istringstream s(args[idx].atom.c_str());
    s.unsetf(std::ios::dec);
    s.unsetf(std::ios::hex);
    s.unsetf(std::ios::oct);
    unsigned res;
    s  >> res;
//    unsigned res = atoll(args[idx].atom.c_str());
    if (bound && (res < 0 || res >= bound) || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return res;
}


std::ostream &operator <<(std::ostream &s, const __strlit &t){
    s << "\"" << t.c_str() << "\"";
    return s;
}

template <>
__strlit _arg<__strlit>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
    if (args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return args[idx].atom;
}

template <class T> void __ser(ivy_ser &res, const T &inp);

template <>
void __ser<int>(ivy_ser &res, const int &inp) {
    res.set((long long)inp);
}

template <>
void __ser<long long>(ivy_ser &res, const long long &inp) {
    res.set(inp);
}

template <>
void __ser<unsigned long long>(ivy_ser &res, const unsigned long long &inp) {
    res.set((long long)inp);
}

template <>
void __ser<unsigned>(ivy_ser &res, const unsigned &inp) {
    res.set((long long)inp);
}

template <>
void __ser<bool>(ivy_ser &res, const bool &inp) {
    res.set(inp);
}

template <>
void __ser<__strlit>(ivy_ser &res, const __strlit &inp) {
    res.set(inp);
}

template <class T> void __deser(ivy_deser &inp, T &res);

template <>
void __deser<int>(ivy_deser &inp, int &res) {
    long long temp;
    inp.get(temp);
    res = temp;
}

template <>
void __deser<long long>(ivy_deser &inp, long long &res) {
    inp.get(res);
}

template <>
void __deser<unsigned long long>(ivy_deser &inp, unsigned long long &res) {
    long long temp;
    inp.get(temp);
    res = temp;
}

template <>
void __deser<unsigned>(ivy_deser &inp, unsigned &res) {
    long long temp;
    inp.get(temp);
    res = temp;
}

template <>
void __deser<__strlit>(ivy_deser &inp, __strlit &res) {
    inp.get(res);
}

template <>
void __deser<bool>(ivy_deser &inp, bool &res) {
    long long thing;
    inp.get(thing);
    res = thing;
}

class gen;

std::ostream &operator <<(std::ostream &s, const raft::msgkind &t);
template <>
raft::msgkind _arg<raft::msgkind>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::msgkind>(ivy_ser &res, const raft::msgkind&);
template <>
void  __deser<raft::msgkind>(ivy_deser &inp, raft::msgkind &res);
std::ostream &operator <<(std::ostream &s, const raft::commited_ix &t);
template <>
raft::commited_ix _arg<raft::commited_ix>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::commited_ix>(ivy_ser &res, const raft::commited_ix&);
template <>
void  __deser<raft::commited_ix>(ivy_deser &inp, raft::commited_ix &res);
std::ostream &operator <<(std::ostream &s, const raft::hist &t);
template <>
raft::hist _arg<raft::hist>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::hist>(ivy_ser &res, const raft::hist&);
template <>
void  __deser<raft::hist>(ivy_deser &inp, raft::hist &res);
std::ostream &operator <<(std::ostream &s, const raft::hist__ent &t);
template <>
raft::hist__ent _arg<raft::hist__ent>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::hist__ent>(ivy_ser &res, const raft::hist__ent&);
template <>
void  __deser<raft::hist__ent>(ivy_deser &inp, raft::hist__ent &res);
std::ostream &operator <<(std::ostream &s, const raft::msg &t);
template <>
raft::msg _arg<raft::msg>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::msg>(ivy_ser &res, const raft::msg&);
template <>
void  __deser<raft::msg>(ivy_deser &inp, raft::msg &res);
std::ostream &operator <<(std::ostream &s, const raft::node__iter__t &t);
template <>
raft::node__iter__t _arg<raft::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::node__iter__t>(ivy_ser &res, const raft::node__iter__t&);
template <>
void  __deser<raft::node__iter__t>(ivy_deser &inp, raft::node__iter__t &res);
std::ostream &operator <<(std::ostream &s, const raft::node_term &t);
template <>
raft::node_term _arg<raft::node_term>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::node_term>(ivy_ser &res, const raft::node_term&);
template <>
void  __deser<raft::node_term>(ivy_deser &inp, raft::node_term &res);
std::ostream &operator <<(std::ostream &s, const raft::nset &t);
template <>
raft::nset _arg<raft::nset>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<raft::nset>(ivy_ser &res, const raft::nset&);
template <>
void  __deser<raft::nset>(ivy_deser &inp, raft::nset &res);
	    std::ostream &operator <<(std::ostream &s, const raft::nset__arr &a) {
	        s << '[';
		for (unsigned i = 0; i < a.size(); i++) {
		    if (i != 0)
		        s << ',';
		    s << a[i];
		}
	        s << ']';
		return s;
            }

	    template <>
	    raft::nset__arr _arg<raft::nset__arr>(std::vector<ivy_value> &args, unsigned idx, int bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        raft::nset__arr a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<raft::node>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<raft::nset__arr>(ivy_deser &inp, raft::nset__arr &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<raft::nset__arr>(ivy_ser &res, const raft::nset__arr &inp) {
	        int sz = inp.size();
	        res.open_list(sz);
	        for (unsigned i = 0; i < (unsigned)sz; i++) {
		    res.open_list_elem();
	            __ser(res,inp[i]);
		    res.close_list_elem();
                }
	        res.close_list();
	    }

	    #ifdef Z3PP_H_
	    template <>
            z3::expr __to_solver(gen& g, const z3::expr& z3val, raft::nset__arr& val) {
	        z3::expr z3end = g.apply("nset.arr.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("nset.arr.value",z3val,g.int_to_z3(g.sort("nset.index"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<raft::nset__arr>( gen &g, const  z3::expr &v,raft::nset__arr &res){
	        int __end;
	        __from_solver(g,g.apply("nset.arr.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("nset.arr.value",v,g.int_to_z3(g.sort("nset.index"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<raft::nset__arr>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("nset.index"),__sz);
                z3::expr pred =  g.apply("nset.arr.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<raft::node>(g,g.apply("nset.arr.value",v,g.int_to_z3(g.sort("nset.index"),__i)));
	    }
	    #endif

	        template <typename T>
        T __array_segment(const T &a, long long lo, long long hi) {
            T res;
            lo = (lo < 0) ? 0 : lo;
            hi = (hi > a.size()) ? a.size() : hi;
            if (hi > lo) {
                res.resize(hi-lo);
                std::copy(a.begin()+lo,a.begin()+hi,res.begin());
            }
            return res;
        }
        	    std::ostream &operator <<(std::ostream &s, const raft::hist__arrlog &a) {
	        s << '[';
		for (unsigned i = 0; i < a.size(); i++) {
		    if (i != 0)
		        s << ',';
		    s << a[i];
		}
	        s << ']';
		return s;
            }

	    template <>
	    raft::hist__arrlog _arg<raft::hist__arrlog>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        raft::hist__arrlog a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<raft::hist__ent>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<raft::hist__arrlog>(ivy_deser &inp, raft::hist__arrlog &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<raft::hist__arrlog>(ivy_ser &res, const raft::hist__arrlog &inp) {
	        int sz = inp.size();
	        res.open_list(sz);
	        for (unsigned i = 0; i < (unsigned)sz; i++) {
		    res.open_list_elem();
	            __ser(res,inp[i]);
		    res.close_list_elem();
                }
	        res.close_list();
	    }

	    #ifdef Z3PP_H_
	    template <>
            z3::expr __to_solver(gen& g, const z3::expr& z3val, raft::hist__arrlog& val) {
	        z3::expr z3end = g.apply("hist.arrlog.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("hist.arrlog.value",z3val,g.int_to_z3(g.sort("index"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<raft::hist__arrlog>( gen &g, const  z3::expr &v,raft::hist__arrlog &res){
	        int __end;
	        __from_solver(g,g.apply("hist.arrlog.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("hist.arrlog.value",v,g.int_to_z3(g.sort("index"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<raft::hist__arrlog>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("index"),__sz);
                z3::expr pred =  g.apply("hist.arrlog.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<raft::hist__ent>(g,g.apply("hist.arrlog.value",v,g.int_to_z3(g.sort("index"),__i)));
	    }
	    #endif

	
   // Maximum number of sent packets to queue on a channel. Because TCP also
   // buffers, the total number of untransmitted backets that can back up will be greater
   // than this. This number *must* be at least one to void packet corruption.

   #define MAX_TCP_SEND_QUEUE 16

   struct tcp_mutex {
#ifdef _WIN32
       HANDLE mutex;
       tcp_mutex() { mutex = CreateMutex(NULL,FALSE,NULL); }
       void lock() { WaitForSingleObject(mutex,INFINITE); }
       void unlock() { ReleaseMutex(mutex); }
#else
       pthread_mutex_t mutex;
       tcp_mutex() { pthread_mutex_init(&mutex,NULL); }
       void lock() { pthread_mutex_lock(&mutex); }
       void unlock() { pthread_mutex_unlock(&mutex); }
#endif
   };

   struct tcp_sem {
       sem_t sem;
       tcp_sem() { sem_init(&sem,0,0); }
       void up() {sem_post(&sem); }
       void down() {sem_wait(&sem);}
   };

   class tcp_queue {
       tcp_mutex mutex; 
       tcp_sem sem;
       bool closed;
       bool reported_closed;
       std::list<std::vector<char> > bufs;
    public:
       int other; // only acces while holding lock!
       tcp_queue(int other) : closed(false), reported_closed(false), other(other) {}
       bool enqueue_swap(std::vector<char> &buf) {
           mutex.lock();
           if (closed) {
               mutex.unlock();
               return true;
           }
           if (bufs.size() < MAX_TCP_SEND_QUEUE) {
               bufs.push_back(std::vector<char>());
               buf.swap(bufs.back());
           }
           mutex.unlock();
           sem.up();
           return false;
       }
       bool dequeue_swap(std::vector<char> &buf) {
           while(true) {
               sem.down();
               // std::cout << "DEQUEUEING" << closed << std::endl;
               mutex.lock();
               if (closed) {
                   if (reported_closed) {
                       mutex.unlock();
                       continue;
                   }
                   reported_closed = true;
                   mutex.unlock();
                   // std::cout << "REPORTING CLOSED" << std::endl;
                   return true;
               }
               if (bufs.size() > 0) {
                   buf.swap(bufs.front());
                   bufs.erase(bufs.begin());
                   mutex.unlock();
                   return false;
               }
               mutex.unlock();
            }
       }
       void set_closed(bool report=true) {
           mutex.lock();
           closed = true;
           bufs.clear();
           if (!report)
               reported_closed = true;
           mutex.unlock();
           sem.up();
       }
       void set_open(int _other) {
           mutex.lock();
           closed = false;
           reported_closed = false;
           other = _other;
           mutex.unlock();
           sem.up();
       }
       void wait_open(bool closed_val = false){
           while (true) {
               mutex.lock();
               if (closed == closed_val) {
                   mutex.unlock();
                   return;
               }
               mutex.unlock();
               sem.down();
            }
       }

   };

   // The default configuration gives address 127.0.0.1 and port port_base + id.

    void tcp_config::get(int id, unsigned long &inetaddr, unsigned long &inetport) {
#ifdef _WIN32
            inetaddr = ntohl(inet_addr("127.0.0.1")); // can't send to INADDR_ANY in windows
#else
            inetaddr = INADDR_ANY;
#endif
            inetport = 5990+ id;
    }

    // This reverses the default configuration's map. Note, this is a little dangerous
    // since an attacker could cause a bogus id to be returned. For the moment we have
    // no way to know the correct range of endpoint ids.

    int tcp_config::rev(unsigned long inetaddr, unsigned long inetport) {
        return inetport - 5990; // don't use this for real, it's vulnerable
    }

    // construct a sockaddr_in for a specified process id using the configuration

    void get_tcp_addr(ivy_class *ivy, int my_id, sockaddr_in &myaddr) {
        memset((char *)&myaddr, 0, sizeof(myaddr));
        unsigned long inetaddr;
        unsigned long inetport;
        ivy->get_tcp_config() -> get(my_id,inetaddr,inetport);
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(inetaddr);
        myaddr.sin_port = htons(inetport);
    }

    // get the process id of a sockaddr_in using the configuration in reverse

    int get_tcp_id(ivy_class *ivy, const sockaddr_in &myaddr) {
       return ivy->get_tcp_config() -> rev(ntohl(myaddr.sin_addr.s_addr), ntohs(myaddr.sin_port));
    }

    // get a new TCP socket

    int make_tcp_socket() {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            { std::cerr << "cannot create socket\n"; exit(1); }
        int one = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0) 
            { perror("setsockopt failed"); exit(1); }
        return sock;
    }
    

    // This structure holds all the callbacks for the endpoint. These are function objects
    // that are called asynchronously.

    struct tcp_callbacks {
        thunk__net__tcp__impl__handle_accept acb;
        thunk__net__tcp__impl__handle_recv rcb;
        thunk__net__tcp__impl__handle_fail fcb;
        thunk__net__tcp__impl__handle_connected ccb;
        tcp_callbacks(const thunk__net__tcp__impl__handle_accept &acb,
                      const thunk__net__tcp__impl__handle_recv &rcb,
                      const thunk__net__tcp__impl__handle_fail &fcb,
                      const thunk__net__tcp__impl__handle_connected ccb)
            : acb(acb), rcb(rcb), fcb(fcb), ccb(ccb) {}
    };

    // This is a general class for an asynchronous task. These objects are called in a loop
    // by a thread allocated by the runtime. The fdes method returns a file descriptor
    // associated with the object. If fdes returns a negative value, the thread deletes the
    // object and terminates.

    class tcp_task : public reader {
      protected:
        int sock;           // socket associated to this task, or -1 if task complete
        int my_id;          // endpoint id associated to this task
        tcp_callbacks cb;   // callbacks to ivy
        ivy_class *ivy;     // pointer to main ivy object (mainly to get lock)

      public:

        tcp_task(int my_id, int sock, const tcp_callbacks &cb, ivy_class *ivy)
          : my_id(my_id), sock(sock), cb(cb), ivy(ivy) {} 

        virtual int fdes() {
            return sock;
        }


    };


    // This task reads messages from a socket and calls the "recv" callback.

    class tcp_reader : public tcp_task {
        std::vector<char> buf;
      public:
        tcp_reader(int my_id, int sock, const tcp_callbacks &cb, ivy_class *ivy)
            : tcp_task(my_id, sock, cb, ivy) {
        }

        // This is called in a loop by the task thread.

        virtual void read() {
//            std::cout << "RECEIVING\n";

            raft::msg pkt;                      // holds received message
            ivy_socket_deser ds(sock,buf);  // initializer deserialize with any leftover bytes
            buf.clear();                    // clear the leftover bytes

            try {
                __deser(ds,pkt);            // read the message
            } 

            // If packet has bad syntax, we drop it, close the socket, call the "failed"
            // callback and terminate the task.

            catch (deser_err &){
                if (ds.pos > 0)
                    std::cout << "BAD PACKET RECEIVED\n";
                else
                    std::cout << "EOF ON SOCKET\n";
                cb.fcb(sock);
                close(sock);
                sock = -1;
                return;
            }

            // copy the leftover bytes to buf

            buf.resize(ds.inp.size()-ds.pos);
            std::copy(ds.inp.begin()+ds.pos,ds.inp.end(),buf.begin());

            // call the "recv" callback with the received message

            ivy->__lock();
            cb.rcb(sock,pkt);
            ivy->__unlock();
        }
    };


    // This class writes queued packets to a socket. Packets can be added
    // asynchronously to the tail of the queue. If the socket is closed,
    // the queue will be emptied asynchrnonously. When the queue is empty the writer deletes
    // the queue and exits.

    // invariant: if socket is closed, queue is closed

    class tcp_writer : public tcp_task {
        tcp_queue *queue;
        bool connected;
      public:
        tcp_writer(int my_id, int sock, tcp_queue *queue, const tcp_callbacks &cb, ivy_class *ivy)
            : tcp_task(my_id,sock,cb,ivy), queue(queue), connected(false) {
        }

        virtual int fdes() {
            return sock;
        }

        // This is called in a loop by the task thread.

        virtual void read() {

            if (!connected) {
            
                // if the socket is not connected, wait for the queue to be open,
                // then connect

                queue->wait_open();
                connect();
                return;
            }

            // dequeue a packet to send

            std::vector<char> buf;
            bool qclosed = queue->dequeue_swap(buf);        

            // if queue has been closed asynchrononously, close the socket. 

            if (qclosed) {
                // std::cout << "CLOSING " << sock << std::endl;
                ::close(sock);
                connected = false;
                return;
            }

            // try a blocking send

            int bytes = send(sock,&buf[0],buf.size(),MSG_NOSIGNAL);
        
            // std::cout << "SENT\n";

            // if not all bytes sent, channel has failed, close the queue

            if (bytes < (int)buf.size())
                fail_close();
        }

        void connect() {

            // Get the address of the other from the configuration

            // std::cout << "ENTERING CONNECT " << sock << std::endl;

            ivy -> __lock();               // can be asynchronous, so must lock ivy!
            struct sockaddr_in myaddr;
            int other = queue->other;
            get_tcp_addr(ivy,other,myaddr);
            ivy -> __unlock(); 

            // Call connect to make connection

            // std::cout << "CONNECTING sock=" << sock << "other=" << other << std::endl;

            int res = ::connect(sock,(sockaddr *)&myaddr,sizeof(myaddr));

            // If successful, call the "connected" callback, else "failed"
            
            ivy->__lock();
            if (res >= 0) {
                // std::cout << "CONNECT SUCCEEDED " << sock << std::endl;
                cb.ccb(sock);
                connected = true;
            }
            else {
                // std::cout << "CONNECT FAILED " << sock << std::endl;
                fail_close();
            }
            ivy->__unlock();

        }

        void fail_close() {
            queue -> set_closed(false);  // close queue synchronously

            // make sure socket is closed before fail callback, since this
            // might open another socket, and we don't want to build up
            // zombie sockets.

            // std::cout << "CLOSING ON FAILURE " << sock << std::endl;
            ::close(sock);
            cb.fcb(sock);
            connected = false;
        }

    };

    // This task listens for connections on a socket in the background. 

    class tcp_listener : public tcp_task {
      public:

        // The constructor creates a socket to listen on.

        tcp_listener(int my_id, const tcp_callbacks &cb, ivy_class *ivy)
            : tcp_task(my_id,0,cb,ivy) {
            sock = make_tcp_socket();
        }

        // The bind method is called by the runtime once, after initialization.
        // This allows us to query the configuration for our address and bind the socket.

        virtual void bind() {
            ivy -> __lock();  // can be asynchronous, so must lock ivy!

            // Get our endpoint address from the configuration
            struct sockaddr_in myaddr;
            get_tcp_addr(ivy,my_id,myaddr);

                    std::cout << "binding id: " << my_id << " port: " << ntohs(myaddr.sin_port) << std::endl;

            // Bind the socket to our address
            if (::bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
                { perror("bind failed"); exit(1); }

            // Start lisetning on the socket
            if (listen(sock,2) < 0) 
                { std::cerr << "cannot listen on socket\n"; exit(1); }

            ivy -> __unlock();
        }

        // After binding, the thread calls read in a loop. In this case, we don't read,
        // we try accepting a connection. BUG: We should first call select to wait for a connection
        // to be available, then call accept while holding the ivy lock. This is needed to
        // guarantee the "accepted" appears to occur before "connected" which is required by
        // the the tcp interface specification.

        virtual void read() {
            // std::cout << "ACCEPTING\n";

            // Call accept to get an incoming connection request. May block.
            sockaddr_in other_addr;
            socklen_t addrlen = sizeof(other_addr);    
            int new_sock = accept(sock, (sockaddr *)&other_addr, &addrlen);

            // If this fails, something is very wrong: fail stop.
            if (new_sock < 0)
                { perror("accept failed"); exit(1); }

            // Get the endpoint id of the other from its address.
            int other = get_tcp_id(ivy,other_addr);

            // Run the "accept" callback. Since it's async, we must lock.
            ivy->__lock();
            cb.acb(new_sock,other);
            ivy->__unlock();

            // Install a reader task to read messages from the new socket.
            ivy->install_reader(new tcp_reader(my_id,new_sock,cb,ivy));
        }
    };



	    std::ostream &operator <<(std::ostream &s, const raft::replierslog &a) {
	        s << '[';
		for (unsigned i = 0; i < a.size(); i++) {
		    if (i != 0)
		        s << ',';
		    s << a[i];
		}
	        s << ']';
		return s;
            }

	    template <>
	    raft::replierslog _arg<raft::replierslog>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        raft::replierslog a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<raft::nset>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<raft::replierslog>(ivy_deser &inp, raft::replierslog &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<raft::replierslog>(ivy_ser &res, const raft::replierslog &inp) {
	        int sz = inp.size();
	        res.open_list(sz);
	        for (unsigned i = 0; i < (unsigned)sz; i++) {
		    res.open_list_elem();
	            __ser(res,inp[i]);
		    res.close_list_elem();
                }
	        res.close_list();
	    }

	    #ifdef Z3PP_H_
	    template <>
            z3::expr __to_solver(gen& g, const z3::expr& z3val, raft::replierslog& val) {
	        z3::expr z3end = g.apply("replierslog.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("replierslog.value",z3val,g.int_to_z3(g.sort("index"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<raft::replierslog>( gen &g, const  z3::expr &v,raft::replierslog &res){
	        int __end;
	        __from_solver(g,g.apply("replierslog.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("replierslog.value",v,g.int_to_z3(g.sort("index"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<raft::replierslog>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("index"),__sz);
                z3::expr pred =  g.apply("replierslog.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<raft::nset>(g,g.apply("replierslog.value",v,g.int_to_z3(g.sort("index"),__i)));
	    }
	    #endif

		class sec_timer : public timer {
	    thunk__sec__impl__handle_timeout rcb;
            int ttl;
	    ivy_class *ivy;
	  public:
	    sec_timer(thunk__sec__impl__handle_timeout rcb, ivy_class *ivy)
	        : rcb(rcb), ivy(ivy) {
                ttl = 1000;
	    }
	    virtual int ms_delay() {
		return ttl;
	    }
	    virtual void timeout(int elapse) {
                ttl -= elapse;
                if (ttl <= 0) {
                    ttl = 1000;
		    ivy->__lock();
		    rcb();
		    ivy->__unlock();
                }
	    }
	};
    int raft::___ivy_choose(int rng,const char *name,int id) {
        return 0;
    }
bool raft::nset__member(const node& N, const nset& S){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    int __tmp0;
    __tmp0 = 0;
    for (int I = 0; I < nset__arr__end(S.repr); I++) {
        if ((((0 < I) || (0 == I)) && (I < nset__arr__end(S.repr)) && nset__arr__value(S.repr,I,N))) __tmp0 = 1;
    }
    val = __tmp0;
    return val;
}
bool raft::nset__majority(const nset& S){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val = (nset__card[nset__all] < (nset__card[S] + nset__card[S]));
    return val;
}
bool raft::hist__logtix(const hist& H, int IX, int LOGT){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val = (hist__filled(H,IX) && (hist__arrlog__value(H.repr,IX).ent_logt == LOGT));
    return val;
}
bool raft::hist__filled(const hist& H, int IX){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val = (IX < hist__arrlog__end(H.repr));
    return val;
}
__strlit raft::hist__valix(const hist& H, int IX){
    __strlit val;
    val = hist__arrlog__value(H.repr,IX).ent_value;
    return val;
}
raft::node raft::__num0(){
    raft::node val;
    val =  0 ;
    return val;
}
bool raft::nset__arr__value(const nset__arr& a, int i, const node& v){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
    return val;
}
int raft::nset__arr__end(const nset__arr& a){
    int val;
    val = (int)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
raft::hist__ent raft::hist__arrlog__value(const hist__arrlog& a, int i){
    raft::hist__ent val;
    val.ent_logt = (int)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
int raft::hist__arrlog__end(const hist__arrlog& a){
    int val;
    val = (int)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
raft::nset raft::replierslog__value(const replierslog& a, int i){
    raft::nset val;
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
int raft::replierslog__end(const replierslog& a){
    int val;
    val = (int)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
raft::nset raft::ext__localstate__get_voters(){
    raft::nset voters;
    {
        voters = localstate__my_voters;
    }
    return voters;
}
void raft::ext__node__iter__next(node__iter__t& x){
    {
        {
            bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1556);
            {
                loc__0 = ext__node__is_max(x.val);
                if(loc__0){
                    x = ext__node__iter__end();
                }
                else {
                    {
                        node loc__0;
                        {
                            loc__0 = ext__node__next(x.val);
                            x = ext__node__iter__create(loc__0);
                        }
                    }
                }
            }
        }
    }
}
raft::node_term raft::ext__node_term__set(int t){
    raft::node_term nter;
    nter._term = (int)___ivy_choose(0,"fml:nter",0);
    {
        nter._term = t;
    }
    return nter;
}
void raft::imp__shim__recv_debug(const msg& m){
    {
    }
}
raft::hist raft::ext__hist__clear(){
    raft::hist h;
    {
        h.repr = ext__hist__arrlog__empty();
    }
    return h;
}
void raft::net__tcp__impl__handle_fail(int s){
    ext__net__tcp__failed(s);
}
raft::node__iter__t raft::ext__node__iter__end(){
    raft::node__iter__t y;
    y.is_end = (bool)___ivy_choose(0,"fml:y",0);
    {
        y.is_end = true;
        y.val = __num0();
    }
    return y;
}
void raft::ext__shim__handle_rqst_vote(int logt, int ix, int t, const node& cand){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1565);
            {
                loc__ok = true;
                if(hist__filled(localstate__myhist,0)){
                    {
                        int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1562);
                        int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1562);
                        {
                            loc__0 = ext__hist__get_next_ix(localstate__myhist);
                            loc__1 = ext__index__prev(loc__0);
                            {
                                int loc__lastix;
    loc__lastix = (int)___ivy_choose(0,"loc:lastix",1561);
                                {
                                    loc__lastix = loc__1;
                                    {
                                        int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1560);
                                        {
                                            loc__0 = ext__hist__get_logt(localstate__myhist, loc__lastix);
                                            {
                                                int loc__lastlogt;
    loc__lastlogt = (int)___ivy_choose(0,"loc:lastlogt",1559);
                                                {
                                                    loc__lastlogt = loc__0;
                                                    if(((logt < loc__lastlogt) || ((loc__lastlogt == logt) && (ix < loc__lastix)))){
                                                        {
                                                            loc__ok = false;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1564);
                    {
                        loc__0 = ext__node_term__get(localstate__nter);
                        {
                            int loc__node_t;
    loc__node_t = (int)___ivy_choose(0,"loc:node_t",1563);
                            {
                                loc__node_t = loc__0;
                                loc__ok = (loc__ok && (loc__node_t < t));
                                if(loc__ok){
                                    {
                                        ext__send_vote_cand_msg(n, t, cand);
                                        ext__localstate__move_to_term(t);
                                        ext__localstate__delay_leader_election();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
bool raft::ext__system__req_replicate_entry_from_log(int ix, bool isrecover, const node& recovernode){
    bool rejected;
    rejected = (bool)___ivy_choose(0,"fml:rejected",0);
    {
        {
            int loc__previx;
    loc__previx = (int)___ivy_choose(0,"loc:previx",1573);
            {
                {
                    int loc__prevt;
    loc__prevt = (int)___ivy_choose(0,"loc:prevt",1572);
                    {
                        {
                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1571);
                            {
                                loc__0 = ext__localstate__get_term();
                                {
                                    int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1570);
                                    {
                                        loc__t = loc__0;
                                        {
                                            __strlit loc__v;
                                            {
                                                loc__v = hist__valix(localstate__myhist,ix);
                                                {
                                                    int loc__logt;
    loc__logt = (int)___ivy_choose(0,"loc:logt",1568);
                                                    {
                                                        if((localstate__is_leader && hist__filled(localstate__myhist,ix))){
                                                            {
                                                                rejected = false;
                                                                loc__logt = ext__hist__get_logt(localstate__myhist, ix);
                                                                if(!(ix == 0)){
                                                                    {
                                                                        loc__previx = ext__index__prev(ix);
                                                                        loc__prevt = ext__hist__get_logt(localstate__myhist, loc__previx);
                                                                    }
                                                                }
                                                                {
                                                                    msg loc__0;
    loc__0.m_kind = (msgkind)___ivy_choose(0,"loc:0",1567);
    loc__0.m_ix = (int)___ivy_choose(0,"loc:0",1567);
    loc__0.m_term = (int)___ivy_choose(0,"loc:0",1567);
    loc__0.m_logt = (int)___ivy_choose(0,"loc:0",1567);
    loc__0.m_prevlogt = (int)___ivy_choose(0,"loc:0",1567);
    loc__0.m_isrecover = (bool)___ivy_choose(0,"loc:0",1567);
                                                                    {
                                                                        loc__0 = ext__msg_append_send(loc__t, loc__v, loc__logt, ix, loc__prevt, isrecover, recovernode);
                                                                        {
                                                                            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1566);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1566);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1566);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1566);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1566);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1566);
                                                                            {
                                                                                loc__m = loc__0;
                                                                                ext__safetyproof__remember_previx_ghost(loc__m, loc__previx);
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        else {
                                                            rejected = true;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return rejected;
}
void raft::ext__nset__add(nset& s, const node& n){
    {
        if(!nset__member(n,s)){
            ext__nset__arr__append(s.repr, n);
        }
    }
}
void raft::ext__shim__recv_debug(const msg& m){
    imp__shim__recv_debug(m);
}
bool raft::ext__localstate__is_leader_too_quiet(){
    bool res;
    res = (bool)___ivy_choose(0,"fml:res",0);
    {
        res = ((3 < (localstate__mytime - localstate__last_heard_from_leader)) || ((localstate__mytime - localstate__last_heard_from_leader) == 3));
    }
    return res;
}
int raft::ext__commited_ix__get_first_uncommited(const commited_ix& cix){
    int nextix;
    nextix = (int)___ivy_choose(0,"fml:nextix",0);
    {
        if(cix.non_empty){
            nextix = ext__index__next(cix.last_ix);
        }
        else {
            nextix = 0;
        }
    }
    return nextix;
}
void raft::ext__localstate__add_vote(const node& voter){
    {
        ext__nset__add(localstate__my_voters, voter);
    }
}
void raft::ext__safetyproof__update_term_hist_ghost(int t, const hist& h){
    {
    }
}
void raft::ext__localstate___initialize_term(){
    {
        localstate__my_voters = ext__nset__emptyset();
        localstate__my_repliers = ext__replierslog__empty();
    }
}
void raft::ext__shim__send_debug(const msg& m){
    imp__shim__send_debug(m);
}
int raft::ext__hist__get_next_ix(const hist& h){
    int ix;
    ix = (int)___ivy_choose(0,"fml:ix",0);
    {
        ix = hist__arrlog__end(h.repr);
    }
    return ix;
}
int raft::ext__node_term__get(const node_term& nter){
    int t;
    t = (int)___ivy_choose(0,"fml:t",0);
    {
        t = nter._term;
    }
    return t;
}
void raft::ext__hist__strip(hist& h, int ix){
    {
        {
            hist__ent loc__dummy_ent;
    loc__dummy_ent.ent_logt = (int)___ivy_choose(0,"loc:dummy_ent",1575);
            {
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1574);
                    {
                        loc__0 = ext__index__next(ix);
                        ext__hist__arrlog__resize(h.repr, loc__0, loc__dummy_ent);
                    }
                }
            }
        }
    }
}
void raft::imp__system__commited_at_ix(int ix, __strlit v){
    {
    }
}
void raft::ext__localstate__become_leader(){
    {
        localstate__is_leader = true;
    }
}
void raft::ext__shim__handle_keepalive(int t){
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1576);
            {
                loc__0 = ext__localstate__get_term();
                if((loc__0 < t)){
                    {
                        ext__localstate__move_to_term(t);
                    }
                }
            }
        }
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1577);
            {
                loc__0 = ext__localstate__get_term();
                if((loc__0 == t)){
                    {
                        ext__localstate__delay_leader_election();
                    }
                }
            }
        }
    }
}
void raft::ext__safetyproof__set_valid_ghost(const hist& h){
    {
    }
}
void raft::ext__system__announce_candidacy(){
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1578);
            int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1578);
            int loc__2;
    loc__2 = (int)___ivy_choose(0,"loc:2",1578);
            int loc__3;
    loc__3 = (int)___ivy_choose(0,"loc:3",1578);
            {
                loc__0 = ext__typeconvert__from_nodeid_to_term(n);
                loc__1 = ext__node_term__get(localstate__nter);
                loc__2 = ext__term__add(loc__1, 1);
                loc__3 = ext__term__add(loc__2, loc__0);
                ext__localstate__move_to_term(loc__3);
            }
        }
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1584);
            {
                loc__0 = ext__localstate__get_term();
                {
                    int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1583);
                    {
                        loc__t = loc__0;
                        if(hist__filled(localstate__myhist,0)){
                            {
                                {
                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1582);
                                    int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1582);
                                    {
                                        loc__0 = ext__hist__get_next_ix(localstate__myhist);
                                        loc__1 = ext__index__prev(loc__0);
                                        {
                                            int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1581);
                                            {
                                                loc__ix = loc__1;
                                                {
                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1580);
                                                    {
                                                        loc__0 = ext__hist__get_logt(localstate__myhist, loc__ix);
                                                        {
                                                            int loc__logt;
    loc__logt = (int)___ivy_choose(0,"loc:logt",1579);
                                                            {
                                                                loc__logt = loc__0;
                                                                ext__send_rqst_vote_msg(n, loc__logt, loc__ix, loc__t);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            {
                                ext__send_rqst_vote_m_nolog(n, loc__t);
                            }
                        }
                        ext__send_vote_cand_msg(n, loc__t, n);
                        ext__localstate__add_vote(n);
                        ext__localstate__delay_leader_election();
                    }
                }
            }
        }
    }
}
raft::nset raft::ext__nset__emptyset(){
    raft::nset s;
    {
        s.repr = ext__nset__arr__empty();
    }
    return s;
}
int raft::ext__net__tcp__connect(const node& other){
    int s;
    s = (int)___ivy_choose(0,"fml:s",0);
    {
        s = make_tcp_socket();
        // std::cout << "SOCKET " << s << std::endl;

        // create a send queue for this socket, if needed, along with
        // its thread. if the queue exists, it must be closed, so
        // we open it.

        tcp_queue *queue;
        if (net__tcp__impl__send_queue.find(s) == net__tcp__impl__send_queue.end()) {
            net__tcp__impl__send_queue[s] = queue = new tcp_queue(other);
             install_thread(new tcp_writer(n,s,queue,*net__tcp__impl__cb,this));
        } else
            net__tcp__impl__send_queue[s] -> set_open(other);
    }
    return s;
}
void raft::ext__net__tcp__accept(int s, const node& other){
    {
    }
}
void raft::ext__shim__broadcast(const msg& m){
    {
        if(!(m.m_kind == keepalive)){
            {
                ext__shim__send_debug(m);
            }
        }
        {
            node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(0,"loc:0",1587);
            {
                loc__0 = ext__node__iter__create(__num0());
                {
                    node__iter__t loc__iter;
    loc__iter.is_end = (bool)___ivy_choose(0,"loc:iter",1586);
                    {
                        loc__iter = loc__0;
                        while(!loc__iter.is_end){
                            {
                                {
                                    node loc__dst;
                                    {
                                        loc__dst = loc__iter.val;
                                        ext__net__send(loc__dst, m);
                                        ext__node__iter__next(loc__iter);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void raft::ext__net__tcp__recv(int s, const msg& p){
    {
        ext__net__recv(p);
    }
}
__strlit raft::ext__system__get_value(int i){
    __strlit v;
    v = hist__valix(localstate__myhist,i);
    return v;
}
void raft::ext__send_appended_reply_msg(const node& leader, int t, const node& n, int ix, bool isrecover){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1644);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1644);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1644);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1644);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1644);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1644);
            {
                loc__m.m_kind = enappeneded;
                loc__m.m_term = t;
                loc__m.m_node = n;
                loc__m.m_ix = ix;
                loc__m.m_isrecover = isrecover;
                ext__net__send(leader, loc__m);
            }
        }
    }
}
void raft::ext__shim__handle_appended_reply_msg(int t, const node& replier, int ix, bool isrecover){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1601);
            {
                loc__ok = true;
                {
                    int loc__nextreportix;
    loc__nextreportix = (int)___ivy_choose(0,"loc:nextreportix",1600);
                    {
                        loc__nextreportix = ix;
                        loc__ok = (loc__ok && localstate__is_leader);
                        if((loc__ok && isrecover && hist__filled(localstate__myhist,0))){
                            {
                                int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1596);
                                int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1596);
                                {
                                    loc__0 = ext__hist__get_next_ix(localstate__myhist);
                                    loc__1 = ext__index__prev(loc__0);
                                    {
                                        int loc__lastusedix;
    loc__lastusedix = (int)___ivy_choose(0,"loc:lastusedix",1595);
                                        {
                                            loc__lastusedix = loc__1;
                                            if((ix < loc__lastusedix)){
                                                {
                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1594);
                                                    bool loc__1;
    loc__1 = (bool)___ivy_choose(0,"loc:1",1594);
                                                    {
                                                        loc__0 = ext__index__next(ix);
                                                        loc__1 = ext__system__req_replicate_entry_from_log(loc__0, isrecover, replier);
                                                        {
                                                            bool loc__rejected;
    loc__rejected = (bool)___ivy_choose(0,"loc:rejected",1593);
                                                            {
                                                                loc__rejected = loc__1;
                                                                ivy_assume(!loc__rejected, "raft.ivy: line 910");
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        {
                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1597);
                            {
                                loc__0 = ext__localstate__get_term();
                                loc__ok = (loc__ok && (loc__0 == t));
                            }
                        }
                        loc__ok = (loc__ok && hist__logtix(localstate__myhist,ix,t));
                        loc__nextreportix = ext__commited_ix__get_first_uncommited(localstate__commited);
                        loc__ok = (loc__ok && ((loc__nextreportix < ix) || (loc__nextreportix == ix)));
                        if(loc__ok){
                            {
                                ext__localstate__add_replier(ix, replier);
                            }
                        }
                        {
                            nset loc__0;
                            {
                                loc__0 = ext__localstate__get_repliers(ix);
                                {
                                    nset loc__repliers;
                                    {
                                        loc__repliers = loc__0;
                                        loc__ok = (loc__ok && nset__majority(loc__repliers));
                                        if(loc__ok){
                                            {
                                                ext__commited_ix__update_ghost_by(n, ix, t, loc__repliers);
                                                localstate__commited = ext__commited_ix__wrap(ix);
                                                ext__system__report_commits(loc__nextreportix, ix);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void raft::ext__safetyproof__remember_previx_ghost(const msg& m, int previx){
    {
    }
}
void raft::ext__shim__handle_rqst_vote_nolog(int t, const node& cand){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1604);
            {
                loc__ok = !hist__filled(localstate__myhist,0);
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1603);
                    {
                        loc__0 = ext__node_term__get(localstate__nter);
                        {
                            int loc__node_t;
    loc__node_t = (int)___ivy_choose(0,"loc:node_t",1602);
                            {
                                loc__node_t = loc__0;
                                loc__ok = (loc__ok && (loc__node_t < t));
                                if(loc__ok){
                                    {
                                        ext__send_vote_cand_msg(n, t, cand);
                                        ext__localstate__move_to_term(t);
                                        ext__localstate__delay_leader_election();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void raft::ext__net__send(const node& dst, const msg& v){
    {
        if(!net__proc__isup[dst]){
            if(!net__proc__pend[dst]){
                {
                    net__proc__sock[dst] = ext__net__tcp__connect(dst);
                    net__proc__pend[dst] = true;
                }
            }
        }
        else {
            {
                bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1606);
                {
                    loc__0 = ext__net__tcp__send(net__proc__sock[dst], v);
                    {
                        bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1605);
                        {
                            loc__ok = loc__0;
                            if(!loc__ok){
                                {
                                    ext__net__tcp__close(net__proc__sock[dst]);
                                    net__proc__sock[dst] = ext__net__tcp__connect(dst);
                                    net__proc__isup[dst] = false;
                                    net__proc__pend[dst] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
raft::node raft::ext__node__next(const node& x){
    raft::node y;
    {
        y = x + 1;
    }
    return y;
}
void raft::ext__system__report_commits(int firstix, int lastix){
    {
        int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1607);
        {
            loc__ix = firstix;
            while(((loc__ix < lastix) || (loc__ix == lastix))){
                {
                    ext__system__commited_at_ix(loc__ix, hist__valix(localstate__myhist,loc__ix));
                    loc__ix = ext__index__next(loc__ix);
                }
            }
        }
    }
}
raft::nset__arr raft::ext__nset__arr__empty(){
    raft::nset__arr a;
    {
        
    }
    return a;
}
void raft::imp__shim__send_debug(const msg& m){
    {
    }
}
void raft::ext__sec__timeout(){
    {
        if(!localstate__is_leader){
            {
                ext__localstate__increase_time();
                {
                    bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1608);
                    {
                        loc__0 = ext__localstate__is_leader_too_quiet();
                        if(loc__0){
                            ext__system__announce_candidacy();
                        }
                    }
                }
            }
        }
        else {
            {
                int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1609);
                {
                    loc__0 = ext__localstate__get_term();
                    ext__send_keepalive(loc__0);
                }
            }
        }
    }
}
raft::commited_ix raft::ext__commited_ix__wrap(int maxix){
    raft::commited_ix cix;
    cix.last_ix = (int)___ivy_choose(0,"fml:cix",0);
    cix.non_empty = (bool)___ivy_choose(0,"fml:cix",0);
    {
        cix.last_ix = maxix;
        cix.non_empty = true;
    }
    return cix;
}
int raft::ext__term__add(int x, int y){
    int z;
    z = (int)___ivy_choose(0,"fml:z",0);
    {
        z = (x + y);
    }
    return z;
}
void raft::ext__net__tcp__close(int s){
    {
        
        // We don't want to close a socket when there is another thread
        // waiting, because the other thread won't know what to do with the
        // error.
    
        // Instead we shut down the socket and let the other thread close it.
        // If there is a reader thread, it will see EOF and close the socket. If there is
        // on open writer thread, it will close the socket after we close the
        // send queue. If the queue is already closed, closing it has no effect.

        // invariant: if a socket is open there is a reader thread or
        // an open writer thread, but not both.

        // Because of this invariant, the socket will be closed exactly once.

        ::shutdown(s,SHUT_RDWR);

        if (net__tcp__impl__send_queue.find(s) != net__tcp__impl__send_queue.end())
            net__tcp__impl__send_queue[s] -> set_closed();
    }
}
void raft::ext__hist__append(hist& h, int ix, int logt, __strlit v){
    {
        {
            hist__ent loc__new_ent;
    loc__new_ent.ent_logt = (int)___ivy_choose(0,"loc:new_ent",1611);
            {
                loc__new_ent.ent_logt = logt;
                loc__new_ent.ent_value = v;
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1610);
                    {
                        loc__0 = ext__index__next(ix);
                        ext__hist__arrlog__resize(h.repr, loc__0, loc__new_ent);
                    }
                }
            }
        }
    }
}
int raft::ext__index__next(int x){
    int y;
    y = (int)___ivy_choose(0,"fml:y",0);
    {
        y = (x + 1);
    }
    return y;
}
void raft::ext__send_vote_cand_msg(const node& n, int t, const node& cand){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1612);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1612);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1612);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1612);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1612);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1612);
            {
                loc__m.m_kind = vtcandidate;
                loc__m.m_node = n;
                loc__m.m_term = t;
                loc__m.m_cand = cand;
                ext__net__send(cand, loc__m);
                ext__shim__send_debug(loc__m);
            }
        }
    }
}
void raft::ext__safetyproof__set_leader_ghost(int t, const node& leader, const nset& voters){
    {
    }
}
raft::nset raft::ext__replierslog__get(const replierslog& a, int x){
    raft::nset y;
    {

        if (0 <= x && x < (int)a.size())
            y = a[x];
    }
    return y;
}
void raft::ext__nset__arr__append(nset__arr& a, const node& v){
    {

        a.push_back(v);
    }
}
void raft::ext__net__tcp__connected(int s){
    {
        {
            node loc__other;
            int __tmp1;
            __tmp1 = 0;
            for (int X__0 = 0; X__0 < node__size; X__0++) {
                if((net__proc__pend[X__0] && (net__proc__sock[X__0] == s))){
                    loc__other = X__0;
                    __tmp1= 1;
                }
            }
            if(__tmp1){
                {
                    net__proc__pend[loc__other] = false;
                    net__proc__isup[loc__other] = true;
                }
            }
        }
    }
}
bool raft::ext__node__is_max(const node& x){
    bool r;
    r = (bool)___ivy_choose(0,"fml:r",0);
    {
        r = (x == node__size - 1);
    }
    return r;
}
raft::node__iter__t raft::ext__node__iter__create(const node& x){
    raft::node__iter__t y;
    y.is_end = (bool)___ivy_choose(0,"fml:y",0);
    {
        y.is_end = false;
        y.val = x;
    }
    return y;
}
void raft::ext__net__tcp__failed(int s){
    {
        {
            node loc__other;
            int __tmp2;
            __tmp2 = 0;
            for (int X__0 = 0; X__0 < node__size; X__0++) {
                if(((net__proc__isup[X__0] || net__proc__pend[X__0]) && (net__proc__sock[X__0] == s))){
                    loc__other = X__0;
                    __tmp2= 1;
                }
            }
            if(__tmp2){
                {
                    net__proc__isup[loc__other] = false;
                    net__proc__pend[loc__other] = false;
                }
            }
        }
    }
}
void raft::ext__net__recv(const msg& v){
    {
        if(!(v.m_kind == keepalive)){
            {
                ext__shim__recv_debug(v);
            }
        }
        if((v.m_kind == appentry)){
            {
                ext__shim__handle_append_entries(v);
            }
        }
        if((v.m_kind == rqvote)){
            {
                ext__shim__handle_rqst_vote(v.m_logt, v.m_ix, v.m_term, v.m_cand);
            }
        }
        if((v.m_kind == rqvotenolog)){
            {
                ext__shim__handle_rqst_vote_nolog(v.m_term, v.m_cand);
            }
        }
        if((v.m_kind == enappeneded)){
            {
                ext__shim__handle_appended_reply_msg(v.m_term, v.m_node, v.m_ix, v.m_isrecover);
            }
        }
        if((v.m_kind == vtcandidate)){
            {
                ext__shim__handle_vote_cand_msg(v.m_cand, v.m_term, v.m_node);
            }
        }
        if((v.m_kind == keepalive)){
            ext__shim__handle_keepalive(v.m_term);
        }
        if((v.m_kind == nack)){
            ext__shim__handle_nack(v.m_node, v.m_term, v.m_ix);
        }
    }
}
raft::nset raft::ext__localstate__get_repliers(int ix){
    raft::nset repliers;
    {
        if((ix < replierslog__end(localstate__my_repliers))){
            {
                repliers = ext__replierslog__get(localstate__my_repliers, ix);
            }
        }
        else {
            {
                repliers = ext__nset__emptyset();
            }
        }
    }
    return repliers;
}
raft::hist__arrlog raft::ext__hist__arrlog__empty(){
    raft::hist__arrlog a;
    {
        
    }
    return a;
}
raft::replierslog raft::ext__replierslog__empty(){
    raft::replierslog a;
    {
        
    }
    return a;
}
void raft::sec__impl__handle_timeout(){
    ext__sec__timeout();
}
int raft::ext__typeconvert__from_nodeid_to_term(const node& n){
    int t;
    t = (int)___ivy_choose(0,"fml:t",0);
    {

        t = n;
    }
    return t;
}
void raft::ext__send_keepalive(int t){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1613);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1613);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1613);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1613);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1613);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1613);
            {
                loc__m.m_kind = keepalive;
                loc__m.m_term = t;
                ext__shim__broadcast(loc__m);
            }
        }
    }
}
void raft::ext__replierslog__resize(replierslog& a, int s, const nset& v){
    {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
    }
}
int raft::ext__hist__get_logt(const hist& h, int ix){
    int logt;
    logt = (int)___ivy_choose(0,"fml:logt",0);
    {
        logt = hist__arrlog__value(h.repr,ix).ent_logt;
    }
    return logt;
}
void raft::ext__shim__handle_nack(const node& n, int t, int ix){
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1617);
            {
                loc__0 = ext__localstate__get_term();
                if(((loc__0 == t) && localstate__is_leader && hist__filled(localstate__myhist,ix))){
                    {
                        bool loc__isrecover;
    loc__isrecover = (bool)___ivy_choose(0,"loc:isrecover",1616);
                        {
                            loc__isrecover = true;
                            {
                                bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1615);
                                {
                                    loc__0 = ext__system__req_replicate_entry_from_log(ix, loc__isrecover, n);
                                    {
                                        bool loc__rejected;
    loc__rejected = (bool)___ivy_choose(0,"loc:rejected",1614);
                                        {
                                            loc__rejected = loc__0;
                                            ivy_assume(!loc__rejected, "raft.ivy: line 711");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void raft::ext__send_rqst_vote_m_nolog(const node& cand, int t){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1618);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1618);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1618);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1618);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1618);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1618);
            {
                loc__m.m_kind = rqvotenolog;
                loc__m.m_cand = cand;
                loc__m.m_term = t;
                ext__shim__broadcast(loc__m);
            }
        }
    }
}
int raft::ext__index__prev(int x){
    int y;
    y = (int)___ivy_choose(0,"fml:y",0);
    {
        y = (x - 1);
    }
    return y;
}
void raft::ext__localstate__delay_leader_election(){
    {
        localstate__last_heard_from_leader = localstate__mytime;
    }
}
void raft::ext__replierslog__set(replierslog& a, int x, const nset& y){
    {

        if (0 <= x && x < (int)a.size())
            a[x] = y;
    }
}
void raft::ext__localstate__increase_time(){
    {
        localstate__mytime = (localstate__mytime + 1);
    }
}
            struct __thunk__0 : thunk<raft::nset,int>{
                __thunk__0()  {
                }
                int operator()(const raft::nset &arg){
                    return 0;
                }
            };
                                    struct __thunk__1 : thunk<raft::nset,int>{
    hash_thunk<raft::nset,int> nset__card;
    raft::node__iter__t loc__i;
bool nset__arr__value(const raft::nset__arr& a, int i, const raft::node& v){
                                            bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
                                            val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
                                            return val;
}
bool nset__member(const raft::node& N, const raft::nset& S){
                                            bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
                                            int __tmp3;
                                            __tmp3 = 0;
                                            for (int I = 0; I < nset__arr__end(S.repr); I++) {
                                                if ((((0 < I) || (0 == I)) && (I < nset__arr__end(S.repr)) && nset__arr__value(S.repr,I,N))) __tmp3 = 1;
                                            }
                                            val = __tmp3;
                                            return val;
}
int nset__arr__end(const raft::nset__arr& a){
                                            int val;
    val = (int)___ivy_choose(0,"ret:val",0);
                                            val =  a.size() ;
                                            return val;
}
                                        __thunk__1(hash_thunk<raft::nset,int> nset__card,raft::node__iter__t loc__i) : nset__card(nset__card),loc__i(loc__i){
                                        }
                                        int operator()(const raft::nset &arg){
                                            return (nset__member(loc__i.val,arg) ? (nset__card[arg] + 1) : nset__card[arg]);
                                        }
                                    };
            struct __thunk__2 : thunk<raft::node,bool>{
                __thunk__2()  {
                }
                bool operator()(const raft::node &arg){
                    return false;
                }
            };
            struct __thunk__3 : thunk<raft::node,bool>{
                __thunk__3()  {
                }
                bool operator()(const raft::node &arg){
                    return false;
                }
            };
void raft::__init(){
    {
        {
            nset__card = hash_thunk<raft::nset,int>(new __thunk__0());
            nset__all.repr = ext__nset__arr__empty();
            {
                node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(0,"loc:0",1646);
                {
                    loc__0 = ext__node__iter__create(__num0());
                    {
                        node__iter__t loc__i;
    loc__i.is_end = (bool)___ivy_choose(0,"loc:i",1645);
                        {
                            loc__i = loc__0;
                            while(!loc__i.is_end){
                                {
                                    ext__nset__arr__append(nset__all.repr, loc__i.val);
                                    nset__card = hash_thunk<raft::nset,int>(new __thunk__1(nset__card,loc__i));
                                    ext__node__iter__next(loc__i);
                                }
                            }
                        }
                    }
                }
            }
        }
        {
            hist__empty = ext__hist__clear();
        }
        {
            net__proc__isup = hash_thunk<raft::node,bool>(new __thunk__2());
            net__proc__pend = hash_thunk<raft::node,bool>(new __thunk__3());
        }
        {
            localstate__myhist = hist__empty;
        }
        {
            localstate__nter = ext__node_term__set(0);
        }
        {
            localstate__is_leader = false;
        }
        {
            localstate__commited = ext__commited_ix__empty();
        }
        {
            localstate__my_voters = ext__nset__emptyset();
            localstate__my_repliers = ext__replierslog__empty();
            localstate__last_heard_from_leader = 0;
            localstate__mytime = 0;
        }
    }
}
int raft::ext__localstate__get_term(){
    int t;
    t = (int)___ivy_choose(0,"fml:t",0);
    {
        t = ext__node_term__get(localstate__nter);
    }
    return t;
}
void raft::ext__localstate__move_to_term(int t){
    {
        localstate__nter = ext__node_term__set(t);
        ext__localstate___initialize_term();
        localstate__is_leader = false;
    }
}
void raft::ext__send_rqst_vote_msg(const node& cand, int logt, int ix, int t){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1619);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1619);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1619);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1619);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1619);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1619);
            {
                loc__m.m_kind = rqvote;
                loc__m.m_cand = cand;
                loc__m.m_logt = logt;
                loc__m.m_ix = ix;
                loc__m.m_term = t;
                ext__shim__broadcast(loc__m);
            }
        }
    }
}
void raft::ext__system__commited_at_ix(int ix, __strlit v){
    imp__system__commited_at_ix(ix, v);
}
void raft::net__tcp__impl__handle_accept(int s, const node& other){
    ext__net__tcp__accept(s, other);
}
bool raft::ext__net__tcp__send(int s, const msg& p){
    bool ok;
    ok = (bool)___ivy_choose(0,"fml:ok",0);
    {
                        ivy_binary_ser sr;
                        __ser(sr,p);
        //                std::cout << "SENDING\n";
        
                        // if the send queue for this sock doesn's exist, it isn't open,
                        // so the client has vioalted the precondition. we do the bad client
                        // the service of not crashing.
        
                        if (net__tcp__impl__send_queue.find(s) == net__tcp__impl__send_queue.end())
                            ok = true;
        
                        else {
                            // get the send queue, and enqueue the packet, returning false if
                            // the queue is closed.
        
                            ok = !net__tcp__impl__send_queue[s]->enqueue_swap(sr.res);
                       }
    }
    return ok;
}
void raft::net__tcp__impl__handle_recv(int s, const msg& x){
    ext__net__tcp__recv(s, x);
}
void raft::ext__commited_ix__update_ghost_by(const node& n, int maxix, int maxt, const nset& q){
    {
    }
}
void raft::ext__hist__arrlog__resize(hist__arrlog& a, int s, const hist__ent& v){
    {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
    }
}
bool raft::ext__system__req_append_new_entry(__strlit v){
    bool rejected;
    rejected = (bool)___ivy_choose(0,"fml:rejected",0);
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1627);
            {
                loc__0 = ext__localstate__get_term();
                {
                    int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1626);
                    {
                        loc__t = loc__0;
                        {
                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1625);
                            {
                                loc__0 = ext__hist__get_next_ix(localstate__myhist);
                                {
                                    int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1624);
                                    {
                                        loc__ix = loc__0;
                                        if(localstate__is_leader){
                                            {
                                                ext__hist__append(localstate__myhist, loc__ix, loc__t, v);
                                                ext__safetyproof__update_term_hist_ghost(loc__t, localstate__myhist);
                                                ext__localstate__add_replier(loc__ix, n);
                                                {
                                                    bool loc__isrecover;
    loc__isrecover = (bool)___ivy_choose(0,"loc:isrecover",1623);
                                                    {
                                                        loc__isrecover = false;
                                                        {
                                                            node loc__dummy_recovernode;
                                                            {
                                                                rejected = ext__system__req_replicate_entry_from_log(loc__ix, loc__isrecover, loc__dummy_recovernode);
                                                                ivy_assume(!rejected, "raft.ivy: line 757");
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else {
                                            rejected = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return rejected;
}
raft::msg raft::ext__msg_append_send(int t, __strlit v, int logt, int ix, int prevlogt, bool isrecover, const node& recovernode){
    raft::msg m;
    m.m_kind = (msgkind)___ivy_choose(0,"fml:m",0);
    m.m_ix = (int)___ivy_choose(0,"fml:m",0);
    m.m_term = (int)___ivy_choose(0,"fml:m",0);
    m.m_logt = (int)___ivy_choose(0,"fml:m",0);
    m.m_prevlogt = (int)___ivy_choose(0,"fml:m",0);
    m.m_isrecover = (bool)___ivy_choose(0,"fml:m",0);
    {
        m.m_kind = appentry;
        m.m_term = t;
        m.m_val = v;
        m.m_logt = logt;
        m.m_node = n;
        m.m_ix = ix;
        m.m_prevlogt = prevlogt;
        m.m_isrecover = isrecover;
        if(!isrecover){
            {
                ext__shim__broadcast(m);
            }
        }
        else {
            {
                ext__net__send(recovernode, m);
                ext__shim__send_debug(m);
            }
        }
    }
    return m;
}
void raft::ext__send_nack(const node& leader, int t, int ix){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(0,"loc:m",1628);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1628);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1628);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1628);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1628);
    loc__m.m_isrecover = (bool)___ivy_choose(0,"loc:m",1628);
            {
                loc__m.m_kind = nack;
                loc__m.m_term = t;
                loc__m.m_node = n;
                loc__m.m_cand = leader;
                loc__m.m_ix = ix;
                ext__net__send(leader, loc__m);
                ext__shim__send_debug(loc__m);
            }
        }
    }
}
void raft::ext__localstate__add_replier(int ix, const node& replier){
    {
        if(((replierslog__end(localstate__my_repliers) < ix) || (replierslog__end(localstate__my_repliers) == ix))){
            {
                int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1629);
                nset loc__1;
                {
                    loc__0 = ext__index__next(ix);
                    loc__1 = ext__nset__emptyset();
                    ext__replierslog__resize(localstate__my_repliers, loc__0, loc__1);
                }
            }
        }
        {
            nset loc__ix_repliers;
            {
                loc__ix_repliers = replierslog__value(localstate__my_repliers,ix);
                ext__nset__add(loc__ix_repliers, replier);
                ext__replierslog__set(localstate__my_repliers, ix, loc__ix_repliers);
            }
        }
    }
}
raft::commited_ix raft::ext__commited_ix__empty(){
    raft::commited_ix cix;
    cix.last_ix = (int)___ivy_choose(0,"fml:cix",0);
    cix.non_empty = (bool)___ivy_choose(0,"fml:cix",0);
    {
        cix.non_empty = false;
    }
    return cix;
}
void raft::ext__shim__handle_append_entries(const msg& m){
    {
        {
            int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1643);
            {
                loc__t = m.m_term;
                {
                    int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1642);
                    {
                        loc__ix = m.m_ix;
                        {
                            int loc__logt;
    loc__logt = (int)___ivy_choose(0,"loc:logt",1641);
                            {
                                loc__logt = m.m_logt;
                                {
                                    __strlit loc__v;
                                    {
                                        loc__v = m.m_val;
                                        {
                                            int loc__prevt;
    loc__prevt = (int)___ivy_choose(0,"loc:prevt",1639);
                                            {
                                                loc__prevt = m.m_prevlogt;
                                                {
                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1638);
                                                    {
                                                        loc__0 = ext__localstate__get_term();
                                                        {
                                                            int loc__node_term;
    loc__node_term = (int)___ivy_choose(0,"loc:node_term",1637);
                                                            {
                                                                loc__node_term = loc__0;
                                                                {
                                                                    bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1636);
                                                                    {
                                                                        loc__ok = true;
                                                                        loc__ok = (loc__ok && !localstate__is_leader);
                                                                        loc__ok = (loc__ok && ((loc__node_term < loc__t) || (loc__node_term == loc__t)));
                                                                        if(loc__ok){
                                                                            {
                                                                                ext__localstate__move_to_term(loc__t);
                                                                                ext__localstate__delay_leader_election();
                                                                            }
                                                                        }
                                                                        if((!(loc__ix == 0) && loc__ok)){
                                                                            {
                                                                                {
                                                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1633);
                                                                                    {
                                                                                        loc__0 = ext__index__prev(loc__ix);
                                                                                        {
                                                                                            bool loc__has_previous;
    loc__has_previous = (bool)___ivy_choose(0,"loc:has_previous",1632);
                                                                                            {
                                                                                                loc__has_previous = hist__logtix(localstate__myhist,loc__0,loc__prevt);
                                                                                                if(!loc__has_previous){
                                                                                                    {
                                                                                                        {
                                                                                                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1631);
                                                                                                            {
                                                                                                                loc__0 = ext__index__prev(loc__ix);
                                                                                                                ext__send_nack(m.m_node, loc__t, loc__0);
                                                                                                            }
                                                                                                        }
                                                                                                        loc__ok = false;
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                        if(loc__ok){
                                                                            {
                                                                                if(!hist__logtix(localstate__myhist,loc__ix,loc__logt)){
                                                                                    {
                                                                                        if((loc__ix == 0)){
                                                                                            {
                                                                                                localstate__myhist = ext__hist__clear();
                                                                                            }
                                                                                        }
                                                                                        else {
                                                                                            {
                                                                                                {
                                                                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1635);
                                                                                                    {
                                                                                                        loc__0 = ext__index__prev(loc__ix);
                                                                                                        {
                                                                                                            int loc__previx;
    loc__previx = (int)___ivy_choose(0,"loc:previx",1634);
                                                                                                            {
                                                                                                                loc__previx = loc__0;
                                                                                                                ext__hist__strip(localstate__myhist, loc__previx);
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                        ext__hist__append(localstate__myhist, loc__ix, loc__logt, loc__v);
                                                                                        ext__safetyproof__set_valid_ghost(localstate__myhist);
                                                                                        ext__send_appended_reply_msg(m.m_node, loc__t, n, loc__ix, m.m_isrecover);
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void raft::net__tcp__impl__handle_connected(int s){
    ext__net__tcp__connected(s);
}
void raft::ext__shim__handle_vote_cand_msg(const node& cand, int t, const node& voter){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1592);
            {
                loc__ok = true;
                loc__ok = (loc__ok && (n == cand));
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1591);
                    {
                        loc__0 = ext__node_term__get(localstate__nter);
                        {
                            int loc__node_t;
    loc__node_t = (int)___ivy_choose(0,"loc:node_t",1590);
                            {
                                loc__node_t = loc__0;
                                loc__ok = (loc__ok && (loc__node_t == t));
                                loc__ok = (loc__ok && !localstate__is_leader);
                                if(loc__ok){
                                    {
                                        ext__localstate__add_vote(voter);
                                        {
                                            nset loc__0;
                                            {
                                                loc__0 = ext__localstate__get_voters();
                                                {
                                                    nset loc__voters;
                                                    {
                                                        loc__voters = loc__0;
                                                        if(nset__majority(loc__voters)){
                                                            {
                                                                ext__localstate__become_leader();
                                                                ext__safetyproof__set_leader_ghost(t, n, loc__voters);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void raft::__tick(int __timeout){
}
raft::raft(node node__size, node n){
#ifdef _WIN32
mutex = CreateMutex(NULL,FALSE,NULL);
#else
pthread_mutex_init(&mutex,NULL);
#endif
__lock();
    __CARD__index = 0;
    __CARD__term = 0;
    __CARD__value = 0;
    __CARD__nset__index = 0;
    __CARD__net__tcp__socket = 0;
    __CARD__localstate__time = 0;

    the_tcp_config = 0;

    // Create the callbacks. In a parameterized instance, this creates
    // one set of callbacks for each endpoint id. When you put an
    // action in anti-quotes it creates a function object (a "thunk")
    // that captures the instance environment, in this case including
    // the instance's endpoint id "me".

    net__tcp__impl__cb = new tcp_callbacks(thunk__net__tcp__impl__handle_accept(this),thunk__net__tcp__impl__handle_recv(this),thunk__net__tcp__impl__handle_fail(this),thunk__net__tcp__impl__handle_connected(this));

    // Install a listener task for this endpoint. If parameterized, this creates
    // one for each endpoint.

    install_reader(net__tcp__impl__rdr = new tcp_listener(n,*net__tcp__impl__cb,this));
    install_timer(sec__impl__tmr = new sec_timer(thunk__sec__impl__handle_timeout(this),this));
    this->node__size = node__size;
    localstate__mytime = (int)___ivy_choose(0,"init",0);
    localstate__nter._term = (int)___ivy_choose(0,"init",0);
    this->n = n;
    localstate__commited.last_ix = (int)___ivy_choose(0,"init",0);
    localstate__commited.non_empty = (bool)___ivy_choose(0,"init",0);
    localstate__is_leader = (bool)___ivy_choose(0,"init",0);
struct __thunk__4 : thunk<raft::node,bool>{
    __thunk__4()  {
    }
    bool operator()(const raft::node &arg){
        bool __tmp4;
    __tmp4 = (bool)___ivy_choose(0,"init",0);
        return __tmp4;
    }
};
net__proc__isup = hash_thunk<raft::node,bool>(new __thunk__4());
struct __thunk__5 : thunk<raft::nset,int>{
    __thunk__5()  {
    }
    int operator()(const raft::nset &arg){
        int __tmp5;
    __tmp5 = (int)___ivy_choose(0,"init",0);
        return __tmp5;
    }
};
nset__card = hash_thunk<raft::nset,int>(new __thunk__5());
    _generating = (bool)___ivy_choose(0,"init",0);
struct __thunk__6 : thunk<raft::node,bool>{
    __thunk__6()  {
    }
    bool operator()(const raft::node &arg){
        bool __tmp6;
    __tmp6 = (bool)___ivy_choose(0,"init",0);
        return __tmp6;
    }
};
net__proc__pend = hash_thunk<raft::node,bool>(new __thunk__6());
    localstate__last_heard_from_leader = (int)___ivy_choose(0,"init",0);
struct __thunk__7 : thunk<raft::node,int>{
    __thunk__7()  {
    }
    int operator()(const raft::node &arg){
        int __tmp7;
    __tmp7 = (int)___ivy_choose(0,"init",0);
        return __tmp7;
    }
};
net__proc__sock = hash_thunk<raft::node,int>(new __thunk__7());
}
raft::~raft(){
    __lock(); // otherwise, thread may die holding lock!
    for (unsigned i = 0; i < thread_ids.size(); i++){
#ifdef _WIN32
       // No idea how to cancel a thread on Windows. We just suspend it
       // so it can't cause any harm as we destruct this object.
       SuspendThread(thread_ids[i]);
#else
        pthread_cancel(thread_ids[i]);
        pthread_join(thread_ids[i],NULL);
#endif
    }
    __unlock();
}
std::ostream &operator <<(std::ostream &s, const raft::node__iter__t &t){
    s<<"{";
    s<< "is_end:";
    s << t.is_end;
    s<<",";
    s<< "val:";
    s << t.val;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::node__iter__t>(ivy_ser &res, const raft::node__iter__t&t){
    res.open_struct();
    res.open_field("is_end");
    __ser<bool>(res,t.is_end);
    res.close_field();
    res.open_field("val");
    __ser<raft::node>(res,t.val);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::nset &t){
    s<<"{";
    s<< "repr:";
    s << t.repr;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::nset>(ivy_ser &res, const raft::nset&t){
    res.open_struct();
    res.open_field("repr");
    __ser<raft::nset__arr>(res,t.repr);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::hist__ent &t){
    s<<"{";
    s<< "ent_logt:";
    s << t.ent_logt;
    s<<",";
    s<< "ent_value:";
    s << t.ent_value;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::hist__ent>(ivy_ser &res, const raft::hist__ent&t){
    res.open_struct();
    res.open_field("ent_logt");
    __ser<int>(res,t.ent_logt);
    res.close_field();
    res.open_field("ent_value");
    __ser<__strlit>(res,t.ent_value);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::hist &t){
    s<<"{";
    s<< "repr:";
    s << t.repr;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::hist>(ivy_ser &res, const raft::hist&t){
    res.open_struct();
    res.open_field("repr");
    __ser<raft::hist__arrlog>(res,t.repr);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::msg &t){
    s<<"{";
    s<< "m_kind:";
    s << t.m_kind;
    s<<",";
    s<< "m_ix:";
    s << t.m_ix;
    s<<",";
    s<< "m_term:";
    s << t.m_term;
    s<<",";
    s<< "m_node:";
    s << t.m_node;
    s<<",";
    s<< "m_cand:";
    s << t.m_cand;
    s<<",";
    s<< "m_val:";
    s << t.m_val;
    s<<",";
    s<< "m_logt:";
    s << t.m_logt;
    s<<",";
    s<< "m_prevlogt:";
    s << t.m_prevlogt;
    s<<",";
    s<< "m_isrecover:";
    s << t.m_isrecover;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::msg>(ivy_ser &res, const raft::msg&t){
    res.open_struct();
    res.open_field("m_kind");
    __ser<raft::msgkind>(res,t.m_kind);
    res.close_field();
    res.open_field("m_ix");
    __ser<int>(res,t.m_ix);
    res.close_field();
    res.open_field("m_term");
    __ser<int>(res,t.m_term);
    res.close_field();
    res.open_field("m_node");
    __ser<raft::node>(res,t.m_node);
    res.close_field();
    res.open_field("m_cand");
    __ser<raft::node>(res,t.m_cand);
    res.close_field();
    res.open_field("m_val");
    __ser<__strlit>(res,t.m_val);
    res.close_field();
    res.open_field("m_logt");
    __ser<int>(res,t.m_logt);
    res.close_field();
    res.open_field("m_prevlogt");
    __ser<int>(res,t.m_prevlogt);
    res.close_field();
    res.open_field("m_isrecover");
    __ser<bool>(res,t.m_isrecover);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::commited_ix &t){
    s<<"{";
    s<< "last_ix:";
    s << t.last_ix;
    s<<",";
    s<< "non_empty:";
    s << t.non_empty;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::commited_ix>(ivy_ser &res, const raft::commited_ix&t){
    res.open_struct();
    res.open_field("last_ix");
    __ser<int>(res,t.last_ix);
    res.close_field();
    res.open_field("non_empty");
    __ser<bool>(res,t.non_empty);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::node_term &t){
    s<<"{";
    s<< "_term:";
    s << t._term;
    s<<"}";
    return s;
}
template <>
void  __ser<raft::node_term>(ivy_ser &res, const raft::node_term&t){
    res.open_struct();
    res.open_field("_term");
    __ser<int>(res,t._term);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const raft::msgkind &t){
    if (t == raft::rqvote) s<<"rqvote";
    if (t == raft::rqvotenolog) s<<"rqvotenolog";
    if (t == raft::vtcandidate) s<<"vtcandidate";
    if (t == raft::appentry) s<<"appentry";
    if (t == raft::enappeneded) s<<"enappeneded";
    if (t == raft::keepalive) s<<"keepalive";
    if (t == raft::nack) s<<"nack";
    return s;
}
template <>
void  __ser<raft::msgkind>(ivy_ser &res, const raft::msgkind&t){
    __ser(res,(int)t);
}
template <>
raft::commited_ix _arg<raft::commited_ix>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::commited_ix res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 2) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "last_ix") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.last_ix = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field last_ix: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "non_empty") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.non_empty = _arg<bool>(tmp_args,0,2);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field non_empty: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::commited_ix>(ivy_deser &inp, raft::commited_ix &res){
    inp.open_struct();
    inp.open_field("last_ix");
    __deser(inp,res.last_ix);
    inp.close_field();
    inp.open_field("non_empty");
    __deser(inp,res.non_empty);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::hist _arg<raft::hist>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::hist res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 1) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "repr") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.repr = _arg<raft::hist__arrlog>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field repr: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::hist>(ivy_deser &inp, raft::hist &res){
    inp.open_struct();
    inp.open_field("repr");
    __deser(inp,res.repr);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::hist__ent _arg<raft::hist__ent>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::hist__ent res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 2) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "ent_logt") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.ent_logt = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field ent_logt: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "ent_value") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.ent_value = _arg<__strlit>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field ent_value: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::hist__ent>(ivy_deser &inp, raft::hist__ent &res){
    inp.open_struct();
    inp.open_field("ent_logt");
    __deser(inp,res.ent_logt);
    inp.close_field();
    inp.open_field("ent_value");
    __deser(inp,res.ent_value);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::msg _arg<raft::msg>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::msg res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 9) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "m_kind") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.m_kind = _arg<raft::msgkind>(tmp_args,0,7);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_kind: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "m_ix") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.m_ix = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_ix: " + err.txt,err.pos);
    }
    if (arg.fields[2].is_member()){
        tmp_args[0] = arg.fields[2].fields[0];
        if (arg.fields[2].atom != "m_term") throw out_of_bounds("unexpected field: " + arg.fields[2].atom,arg.fields[2].pos);
    }
    else{
        tmp_args[0] = arg.fields[2];
    }
    try{
        res.m_term = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_term: " + err.txt,err.pos);
    }
    if (arg.fields[3].is_member()){
        tmp_args[0] = arg.fields[3].fields[0];
        if (arg.fields[3].atom != "m_node") throw out_of_bounds("unexpected field: " + arg.fields[3].atom,arg.fields[3].pos);
    }
    else{
        tmp_args[0] = arg.fields[3];
    }
    try{
        res.m_node = _arg<raft::node>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_node: " + err.txt,err.pos);
    }
    if (arg.fields[4].is_member()){
        tmp_args[0] = arg.fields[4].fields[0];
        if (arg.fields[4].atom != "m_cand") throw out_of_bounds("unexpected field: " + arg.fields[4].atom,arg.fields[4].pos);
    }
    else{
        tmp_args[0] = arg.fields[4];
    }
    try{
        res.m_cand = _arg<raft::node>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_cand: " + err.txt,err.pos);
    }
    if (arg.fields[5].is_member()){
        tmp_args[0] = arg.fields[5].fields[0];
        if (arg.fields[5].atom != "m_val") throw out_of_bounds("unexpected field: " + arg.fields[5].atom,arg.fields[5].pos);
    }
    else{
        tmp_args[0] = arg.fields[5];
    }
    try{
        res.m_val = _arg<__strlit>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_val: " + err.txt,err.pos);
    }
    if (arg.fields[6].is_member()){
        tmp_args[0] = arg.fields[6].fields[0];
        if (arg.fields[6].atom != "m_logt") throw out_of_bounds("unexpected field: " + arg.fields[6].atom,arg.fields[6].pos);
    }
    else{
        tmp_args[0] = arg.fields[6];
    }
    try{
        res.m_logt = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_logt: " + err.txt,err.pos);
    }
    if (arg.fields[7].is_member()){
        tmp_args[0] = arg.fields[7].fields[0];
        if (arg.fields[7].atom != "m_prevlogt") throw out_of_bounds("unexpected field: " + arg.fields[7].atom,arg.fields[7].pos);
    }
    else{
        tmp_args[0] = arg.fields[7];
    }
    try{
        res.m_prevlogt = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_prevlogt: " + err.txt,err.pos);
    }
    if (arg.fields[8].is_member()){
        tmp_args[0] = arg.fields[8].fields[0];
        if (arg.fields[8].atom != "m_isrecover") throw out_of_bounds("unexpected field: " + arg.fields[8].atom,arg.fields[8].pos);
    }
    else{
        tmp_args[0] = arg.fields[8];
    }
    try{
        res.m_isrecover = _arg<bool>(tmp_args,0,2);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_isrecover: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::msg>(ivy_deser &inp, raft::msg &res){
    inp.open_struct();
    inp.open_field("m_kind");
    __deser(inp,res.m_kind);
    inp.close_field();
    inp.open_field("m_ix");
    __deser(inp,res.m_ix);
    inp.close_field();
    inp.open_field("m_term");
    __deser(inp,res.m_term);
    inp.close_field();
    inp.open_field("m_node");
    __deser(inp,res.m_node);
    inp.close_field();
    inp.open_field("m_cand");
    __deser(inp,res.m_cand);
    inp.close_field();
    inp.open_field("m_val");
    __deser(inp,res.m_val);
    inp.close_field();
    inp.open_field("m_logt");
    __deser(inp,res.m_logt);
    inp.close_field();
    inp.open_field("m_prevlogt");
    __deser(inp,res.m_prevlogt);
    inp.close_field();
    inp.open_field("m_isrecover");
    __deser(inp,res.m_isrecover);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::node__iter__t _arg<raft::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::node__iter__t res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 2) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "is_end") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.is_end = _arg<bool>(tmp_args,0,2);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field is_end: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "val") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.val = _arg<raft::node>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field val: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::node__iter__t>(ivy_deser &inp, raft::node__iter__t &res){
    inp.open_struct();
    inp.open_field("is_end");
    __deser(inp,res.is_end);
    inp.close_field();
    inp.open_field("val");
    __deser(inp,res.val);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::node_term _arg<raft::node_term>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::node_term res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 1) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "_term") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res._term = _arg<int>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field _term: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::node_term>(ivy_deser &inp, raft::node_term &res){
    inp.open_struct();
    inp.open_field("_term");
    __deser(inp,res._term);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::nset _arg<raft::nset>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    raft::nset res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 1) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "repr") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.repr = _arg<raft::nset__arr>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field repr: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<raft::nset>(ivy_deser &inp, raft::nset &res){
    inp.open_struct();
    inp.open_field("repr");
    __deser(inp,res.repr);
    inp.close_field();
    inp.close_struct();
}
template <>
raft::msgkind _arg<raft::msgkind>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    ivy_value &arg = args[idx];
    if (arg.atom.size() == 0 || arg.fields.size() != 0) throw out_of_bounds(idx,arg.pos);
    if(arg.atom == "rqvote") return raft::rqvote;
    if(arg.atom == "rqvotenolog") return raft::rqvotenolog;
    if(arg.atom == "vtcandidate") return raft::vtcandidate;
    if(arg.atom == "appentry") return raft::appentry;
    if(arg.atom == "enappeneded") return raft::enappeneded;
    if(arg.atom == "keepalive") return raft::keepalive;
    if(arg.atom == "nack") return raft::nack;
    throw out_of_bounds("bad value: " + arg.atom,arg.pos);
}
template <>
void __deser<raft::msgkind>(ivy_deser &inp, raft::msgkind &res){
    int __res;
    __deser(inp,__res);
    res = (raft::msgkind)__res;
}
