#include "multi_paxos_system.h"

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
typedef multi_paxos_system ivy_class;
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

void multi_paxos_system::install_reader(reader *r) {
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

void multi_paxos_system::install_thread(reader *r) {
    install_reader(r);
}

void multi_paxos_system::install_timer(timer *r) {
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
    void multi_paxos_system::__lock() { WaitForSingleObject(mutex,INFINITE); }
    void multi_paxos_system::__unlock() { ReleaseMutex(mutex); }
#else
    void multi_paxos_system::__lock() { pthread_mutex_lock(&mutex); }
    void multi_paxos_system::__unlock() { pthread_mutex_unlock(&mutex); }
#endif
struct thunk__net__tcp__impl__handle_accept{
    multi_paxos_system *__ivy;
    thunk__net__tcp__impl__handle_accept(multi_paxos_system *__ivy): __ivy(__ivy){}
    void operator()(int s, multi_paxos_system::node other) const {
        __ivy->net__tcp__impl__handle_accept(s,other);
    }
};
struct thunk__net__tcp__impl__handle_connected{
    multi_paxos_system *__ivy;
    thunk__net__tcp__impl__handle_connected(multi_paxos_system *__ivy): __ivy(__ivy){}
    void operator()(int s) const {
        __ivy->net__tcp__impl__handle_connected(s);
    }
};
struct thunk__net__tcp__impl__handle_fail{
    multi_paxos_system *__ivy;
    thunk__net__tcp__impl__handle_fail(multi_paxos_system *__ivy): __ivy(__ivy){}
    void operator()(int s) const {
        __ivy->net__tcp__impl__handle_fail(s);
    }
};
struct thunk__net__tcp__impl__handle_recv{
    multi_paxos_system *__ivy;
    thunk__net__tcp__impl__handle_recv(multi_paxos_system *__ivy): __ivy(__ivy){}
    void operator()(int s, multi_paxos_system::msg x) const {
        __ivy->net__tcp__impl__handle_recv(s,x);
    }
};
struct thunk__system__server__timer__sec__impl__handle_timeout{
    multi_paxos_system *__ivy;
    thunk__system__server__timer__sec__impl__handle_timeout(multi_paxos_system *__ivy): __ivy(__ivy){}
    void operator()() const {
        __ivy->system__server__timer__sec__impl__handle_timeout();
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

std::ostream &operator <<(std::ostream &s, const multi_paxos_system::msg_kind &t);
template <>
multi_paxos_system::msg_kind _arg<multi_paxos_system::msg_kind>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::msg_kind>(ivy_ser &res, const multi_paxos_system::msg_kind&);
template <>
void  __deser<multi_paxos_system::msg_kind>(ivy_deser &inp, multi_paxos_system::msg_kind &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::msg &t);
template <>
multi_paxos_system::msg _arg<multi_paxos_system::msg>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::msg>(ivy_ser &res, const multi_paxos_system::msg&);
template <>
void  __deser<multi_paxos_system::msg>(ivy_deser &inp, multi_paxos_system::msg &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::node__iter__t &t);
template <>
multi_paxos_system::node__iter__t _arg<multi_paxos_system::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::node__iter__t>(ivy_ser &res, const multi_paxos_system::node__iter__t&);
template <>
void  __deser<multi_paxos_system::node__iter__t>(ivy_deser &inp, multi_paxos_system::node__iter__t &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::nset &t);
template <>
multi_paxos_system::nset _arg<multi_paxos_system::nset>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::nset>(ivy_ser &res, const multi_paxos_system::nset&);
template <>
void  __deser<multi_paxos_system::nset>(ivy_deser &inp, multi_paxos_system::nset &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__ballot_status &t);
template <>
multi_paxos_system::system__ballot_status _arg<multi_paxos_system::system__ballot_status>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::system__ballot_status>(ivy_ser &res, const multi_paxos_system::system__ballot_status&);
template <>
void  __deser<multi_paxos_system::system__ballot_status>(ivy_deser &inp, multi_paxos_system::system__ballot_status &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__decision_struct &t);
template <>
multi_paxos_system::system__decision_struct _arg<multi_paxos_system::system__decision_struct>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::system__decision_struct>(ivy_ser &res, const multi_paxos_system::system__decision_struct&);
template <>
void  __deser<multi_paxos_system::system__decision_struct>(ivy_deser &inp, multi_paxos_system::system__decision_struct &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::vote_struct &t);
template <>
multi_paxos_system::vote_struct _arg<multi_paxos_system::vote_struct>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::vote_struct>(ivy_ser &res, const multi_paxos_system::vote_struct&);
template <>
void  __deser<multi_paxos_system::vote_struct>(ivy_deser &inp, multi_paxos_system::vote_struct &res);
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::votemap_seg &t);
template <>
multi_paxos_system::votemap_seg _arg<multi_paxos_system::votemap_seg>(std::vector<ivy_value> &args, unsigned idx, long long bound);
template <>
void  __ser<multi_paxos_system::votemap_seg>(ivy_ser &res, const multi_paxos_system::votemap_seg&);
template <>
void  __deser<multi_paxos_system::votemap_seg>(ivy_deser &inp, multi_paxos_system::votemap_seg &res);
	    std::ostream &operator <<(std::ostream &s, const multi_paxos_system::nset__arr &a) {
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
	    multi_paxos_system::nset__arr _arg<multi_paxos_system::nset__arr>(std::vector<ivy_value> &args, unsigned idx, int bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        multi_paxos_system::nset__arr a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<multi_paxos_system::node>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<multi_paxos_system::nset__arr>(ivy_deser &inp, multi_paxos_system::nset__arr &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<multi_paxos_system::nset__arr>(ivy_ser &res, const multi_paxos_system::nset__arr &inp) {
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
            z3::expr __to_solver(gen& g, const z3::expr& z3val, multi_paxos_system::nset__arr& val) {
	        z3::expr z3end = g.apply("nset.arr.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("nset.arr.value",z3val,g.int_to_z3(g.sort("nset.index"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<multi_paxos_system::nset__arr>( gen &g, const  z3::expr &v,multi_paxos_system::nset__arr &res){
	        int __end;
	        __from_solver(g,g.apply("nset.arr.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("nset.arr.value",v,g.int_to_z3(g.sort("nset.index"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<multi_paxos_system::nset__arr>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("nset.index"),__sz);
                z3::expr pred =  g.apply("nset.arr.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<multi_paxos_system::node>(g,g.apply("nset.arr.value",v,g.int_to_z3(g.sort("nset.index"),__i)));
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
        	    std::ostream &operator <<(std::ostream &s, const multi_paxos_system::votemap &a) {
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
	    multi_paxos_system::votemap _arg<multi_paxos_system::votemap>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        multi_paxos_system::votemap a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<multi_paxos_system::vote_struct>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<multi_paxos_system::votemap>(ivy_deser &inp, multi_paxos_system::votemap &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<multi_paxos_system::votemap>(ivy_ser &res, const multi_paxos_system::votemap &inp) {
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
            z3::expr __to_solver(gen& g, const z3::expr& z3val, multi_paxos_system::votemap& val) {
	        z3::expr z3end = g.apply("votemap.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("votemap.value",z3val,g.int_to_z3(g.sort("inst"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<multi_paxos_system::votemap>( gen &g, const  z3::expr &v,multi_paxos_system::votemap &res){
	        unsigned long long __end;
	        __from_solver(g,g.apply("votemap.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("votemap.value",v,g.int_to_z3(g.sort("inst"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<multi_paxos_system::votemap>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("inst"),__sz);
                z3::expr pred =  g.apply("votemap.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<multi_paxos_system::vote_struct>(g,g.apply("votemap.value",v,g.int_to_z3(g.sort("inst"),__i)));
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

            multi_paxos_system::msg pkt;                      // holds received message
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



	    std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__ballot_status_array &a) {
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
	    multi_paxos_system::system__ballot_status_array _arg<multi_paxos_system::system__ballot_status_array>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        multi_paxos_system::system__ballot_status_array a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<multi_paxos_system::system__ballot_status>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<multi_paxos_system::system__ballot_status_array>(ivy_deser &inp, multi_paxos_system::system__ballot_status_array &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<multi_paxos_system::system__ballot_status_array>(ivy_ser &res, const multi_paxos_system::system__ballot_status_array &inp) {
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
            z3::expr __to_solver(gen& g, const z3::expr& z3val, multi_paxos_system::system__ballot_status_array& val) {
	        z3::expr z3end = g.apply("system.ballot_status_array.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("system.ballot_status_array.value",z3val,g.int_to_z3(g.sort("inst"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<multi_paxos_system::system__ballot_status_array>( gen &g, const  z3::expr &v,multi_paxos_system::system__ballot_status_array &res){
	        unsigned long long __end;
	        __from_solver(g,g.apply("system.ballot_status_array.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("system.ballot_status_array.value",v,g.int_to_z3(g.sort("inst"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<multi_paxos_system::system__ballot_status_array>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("inst"),__sz);
                z3::expr pred =  g.apply("system.ballot_status_array.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<multi_paxos_system::system__ballot_status>(g,g.apply("system.ballot_status_array.value",v,g.int_to_z3(g.sort("inst"),__i)));
	    }
	    #endif

		    std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__timearray &a) {
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
	    multi_paxos_system::system__timearray _arg<multi_paxos_system::system__timearray>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        multi_paxos_system::system__timearray a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<unsigned long long>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<multi_paxos_system::system__timearray>(ivy_deser &inp, multi_paxos_system::system__timearray &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<multi_paxos_system::system__timearray>(ivy_ser &res, const multi_paxos_system::system__timearray &inp) {
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
            z3::expr __to_solver(gen& g, const z3::expr& z3val, multi_paxos_system::system__timearray& val) {
	        z3::expr z3end = g.apply("system.timearray.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("system.timearray.value",z3val,g.int_to_z3(g.sort("inst"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<multi_paxos_system::system__timearray>( gen &g, const  z3::expr &v,multi_paxos_system::system__timearray &res){
	        unsigned long long __end;
	        __from_solver(g,g.apply("system.timearray.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("system.timearray.value",v,g.int_to_z3(g.sort("inst"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<multi_paxos_system::system__timearray>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("inst"),__sz);
                z3::expr pred =  g.apply("system.timearray.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<unsigned long long>(g,g.apply("system.timearray.value",v,g.int_to_z3(g.sort("inst"),__i)));
	    }
	    #endif

		    std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__log &a) {
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
	    multi_paxos_system::system__log _arg<multi_paxos_system::system__log>(std::vector<ivy_value> &args, unsigned idx, long long bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        multi_paxos_system::system__log a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<multi_paxos_system::system__decision_struct>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<multi_paxos_system::system__log>(ivy_deser &inp, multi_paxos_system::system__log &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<multi_paxos_system::system__log>(ivy_ser &res, const multi_paxos_system::system__log &inp) {
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
            z3::expr __to_solver(gen& g, const z3::expr& z3val, multi_paxos_system::system__log& val) {
	        z3::expr z3end = g.apply("system.log.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("system.log.value",z3val,g.int_to_z3(g.sort("inst"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<multi_paxos_system::system__log>( gen &g, const  z3::expr &v,multi_paxos_system::system__log &res){
	        unsigned long long __end;
	        __from_solver(g,g.apply("system.log.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("system.log.value",v,g.int_to_z3(g.sort("inst"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<multi_paxos_system::system__log>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("inst"),__sz);
                z3::expr pred =  g.apply("system.log.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<multi_paxos_system::system__decision_struct>(g,g.apply("system.log.value",v,g.int_to_z3(g.sort("inst"),__i)));
	    }
	    #endif

		class sec_timer : public timer {
	    thunk__system__server__timer__sec__impl__handle_timeout rcb;
            int ttl;
	    ivy_class *ivy;
	  public:
	    sec_timer(thunk__system__server__timer__sec__impl__handle_timeout rcb, ivy_class *ivy)
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
    int multi_paxos_system::___ivy_choose(int rng,const char *name,int id) {
        return 0;
    }
bool multi_paxos_system::nset__member(const node& N, const nset& S){
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
bool multi_paxos_system::nset__majority(const nset& S){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val = (nset__card[nset__all] < (nset__card[S] + nset__card[S]));
    return val;
}
multi_paxos_system::vote_struct multi_paxos_system::votemap_seg__value(const votemap_seg& X, unsigned long long N){
    multi_paxos_system::vote_struct val;
    val.maxr = (unsigned long long)___ivy_choose(0,"ret:val",0);
    unsigned long long __tmp1;
    __tmp1 = N;
    unsigned long long __tmp2;
    __tmp2 = X.offset;
    val = votemap__value(X.elems,( __tmp1 < __tmp2 ? 0 : __tmp1 - __tmp2));
    return val;
}
unsigned long long multi_paxos_system::votemap_seg__first(const votemap_seg& X){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val = X.offset;
    return val;
}
unsigned long long multi_paxos_system::votemap_seg__upper(const votemap_seg& x){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val = (votemap__end(x.elems) + x.offset);
    return val;
}
unsigned long long multi_paxos_system::none(){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val = 0;
    return val;
}
multi_paxos_system::vote_struct multi_paxos_system::votemap_seg_ops__maxvote(const votemap_seg& M, unsigned long long I){
    multi_paxos_system::vote_struct val;
    val.maxr = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val = ((((votemap_seg__first(M) < I) || (votemap_seg__first(M) == I)) && (I < votemap_seg__upper(M))) ? votemap_seg__value(M,I) : not_a_vote);
    return val;
}
bool multi_paxos_system::leader_of(const node& N, unsigned long long R){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val = (!(R == 0) && (N == round_leader__leader_fun(R)));
    return val;
}
bool multi_paxos_system::system__server__is_decided(unsigned long long J){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val = ((J < system__log__end(system__server__my_log)) && system__log__value(system__server__my_log,J).present);
    return val;
}
__strlit multi_paxos_system::no_op(){
    __strlit val;
    val = "";
    return val;
}
multi_paxos_system::node multi_paxos_system::__num0(){
    multi_paxos_system::node val;
    val =  0 ;
    return val;
}
bool multi_paxos_system::nset__arr__value(const nset__arr& a, int i, const node& v){
    bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
    return val;
}
int multi_paxos_system::nset__arr__end(const nset__arr& a){
    int val;
    val = (int)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
multi_paxos_system::vote_struct multi_paxos_system::votemap__value(const votemap& a, unsigned long long i){
    multi_paxos_system::vote_struct val;
    val.maxr = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
unsigned long long multi_paxos_system::votemap__end(const votemap& a){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
multi_paxos_system::system__ballot_status multi_paxos_system::system__ballot_status_array__value(const system__ballot_status_array& a, unsigned long long i){
    multi_paxos_system::system__ballot_status val;
    val.active = (bool)___ivy_choose(0,"ret:val",0);
    val.decided = (bool)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
unsigned long long multi_paxos_system::system__ballot_status_array__end(const system__ballot_status_array& a){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
unsigned long long multi_paxos_system::system__timearray__value(const system__timearray& a, unsigned long long i){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
unsigned long long multi_paxos_system::system__timearray__end(const system__timearray& a){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
multi_paxos_system::system__decision_struct multi_paxos_system::system__log__value(const system__log& a, unsigned long long i){
    multi_paxos_system::system__decision_struct val;
    val.present = (bool)___ivy_choose(0,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
unsigned long long multi_paxos_system::system__log__end(const system__log& a){
    unsigned long long val;
    val = (unsigned long long)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
multi_paxos_system::node multi_paxos_system::round_leader__leader_fun(unsigned long long r){
    multi_paxos_system::node val;
    val =  r % node__size ;
    return val;
}
void multi_paxos_system::ext__system__server__catch_up__tick(){
    {
        if((!leader_of(self,system__server__current_round) && !(system__server__current_round == 0))){
            ext__system__server__catch_up__ask_missing(votemap__end(system__server__my_votes));
        }
        system__server__catch_up__current_time = ext__system__time__next(system__server__catch_up__current_time);
    }
}
unsigned long long multi_paxos_system::ext__system__ballot_status_array__size(const system__ballot_status_array& a){
    unsigned long long s;
    s = (unsigned long long)___ivy_choose(0,"fml:s",0);
    {

        s = (unsigned long long) a.size();
    }
    return s;
}
void multi_paxos_system::ext__nset__add(nset& s, const node& n){
    {
        if(!nset__member(n,s)){
            ext__nset__arr__append(s.repr, n);
        }
    }
}
void multi_paxos_system::ext__node__iter__next(node__iter__t& x){
    {
        {
            bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1225);
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
void multi_paxos_system::ext__system__server__start_round(){
    {
        {
            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1226);
            {
                loc__0 = ext__system__server__next_self_leader_round(system__server__current_round);
                ext__system__server__change_round(loc__0);
            }
        }
        ext__system__server__current_leader__set(self);
        ext__nset__add(system__server__joined, self);
        {
            unsigned long long loc__end;
    loc__end = (unsigned long long)___ivy_choose(0,"loc:end",1228);
            {
                if(((system__server__first_undecided < votemap__end(system__server__my_votes)) || (system__server__first_undecided == votemap__end(system__server__my_votes)))){
                    loc__end = votemap__end(system__server__my_votes);
                }
                else {
                    loc__end = system__server__first_undecided;
                }
                system__server__joined_votes = ext__votemap_seg__make(system__server__my_votes, system__server__first_undecided, loc__end);
                {
                    msg loc__m;
    loc__m.m_kind = (msg_kind)___ivy_choose(0,"loc:m",1227);
    loc__m.m_round = (unsigned long long)___ivy_choose(0,"loc:m",1227);
    loc__m.m_inst = (unsigned long long)___ivy_choose(0,"loc:m",1227);
    loc__m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m",1227);
                    {
                        loc__m.m_kind = msg_kind__one_a;
                        loc__m.m_round = system__server__current_round;
                        loc__m.m_node = self;
                        loc__m.m_inst = system__server__first_undecided;
                        ext__shim__bcast(loc__m);
                    }
                }
            }
        }
    }
}
unsigned long long multi_paxos_system::ext__inst__next(unsigned long long x){
    unsigned long long y;
    y = (unsigned long long)___ivy_choose(0,"fml:y",0);
    {
        y = (x + 1);
    }
    return y;
}
multi_paxos_system::system__ballot_status_array multi_paxos_system::ext__system__ballot_status_array__empty(){
    multi_paxos_system::system__ballot_status_array a;
    {
        
    }
    return a;
}
void multi_paxos_system::ext__system__timearray__set(system__timearray& a, unsigned long long x, unsigned long long y){
    {

        if (0 <= x && x < (unsigned long long)a.size())
            a[x] = y;
    }
}
void multi_paxos_system::net__tcp__impl__handle_fail(int s){
    ext__net__tcp__failed(s);
}
multi_paxos_system::node__iter__t multi_paxos_system::ext__node__iter__end(){
    multi_paxos_system::node__iter__t y;
    y.is_end = (bool)___ivy_choose(0,"fml:y",0);
    {
        y.is_end = true;
        y.val = __num0();
    }
    return y;
}
multi_paxos_system::msg multi_paxos_system::ext__system__server__build_decide_msg(unsigned long long i){
    multi_paxos_system::msg m;
    m.m_kind = (msg_kind)___ivy_choose(0,"fml:m",0);
    m.m_round = (unsigned long long)___ivy_choose(0,"fml:m",0);
    m.m_inst = (unsigned long long)___ivy_choose(0,"fml:m",0);
    m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"fml:m",0);
    {
        m.m_kind = msg_kind__decide;
        m.m_inst = i;
        m.m_round = system__server__current_round;
        {
            system__decision_struct loc__0;
    loc__0.present = (bool)___ivy_choose(0,"loc:0",1229);
            {
                loc__0 = ext__system__log__get(system__server__my_log, i);
                m.m_value = loc__0.decision;
            }
        }
        m.m_node = self;
    }
    return m;
}
void multi_paxos_system::ext__system__server__decide(unsigned long long i, __strlit v){
    imp__system__server__decide(i, v);
}
void multi_paxos_system::ext__system__ballot_status_array__resize(system__ballot_status_array& a, unsigned long long s, const system__ballot_status& v){
    {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
    }
}
void multi_paxos_system::ext__protocol__join_round(const node& n, unsigned long long r){
    {
    }
}
void multi_paxos_system::ext__system__server__catch_up__notify_two_a(unsigned long long i){
    {
        if(((system__timearray__end(system__server__catch_up__two_a_times) < i) || (system__timearray__end(system__server__catch_up__two_a_times) == i))){
            {
                unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1292);
                {
                    loc__0 = ext__inst__next(i);
                    ext__system__timearray__resize(system__server__catch_up__two_a_times, loc__0, 0);
                }
            }
        }
        ext__system__timearray__set(system__server__catch_up__two_a_times, i, system__server__catch_up__current_time);
    }
}
multi_paxos_system::system__decision_struct multi_paxos_system::ext__system__server__query(unsigned long long i){
    multi_paxos_system::system__decision_struct d;
    d.present = (bool)___ivy_choose(0,"fml:d",0);
    {
        if(system__server__is_decided(i)){
            {
                d.decision = system__log__value(system__server__my_log,i).decision;
                d.present = true;
            }
        }
        else {
            d.present = false;
        }
    }
    return d;
}
unsigned long long multi_paxos_system::ext__system__log__size(const system__log& a){
    unsigned long long s;
    s = (unsigned long long)___ivy_choose(0,"fml:s",0);
    {

        s = (unsigned long long) a.size();
    }
    return s;
}
void multi_paxos_system::ext__shim__two_a_handler__handle(const msg& m){
    {
        if(((system__server__current_round < m.m_round) || (system__server__current_round == m.m_round))){
            {
                if((system__server__current_round < m.m_round)){
                    {
                        ext__system__server__change_round(m.m_round);
                        ext__system__server__current_leader__set(m.m_node);
                    }
                }
                if(!system__server__is_decided(m.m_inst)){
                    {
                        ext__system__server__vote(m.m_node, m.m_inst, m.m_value);
                        ext__system__server__catch_up__notify_two_a(m.m_inst);
                    }
                }
                else {
                    if(!(m.m_node == self)){
                        {
                            {
                                msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1264);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1264);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1264);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1264);
                                {
                                    loc__0 = ext__system__server__build_decide_msg(m.m_inst);
                                    {
                                        msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1263);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1263);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1263);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1263);
                                        {
                                            loc__m2 = loc__0;
                                            ext__shim__unicast(m.m_node, loc__m2);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if((system__server__first_undecided < m.m_inst)){
                    {
                        ext__system__server__ask_for_retransmission(m.m_inst);
                    }
                }
            }
        }
    }
}
void multi_paxos_system::ext__shim__two_b_handler__handle(const msg& m){
    {
        {
            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1233);
            unsigned long long loc__1;
    loc__1 = (unsigned long long)___ivy_choose(0,"loc:1",1233);
            {
                loc__0 = ext__system__ballot_status_array__size(system__server__inst_status);
                loc__1 = ext__inst__next(m.m_inst);
                if(((loc__0 < loc__1) || (loc__0 == loc__1))){
                    {
                        {
                            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1232);
                            {
                                loc__0 = ext__inst__next(m.m_inst);
                                ext__system__ballot_status_array__resize(system__server__inst_status, loc__0, system__init_status);
                            }
                        }
                    }
                }
            }
        }
        {
            system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1240);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1240);
            {
                loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, m.m_inst);
                {
                    system__ballot_status loc__status;
    loc__status.active = (bool)___ivy_choose(0,"loc:status",1239);
    loc__status.decided = (bool)___ivy_choose(0,"loc:status",1239);
                    {
                        loc__status = loc__0;
                        if((loc__status.active && !loc__status.decided && (m.m_round == system__server__current_round))){
                            {
                                ext__nset__add(loc__status.voters, m.m_node);
                                loc__status.active = true;
                                loc__status.proposal = m.m_value;
                                if(nset__majority(loc__status.voters)){
                                    {
                                        loc__status.decided = true;
                                        {
                                            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1235);
                                            unsigned long long loc__1;
    loc__1 = (unsigned long long)___ivy_choose(0,"loc:1",1235);
                                            {
                                                loc__0 = ext__system__log__size(system__server__my_log);
                                                loc__1 = ext__inst__next(m.m_inst);
                                                if(((loc__0 < loc__1) || (loc__0 == loc__1))){
                                                    {
                                                        {
                                                            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1234);
                                                            {
                                                                loc__0 = ext__inst__next(m.m_inst);
                                                                ext__system__log__resize(system__server__my_log, loc__0, system__server__no_decision);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        {
                                            system__decision_struct loc__d;
    loc__d.present = (bool)___ivy_choose(0,"loc:d",1238);
                                            {
                                                loc__d.present = true;
                                                loc__d.decision = m.m_value;
                                                ext__system__log__set(system__server__my_log, m.m_inst, loc__d);
                                                ext__system__server__update_first_undecided(m.m_inst);
                                                {
                                                    msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1237);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1237);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1237);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1237);
                                                    {
                                                        loc__0 = ext__system__server__build_decide_msg(m.m_inst);
                                                        {
                                                            msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1236);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1236);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1236);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1236);
                                                            {
                                                                loc__m2 = loc__0;
                                                                ext__shim__bcast(loc__m2);
                                                                ext__protocol__decide(self, m.m_inst, m.m_round, m.m_value, loc__status.voters);
                                                                ext__system__server__decide(m.m_inst, m.m_value);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                ext__system__ballot_status_array__set(system__server__inst_status, m.m_inst, loc__status);
                            }
                        }
                    }
                }
            }
        }
    }
}
multi_paxos_system::system__timearray multi_paxos_system::ext__system__timearray__empty(){
    multi_paxos_system::system__timearray a;
    {
        
    }
    return a;
}
bool multi_paxos_system::ext__net__tcp__send(int s, const msg& p){
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
unsigned long long multi_paxos_system::ext__round__next(unsigned long long x){
    unsigned long long y;
    y = (unsigned long long)___ivy_choose(0,"fml:y",0);
    {
        y = (x + 1);
    }
    return y;
}
multi_paxos_system::nset multi_paxos_system::ext__nset__emptyset(){
    multi_paxos_system::nset s;
    {
        s.repr = ext__nset__arr__empty();
    }
    return s;
}
void multi_paxos_system::ext__protocol__propose(unsigned long long r, unsigned long long i, __strlit v){
    {
    }
}
void multi_paxos_system::ext__system__server__ask_for_retransmission(unsigned long long i){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msg_kind)___ivy_choose(0,"loc:m",1242);
    loc__m.m_round = (unsigned long long)___ivy_choose(0,"loc:m",1242);
    loc__m.m_inst = (unsigned long long)___ivy_choose(0,"loc:m",1242);
    loc__m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m",1242);
            {
                loc__m.m_node = self;
                loc__m.m_round = system__server__current_round;
                if(!system__ballot_status_array__value(system__server__inst_status,i).active){
                    loc__m.m_kind = msg_kind__missing_two_a;
                }
                else {
                    loc__m.m_kind = msg_kind__missing_decision;
                }
                loc__m.m_inst = i;
                {
                    node loc__0;
                    {
                        loc__0 = ext__system__server__current_leader__get();
                        ext__shim__unicast(loc__0, loc__m);
                    }
                }
            }
        }
    }
}
int multi_paxos_system::ext__net__tcp__connect(const node& other){
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
             install_thread(new tcp_writer(self,s,queue,*net__tcp__impl__cb,this));
        } else
            net__tcp__impl__send_queue[s] -> set_open(other);
    }
    return s;
}
void multi_paxos_system::ext__shim__decide_handler__handle(const msg& m){
    {
        if(!system__server__is_decided(m.m_inst)){
            {
                {
                    unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1294);
                    unsigned long long loc__1;
    loc__1 = (unsigned long long)___ivy_choose(0,"loc:1",1294);
                    {
                        loc__0 = ext__system__ballot_status_array__size(system__server__inst_status);
                        loc__1 = ext__inst__next(m.m_inst);
                        if(((loc__0 < loc__1) || (loc__0 == loc__1))){
                            {
                                {
                                    unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1293);
                                    {
                                        loc__0 = ext__inst__next(m.m_inst);
                                        ext__system__ballot_status_array__resize(system__server__inst_status, loc__0, system__init_status);
                                    }
                                }
                            }
                        }
                    }
                }
                {
                    unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1296);
                    unsigned long long loc__1;
    loc__1 = (unsigned long long)___ivy_choose(0,"loc:1",1296);
                    {
                        loc__0 = ext__system__log__size(system__server__my_log);
                        loc__1 = ext__inst__next(m.m_inst);
                        if(((loc__0 < loc__1) || (loc__0 == loc__1))){
                            {
                                {
                                    unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1295);
                                    {
                                        loc__0 = ext__inst__next(m.m_inst);
                                        ext__system__log__resize(system__server__my_log, loc__0, system__server__no_decision);
                                    }
                                }
                            }
                        }
                    }
                }
                {
                    system__decision_struct loc__d;
    loc__d.present = (bool)___ivy_choose(0,"loc:d",1299);
                    {
                        loc__d.present = true;
                        loc__d.decision = m.m_value;
                        ext__system__log__set(system__server__my_log, m.m_inst, loc__d);
                        ext__system__server__update_first_undecided(m.m_inst);
                        {
                            system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1298);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1298);
                            {
                                loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, m.m_inst);
                                {
                                    system__ballot_status loc__status;
    loc__status.active = (bool)___ivy_choose(0,"loc:status",1297);
    loc__status.decided = (bool)___ivy_choose(0,"loc:status",1297);
                                    {
                                        loc__status = loc__0;
                                        loc__status.decided = true;
                                        ext__system__ballot_status_array__set(system__server__inst_status, m.m_inst, loc__status);
                                        ext__system__server__decide(m.m_inst, m.m_value);
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
void multi_paxos_system::ext__votemap_seg__resize(votemap_seg& seg, unsigned long long x, const vote_struct& y){
    {
        unsigned long long __tmp3;
        __tmp3 = x;
        unsigned long long __tmp4;
        __tmp4 = seg.offset;
        ext__votemap__resize(seg.elems, ( __tmp3 < __tmp4 ? 0 : __tmp3 - __tmp4), y);
    }
}
void multi_paxos_system::ext__system__log__set(system__log& a, unsigned long long x, const system__decision_struct& y){
    {

        if (0 <= x && x < (unsigned long long)a.size())
            a[x] = y;
    }
}
void multi_paxos_system::ext__net__tcp__recv(int s, const msg& p){
    {
        ext__net__recv(p);
    }
}
multi_paxos_system::votemap multi_paxos_system::ext__votemap__empty(){
    multi_paxos_system::votemap a;
    {
        
    }
    return a;
}
void multi_paxos_system::ext__shim__one_b_handler__handle(const msg& m){
    {
        if(((system__server__current_round == m.m_round) && !system__server__round_active)){
            {
                ext__nset__add(system__server__joined, m.m_node);
                system__server__joined_votes = ext__votemap_seg_ops__zip_max(system__server__joined_votes, m.m_votemap);
                if(nset__majority(system__server__joined)){
                    {
                        system__server__round_active = true;
                        system__server__next_inst = votemap_seg__upper(system__server__joined_votes);
                        if(((system__ballot_status_array__end(system__server__inst_status) < votemap_seg__upper(system__server__joined_votes)) || (system__ballot_status_array__end(system__server__inst_status) == votemap_seg__upper(system__server__joined_votes)))){
                            {
                                ext__system__ballot_status_array__resize(system__server__inst_status, votemap_seg__upper(system__server__joined_votes), system__init_status);
                            }
                        }
                        {
                            unsigned long long loc__j;
    loc__j = (unsigned long long)___ivy_choose(0,"loc:j",1249);
                            {
                                loc__j = votemap_seg__first(system__server__joined_votes);
                                while(!(loc__j == votemap_seg__upper(system__server__joined_votes))){
                                    {
                                        if(!system__server__is_decided(loc__j)){
                                            {
                                                {
                                                    __strlit loc__prop;
                                                    {
                                                        if(!(votemap_seg_ops__maxvote(system__server__joined_votes,loc__j).maxr == none())){
                                                            {
                                                                loc__prop = votemap_seg_ops__maxvote(system__server__joined_votes,loc__j).maxv;
                                                            }
                                                        }
                                                        else {
                                                            {
                                                                loc__prop = no_op();
                                                            }
                                                        }
                                                        {
                                                            system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1247);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1247);
                                                            {
                                                                loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, loc__j);
                                                                {
                                                                    system__ballot_status loc__status;
    loc__status.active = (bool)___ivy_choose(0,"loc:status",1246);
    loc__status.decided = (bool)___ivy_choose(0,"loc:status",1246);
                                                                    {
                                                                        loc__status = loc__0;
                                                                        loc__status.active = true;
                                                                        loc__status.proposal = loc__prop;
                                                                        {
                                                                            msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1245);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1245);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1245);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1245);
                                                                            {
                                                                                loc__0 = ext__system__server__build_proposal(loc__j, loc__prop);
                                                                                {
                                                                                    msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1244);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1244);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1244);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1244);
                                                                                    {
                                                                                        loc__m2 = loc__0;
                                                                                        ext__shim__bcast(loc__m2);
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
                                        loc__j = ext__inst__next(loc__j);
                                    }
                                }
                                ext__protocol__receive_join_acks(system__server__current_round, system__server__joined, system__server__joined_votes);
                            }
                        }
                    }
                }
            }
        }
    }
}
void multi_paxos_system::ext__votemap__set(votemap& a, unsigned long long x, const vote_struct& y){
    {

        if (0 <= x && x < (unsigned long long)a.size())
            a[x] = y;
    }
}
multi_paxos_system::msg multi_paxos_system::ext__system__server__build_proposal(unsigned long long j, __strlit v){
    multi_paxos_system::msg m;
    m.m_kind = (msg_kind)___ivy_choose(0,"fml:m",0);
    m.m_round = (unsigned long long)___ivy_choose(0,"fml:m",0);
    m.m_inst = (unsigned long long)___ivy_choose(0,"fml:m",0);
    m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"fml:m",0);
    {
        m.m_kind = msg_kind__two_a;
        m.m_round = system__server__current_round;
        m.m_node = self;
        m.m_inst = j;
        m.m_value = v;
    }
    return m;
}
bool multi_paxos_system::ext__system__server__propose(__strlit v){
    bool r;
    r = (bool)___ivy_choose(0,"fml:r",0);
    {
        if((leader_of(self,system__server__current_round) && system__server__round_active)){
            {
                ext__protocol__propose(system__server__current_round, system__server__next_inst, v);
                {
                    msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1254);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1254);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1254);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1254);
                    {
                        loc__0 = ext__system__server__build_proposal(system__server__next_inst, v);
                        {
                            msg loc__m;
    loc__m.m_kind = (msg_kind)___ivy_choose(0,"loc:m",1253);
    loc__m.m_round = (unsigned long long)___ivy_choose(0,"loc:m",1253);
    loc__m.m_inst = (unsigned long long)___ivy_choose(0,"loc:m",1253);
    loc__m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m",1253);
                            {
                                loc__m = loc__0;
                                ext__shim__bcast(loc__m);
                                ext__system__server__vote(self, system__server__next_inst, v);
                                ext__system__server__two_a_retransmitter__notify_two_a(loc__m.m_inst);
                                if(((system__ballot_status_array__end(system__server__inst_status) < system__server__next_inst) || (system__ballot_status_array__end(system__server__inst_status) == system__server__next_inst))){
                                    {
                                        {
                                            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1250);
                                            {
                                                loc__0 = ext__inst__next(system__server__next_inst);
                                                ext__system__ballot_status_array__resize(system__server__inst_status, loc__0, system__init_status);
                                            }
                                        }
                                    }
                                }
                                {
                                    system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1252);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1252);
                                    {
                                        loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, system__server__next_inst);
                                        {
                                            system__ballot_status loc__status;
    loc__status.active = (bool)___ivy_choose(0,"loc:status",1251);
    loc__status.decided = (bool)___ivy_choose(0,"loc:status",1251);
                                            {
                                                loc__status = loc__0;
                                                loc__status.active = true;
                                                loc__status.proposal = v;
                                                system__server__next_inst = ext__inst__next(system__server__next_inst);
                                                r = true;
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
                r = false;
            }
        }
    }
    return r;
}
void multi_paxos_system::ext__net__send(const node& dst, const msg& v){
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
    loc__0 = (bool)___ivy_choose(0,"loc:0",1256);
                {
                    loc__0 = ext__net__tcp__send(net__proc__sock[dst], v);
                    {
                        bool loc__ok;
    loc__ok = (bool)___ivy_choose(0,"loc:ok",1255);
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
void multi_paxos_system::ext__system__server__two_a_retransmitter__tick(){
    {
        if((leader_of(self,system__server__current_round) && system__server__round_active)){
            {
                msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1231);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1231);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1231);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1231);
                {
                    loc__m2.m_kind = msg_kind__two_a;
                    loc__m2.m_node = self;
                    loc__m2.m_round = system__server__current_round;
                    {
                        unsigned long long loc__i;
    loc__i = (unsigned long long)___ivy_choose(0,"loc:i",1230);
                        {
                            loc__i = system__server__first_undecided;
                            if((loc__i < system__ballot_status_array__end(system__server__inst_status))){
                                while((loc__i < system__ballot_status_array__end(system__server__inst_status))){
                                    {
                                        if((!system__server__is_decided(loc__i) && (loc__i < system__ballot_status_array__end(system__server__inst_status)) && system__ballot_status_array__value(system__server__inst_status,loc__i).active && (loc__i < system__timearray__end(system__server__two_a_retransmitter__two_a_times)) && (system__timearray__value(system__server__two_a_retransmitter__two_a_times,loc__i) < system__server__two_a_retransmitter__current_time))){
                                            {
                                                loc__m2.m_inst = loc__i;
                                                loc__m2.m_value = system__ballot_status_array__value(system__server__inst_status,loc__i).proposal;
                                                ext__shim__bcast(loc__m2);
                                                ext__system__server__two_a_retransmitter__notify_two_a(loc__i);
                                            }
                                        }
                                        loc__i = ext__inst__next(loc__i);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        system__server__two_a_retransmitter__current_time = ext__system__time__next(system__server__two_a_retransmitter__current_time);
    }
}
multi_paxos_system::msg multi_paxos_system::ext__system__server__build_vote_msg(unsigned long long i, __strlit v){
    multi_paxos_system::msg m;
    m.m_kind = (msg_kind)___ivy_choose(0,"fml:m",0);
    m.m_round = (unsigned long long)___ivy_choose(0,"fml:m",0);
    m.m_inst = (unsigned long long)___ivy_choose(0,"fml:m",0);
    m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"fml:m",0);
    {
        m.m_kind = msg_kind__two_b;
        m.m_round = system__server__current_round;
        m.m_node = self;
        m.m_inst = i;
        m.m_value = v;
    }
    return m;
}
multi_paxos_system::node multi_paxos_system::ext__node__next(const node& x){
    multi_paxos_system::node y;
    {
        y = x + 1;
    }
    return y;
}
multi_paxos_system::system__decision_struct multi_paxos_system::ext__system__log__get(const system__log& a, unsigned long long x){
    multi_paxos_system::system__decision_struct y;
    y.present = (bool)___ivy_choose(0,"fml:y",0);
    {

        if (0 <= x && x < (unsigned long long)a.size())
            y = a[x];
    }
    return y;
}
void multi_paxos_system::ext__system__log__resize(system__log& a, unsigned long long s, const system__decision_struct& v){
    {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
    }
}
multi_paxos_system::node multi_paxos_system::ext__system__server__current_leader__get(){
    multi_paxos_system::node v;
    {
        v = system__server__current_leader__impl__val;
    }
    return v;
}
void multi_paxos_system::ext__net__tcp__close(int s){
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
bool multi_paxos_system::ext__system__server__leader_election__is_leader_too_quiet(){
    bool res;
    res = (bool)___ivy_choose(0,"fml:res",0);
    {
        unsigned long long __tmp5;
        __tmp5 = system__server__leader_election__my_time;
        unsigned long long __tmp6;
        __tmp6 = system__server__leader_election__last_heard_from_leader;
        unsigned long long __tmp7;
        __tmp7 = system__server__leader_election__my_time;
        unsigned long long __tmp8;
        __tmp8 = system__server__leader_election__last_heard_from_leader;
        res = ((system__server__leader_election__timeout < ( __tmp5 < __tmp6 ? 0 : __tmp5 - __tmp6)) || (( __tmp7 < __tmp8 ? 0 : __tmp7 - __tmp8) == system__server__leader_election__timeout));
    }
    return res;
}
void multi_paxos_system::imp__system__server__decide(unsigned long long i, __strlit v){
    {
    }
}
void multi_paxos_system::imp__shim__debug_receiving(const msg& m){
    {
    }
}
multi_paxos_system::nset__arr multi_paxos_system::ext__nset__arr__empty(){
    multi_paxos_system::nset__arr a;
    {
        
    }
    return a;
}
void multi_paxos_system::ext__nset__arr__append(nset__arr& a, const node& v){
    {

        a.push_back(v);
    }
}
void multi_paxos_system::ext__shim__debug_receiving(const msg& m){
    imp__shim__debug_receiving(m);
}
void multi_paxos_system::ext__shim__missing_decision_handler__handle(const msg& m){
    {
        if(((0 < m.m_round) && (m.m_round == system__server__current_round) && leader_of(self,system__server__current_round))){
            if(system__server__is_decided(m.m_inst)){
                {
                    {
                        msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1261);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1261);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1261);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1261);
                        {
                            loc__0 = ext__system__server__build_decide_msg(m.m_inst);
                            {
                                msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1260);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1260);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1260);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1260);
                                {
                                    loc__m2 = loc__0;
                                    ext__shim__unicast(m.m_node, loc__m2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
bool multi_paxos_system::ext__node__is_max(const node& x){
    bool r;
    r = (bool)___ivy_choose(0,"fml:r",0);
    {
        r = (x == node__size - 1);
    }
    return r;
}
void multi_paxos_system::ext__system__server__two_a_retransmitter__notify_two_a(unsigned long long i){
    {
        if(((system__timearray__end(system__server__two_a_retransmitter__two_a_times) < i) || (system__timearray__end(system__server__two_a_retransmitter__two_a_times) == i))){
            {
                unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1262);
                {
                    loc__0 = ext__inst__next(i);
                    ext__system__timearray__resize(system__server__two_a_retransmitter__two_a_times, loc__0, 0);
                }
            }
        }
        ext__system__timearray__set(system__server__two_a_retransmitter__two_a_times, i, system__server__two_a_retransmitter__current_time);
    }
}
multi_paxos_system::votemap_seg multi_paxos_system::ext__votemap_seg__make(const votemap& data, unsigned long long begin, unsigned long long end){
    multi_paxos_system::votemap_seg seg;
    seg.offset = (unsigned long long)___ivy_choose(0,"fml:seg",0);
    {
        seg.offset = begin;
        seg.elems = ext__votemap__empty();
        if(((begin < votemap__end(data)) && ((end < votemap__end(data)) || (end == votemap__end(data))))){
            {
                unsigned long long loc__idx;
    loc__idx = (unsigned long long)___ivy_choose(0,"loc:idx",1243);
                {
                    loc__idx = begin;
                    while((loc__idx < end)){
                        {
                            ext__votemap__append(seg.elems, votemap__value(data,loc__idx));
                            loc__idx = ext__inst__next(loc__idx);
                        }
                    }
                }
            }
        }
    }
    return seg;
}
multi_paxos_system::node__iter__t multi_paxos_system::ext__node__iter__create(const node& x){
    multi_paxos_system::node__iter__t y;
    y.is_end = (bool)___ivy_choose(0,"fml:y",0);
    {
        y.is_end = false;
        y.val = x;
    }
    return y;
}
void multi_paxos_system::ext__net__tcp__failed(int s){
    {
        {
            node loc__other;
            int __tmp9;
            __tmp9 = 0;
            for (int X__0 = 0; X__0 < node__size; X__0++) {
                if(((net__proc__isup[X__0] || net__proc__pend[X__0]) && (net__proc__sock[X__0] == s))){
                    loc__other = X__0;
                    __tmp9= 1;
                }
            }
            if(__tmp9){
                {
                    net__proc__isup[loc__other] = false;
                    net__proc__pend[loc__other] = false;
                }
            }
        }
    }
}
void multi_paxos_system::system__server__timer__sec__impl__handle_timeout(){
    ext__system__server__timer__sec__timeout();
}
void multi_paxos_system::ext__shim__unicast(const node& dst, const msg& m){
    {
        ext__shim__debug_sending(dst, m);
        ext__net__send(dst, m);
    }
}
void multi_paxos_system::ext__net__recv(const msg& v){
    {
        if(!(v.m_kind == msg_kind__keep_alive)){
            {
                ext__shim__debug_receiving(v);
            }
        }
        if((v.m_kind == msg_kind__one_a)){
            ext__shim__one_a_handler__handle(v);
        }
        else {
            if((v.m_kind == msg_kind__one_b)){
                ext__shim__one_b_handler__handle(v);
            }
            else {
                if((v.m_kind == msg_kind__two_a)){
                    ext__shim__two_a_handler__handle(v);
                }
                else {
                    if((v.m_kind == msg_kind__two_b)){
                        ext__shim__two_b_handler__handle(v);
                    }
                    else {
                        if((v.m_kind == msg_kind__keep_alive)){
                            ext__shim__keep_alive_handler__handle(v);
                        }
                        else {
                            if((v.m_kind == msg_kind__missing_two_a)){
                                ext__shim__missing_two_a_handler__handle(v);
                            }
                            else {
                                if((v.m_kind == msg_kind__missing_decision)){
                                    ext__shim__missing_decision_handler__handle(v);
                                }
                                else {
                                    if((v.m_kind == msg_kind__decide)){
                                        ext__shim__decide_handler__handle(v);
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
unsigned long long multi_paxos_system::ext__system__time__next(unsigned long long x){
    unsigned long long y;
    y = (unsigned long long)___ivy_choose(0,"fml:y",0);
    {
        y = (x + 1);
    }
    return y;
}
void multi_paxos_system::ext__votemap__append(votemap& a, const vote_struct& v){
    {

        a.push_back(v);
    }
}
            struct __thunk__0 : thunk<multi_paxos_system::nset,int>{
                __thunk__0()  {
                }
                int operator()(const multi_paxos_system::nset &arg){
                    return 0;
                }
            };
                                    struct __thunk__1 : thunk<multi_paxos_system::nset,int>{
    hash_thunk<multi_paxos_system::nset,int> nset__card;
    multi_paxos_system::node__iter__t loc__i;
bool nset__arr__value(const multi_paxos_system::nset__arr& a, int i, const multi_paxos_system::node& v){
                                            bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
                                            val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
                                            return val;
}
bool nset__member(const multi_paxos_system::node& N, const multi_paxos_system::nset& S){
                                            bool val;
    val = (bool)___ivy_choose(0,"ret:val",0);
                                            int __tmp10;
                                            __tmp10 = 0;
                                            for (int I = 0; I < nset__arr__end(S.repr); I++) {
                                                if ((((0 < I) || (0 == I)) && (I < nset__arr__end(S.repr)) && nset__arr__value(S.repr,I,N))) __tmp10 = 1;
                                            }
                                            val = __tmp10;
                                            return val;
}
int nset__arr__end(const multi_paxos_system::nset__arr& a){
                                            int val;
    val = (int)___ivy_choose(0,"ret:val",0);
                                            val =  a.size() ;
                                            return val;
}
                                        __thunk__1(hash_thunk<multi_paxos_system::nset,int> nset__card,multi_paxos_system::node__iter__t loc__i) : nset__card(nset__card),loc__i(loc__i){
                                        }
                                        int operator()(const multi_paxos_system::nset &arg){
                                            return (nset__member(loc__i.val,arg) ? (nset__card[arg] + 1) : nset__card[arg]);
                                        }
                                    };
            struct __thunk__2 : thunk<multi_paxos_system::node,bool>{
                __thunk__2()  {
                }
                bool operator()(const multi_paxos_system::node &arg){
                    return false;
                }
            };
            struct __thunk__3 : thunk<multi_paxos_system::node,bool>{
                __thunk__3()  {
                }
                bool operator()(const multi_paxos_system::node &arg){
                    return false;
                }
            };
void multi_paxos_system::__init(){
    {
        {
            nset__card = hash_thunk<multi_paxos_system::nset,int>(new __thunk__0());
            nset__all.repr = ext__nset__arr__empty();
            {
                node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(0,"loc:0",1304);
                {
                    loc__0 = ext__node__iter__create(__num0());
                    {
                        node__iter__t loc__i;
    loc__i.is_end = (bool)___ivy_choose(0,"loc:i",1303);
                        {
                            loc__i = loc__0;
                            while(!loc__i.is_end){
                                {
                                    ext__nset__arr__append(nset__all.repr, loc__i.val);
                                    nset__card = hash_thunk<multi_paxos_system::nset,int>(new __thunk__1(nset__card,loc__i));
                                    ext__node__iter__next(loc__i);
                                }
                            }
                        }
                    }
                }
            }
        }
        not_a_vote.maxr = none();
        {
            net__proc__isup = hash_thunk<multi_paxos_system::node,bool>(new __thunk__2());
            net__proc__pend = hash_thunk<multi_paxos_system::node,bool>(new __thunk__3());
        }
        {
            system__init_status.active = false;
            system__init_status.decided = false;
            system__init_status.voters = ext__nset__emptyset();
        }
        system__server__no_decision.present = false;
        system__server__current_leader__impl__full = false;
        {
            system__server__current_round = 0;
            system__server__next_inst = 0;
            system__server__my_votes = ext__votemap__empty();
            system__server__joined = ext__nset__emptyset();
            system__server__joined_votes = ext__votemap_seg__empty();
            system__server__round_active = false;
            system__server__inst_status = ext__system__ballot_status_array__empty();
            system__server__my_log = ext__system__log__empty();
            system__server__first_undecided = 0;
        }
        {
            system__server__leader_election__last_heard_from_leader = system__server__leader_election__my_time;
            system__server__leader_election__last_start_round = system__server__leader_election__my_time;
            system__server__leader_election__timeout = 3;
        }
        {
            system__server__two_a_retransmitter__current_time = 0;
            system__server__two_a_retransmitter__two_a_times = ext__system__timearray__empty();
        }
        {
            system__server__catch_up__current_time = 0;
            system__server__catch_up__two_a_times = ext__system__timearray__empty();
        }
    }
}
multi_paxos_system::votemap_seg multi_paxos_system::ext__votemap_seg_ops__zip_max(const votemap_seg& seg1, const votemap_seg& seg2){
    multi_paxos_system::votemap_seg res;
    res.offset = (unsigned long long)___ivy_choose(0,"fml:res",0);
    {
        res = seg1;
        if((votemap_seg__upper(res) < votemap_seg__upper(seg2))){
            {
                ext__votemap_seg__resize(res, votemap_seg__upper(seg2), not_a_vote);
            }
        }
        {
            unsigned long long loc__upper;
    loc__upper = (unsigned long long)___ivy_choose(0,"loc:upper",1267);
            {
                loc__upper = votemap_seg__upper(res);
                {
                    unsigned long long loc__first;
    loc__first = (unsigned long long)___ivy_choose(0,"loc:first",1266);
                    {
                        loc__first = votemap_seg__first(res);
                        {
                            unsigned long long loc__i;
    loc__i = (unsigned long long)___ivy_choose(0,"loc:i",1265);
                            {
                                loc__i = votemap_seg__first(res);
                                while(!(loc__i == votemap_seg__upper(seg2))){
                                    {
                                        if((votemap_seg_ops__maxvote(res,loc__i).maxr < votemap_seg_ops__maxvote(seg2,loc__i).maxr)){
                                            {
                                                ext__votemap_seg__set(res, loc__i, votemap_seg_ops__maxvote(seg2,loc__i));
                                            }
                                        }
                                        loc__i = ext__inst__next(loc__i);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return res;
}
void multi_paxos_system::ext__votemap__resize(votemap& a, unsigned long long s, const vote_struct& v){
    {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
    }
}
void multi_paxos_system::ext__shim__bcast(const msg& m){
    {
        {
            node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(0,"loc:0",1270);
            {
                loc__0 = ext__node__iter__create(__num0());
                {
                    node__iter__t loc__iter;
    loc__iter.is_end = (bool)___ivy_choose(0,"loc:iter",1269);
                    {
                        loc__iter = loc__0;
                        while(!loc__iter.is_end){
                            {
                                {
                                    node loc__n;
                                    {
                                        loc__n = loc__iter.val;
                                        ext__shim__debug_sending(loc__n, m);
                                        ext__net__send(loc__n, m);
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
unsigned long long multi_paxos_system::ext__system__server__next_self_leader_round(unsigned long long r){
    unsigned long long s;
    s = (unsigned long long)___ivy_choose(0,"fml:s",0);
    {
        {
            unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1272);
            {
                loc__0 = ext__round__next(system__server__current_round);
                {
                    unsigned long long loc__iter;
    loc__iter = (unsigned long long)___ivy_choose(0,"loc:iter",1271);
                    {
                        loc__iter = loc__0;
                        while(!leader_of(self,loc__iter)){
                            loc__iter = ext__round__next(loc__iter);
                        }
                        s = loc__iter;
                    }
                }
            }
        }
    }
    return s;
}
bool multi_paxos_system::ext__system__server__leader_election__start_round_timed_out(){
    bool res;
    res = (bool)___ivy_choose(0,"fml:res",0);
    {
        unsigned long long __tmp11;
        __tmp11 = system__server__leader_election__my_time;
        unsigned long long __tmp12;
        __tmp12 = system__server__leader_election__last_start_round;
        unsigned long long __tmp13;
        __tmp13 = system__server__leader_election__my_time;
        unsigned long long __tmp14;
        __tmp14 = system__server__leader_election__last_start_round;
        res = ((system__server__leader_election__timeout < ( __tmp11 < __tmp12 ? 0 : __tmp11 - __tmp12)) || (( __tmp13 < __tmp14 ? 0 : __tmp13 - __tmp14) == system__server__leader_election__timeout));
    }
    return res;
}
void multi_paxos_system::ext__shim__keep_alive_handler__handle(const msg& m){
    {
        {
            unsigned long long loc__r;
    loc__r = (unsigned long long)___ivy_choose(0,"loc:r",1273);
            {
                loc__r = m.m_round;
                if((system__server__current_round < loc__r)){
                    {
                        ext__system__server__change_round(loc__r);
                        ext__system__server__current_leader__set(m.m_node);
                    }
                }
                system__server__leader_election__last_heard_from_leader = system__server__leader_election__my_time;
            }
        }
    }
}
void multi_paxos_system::ext__net__tcp__accept(int s, const node& other){
    {
    }
}
void multi_paxos_system::ext__votemap_seg__set(votemap_seg& seg, unsigned long long x, const vote_struct& y){
    {
        unsigned long long __tmp15;
        __tmp15 = x;
        unsigned long long __tmp16;
        __tmp16 = seg.offset;
        ext__votemap__set(seg.elems, ( __tmp15 < __tmp16 ? 0 : __tmp15 - __tmp16), y);
    }
}
void multi_paxos_system::ext__shim__debug_sending(const node& dst, const msg& m){
    imp__shim__debug_sending(dst, m);
}
void multi_paxos_system::ext__system__server__current_leader__set(const node& v){
    {
        system__server__current_leader__impl__val = v;
        system__server__current_leader__impl__full = true;
    }
}
void multi_paxos_system::ext__system__server__catch_up__ask_missing(unsigned long long j){
    if(((system__server__first_undecided < j) || (system__server__first_undecided == j))){
        {
            unsigned long long loc__i;
    loc__i = (unsigned long long)___ivy_choose(0,"loc:i",1257);
            {
                loc__i = system__server__first_undecided;
                while((loc__i < j)){
                    {
                        if((!system__server__is_decided(loc__i) && (loc__i < system__timearray__end(system__server__catch_up__two_a_times)) && (system__timearray__value(system__server__catch_up__two_a_times,loc__i) < system__server__catch_up__current_time))){
                            {
                                ext__system__server__ask_for_retransmission(loc__i);
                            }
                        }
                        loc__i = ext__inst__next(loc__i);
                    }
                }
            }
        }
    }
}
void multi_paxos_system::ext__shim__one_a_handler__handle(const msg& m){
    {
        if((system__server__current_round < m.m_round)){
            {
                ext__system__server__change_round(m.m_round);
                ext__system__server__current_leader__set(m.m_node);
                {
                    msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1276);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1276);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1276);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1276);
                    {
                        loc__m2.m_kind = msg_kind__one_b;
                        {
                            unsigned long long loc__end;
    loc__end = (unsigned long long)___ivy_choose(0,"loc:end",1275);
                            {
                                if(((m.m_inst < votemap__end(system__server__my_votes)) || (m.m_inst == votemap__end(system__server__my_votes)))){
                                    loc__end = votemap__end(system__server__my_votes);
                                }
                                else {
                                    loc__end = m.m_inst;
                                }
                                loc__m2.m_votemap = ext__votemap_seg__make(system__server__my_votes, m.m_inst, loc__end);
                                loc__m2.m_round = m.m_round;
                                loc__m2.m_node = self;
                                loc__m2.m_inst = m.m_inst;
                                {
                                    node loc__leader;
                                    {
                                        loc__leader = m.m_node;
                                        ext__shim__unicast(loc__leader, loc__m2);
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
void multi_paxos_system::ext__shim__missing_two_a_handler__handle(const msg& m){
    {
        if(((0 < m.m_round) && (m.m_round == system__server__current_round) && leader_of(self,system__server__current_round))){
            if(system__server__is_decided(m.m_inst)){
                {
                    {
                        msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1278);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1278);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1278);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1278);
                        {
                            loc__0 = ext__system__server__build_decide_msg(m.m_inst);
                            {
                                msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1277);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1277);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1277);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1277);
                                {
                                    loc__m2 = loc__0;
                                    ext__shim__unicast(m.m_node, loc__m2);
                                }
                            }
                        }
                    }
                }
            }
            else {
                if((m.m_inst < system__ballot_status_array__end(system__server__inst_status))){
                    {
                        system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1281);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1281);
                        {
                            loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, m.m_inst);
                            if(loc__0.active){
                                {
                                    {
                                        system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1280);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1280);
                                        msg loc__1;
    loc__1.m_kind = (msg_kind)___ivy_choose(0,"loc:1",1280);
    loc__1.m_round = (unsigned long long)___ivy_choose(0,"loc:1",1280);
    loc__1.m_inst = (unsigned long long)___ivy_choose(0,"loc:1",1280);
    loc__1.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:1",1280);
                                        {
                                            loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, m.m_inst);
                                            loc__1 = ext__system__server__build_proposal(m.m_inst, loc__0.proposal);
                                            {
                                                msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1279);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1279);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1279);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1279);
                                                {
                                                    loc__m2 = loc__1;
                                                    ext__shim__unicast(m.m_node, loc__m2);
                                                    ext__system__server__two_a_retransmitter__notify_two_a(loc__m2.m_inst);
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
multi_paxos_system::system__ballot_status multi_paxos_system::ext__system__ballot_status_array__get(const system__ballot_status_array& a, unsigned long long x){
    multi_paxos_system::system__ballot_status y;
    y.active = (bool)___ivy_choose(0,"fml:y",0);
    y.decided = (bool)___ivy_choose(0,"fml:y",0);
    {

        if (0 <= x && x < (unsigned long long)a.size())
            y = a[x];
    }
    return y;
}
void multi_paxos_system::ext__system__server__timer__sec__timeout(){
    {
        ext__system__server__leader_election__tick();
        ext__system__server__catch_up__tick();
        ext__system__server__two_a_retransmitter__tick();
    }
}
void multi_paxos_system::ext__system__server__vote(const node& leader, unsigned long long i, __strlit v){
    {
        if(((votemap__end(system__server__my_votes) < i) || (votemap__end(system__server__my_votes) == i))){
            {
                unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1282);
                {
                    loc__0 = ext__inst__next(i);
                    ext__votemap__resize(system__server__my_votes, loc__0, not_a_vote);
                }
            }
        }
        if(((system__ballot_status_array__end(system__server__inst_status) < i) || (system__ballot_status_array__end(system__server__inst_status) == i))){
            {
                unsigned long long loc__0;
    loc__0 = (unsigned long long)___ivy_choose(0,"loc:0",1283);
                {
                    loc__0 = ext__inst__next(i);
                    ext__system__ballot_status_array__resize(system__server__inst_status, loc__0, system__init_status);
                }
            }
        }
        {
            vote_struct loc__new_vote;
    loc__new_vote.maxr = (unsigned long long)___ivy_choose(0,"loc:new_vote",1288);
            {
                loc__new_vote.maxr = system__server__current_round;
                loc__new_vote.maxv = v;
                ext__votemap__set(system__server__my_votes, i, loc__new_vote);
                {
                    system__ballot_status loc__0;
    loc__0.active = (bool)___ivy_choose(0,"loc:0",1287);
    loc__0.decided = (bool)___ivy_choose(0,"loc:0",1287);
                    {
                        loc__0 = ext__system__ballot_status_array__get(system__server__inst_status, i);
                        {
                            system__ballot_status loc__status;
    loc__status.active = (bool)___ivy_choose(0,"loc:status",1286);
    loc__status.decided = (bool)___ivy_choose(0,"loc:status",1286);
                            {
                                loc__status = loc__0;
                                ext__nset__add(loc__status.voters, self);
                                loc__status.proposal = v;
                                loc__status.active = true;
                                ext__system__ballot_status_array__set(system__server__inst_status, i, loc__status);
                                ext__protocol__cast_vote(self, i, system__server__current_round, v);
                                {
                                    msg loc__0;
    loc__0.m_kind = (msg_kind)___ivy_choose(0,"loc:0",1285);
    loc__0.m_round = (unsigned long long)___ivy_choose(0,"loc:0",1285);
    loc__0.m_inst = (unsigned long long)___ivy_choose(0,"loc:0",1285);
    loc__0.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:0",1285);
                                    {
                                        loc__0 = ext__system__server__build_vote_msg(i, v);
                                        {
                                            msg loc__m2;
    loc__m2.m_kind = (msg_kind)___ivy_choose(0,"loc:m2",1284);
    loc__m2.m_round = (unsigned long long)___ivy_choose(0,"loc:m2",1284);
    loc__m2.m_inst = (unsigned long long)___ivy_choose(0,"loc:m2",1284);
    loc__m2.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m2",1284);
                                            {
                                                loc__m2 = loc__0;
                                                ext__shim__unicast(leader, loc__m2);
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
void multi_paxos_system::ext__protocol__receive_join_acks(unsigned long long r, const nset& q, const votemap_seg& m){
    {
    }
}
void multi_paxos_system::ext__system__timearray__resize(system__timearray& a, unsigned long long s, unsigned long long v){
    {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
    }
}
void multi_paxos_system::imp__shim__debug_sending(const node& dst, const msg& m){
    {
    }
}
void multi_paxos_system::ext__system__server__leader_election__tick(){
    {
        system__server__leader_election__my_time = ext__system__time__next(system__server__leader_election__my_time);
        if((system__server__current_round == 0)){
            ext__system__server__start_round();
        }
        else {
            if(!leader_of(self,system__server__current_round)){
                {
                    bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1289);
                    {
                        loc__0 = ext__system__server__leader_election__is_leader_too_quiet();
                        if(loc__0){
                            ext__system__server__start_round();
                        }
                    }
                }
            }
            else {
                if(!system__server__round_active){
                    {
                        bool loc__0;
    loc__0 = (bool)___ivy_choose(0,"loc:0",1290);
                        {
                            loc__0 = ext__system__server__leader_election__start_round_timed_out();
                            if(loc__0){
                                ext__system__server__start_round();
                            }
                        }
                    }
                }
                else {
                    {
                        {
                            msg loc__m;
    loc__m.m_kind = (msg_kind)___ivy_choose(0,"loc:m",1291);
    loc__m.m_round = (unsigned long long)___ivy_choose(0,"loc:m",1291);
    loc__m.m_inst = (unsigned long long)___ivy_choose(0,"loc:m",1291);
    loc__m.m_votemap.offset = (unsigned long long)___ivy_choose(0,"loc:m",1291);
                            {
                                loc__m.m_kind = msg_kind__keep_alive;
                                loc__m.m_round = system__server__current_round;
                                loc__m.m_node = self;
                                ext__shim__bcast(loc__m);
                            }
                        }
                    }
                }
            }
        }
    }
}
void multi_paxos_system::net__tcp__impl__handle_accept(int s, const node& other){
    ext__net__tcp__accept(s, other);
}
void multi_paxos_system::ext__protocol__cast_vote(const node& n, unsigned long long i, unsigned long long r, __strlit v){
    {
    }
}
void multi_paxos_system::net__tcp__impl__handle_recv(int s, const msg& x){
    ext__net__tcp__recv(s, x);
}
void multi_paxos_system::net__tcp__impl__handle_connected(int s){
    ext__net__tcp__connected(s);
}
void multi_paxos_system::ext__system__ballot_status_array__set(system__ballot_status_array& a, unsigned long long x, const system__ballot_status& y){
    {

        if (0 <= x && x < (unsigned long long)a.size())
            a[x] = y;
    }
}
void multi_paxos_system::ext__system__server__leader_election__notify_join_round(unsigned long long r){
    {
        if(leader_of(self,r)){
            system__server__leader_election__last_start_round = system__server__leader_election__my_time;
        }
        system__server__leader_election__last_heard_from_leader = system__server__leader_election__my_time;
    }
}
void multi_paxos_system::ext__protocol__decide(const node& n, unsigned long long i, unsigned long long r, __strlit v, const nset& q){
    {
    }
}
void multi_paxos_system::ext__system__server__update_first_undecided(unsigned long long i){
    if((i == system__server__first_undecided)){
        {
            bool loc__continue;
    loc__continue = (bool)___ivy_choose(0,"loc:continue",1300);
            {
                loc__continue = true;
                while((((i < system__log__end(system__server__my_log)) || (i == system__log__end(system__server__my_log))) && loc__continue)){
                    if(!system__server__is_decided(i)){
                        {
                            loc__continue = false;
                        }
                    }
                    else {
                        {
                            i = ext__inst__next(i);
                        }
                    }
                }
                system__server__first_undecided = i;
            }
        }
    }
}
multi_paxos_system::system__log multi_paxos_system::ext__system__log__empty(){
    multi_paxos_system::system__log a;
    {
        
    }
    return a;
}
void multi_paxos_system::ext__system__server__change_round(unsigned long long r){
    {
        system__server__current_round = r;
        ext__protocol__join_round(self, r);
        ext__system__server__leader_election__notify_join_round(system__server__current_round);
        system__server__round_active = false;
        system__server__inst_status = ext__system__ballot_status_array__empty();
        system__server__joined = ext__nset__emptyset();
    }
}
multi_paxos_system::votemap_seg multi_paxos_system::ext__votemap_seg__empty(){
    multi_paxos_system::votemap_seg seg;
    seg.offset = (unsigned long long)___ivy_choose(0,"fml:seg",0);
    {
        seg.offset = 0;
        seg.elems = ext__votemap__empty();
    }
    return seg;
}
void multi_paxos_system::ext__net__tcp__connected(int s){
    {
        {
            node loc__other;
            int __tmp17;
            __tmp17 = 0;
            for (int X__0 = 0; X__0 < node__size; X__0++) {
                if((net__proc__pend[X__0] && (net__proc__sock[X__0] == s))){
                    loc__other = X__0;
                    __tmp17= 1;
                }
            }
            if(__tmp17){
                {
                    net__proc__pend[loc__other] = false;
                    net__proc__isup[loc__other] = true;
                }
            }
        }
    }
}
void multi_paxos_system::__tick(int __timeout){
}
multi_paxos_system::multi_paxos_system(node node__size, node self){
#ifdef _WIN32
mutex = CreateMutex(NULL,FALSE,NULL);
#else
pthread_mutex_init(&mutex,NULL);
#endif
__lock();
    __CARD__value = 0;
    __CARD__system__time = 0;
    __CARD__nset__index = 0;
    __CARD__inst = 0;
    __CARD__net__tcp__socket = 0;
    __CARD__round = 0;

    the_tcp_config = 0;

    // Create the callbacks. In a parameterized instance, this creates
    // one set of callbacks for each endpoint id. When you put an
    // action in anti-quotes it creates a function object (a "thunk")
    // that captures the instance environment, in this case including
    // the instance's endpoint id "me".

    net__tcp__impl__cb = new tcp_callbacks(thunk__net__tcp__impl__handle_accept(this),thunk__net__tcp__impl__handle_recv(this),thunk__net__tcp__impl__handle_fail(this),thunk__net__tcp__impl__handle_connected(this));

    // Install a listener task for this endpoint. If parameterized, this creates
    // one for each endpoint.

    install_reader(net__tcp__impl__rdr = new tcp_listener(self,*net__tcp__impl__cb,this));
    install_timer(system__server__timer__sec__impl__tmr = new sec_timer(thunk__system__server__timer__sec__impl__handle_timeout(this),this));
    this->node__size = node__size;
    system__server__two_a_retransmitter__current_time = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__leader_election__timeout = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__first_undecided = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__leader_election__my_time = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__leader_election__last_heard_from_leader = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__catch_up__current_time = (unsigned long long)___ivy_choose(0,"init",0);
struct __thunk__4 : thunk<multi_paxos_system::node,bool>{
    __thunk__4()  {
    }
    bool operator()(const multi_paxos_system::node &arg){
        bool __tmp18;
    __tmp18 = (bool)___ivy_choose(0,"init",0);
        return __tmp18;
    }
};
net__proc__isup = hash_thunk<multi_paxos_system::node,bool>(new __thunk__4());
    system__server__leader_election__last_start_round = (unsigned long long)___ivy_choose(0,"init",0);
struct __thunk__5 : thunk<multi_paxos_system::nset,int>{
    __thunk__5()  {
    }
    int operator()(const multi_paxos_system::nset &arg){
        int __tmp19;
    __tmp19 = (int)___ivy_choose(0,"init",0);
        return __tmp19;
    }
};
nset__card = hash_thunk<multi_paxos_system::nset,int>(new __thunk__5());
struct __thunk__6 : thunk<multi_paxos_system::node,bool>{
    __thunk__6()  {
    }
    bool operator()(const multi_paxos_system::node &arg){
        bool __tmp20;
    __tmp20 = (bool)___ivy_choose(0,"init",0);
        return __tmp20;
    }
};
net__proc__pend = hash_thunk<multi_paxos_system::node,bool>(new __thunk__6());
    system__server__current_round = (unsigned long long)___ivy_choose(0,"init",0);
struct __thunk__7 : thunk<multi_paxos_system::node,int>{
    __thunk__7()  {
    }
    int operator()(const multi_paxos_system::node &arg){
        int __tmp21;
    __tmp21 = (int)___ivy_choose(0,"init",0);
        return __tmp21;
    }
};
net__proc__sock = hash_thunk<multi_paxos_system::node,int>(new __thunk__7());
    system__init_status.active = (bool)___ivy_choose(0,"init",0);
    system__init_status.decided = (bool)___ivy_choose(0,"init",0);
    system__server__joined_votes.offset = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__round_active = (bool)___ivy_choose(0,"init",0);
    system__server__next_inst = (unsigned long long)___ivy_choose(0,"init",0);
    system__server__current_leader__impl__full = (bool)___ivy_choose(0,"init",0);
    system__server__no_decision.present = (bool)___ivy_choose(0,"init",0);
    not_a_vote.maxr = (unsigned long long)___ivy_choose(0,"init",0);
    _generating = (bool)___ivy_choose(0,"init",0);
    this->self = self;
}
multi_paxos_system::~multi_paxos_system(){
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
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::node__iter__t &t){
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
void  __ser<multi_paxos_system::node__iter__t>(ivy_ser &res, const multi_paxos_system::node__iter__t&t){
    res.open_struct();
    res.open_field("is_end");
    __ser<bool>(res,t.is_end);
    res.close_field();
    res.open_field("val");
    __ser<multi_paxos_system::node>(res,t.val);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::nset &t){
    s<<"{";
    s<< "repr:";
    s << t.repr;
    s<<"}";
    return s;
}
template <>
void  __ser<multi_paxos_system::nset>(ivy_ser &res, const multi_paxos_system::nset&t){
    res.open_struct();
    res.open_field("repr");
    __ser<multi_paxos_system::nset__arr>(res,t.repr);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::vote_struct &t){
    s<<"{";
    s<< "maxr:";
    s << t.maxr;
    s<<",";
    s<< "maxv:";
    s << t.maxv;
    s<<"}";
    return s;
}
template <>
void  __ser<multi_paxos_system::vote_struct>(ivy_ser &res, const multi_paxos_system::vote_struct&t){
    res.open_struct();
    res.open_field("maxr");
    __ser<unsigned long long>(res,t.maxr);
    res.close_field();
    res.open_field("maxv");
    __ser<__strlit>(res,t.maxv);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::votemap_seg &t){
    s<<"{";
    s<< "offset:";
    s << t.offset;
    s<<",";
    s<< "elems:";
    s << t.elems;
    s<<"}";
    return s;
}
template <>
void  __ser<multi_paxos_system::votemap_seg>(ivy_ser &res, const multi_paxos_system::votemap_seg&t){
    res.open_struct();
    res.open_field("offset");
    __ser<unsigned long long>(res,t.offset);
    res.close_field();
    res.open_field("elems");
    __ser<multi_paxos_system::votemap>(res,t.elems);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::msg &t){
    s<<"{";
    s<< "m_kind:";
    s << t.m_kind;
    s<<",";
    s<< "m_round:";
    s << t.m_round;
    s<<",";
    s<< "m_inst:";
    s << t.m_inst;
    s<<",";
    s<< "m_node:";
    s << t.m_node;
    s<<",";
    s<< "m_value:";
    s << t.m_value;
    s<<",";
    s<< "m_votemap:";
    s << t.m_votemap;
    s<<"}";
    return s;
}
template <>
void  __ser<multi_paxos_system::msg>(ivy_ser &res, const multi_paxos_system::msg&t){
    res.open_struct();
    res.open_field("m_kind");
    __ser<multi_paxos_system::msg_kind>(res,t.m_kind);
    res.close_field();
    res.open_field("m_round");
    __ser<unsigned long long>(res,t.m_round);
    res.close_field();
    res.open_field("m_inst");
    __ser<unsigned long long>(res,t.m_inst);
    res.close_field();
    res.open_field("m_node");
    __ser<multi_paxos_system::node>(res,t.m_node);
    res.close_field();
    res.open_field("m_value");
    __ser<__strlit>(res,t.m_value);
    res.close_field();
    res.open_field("m_votemap");
    __ser<multi_paxos_system::votemap_seg>(res,t.m_votemap);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__ballot_status &t){
    s<<"{";
    s<< "active:";
    s << t.active;
    s<<",";
    s<< "voters:";
    s << t.voters;
    s<<",";
    s<< "proposal:";
    s << t.proposal;
    s<<",";
    s<< "decided:";
    s << t.decided;
    s<<"}";
    return s;
}
template <>
void  __ser<multi_paxos_system::system__ballot_status>(ivy_ser &res, const multi_paxos_system::system__ballot_status&t){
    res.open_struct();
    res.open_field("active");
    __ser<bool>(res,t.active);
    res.close_field();
    res.open_field("voters");
    __ser<multi_paxos_system::nset>(res,t.voters);
    res.close_field();
    res.open_field("proposal");
    __ser<__strlit>(res,t.proposal);
    res.close_field();
    res.open_field("decided");
    __ser<bool>(res,t.decided);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::system__decision_struct &t){
    s<<"{";
    s<< "decision:";
    s << t.decision;
    s<<",";
    s<< "present:";
    s << t.present;
    s<<"}";
    return s;
}
template <>
void  __ser<multi_paxos_system::system__decision_struct>(ivy_ser &res, const multi_paxos_system::system__decision_struct&t){
    res.open_struct();
    res.open_field("decision");
    __ser<__strlit>(res,t.decision);
    res.close_field();
    res.open_field("present");
    __ser<bool>(res,t.present);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const multi_paxos_system::msg_kind &t){
    if (t == multi_paxos_system::msg_kind__one_a) s<<"one_a";
    if (t == multi_paxos_system::msg_kind__one_b) s<<"one_b";
    if (t == multi_paxos_system::msg_kind__two_a) s<<"two_a";
    if (t == multi_paxos_system::msg_kind__two_b) s<<"two_b";
    if (t == multi_paxos_system::msg_kind__keep_alive) s<<"keep_alive";
    if (t == multi_paxos_system::msg_kind__missing_two_a) s<<"missing_two_a";
    if (t == multi_paxos_system::msg_kind__missing_decision) s<<"missing_decision";
    if (t == multi_paxos_system::msg_kind__decide) s<<"decide";
    return s;
}
template <>
void  __ser<multi_paxos_system::msg_kind>(ivy_ser &res, const multi_paxos_system::msg_kind&t){
    __ser(res,(int)t);
}
template <>
multi_paxos_system::msg _arg<multi_paxos_system::msg>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::msg res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 6) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "m_kind") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.m_kind = _arg<multi_paxos_system::msg_kind>(tmp_args,0,8);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_kind: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "m_round") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.m_round = _arg<unsigned long long>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_round: " + err.txt,err.pos);
    }
    if (arg.fields[2].is_member()){
        tmp_args[0] = arg.fields[2].fields[0];
        if (arg.fields[2].atom != "m_inst") throw out_of_bounds("unexpected field: " + arg.fields[2].atom,arg.fields[2].pos);
    }
    else{
        tmp_args[0] = arg.fields[2];
    }
    try{
        res.m_inst = _arg<unsigned long long>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_inst: " + err.txt,err.pos);
    }
    if (arg.fields[3].is_member()){
        tmp_args[0] = arg.fields[3].fields[0];
        if (arg.fields[3].atom != "m_node") throw out_of_bounds("unexpected field: " + arg.fields[3].atom,arg.fields[3].pos);
    }
    else{
        tmp_args[0] = arg.fields[3];
    }
    try{
        res.m_node = _arg<multi_paxos_system::node>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_node: " + err.txt,err.pos);
    }
    if (arg.fields[4].is_member()){
        tmp_args[0] = arg.fields[4].fields[0];
        if (arg.fields[4].atom != "m_value") throw out_of_bounds("unexpected field: " + arg.fields[4].atom,arg.fields[4].pos);
    }
    else{
        tmp_args[0] = arg.fields[4];
    }
    try{
        res.m_value = _arg<__strlit>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_value: " + err.txt,err.pos);
    }
    if (arg.fields[5].is_member()){
        tmp_args[0] = arg.fields[5].fields[0];
        if (arg.fields[5].atom != "m_votemap") throw out_of_bounds("unexpected field: " + arg.fields[5].atom,arg.fields[5].pos);
    }
    else{
        tmp_args[0] = arg.fields[5];
    }
    try{
        res.m_votemap = _arg<multi_paxos_system::votemap_seg>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field m_votemap: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::msg>(ivy_deser &inp, multi_paxos_system::msg &res){
    inp.open_struct();
    inp.open_field("m_kind");
    __deser(inp,res.m_kind);
    inp.close_field();
    inp.open_field("m_round");
    __deser(inp,res.m_round);
    inp.close_field();
    inp.open_field("m_inst");
    __deser(inp,res.m_inst);
    inp.close_field();
    inp.open_field("m_node");
    __deser(inp,res.m_node);
    inp.close_field();
    inp.open_field("m_value");
    __deser(inp,res.m_value);
    inp.close_field();
    inp.open_field("m_votemap");
    __deser(inp,res.m_votemap);
    inp.close_field();
    inp.close_struct();
}
template <>
multi_paxos_system::node__iter__t _arg<multi_paxos_system::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::node__iter__t res;
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
        res.val = _arg<multi_paxos_system::node>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field val: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::node__iter__t>(ivy_deser &inp, multi_paxos_system::node__iter__t &res){
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
multi_paxos_system::nset _arg<multi_paxos_system::nset>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::nset res;
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
        res.repr = _arg<multi_paxos_system::nset__arr>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field repr: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::nset>(ivy_deser &inp, multi_paxos_system::nset &res){
    inp.open_struct();
    inp.open_field("repr");
    __deser(inp,res.repr);
    inp.close_field();
    inp.close_struct();
}
template <>
multi_paxos_system::system__ballot_status _arg<multi_paxos_system::system__ballot_status>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::system__ballot_status res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 4) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "active") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.active = _arg<bool>(tmp_args,0,2);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field active: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "voters") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.voters = _arg<multi_paxos_system::nset>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field voters: " + err.txt,err.pos);
    }
    if (arg.fields[2].is_member()){
        tmp_args[0] = arg.fields[2].fields[0];
        if (arg.fields[2].atom != "proposal") throw out_of_bounds("unexpected field: " + arg.fields[2].atom,arg.fields[2].pos);
    }
    else{
        tmp_args[0] = arg.fields[2];
    }
    try{
        res.proposal = _arg<__strlit>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field proposal: " + err.txt,err.pos);
    }
    if (arg.fields[3].is_member()){
        tmp_args[0] = arg.fields[3].fields[0];
        if (arg.fields[3].atom != "decided") throw out_of_bounds("unexpected field: " + arg.fields[3].atom,arg.fields[3].pos);
    }
    else{
        tmp_args[0] = arg.fields[3];
    }
    try{
        res.decided = _arg<bool>(tmp_args,0,2);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field decided: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::system__ballot_status>(ivy_deser &inp, multi_paxos_system::system__ballot_status &res){
    inp.open_struct();
    inp.open_field("active");
    __deser(inp,res.active);
    inp.close_field();
    inp.open_field("voters");
    __deser(inp,res.voters);
    inp.close_field();
    inp.open_field("proposal");
    __deser(inp,res.proposal);
    inp.close_field();
    inp.open_field("decided");
    __deser(inp,res.decided);
    inp.close_field();
    inp.close_struct();
}
template <>
multi_paxos_system::system__decision_struct _arg<multi_paxos_system::system__decision_struct>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::system__decision_struct res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 2) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "decision") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.decision = _arg<__strlit>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field decision: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "present") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.present = _arg<bool>(tmp_args,0,2);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field present: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::system__decision_struct>(ivy_deser &inp, multi_paxos_system::system__decision_struct &res){
    inp.open_struct();
    inp.open_field("decision");
    __deser(inp,res.decision);
    inp.close_field();
    inp.open_field("present");
    __deser(inp,res.present);
    inp.close_field();
    inp.close_struct();
}
template <>
multi_paxos_system::vote_struct _arg<multi_paxos_system::vote_struct>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::vote_struct res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 2) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "maxr") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.maxr = _arg<unsigned long long>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field maxr: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "maxv") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.maxv = _arg<__strlit>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field maxv: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::vote_struct>(ivy_deser &inp, multi_paxos_system::vote_struct &res){
    inp.open_struct();
    inp.open_field("maxr");
    __deser(inp,res.maxr);
    inp.close_field();
    inp.open_field("maxv");
    __deser(inp,res.maxv);
    inp.close_field();
    inp.close_struct();
}
template <>
multi_paxos_system::votemap_seg _arg<multi_paxos_system::votemap_seg>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    multi_paxos_system::votemap_seg res;
    ivy_value &arg = args[idx];
    if (arg.atom.size() || arg.fields.size() != 2) throw out_of_bounds("wrong number of fields",args[idx].pos);
    std::vector<ivy_value> tmp_args(1);
    if (arg.fields[0].is_member()){
        tmp_args[0] = arg.fields[0].fields[0];
        if (arg.fields[0].atom != "offset") throw out_of_bounds("unexpected field: " + arg.fields[0].atom,arg.fields[0].pos);
    }
    else{
        tmp_args[0] = arg.fields[0];
    }
    try{
        res.offset = _arg<unsigned long long>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field offset: " + err.txt,err.pos);
    }
    if (arg.fields[1].is_member()){
        tmp_args[0] = arg.fields[1].fields[0];
        if (arg.fields[1].atom != "elems") throw out_of_bounds("unexpected field: " + arg.fields[1].atom,arg.fields[1].pos);
    }
    else{
        tmp_args[0] = arg.fields[1];
    }
    try{
        res.elems = _arg<multi_paxos_system::votemap>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field elems: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<multi_paxos_system::votemap_seg>(ivy_deser &inp, multi_paxos_system::votemap_seg &res){
    inp.open_struct();
    inp.open_field("offset");
    __deser(inp,res.offset);
    inp.close_field();
    inp.open_field("elems");
    __deser(inp,res.elems);
    inp.close_field();
    inp.close_struct();
}
template <>
multi_paxos_system::msg_kind _arg<multi_paxos_system::msg_kind>(std::vector<ivy_value> &args, unsigned idx, long long bound){
    ivy_value &arg = args[idx];
    if (arg.atom.size() == 0 || arg.fields.size() != 0) throw out_of_bounds(idx,arg.pos);
    if(arg.atom == "one_a") return multi_paxos_system::msg_kind__one_a;
    if(arg.atom == "one_b") return multi_paxos_system::msg_kind__one_b;
    if(arg.atom == "two_a") return multi_paxos_system::msg_kind__two_a;
    if(arg.atom == "two_b") return multi_paxos_system::msg_kind__two_b;
    if(arg.atom == "keep_alive") return multi_paxos_system::msg_kind__keep_alive;
    if(arg.atom == "missing_two_a") return multi_paxos_system::msg_kind__missing_two_a;
    if(arg.atom == "missing_decision") return multi_paxos_system::msg_kind__missing_decision;
    if(arg.atom == "decide") return multi_paxos_system::msg_kind__decide;
    throw out_of_bounds("bad value: " + arg.atom,arg.pos);
}
template <>
void __deser<multi_paxos_system::msg_kind>(ivy_deser &inp, multi_paxos_system::msg_kind &res){
    int __res;
    __deser(inp,__res);
    res = (multi_paxos_system::msg_kind)__res;
}
