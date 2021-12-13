#include "a.h"

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
typedef a ivy_class;
std::ofstream __ivy_out;
std::ofstream __ivy_modelfile;
void __ivy_exit(int code){exit(code);}

class reader {
public:
    virtual int fdes() = 0;
    virtual void read() = 0;
    virtual void bind() {}
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
    timer *cr = (reader *) lpParam;
    while (true) {
        int ms = timer->ms_delay();
        Sleep(ms);
        timer->timeout(ms);
    }
    return 0;
} 
#else
void * _thread_reader(void *rdr_void) {
    reader *rdr = (reader *) rdr_void;
    rdr->bind();
    while(true) {
        rdr->read();
    }
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

void a::install_reader(reader *r) {
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
        thread_ids.push_back(dummy);
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

void a::install_timer(timer *r) {
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
        thread_ids.push_back(dummy);
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
    void a::__lock() { WaitForSingleObject(mutex,INFINITE); }
    void a::__unlock() { ReleaseMutex(mutex); }
#else
    void a::__lock() { pthread_mutex_lock(&mutex); }
    void a::__unlock() { pthread_mutex_unlock(&mutex); }
#endif

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
    void set(long long inp) {
        for (int i = sizeof(long long)-1; i >= 0 ; i--)
            res.push_back((inp>>(8*i))&0xff);
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
    void get(long long &res) {
        if (inp.size() < pos + sizeof(long long))
            throw deser_err();
        res = 0;
        for (int i = 0; i < sizeof(long long); i++)
            res = (res << 8) | (((long long)inp[pos++]) & 0xff);
    }
    void get(std::string &res) {
        while (pos < inp.size() && inp[pos]) {
            if (inp[pos] == '"')
                throw deser_err();
        }
        res.push_back(inp[pos++]);
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
        if (pos != inp.size())
            throw deser_err();
    }
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

template <class T> T _arg(std::vector<ivy_value> &args, unsigned idx, int bound);

template <>
bool _arg<bool>(std::vector<ivy_value> &args, unsigned idx, int bound) {
    if (!(args[idx].atom == "true" || args[idx].atom == "false") || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return args[idx].atom == "true";
}

template <>
int _arg<int>(std::vector<ivy_value> &args, unsigned idx, int bound) {
    int res = atoi(args[idx].atom.c_str());
    if (bound && (res < 0 || res >= bound) || args[idx].fields.size())
        throw out_of_bounds(idx,args[idx].pos);
    return res;
}

std::ostream &operator <<(std::ostream &s, const __strlit &t){
    s << "\"" << t.c_str() << "\"";
    return s;
}

template <>
__strlit _arg<__strlit>(std::vector<ivy_value> &args, unsigned idx, int bound) {
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

std::ostream &operator <<(std::ostream &s, const a::node__iter__t &t);
template <>
a::node__iter__t _arg<a::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<a::node__iter__t>(ivy_ser &res, const a::node__iter__t&);
template <>
void  __deser<a::node__iter__t>(ivy_deser &inp, a::node__iter__t &res);
std::ostream &operator <<(std::ostream &s, const a::nset &t);
template <>
a::nset _arg<a::nset>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<a::nset>(ivy_ser &res, const a::nset&);
template <>
void  __deser<a::nset>(ivy_deser &inp, a::nset &res);
	    std::ostream &operator <<(std::ostream &s, const a::nset__arr &a) {
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
	    a::nset__arr _arg<a::nset__arr>(std::vector<ivy_value> &args, unsigned idx, int bound) {
	        ivy_value &arg = args[idx];
	        if (arg.atom.size()) 
	            throw out_of_bounds(idx);
	        a::nset__arr a;
	        a.resize(arg.fields.size());
		for (unsigned i = 0; i < a.size(); i++) {
		    a[i] = _arg<a::node>(arg.fields,i,0);
	        }
	        return a;
	    }

	    template <>
	    void __deser<a::nset__arr>(ivy_deser &inp, a::nset__arr &res) {
	        inp.open_list();
	        while(inp.open_list_elem()) {
		    res.resize(res.size()+1);
	            __deser(inp,res.back());
		    inp.close_list_elem();
                }
		inp.close_list();
	    }

	    template <>
	    void __ser<a::nset__arr>(ivy_ser &res, const a::nset__arr &inp) {
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
            z3::expr __to_solver(gen& g, const z3::expr& z3val, a::nset__arr& val) {
	        z3::expr z3end = g.apply("nset.arr.end",z3val);
	        z3::expr __ret = z3end  == g.int_to_z3(z3end.get_sort(),val.size());
	        unsigned __sz = val.size();
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __ret = __ret && __to_solver(g,g.apply("nset.arr.value",z3val,g.int_to_z3(g.sort("nset.index"),__i)),val[__i]);
                return __ret;
            }

	    template <>
	    void  __from_solver<a::nset__arr>( gen &g, const  z3::expr &v,a::nset__arr &res){
	        int __end;
	        __from_solver(g,g.apply("nset.arr.end",v),__end);
	        unsigned __sz = (unsigned) __end;
	        res.resize(__sz);
	        for (unsigned __i = 0; __i < __sz; ++__i)
		    __from_solver(g,g.apply("nset.arr.value",v,g.int_to_z3(g.sort("nset.index"),__i)),res[__i]);
	    }

	    template <>
	    void  __randomize<a::nset__arr>( gen &g, const  z3::expr &v){
	        unsigned __sz = rand() % 4;
                z3::expr val_expr = g.int_to_z3(g.sort("nset.index"),__sz);
                z3::expr pred =  g.apply("nset.arr.end",v) == val_expr;
                g.add_alit(pred);
	        for (unsigned __i = 0; __i < __sz; ++__i)
	            __randomize<a::node>(g,g.apply("nset.arr.value",v,g.int_to_z3(g.sort("nset.index"),__i)));
	    }
	    #endif

	int a::___ivy_choose(int rng,const char *name,int id) {
        return 0;
    }
bool a::nset__member(const node& N, const nset& S){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    int __tmp0;
    __tmp0 = 0;
    for (int I = 0; I < nset__arr__end(S.repr); I++) {
        if ((((0 < I) || (0 == I)) && (I < nset__arr__end(S.repr)) && nset__arr__value(S.repr,I,N))) __tmp0 = 1;
    }
    val = __tmp0;
    return val;
}
a::node a::__num0(){
    node val;
    val =  0 ;
    return val;
}
bool a::nset__arr__value(const nset__arr& a, int i, const node& v){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
    return val;
}
int a::nset__arr__end(const nset__arr& a){
    int val;
    val = (int)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
void a::ext__nset__arr__append(nset__arr& a, const node& v){
        {

        a.push_back(v);
    }
}
void a::ext__node__iter__next(node__iter__t& x){
        {
        {
            bool loc__0;
    loc__0 = (bool)___ivy_choose(2,"loc:0",63);
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
a::node a::ext__node__next(const node& x){
    node y;
    {
        y = x + 1;
    }
    return y;
}
bool a::ext__node__is_max(const node& x){
    bool r;
    r = (bool)___ivy_choose(2,"fml:r",0);
    {
        r = (x == node__size - 1);
    }
    return r;
}
a::nset__arr a::ext__nset__arr__empty(){
    nset__arr a;
    {
        
    }
    return a;
}
a::node__iter__t a::ext__node__iter__create(const node& x){
    node__iter__t y;
    y.is_end = (bool)___ivy_choose(2,"fml:y",0);
    {
        y.is_end = false;
        y.val = x;
    }
    return y;
}
a::node__iter__t a::ext__node__iter__end(){
    node__iter__t y;
    y.is_end = (bool)___ivy_choose(2,"fml:y",0);
    {
        y.is_end = true;
        y.val = __num0();
    }
    return y;
}
            struct __thunk__0 : thunk<a::nset,int>{
                __thunk__0()  {
                }
                int operator()(const a::nset &arg){
                    return 0;
                }
            };
                                    struct __thunk__1 : thunk<a::nset,int>{
    hash_thunk<a::nset,int> nset__card;
    a::node__iter__t loc__i;
bool nset__arr__value(const a::nset__arr& a, int i, const a::node& v){
                                            bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
                                            val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
                                            return val;
}
bool nset__member(const a::node& N, const a::nset& S){
                                            bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
                                            int __tmp1;
                                            __tmp1 = 0;
                                            for (int I = 0; I < nset__arr__end(S.repr); I++) {
                                                if ((((0 < I) || (0 == I)) && (I < nset__arr__end(S.repr)) && nset__arr__value(S.repr,I,N))) __tmp1 = 1;
                                            }
                                            val = __tmp1;
                                            return val;
}
int nset__arr__end(const a::nset__arr& a){
                                            int val;
    val = (int)___ivy_choose(0,"ret:val",0);
                                            val =  a.size() ;
                                            return val;
}
                                        __thunk__1(hash_thunk<a::nset,int> nset__card,a::node__iter__t loc__i) : nset__card(nset__card),loc__i(loc__i){
                                        }
                                        int operator()(const a::nset &arg){
                                            return (nset__member(loc__i.val,arg) ? (nset__card[arg] + 1) : nset__card[arg]);
                                        }
                                    };
void a::__init(){
    {
        {
            nset__card = hash_thunk<a::nset,int>(new __thunk__0());
            nset__all.repr = ext__nset__arr__empty();
            {
                node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(2,"loc:0",69);
                {
                    loc__0 = ext__node__iter__create(__num0());
                    {
                        node__iter__t loc__i;
    loc__i.is_end = (bool)___ivy_choose(2,"loc:i",68);
                        {
                            loc__i = loc__0;
                            while(!loc__i.is_end){
                                {
                                    ext__nset__arr__append(nset__all.repr, loc__i.val);
                                    nset__card = hash_thunk<a::nset,int>(new __thunk__1(nset__card,loc__i));
                                    ext__node__iter__next(loc__i);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void a::__tick(int __timeout){
}
a::a(node node__size){
#ifdef _WIN32
mutex = CreateMutex(NULL,FALSE,NULL);
#else
pthread_mutex_init(&mutex,NULL);
#endif
__lock();
    __CARD__nset__index = 0;
    this->node__size = node__size;
struct __thunk__2 : thunk<a::nset,int>{
    __thunk__2()  {
    }
    int operator()(const a::nset &arg){
        int __tmp2;
    __tmp2 = (int)___ivy_choose(0,"init",0);
        return __tmp2;
    }
};
nset__card = hash_thunk<a::nset,int>(new __thunk__2());
{
    {
        struct __thunk__3 : thunk<a::nset,int>{
            __thunk__3()  {
            }
            int operator()(const a::nset &arg){
                return 0;
            }
        };
        nset__card = hash_thunk<a::nset,int>(new __thunk__3());
        nset__all.repr = ext__nset__arr__empty();
        {
            node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(2,"loc:0",69);
            {
                loc__0 = ext__node__iter__create(__num0());
                {
                    node__iter__t loc__i;
    loc__i.is_end = (bool)___ivy_choose(2,"loc:i",68);
                    {
                        loc__i = loc__0;
                        while(!loc__i.is_end){
                            {
                                ext__nset__arr__append(nset__all.repr, loc__i.val);
                                struct __thunk__4 : thunk<a::nset,int>{
    hash_thunk<a::nset,int> nset__card;
    a::node__iter__t loc__i;
bool nset__arr__value(const a::nset__arr& a, int i, const a::node& v){
                                        bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
                                        val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
                                        return val;
}
bool nset__member(const a::node& N, const a::nset& S){
                                        bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
                                        int __tmp3;
                                        __tmp3 = 0;
                                        for (int I = 0; I < nset__arr__end(S.repr); I++) {
                                            if ((((0 < I) || (0 == I)) && (I < nset__arr__end(S.repr)) && nset__arr__value(S.repr,I,N))) __tmp3 = 1;
                                        }
                                        val = __tmp3;
                                        return val;
}
int nset__arr__end(const a::nset__arr& a){
                                        int val;
    val = (int)___ivy_choose(0,"ret:val",0);
                                        val =  a.size() ;
                                        return val;
}
                                    __thunk__4(hash_thunk<a::nset,int> nset__card,a::node__iter__t loc__i) : nset__card(nset__card),loc__i(loc__i){
                                    }
                                    int operator()(const a::nset &arg){
                                        return (nset__member(loc__i.val,arg) ? (nset__card[arg] + 1) : nset__card[arg]);
                                    }
                                };
                                nset__card = hash_thunk<a::nset,int>(new __thunk__4(nset__card,loc__i));
                                ext__node__iter__next(loc__i);
                            }
                        }
                    }
                }
            }
        }
    }
}
}
a::~a(){
    __lock(); // otherwise, thread may die holding lock!
    for (unsigned i = 0; i < thread_ids.size(); i++){
        pthread_cancel(thread_ids[i]);
        pthread_join(thread_ids[i],NULL);
    }
    __unlock();
}
std::ostream &operator <<(std::ostream &s, const a::node__iter__t &t){
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
void  __ser<a::node__iter__t>(ivy_ser &res, const a::node__iter__t&t){
    res.open_struct();
    res.open_field("is_end");
    __ser<bool>(res,t.is_end);
    res.close_field();
    res.open_field("val");
    __ser<a::node>(res,t.val);
    res.close_field();
    res.close_struct();
}
std::ostream &operator <<(std::ostream &s, const a::nset &t){
    s<<"{";
    s<< "repr:";
    s << t.repr;
    s<<"}";
    return s;
}
template <>
void  __ser<a::nset>(ivy_ser &res, const a::nset&t){
    res.open_struct();
    res.open_field("repr");
    __ser<a::nset__arr>(res,t.repr);
    res.close_field();
    res.close_struct();
}
template <>
a::node__iter__t _arg<a::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, int bound){
    a::node__iter__t res;
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
        res.val = _arg<a::node>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field val: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<a::node__iter__t>(ivy_deser &inp, a::node__iter__t &res){
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
a::nset _arg<a::nset>(std::vector<ivy_value> &args, unsigned idx, int bound){
    a::nset res;
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
        res.repr = _arg<a::nset__arr>(tmp_args,0,0);
;
    }
    catch(const out_of_bounds &err){
        throw out_of_bounds("in field repr: " + err.txt,err.pos);
    }
    return res;
}
template <>
void __deser<a::nset>(ivy_deser &inp, a::nset &res){
    inp.open_struct();
    inp.open_field("repr");
    __deser(inp,res.repr);
    inp.close_field();
    inp.close_struct();
}
