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
typedef raft ivy_class;
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
    void raft::__lock() { WaitForSingleObject(mutex,INFINITE); }
    void raft::__unlock() { ReleaseMutex(mutex); }
#else
    void raft::__lock() { pthread_mutex_lock(&mutex); }
    void raft::__unlock() { pthread_mutex_unlock(&mutex); }
#endif
struct thunk__net__impl__handle_recv{
    raft *__ivy;
    thunk__net__impl__handle_recv(raft *__ivy): __ivy(__ivy){}
    void operator()(raft::msg x) const {
        __ivy->net__impl__handle_recv(x);
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
//            if (inp[pos] == '"')
//                throw deser_err();
            res.push_back(inp[pos++]);
        }
        if(!(pos < inp.size() && inp[pos] == 0))
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

std::ostream &operator <<(std::ostream &s, const raft::msgkind &t);
template <>
raft::msgkind _arg<raft::msgkind>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<raft::msgkind>(ivy_ser &res, const raft::msgkind&);
template <>
void  __deser<raft::msgkind>(ivy_deser &inp, raft::msgkind &res);
std::ostream &operator <<(std::ostream &s, const raft::hist &t);
template <>
raft::hist _arg<raft::hist>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<raft::hist>(ivy_ser &res, const raft::hist&);
template <>
void  __deser<raft::hist>(ivy_deser &inp, raft::hist &res);
std::ostream &operator <<(std::ostream &s, const raft::hist__ent &t);
template <>
raft::hist__ent _arg<raft::hist__ent>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<raft::hist__ent>(ivy_ser &res, const raft::hist__ent&);
template <>
void  __deser<raft::hist__ent>(ivy_deser &inp, raft::hist__ent &res);
std::ostream &operator <<(std::ostream &s, const raft::msg &t);
template <>
raft::msg _arg<raft::msg>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<raft::msg>(ivy_ser &res, const raft::msg&);
template <>
void  __deser<raft::msg>(ivy_deser &inp, raft::msg &res);
std::ostream &operator <<(std::ostream &s, const raft::node__iter__t &t);
template <>
raft::node__iter__t _arg<raft::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, int bound);
template <>
void  __ser<raft::node__iter__t>(ivy_ser &res, const raft::node__iter__t&);
template <>
void  __deser<raft::node__iter__t>(ivy_deser &inp, raft::node__iter__t &res);
std::ostream &operator <<(std::ostream &s, const raft::nset &t);
template <>
raft::nset _arg<raft::nset>(std::vector<ivy_value> &args, unsigned idx, int bound);
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
	    raft::hist__arrlog _arg<raft::hist__arrlog>(std::vector<ivy_value> &args, unsigned idx, int bound) {
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

	        void udp_config::get(int id, unsigned long &inetaddr, unsigned long &inetport) {
#ifdef _WIN32
	        inetaddr = ntohl(inet_addr("127.0.0.1")); // can't send to INADDR_ANY in windows
#else
	        inetaddr = INADDR_ANY;
#endif
	        inetport = 4990+ id;
        }
	class udp_reader : public reader {
	    int sock;
	    int my_id;
	    thunk__net__impl__handle_recv rcb;
	    ivy_class *ivy;
	    udp_config *conf;
	    bool bound;
	  public:
	    udp_reader(int _my_id, thunk__net__impl__handle_recv rcb, ivy_class *ivy)
	        : my_id(_my_id), rcb(rcb), ivy(ivy), conf(0), bound(false) {
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0)
		    { std::cerr << "cannot create socket\n"; exit(1); }

            }
            void bind_int() {
                if (!bound) {
                    struct sockaddr_in myaddr;
                    get_addr(my_id,myaddr);
//                    std::cout << "binding id: " << my_id << " port: " << ntohs(myaddr.sin_port) << std::endl;
                    if (::bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
                        { std::cerr << "bind failed\n"; exit(1); }
                }
                bound = true;
            }
                    
	    virtual void bind() {
	        ivy -> __lock();  // can be asynchronous, so must lock ivy!
                bind_int();
	        ivy -> __unlock();
	    }
	    virtual ~udp_reader() {
#ifdef _WIN32
                closesocket(sock);
#else
	        close(sock);
#endif
	    }
	    virtual void get_addr(int my_id, sockaddr_in &myaddr) {
		memset((char *)&myaddr, 0, sizeof(myaddr));
		unsigned long inetaddr;
		unsigned long inetport;
	        if (!conf) {
	            conf = ivy -> get_udp_config();
                }
		conf -> get(my_id,inetaddr,inetport);
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(inetaddr);
		myaddr.sin_port = htons(inetport);
	    }

	    virtual int fdes() {
		return sock;
	    }
	    virtual void read() {
		//std::cout << "RECEIVING\n";
	        int len=0;
                socklen_t lenlen=4;
#ifdef _WIN32
	        if (getsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char *)&len,&lenlen))
#else
	        if (getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&len,&lenlen))
#endif
	            { perror("getsockopt failed"); exit(1); }
	        std::vector<char> buf(len);
	        int bytes;
		if ((bytes = recvfrom(sock,&buf[0],len,0,0,0)) < 0)
		    { std::cerr << "recvfrom failed\n"; exit(1); }
             //   if (pkt.m_kind != raft::keepalive) {
            std::cout << "RECEIVING: ";
            for (int abc=0; abc < bytes; abc++) 
                std::cout << unsigned(buf[abc]) << ",";
            std::cout << std::endl;
        //}

	        buf.resize(bytes);
	        raft::msg pkt;
	        try {
		    ivy_binary_deser ds(buf);
		    __deser(ds,pkt);
	            if (ds.pos < buf.size())
	                throw deser_err();
                } catch (deser_err &){
		    std::cout << "BAD PACKET RECEIVED\n";
		    return;
		}
		ivy->__lock();
		rcb(pkt);
		ivy->__unlock();
	    }
	    virtual void write(int dst, raft::msg pkt) {
	        bind_int();
		struct sockaddr_in dstaddr;
		get_addr(dst,dstaddr);
		ivy_binary_ser sr;
	        __ser(sr,pkt);
        if (pkt.m_kind != raft::keepalive) {
    		std::cout << "SENDING: ";
            for (int abc=0; abc < sr.res.size(); abc++) 
                std::cout << unsigned(sr.res[abc]) << ",";
            std::cout << std::endl;
        }
		if (sendto(sock,&sr.res[0],sr.res.size(),0,(sockaddr *)&dstaddr,sizeof(sockaddr_in)) < 0) 
#ifdef _WIN32
		     { std::cerr << "sendto failed " << WSAGetLastError() << "\n"; exit(1); }
#else
		     { std::cerr << "sendto failed\n"; exit(1); }
#endif
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
	    raft::replierslog _arg<raft::replierslog>(std::vector<ivy_value> &args, unsigned idx, int bound) {
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
    val = (bool)___ivy_choose(2,"ret:val",0);
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
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = (nset__card[nset__all] < (nset__card[S] + nset__card[S]));
    return val;
}
bool raft::hist__logtix(const hist& H, int IX, int LOGT){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = (hist__filled(H,IX) && (hist__arrlog__value(H.repr,IX).ent_logt == LOGT));
    return val;
}
bool raft::hist__filled(const hist& H, int IX){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = (IX < hist__arrlog__end(H.repr));
    return val;
}
__strlit raft::hist__valix(const hist& H, int IX){
    __strlit val;
    val = hist__arrlog__value(H.repr,IX).ent_value;
    return val;
}
bool raft::localstate__is_leader(){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = localstate___is_leader;
    return val;
}
bool raft::localstate__curr_term(int T){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = (localstate__term_val == T);
    return val;
}
bool raft::localstate__term_bigeq(int T){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = ((T < localstate__term_val) || (localstate__term_val == T));
    return val;
}
bool raft::localstate__commited(int IX){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
    val = (localstate__has_commits && ((IX < localstate__last_commited_ix) || (IX == localstate__last_commited_ix)));
    return val;
}
raft::node raft::__num0(){
    node val;
    val =  0 ;
    return val;
}
bool raft::nset__arr__value(const nset__arr& a, int i, const node& v){
    bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
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
    hist__ent val;
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
    nset val;
    val =  (0 <= i && i < a.size()) ? a[i] : val ;
    return val;
}
int raft::replierslog__end(const replierslog& a){
    int val;
    val = (int)___ivy_choose(0,"ret:val",0);
    val =  a.size() ;
    return val;
}
raft::hist raft::ext__hist__clear(){
    hist h;
    {
        h.repr = ext__hist__arrlog__empty();
    }
    return h;
}
void raft::ext__nset__add(nset& s, const node& n){
        {
        if(!nset__member(n,s)){
            ext__nset__arr__append(s.repr, n);
        }
    }
}
void raft::ext__localstate__add_vote(const node& voter){
    {
        ext__nset__add(localstate__my_voters, voter);
    }
}
void raft::imp__shim__recv_debug(const msg& m){
    {
    }
}
raft::nset raft::ext__localstate__get_voters(){
    nset voters;
    {
        voters = localstate__my_voters;
    }
    return voters;
}
int raft::ext__hist__get_logt(const hist& h, int ix){
    int logt;
    logt = (int)___ivy_choose(0,"fml:logt",0);
    {
        logt = hist__arrlog__value(h.repr,ix).ent_logt;
    }
    return logt;
}
raft::node__iter__t raft::ext__node__iter__end(){
    node__iter__t y;
    y.is_end = (bool)___ivy_choose(2,"fml:y",0);
    {
        y.is_end = true;
        y.val = __num0();
    }
    return y;
}
bool raft::ext__system__req_replicate_entry_from_log(int ix, bool isrecover, const node& recovernode){
    bool rejected;
    rejected = (bool)___ivy_choose(2,"fml:rejected",0);
    {
        {
            int loc__previx;
    loc__previx = (int)___ivy_choose(0,"loc:previx",1415);
            {
                {
                    int loc__prevt;
    loc__prevt = (int)___ivy_choose(0,"loc:prevt",1414);
                    {
                        {
                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1413);
                            {
                                loc__0 = ext__localstate__get_term();
                                {
                                    int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1412);
                                    {
                                        loc__t = loc__0;
                                        {
                                            __strlit loc__v;
                                            {
                                                loc__v = hist__valix(localstate__myhist,ix);
                                                {
                                                    int loc__previx;
    loc__previx = (int)___ivy_choose(0,"loc:previx",1410);
                                                    {
                                                        {
                                                            int loc__logt;
    loc__logt = (int)___ivy_choose(0,"loc:logt",1409);
                                                            {
                                                                if((localstate__is_leader() && hist__filled(localstate__myhist,ix))){
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
    loc__0.m_kind = (msgkind)___ivy_choose(7,"loc:0",1408);
    loc__0.m_ix = (int)___ivy_choose(0,"loc:0",1408);
    loc__0.m_term = (int)___ivy_choose(0,"loc:0",1408);
    loc__0.m_logt = (int)___ivy_choose(0,"loc:0",1408);
    loc__0.m_prevlogt = (int)___ivy_choose(0,"loc:0",1408);
    loc__0.m_isrecover = (bool)___ivy_choose(2,"loc:0",1408);
                                                                            {
                                                                                loc__0 = ext__msg_append_send(loc__t, loc__v, loc__logt, ix, loc__prevt, isrecover, recovernode);
                                                                                {
                                                                                    msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1407);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1407);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1407);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1407);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1407);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1407);
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
        }
    }
    return rejected;
}
void raft::ext__sec__timeout(){
    {
        if(!localstate__is_leader()){
            {
                ext__localstate__increase_time();
                {
                    bool loc__0;
    loc__0 = (bool)___ivy_choose(2,"loc:0",1416);
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
    loc__0 = (int)___ivy_choose(0,"loc:0",1417);
                {
                    loc__0 = ext__localstate__get_term();
                    ext__send_keepalive(loc__0);
                }
            }
        }
    }
}
void raft::ext__hist__strip(hist& h, int ix){
        {
        {
            hist__ent loc__dummy_ent;
    loc__dummy_ent.ent_logt = (int)___ivy_choose(0,"loc:dummy_ent",1419);
            {
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1418);
                    {
                        loc__0 = ext__index__next(ix);
                        ext__hist__arrlog__resize(h.repr, loc__0, loc__dummy_ent);
                    }
                }
            }
        }
    }
}
bool raft::ext__localstate__is_leader_too_quiet(){
    bool res;
    res = (bool)___ivy_choose(2,"fml:res",0);
    {
        res = ((2 < (localstate__mytime - localstate__last_heard_from_leader)) || ((localstate__mytime - localstate__last_heard_from_leader) == 2));
    }
    return res;
}
void raft::ext__node__iter__next(node__iter__t& x){
        {
        {
            bool loc__0;
    loc__0 = (bool)___ivy_choose(2,"loc:0",1404);
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
void raft::ext__safetyproof__update_term_hist_ghost(int t, const hist& h){
    {
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
void raft::ext__shim__recv_debug(const msg& m){
    imp__shim__recv_debug(m);
}
void raft::imp__system__commited_at_ix(int ix, __strlit v){
    {
    }
}
void raft::ext__localstate__become_leader(){
    {
        localstate___is_leader = true;
    }
}
void raft::ext__shim__handle_keepalive(int t){
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1420);
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
    loc__0 = (int)___ivy_choose(0,"loc:0",1421);
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
void raft::ext__shim__broadcast(const msg& m){
    {
        if(!(m.m_kind == keepalive)){
            {
                ext__shim__send_debug(m);
            }
        }
        {
            node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(2,"loc:0",1424);
            {
                loc__0 = ext__node__iter__create(__num0());
                {
                    node__iter__t loc__iter;
    loc__iter.is_end = (bool)___ivy_choose(2,"loc:iter",1423);
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
bool raft::ext__system__req_append_new_entry(__strlit v){
    bool rejected;
    rejected = (bool)___ivy_choose(2,"fml:rejected",0);
    {
        {
            int loc__previx;
    loc__previx = (int)___ivy_choose(0,"loc:previx",1449);
            {
                {
                    int loc__prevt;
    loc__prevt = (int)___ivy_choose(0,"loc:prevt",1448);
                    {
                        {
                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1447);
                            {
                                loc__0 = ext__localstate__get_term();
                                {
                                    int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1446);
                                    {
                                        loc__t = loc__0;
                                        {
                                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1445);
                                            {
                                                loc__0 = ext__hist__get_next_ix(localstate__myhist);
                                                {
                                                    int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1444);
                                                    {
                                                        loc__ix = loc__0;
                                                        if(localstate__is_leader()){
                                                            {
                                                                ext__hist__append(localstate__myhist, loc__ix, loc__t, v);
                                                                ext__safetyproof__update_term_hist_ghost(loc__t, localstate__myhist);
                                                                ext__localstate__add_replier(loc__ix, n);
                                                                {
                                                                    bool loc__isrecover;
    loc__isrecover = (bool)___ivy_choose(2,"loc:isrecover",1443);
                                                                    {
                                                                        loc__isrecover = false;
                                                                        {
                                                                            node loc__dummy_recovernode;
                                                                            {
                                                                                rejected = ext__system__req_replicate_entry_from_log(loc__ix, loc__isrecover, loc__dummy_recovernode);
                                                                                ivy_assume(!rejected, "raft.ivy: line 690");
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
void raft::ext__system__announce_candidacy(){
    {
        ext__localstate__increase_term_by_nodeid();
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1430);
            {
                loc__0 = ext__localstate__get_term();
                {
                    int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1429);
                    {
                        loc__t = loc__0;
                        if(hist__filled(localstate__myhist,0)){
                            {
                                {
                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1428);
                                    int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1428);
                                    {
                                        loc__0 = ext__hist__get_next_ix(localstate__myhist);
                                        loc__1 = ext__index__prev(loc__0);
                                        {
                                            int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1427);
                                            {
                                                loc__ix = loc__1;
                                                {
                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1426);
                                                    {
                                                        loc__0 = ext__hist__get_logt(localstate__myhist, loc__ix);
                                                        {
                                                            int loc__logt;
    loc__logt = (int)___ivy_choose(0,"loc:logt",1425);
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
__strlit raft::ext__system__get_value(int i){
    __strlit v;
    v = hist__valix(localstate__myhist,i);
    return v;
}
void raft::ext__send_appended_reply_msg(const node& leader, int t, const node& n, int ix, bool isrecover){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1431);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1431);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1431);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1431);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1431);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1431);
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
int raft::ext__localstate__get_term(){
    int t;
    t = (int)___ivy_choose(0,"fml:t",0);
    {
        t = localstate__term_val;
    }
    return t;
}
void raft::ext__shim__handle_appended_reply_msg(int t, const node& replier, int ix, bool isrecover){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(2,"loc:ok",1441);
            {
                loc__ok = true;
                {
                    int loc__nextreportix;
    loc__nextreportix = (int)___ivy_choose(0,"loc:nextreportix",1440);
                    {
                        loc__nextreportix = ix;
                        loc__ok = (loc__ok && localstate__is_leader());
                        if((loc__ok && isrecover && hist__filled(localstate__myhist,0))){
                            {
                                int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1436);
                                int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1436);
                                {
                                    loc__0 = ext__hist__get_next_ix(localstate__myhist);
                                    loc__1 = ext__index__prev(loc__0);
                                    {
                                        int loc__lastusedix;
    loc__lastusedix = (int)___ivy_choose(0,"loc:lastusedix",1435);
                                        {
                                            loc__lastusedix = loc__1;
                                            if((ix < loc__lastusedix)){
                                                {
                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1434);
                                                    bool loc__1;
    loc__1 = (bool)___ivy_choose(2,"loc:1",1434);
                                                    {
                                                        loc__0 = ext__index__next(ix);
                                                        loc__1 = ext__system__req_replicate_entry_from_log(loc__0, isrecover, replier);
                                                        {
                                                            bool loc__rejected;
    loc__rejected = (bool)___ivy_choose(2,"loc:rejected",1433);
                                                            {
                                                                loc__rejected = loc__1;
                                                                ivy_assume(!loc__rejected, "raft.ivy: line 838");
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
    loc__0 = (int)___ivy_choose(0,"loc:0",1437);
                            {
                                loc__0 = ext__localstate__get_term();
                                loc__ok = (loc__ok && (loc__0 == t));
                            }
                        }
                        loc__ok = (loc__ok && hist__logtix(localstate__myhist,ix,t));
                        if(localstate__commited(0)){
                            {
                                loc__nextreportix = ext__localstate__get_last_commited_ix();
                                loc__ok = (loc__ok && (loc__nextreportix < ix));
                                loc__nextreportix = ext__index__next(loc__nextreportix);
                            }
                        }
                        else {
                            {
                                loc__nextreportix = 0;
                            }
                        }
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
                                                ext__localstate__set_last_commited_ix(ix);
                                                ext__system__report_commits(loc__nextreportix, ix);
                                                ext__safetyproof__update_commited_ghost(localstate__myhist, ix, t, loc__repliers);
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
    loc__ok = (bool)___ivy_choose(2,"loc:ok",1432);
            {
                loc__ok = !hist__filled(localstate__myhist,0);
                loc__ok = (loc__ok && !localstate__term_bigeq(t));
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
void raft::ext__net__send(const node& dst, const msg& v){
    {
        net__impl__rdr->write(dst,v);
    }
}
raft::node raft::ext__node__next(const node& x){
    node y;
    {
        y = x + 1;
    }
    return y;
}
void raft::ext__system__report_commits(int firstix, int lastix){
    {
        int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1452);
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
void raft::ext__localstate__increase_term_by_nodeid(){
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1453);
            int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1453);
            int loc__2;
    loc__2 = (int)___ivy_choose(0,"loc:2",1453);
            {
                loc__0 = ext__typeconvert__from_nodeid_to_term(n);
                loc__1 = ext__term__add(localstate__term_val, 1);
                loc__2 = ext__term__add(loc__1, loc__0);
                ext__localstate__move_to_term(loc__2);
            }
        }
    }
}
raft::nset__arr raft::ext__nset__arr__empty(){
    nset__arr a;
    {
        
    }
    return a;
}
void raft::imp__shim__send_debug(const msg& m){
    {
    }
}
void raft::ext__safetyproof__update_commited_ghost(const hist& h, int ix, int t, const nset& q_append){
    {
    }
}
int raft::ext__term__add(int x, int y){
    int z;
    z = (int)___ivy_choose(0,"fml:z",0);
    {
        z = (x + y);
    }
    return z;
}
int raft::ext__localstate__get_last_commited_ix(){
    int ix;
    ix = (int)___ivy_choose(0,"fml:ix",0);
    {
        ix = localstate__last_commited_ix;
    }
    return ix;
}
void raft::ext__localstate__set_last_commited_ix(int ix){
    {
        localstate__last_commited_ix = ix;
        localstate__has_commits = true;
    }
}
void raft::ext__hist__append(hist& h, int ix, int logt, __strlit v){
        {
        {
            hist__ent loc__new_ent;
    loc__new_ent.ent_logt = (int)___ivy_choose(0,"loc:new_ent",1451);
            {
                loc__new_ent.ent_logt = logt;
                loc__new_ent.ent_value = v;
                {
                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1450);
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
void raft::ext__safetyproof__set_leader_ghost(int t, const node& leader, const nset& voters){
    {
    }
}
raft::nset raft::ext__replierslog__get(const replierslog& a, int x){
    nset y;
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
void raft::ext__shim__handle_vote_cand_msg(const node& cand, int t, const node& voter){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(2,"loc:ok",1487);
            {
                loc__ok = true;
                loc__ok = (loc__ok && (n == cand));
                loc__ok = (loc__ok && localstate__curr_term(t));
                loc__ok = (loc__ok && !localstate__is_leader());
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
bool raft::ext__node__is_max(const node& x){
    bool r;
    r = (bool)___ivy_choose(2,"fml:r",0);
    {
        r = (x == node__size - 1);
    }
    return r;
}
raft::node__iter__t raft::ext__node__iter__create(const node& x){
    node__iter__t y;
    y.is_end = (bool)___ivy_choose(2,"fml:y",0);
    {
        y.is_end = false;
        y.val = x;
    }
    return y;
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
    nset repliers;
    {
        if((ix < replierslog__end(localstate__my_repliers))){
            {
                repliers = ext__replierslog__get(localstate__my_repliers, ix);
            }
        }
        else {
            {
                repliers = nset__emptyset;
            }
        }
    }
    return repliers;
}
raft::hist__arrlog raft::ext__hist__arrlog__empty(){
    hist__arrlog a;
    {
        
    }
    return a;
}
raft::replierslog raft::ext__replierslog__empty(){
    replierslog a;
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
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1455);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1455);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1455);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1455);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1455);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1455);
            {
                loc__m.m_kind = keepalive;
                loc__m.m_term = t;
                ext__shim__broadcast(loc__m);
            }
        }
    }
}
raft::msg raft::ext__msg_append_send(int t, __strlit v, int logt, int ix, int prevlogt, bool isrecover, const node& recovernode){
    msg m;
    m.m_kind = (msgkind)___ivy_choose(7,"fml:m",0);
    m.m_ix = (int)___ivy_choose(0,"fml:m",0);
    m.m_term = (int)___ivy_choose(0,"fml:m",0);
    m.m_logt = (int)___ivy_choose(0,"fml:m",0);
    m.m_prevlogt = (int)___ivy_choose(0,"fml:m",0);
    m.m_isrecover = (bool)___ivy_choose(2,"fml:m",0);
    {
        m.m_kind = appentry;
        m.m_term = t;
        m.m_val = v;
        m.m_logt = logt;
        m.m_node = n;
        m.m_ix = ix;
        m.m_prevlogt = prevlogt;
        m.m_isrecover = isrecover;
        if(isrecover){
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
void raft::ext__shim__handle_nack(const node& n, int t, int ix){
    {
        {
            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1459);
            {
                loc__0 = ext__localstate__get_term();
                if(((loc__0 == t) && localstate__is_leader())){
                    {
                        bool loc__isrecover;
    loc__isrecover = (bool)___ivy_choose(2,"loc:isrecover",1458);
                        {
                            loc__isrecover = true;
                            {
                                bool loc__0;
    loc__0 = (bool)___ivy_choose(2,"loc:0",1457);
                                {
                                    loc__0 = ext__system__req_replicate_entry_from_log(ix, loc__isrecover, n);
                                    {
                                        bool loc__rejected;
    loc__rejected = (bool)___ivy_choose(2,"loc:rejected",1456);
                                        {
                                            loc__rejected = loc__0;
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
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1460);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1460);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1460);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1460);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1460);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1460);
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
    val = (bool)___ivy_choose(2,"ret:val",0);
                                            val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
                                            return val;
}
bool nset__member(const raft::node& N, const raft::nset& S){
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
void raft::__init(){
    {
        {
            nset__card = hash_thunk<raft::nset,int>(new __thunk__0());
            nset__all.repr = ext__nset__arr__empty();
            nset__emptyset.repr = ext__nset__arr__empty();
            {
                node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(2,"loc:0",1489);
                {
                    loc__0 = ext__node__iter__create(__num0());
                    {
                        node__iter__t loc__i;
    loc__i.is_end = (bool)___ivy_choose(2,"loc:i",1488);
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
            localstate__myhist = hist__empty;
        }
        {
            localstate__my_voters = nset__emptyset;
            localstate__my_repliers = ext__replierslog__empty();
            localstate___is_leader = false;
            localstate__term_val = 0;
            localstate__last_heard_from_leader = 0;
            localstate__mytime = 0;
            localstate__has_commits = false;
        }
    }
}
void raft::net__impl__handle_recv(const msg& x){
    ext__net__recv(x);
}
void raft::ext__localstate__move_to_term(int t){
    {
        localstate__term_val = t;
        localstate__my_voters = nset__emptyset;
        localstate__my_repliers = ext__replierslog__empty();
        localstate___is_leader = false;
    }
}
void raft::ext__send_rqst_vote_msg(const node& cand, int logt, int ix, int t){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1461);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1461);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1461);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1461);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1461);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1461);
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
void raft::ext__hist__arrlog__resize(hist__arrlog& a, int s, const hist__ent& v){
        {

        unsigned __old_size = a.size();
        a.resize(s);
        for (unsigned i = __old_size; i < (unsigned)s; i++)
            a[i] = v;
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
void raft::ext__send_nack(const node& leader, int t, int ix){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1464);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1464);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1464);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1464);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1464);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1464);
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
    loc__0 = (int)___ivy_choose(0,"loc:0",1465);
                {
                    loc__0 = ext__index__next(ix);
                    ext__replierslog__resize(localstate__my_repliers, loc__0, nset__emptyset);
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
void raft::ext__shim__handle_append_entries(const msg& m){
    {
        {
            int loc__t;
    loc__t = (int)___ivy_choose(0,"loc:t",1479);
            {
                loc__t = m.m_term;
                {
                    int loc__ix;
    loc__ix = (int)___ivy_choose(0,"loc:ix",1478);
                    {
                        loc__ix = m.m_ix;
                        {
                            int loc__logt;
    loc__logt = (int)___ivy_choose(0,"loc:logt",1477);
                            {
                                loc__logt = m.m_logt;
                                {
                                    __strlit loc__v;
                                    {
                                        loc__v = m.m_val;
                                        {
                                            int loc__prevt;
    loc__prevt = (int)___ivy_choose(0,"loc:prevt",1475);
                                            {
                                                loc__prevt = m.m_prevlogt;
                                                {
                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1474);
                                                    {
                                                        loc__0 = ext__localstate__get_term();
                                                        {
                                                            int loc__node_term;
    loc__node_term = (int)___ivy_choose(0,"loc:node_term",1473);
                                                            {
                                                                loc__node_term = loc__0;
                                                                {
                                                                    bool loc__ok;
    loc__ok = (bool)___ivy_choose(2,"loc:ok",1472);
                                                                    {
                                                                        loc__ok = true;
                                                                        loc__ok = (loc__ok && !localstate__is_leader());
                                                                        loc__ok = (loc__ok && ((loc__node_term < loc__t) || (loc__node_term == loc__t)));
                                                                        if((!(loc__ix == 0) && loc__ok)){
                                                                            {
                                                                                {
                                                                                    int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1469);
                                                                                    {
                                                                                        loc__0 = ext__index__prev(loc__ix);
                                                                                        {
                                                                                            bool loc__has_previous;
    loc__has_previous = (bool)___ivy_choose(2,"loc:has_previous",1468);
                                                                                            {
                                                                                                loc__has_previous = hist__logtix(localstate__myhist,loc__0,loc__prevt);
                                                                                                if(!loc__has_previous){
                                                                                                    {
                                                                                                        {
                                                                                                            int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1467);
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
    loc__0 = (int)___ivy_choose(0,"loc:0",1471);
                                                                                                    {
                                                                                                        loc__0 = ext__index__prev(loc__ix);
                                                                                                        {
                                                                                                            int loc__previx;
    loc__previx = (int)___ivy_choose(0,"loc:previx",1470);
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
                                                                                ext__localstate__move_to_term(loc__t);
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
void raft::ext__send_vote_cand_msg(const node& n, int t, const node& cand){
    {
        {
            msg loc__m;
    loc__m.m_kind = (msgkind)___ivy_choose(7,"loc:m",1454);
    loc__m.m_ix = (int)___ivy_choose(0,"loc:m",1454);
    loc__m.m_term = (int)___ivy_choose(0,"loc:m",1454);
    loc__m.m_logt = (int)___ivy_choose(0,"loc:m",1454);
    loc__m.m_prevlogt = (int)___ivy_choose(0,"loc:m",1454);
    loc__m.m_isrecover = (bool)___ivy_choose(2,"loc:m",1454);
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
void raft::ext__shim__handle_rqst_vote(int logt, int ix, int t, const node& cand){
    {
        {
            bool loc__ok;
    loc__ok = (bool)___ivy_choose(2,"loc:ok",1484);
            {
                loc__ok = true;
                if(hist__filled(localstate__myhist,0)){
                    {
                        int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1483);
                        int loc__1;
    loc__1 = (int)___ivy_choose(0,"loc:1",1483);
                        {
                            loc__0 = ext__hist__get_next_ix(localstate__myhist);
                            loc__1 = ext__index__prev(loc__0);
                            {
                                int loc__lastix;
    loc__lastix = (int)___ivy_choose(0,"loc:lastix",1482);
                                {
                                    loc__lastix = loc__1;
                                    {
                                        int loc__0;
    loc__0 = (int)___ivy_choose(0,"loc:0",1481);
                                        {
                                            loc__0 = ext__hist__get_logt(localstate__myhist, loc__lastix);
                                            {
                                                int loc__lastlogt;
    loc__lastlogt = (int)___ivy_choose(0,"loc:lastlogt",1480);
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
                loc__ok = (loc__ok && !localstate__term_bigeq(t));
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
    __CARD__nset__index = 0;
    __CARD__term = 0;
    __CARD__value = 0;
    __CARD__localstate__time = 0;
    this->node__size = node__size;
    localstate__mytime = (int)___ivy_choose(0,"init",0);
    localstate__last_commited_ix = (int)___ivy_choose(0,"init",0);
    localstate__term_val = (int)___ivy_choose(0,"init",0);
    localstate___is_leader = (bool)___ivy_choose(2,"init",0);
    this->n = n;
    localstate__has_commits = (bool)___ivy_choose(2,"init",0);
struct __thunk__2 : thunk<raft::nset,int>{
    __thunk__2()  {
    }
    int operator()(const raft::nset &arg){
        int __tmp2;
    __tmp2 = (int)___ivy_choose(0,"init",0);
        return __tmp2;
    }
};
nset__card = hash_thunk<raft::nset,int>(new __thunk__2());
    localstate__last_heard_from_leader = (int)___ivy_choose(0,"init",0);
{
    {
        struct __thunk__3 : thunk<raft::nset,int>{
            __thunk__3()  {
            }
            int operator()(const raft::nset &arg){
                return 0;
            }
        };
        nset__card = hash_thunk<raft::nset,int>(new __thunk__3());
        nset__all.repr = ext__nset__arr__empty();
        nset__emptyset.repr = ext__nset__arr__empty();
        {
            node__iter__t loc__0;
    loc__0.is_end = (bool)___ivy_choose(2,"loc:0",1489);
            {
                loc__0 = ext__node__iter__create(__num0());
                {
                    node__iter__t loc__i;
    loc__i.is_end = (bool)___ivy_choose(2,"loc:i",1488);
                    {
                        loc__i = loc__0;
                        while(!loc__i.is_end){
                            {
                                ext__nset__arr__append(nset__all.repr, loc__i.val);
                                struct __thunk__4 : thunk<raft::nset,int>{
    hash_thunk<raft::nset,int> nset__card;
    raft::node__iter__t loc__i;
bool nset__arr__value(const raft::nset__arr& a, int i, const raft::node& v){
                                        bool val;
    val = (bool)___ivy_choose(2,"ret:val",0);
                                        val =  (0 <= i && i < a.size()) ? a[i] == v: false ;
                                        return val;
}
bool nset__member(const raft::node& N, const raft::nset& S){
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
int nset__arr__end(const raft::nset__arr& a){
                                        int val;
    val = (int)___ivy_choose(0,"ret:val",0);
                                        val =  a.size() ;
                                        return val;
}
                                    __thunk__4(hash_thunk<raft::nset,int> nset__card,raft::node__iter__t loc__i) : nset__card(nset__card),loc__i(loc__i){
                                    }
                                    int operator()(const raft::nset &arg){
                                        return (nset__member(loc__i.val,arg) ? (nset__card[arg] + 1) : nset__card[arg]);
                                    }
                                };
                                nset__card = hash_thunk<raft::nset,int>(new __thunk__4(nset__card,loc__i));
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
        localstate__myhist = hist__empty;
    }
    {
        localstate__my_voters = nset__emptyset;
        localstate__my_repliers = ext__replierslog__empty();
        localstate___is_leader = false;
        localstate__term_val = 0;
        localstate__last_heard_from_leader = 0;
        localstate__mytime = 0;
        localstate__has_commits = false;
    }
}
    the_udp_config = 0;
    install_reader(net__impl__rdr = new udp_reader(n,thunk__net__impl__handle_recv(this), this));
    install_timer(sec__impl__tmr = new sec_timer(thunk__sec__impl__handle_timeout(this),this));
}
raft::~raft(){
    __lock(); // otherwise, thread may die holding lock!
    for (unsigned i = 0; i < thread_ids.size(); i++){
        pthread_cancel(thread_ids[i]);
        pthread_join(thread_ids[i],NULL);
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


int ask_ret(int bound) {
    int res;
    while(true) {
        __ivy_out << "? ";
        std::cin >> res;
        if (res >= 0 && res < bound) 
            return res;
        std::cerr << "value out of range" << std::endl;
    }
}



    class raft_repl : public raft {

    public:

    virtual void ivy_assert(bool truth,const char *msg){
        if (!truth) {
            __ivy_out << "assertion_failed(\"" << msg << "\")" << std::endl;
            std::cerr << msg << ": error: assertion failed\n";
            
            __ivy_exit(1);
        }
    }
    virtual void ivy_assume(bool truth,const char *msg){
        if (!truth) {
            __ivy_out << "assumption_failed(\"" << msg << "\")" << std::endl;
            std::cerr << msg << ": error: assumption failed\n";
            
            __ivy_exit(1);
        }
    }
    raft_repl(node node__size, node n) : raft(node__size,n){}
    virtual void imp__shim__recv_debug(const msg& m){
    __ivy_out << "< shim.recv_debug" << "(" << m << ")" << std::endl;
}
    virtual void imp__system__commited_at_ix(int ix, __strlit v){
    __ivy_out << "< system.commited_at_ix" << "(" << ix << "," << v << ")" << std::endl;
}
    virtual void imp__shim__send_debug(const msg& m){
    __ivy_out << "< shim.send_debug" << "(" << m << ")" << std::endl;
}

    };

// Override methods to implement low-level network service

bool is_white(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool is_ident(int c) {
    return c == '_' || c == '.' || (c >= 'A' &&  c <= 'Z')
        || (c >= 'a' &&  c <= 'z')
        || (c >= '0' &&  c <= '9');
}

void skip_white(const std::string& str, int &pos){
    while (pos < str.size() && is_white(str[pos]))
        pos++;
}

struct syntax_error {
    int pos;
    syntax_error(int pos) : pos(pos) {}
};

void throw_syntax(int pos){
    throw syntax_error(pos);
}

std::string get_ident(const std::string& str, int &pos) {
    std::string res = "";
    while (pos < str.size() && is_ident(str[pos])) {
        res.push_back(str[pos]);
        pos++;
    }
    if (res.size() == 0)
        throw_syntax(pos);
    return res;
}

ivy_value parse_value(const std::string& cmd, int &pos) {
    ivy_value res;
    res.pos = pos;
    skip_white(cmd,pos);
    if (pos < cmd.size() && cmd[pos] == '[') {
        while (true) {
            pos++;
            skip_white(cmd,pos);
            if (pos < cmd.size() && cmd[pos] == ']')
                break;
            res.fields.push_back(parse_value(cmd,pos));
            skip_white(cmd,pos);
            if (pos < cmd.size() && cmd[pos] == ']')
                break;
            if (!(pos < cmd.size() && cmd[pos] == ','))
                throw_syntax(pos);
        }
        pos++;
    }
    else if (pos < cmd.size() && cmd[pos] == '{') {
        while (true) {
            ivy_value field;
            pos++;
            skip_white(cmd,pos);
            field.atom = get_ident(cmd,pos);
            skip_white(cmd,pos);
            if (!(pos < cmd.size() && cmd[pos] == ':'))
                 throw_syntax(pos);
            pos++;
            skip_white(cmd,pos);
            field.fields.push_back(parse_value(cmd,pos));
            res.fields.push_back(field);
            skip_white(cmd,pos);
            if (pos < cmd.size() && cmd[pos] == '}')
                break;
            if (!(pos < cmd.size() && cmd[pos] == ','))
                throw_syntax(pos);
        }
        pos++;
    }
    else if (pos < cmd.size() && cmd[pos] == '"') {
        pos++;
        res.atom = "";
        while (pos < cmd.size() && cmd[pos] != '"')
            res.atom.push_back(cmd[pos++]);
        if(pos == cmd.size())
            throw_syntax(pos);
        pos++;
    }
    else 
        res.atom = get_ident(cmd,pos);
    return res;
}

void parse_command(const std::string &cmd, std::string &action, std::vector<ivy_value> &args) {
    int pos = 0;
    skip_white(cmd,pos);
    action = get_ident(cmd,pos);
    skip_white(cmd,pos);
    if (pos < cmd.size() && cmd[pos] == '(') {
        pos++;
        skip_white(cmd,pos);
        args.push_back(parse_value(cmd,pos));
        while(true) {
            skip_white(cmd,pos);
            if (!(pos < cmd.size() && cmd[pos] == ','))
                break;
            pos++;
            args.push_back(parse_value(cmd,pos));
        }
        if (!(pos < cmd.size() && cmd[pos] == ')'))
            throw_syntax(pos);
        pos++;
    }
    skip_white(cmd,pos);
    if (pos != cmd.size())
        throw_syntax(pos);
}

struct bad_arity {
    std::string action;
    int num;
    bad_arity(std::string &_action, unsigned _num) : action(_action), num(_num) {}
};

void check_arity(std::vector<ivy_value> &args, unsigned num, std::string &action) {
    if (args.size() != num)
        throw bad_arity(action,num);
}

template <>
raft::hist _arg<raft::hist>(std::vector<ivy_value> &args, unsigned idx, int bound){
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
raft::hist__ent _arg<raft::hist__ent>(std::vector<ivy_value> &args, unsigned idx, int bound){
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
raft::msg _arg<raft::msg>(std::vector<ivy_value> &args, unsigned idx, int bound){
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
raft::node__iter__t _arg<raft::node__iter__t>(std::vector<ivy_value> &args, unsigned idx, int bound){
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
raft::nset _arg<raft::nset>(std::vector<ivy_value> &args, unsigned idx, int bound){
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
raft::msgkind _arg<raft::msgkind>(std::vector<ivy_value> &args, unsigned idx, int bound){
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


class stdin_reader: public reader {
    std::string buf;
    std::string eof_flag;

public:
    bool eof(){
      return eof_flag.size();
    }
    virtual int fdes(){
        return 0;
    }
    virtual void read() {
        char tmp[257];
        int chars = ::read(0,tmp,256);
        if (chars == 0) {  // EOF
            if (buf.size())
                process(buf);
            eof_flag = "eof";
        }
        tmp[chars] = 0;
        buf += std::string(tmp);
        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0,pos+1);
            buf.erase(0,pos+1);
            process(line);
        }
    }
    virtual void process(const std::string &line) {
        __ivy_out << line;
    }
};

class cmd_reader: public stdin_reader {
    int lineno;
public:
    raft_repl &ivy;    

    cmd_reader(raft_repl &_ivy) : ivy(_ivy) {
        lineno = 1;
        if (isatty(fdes()))
            __ivy_out << "> "; __ivy_out.flush();
    }

    virtual void process(const std::string &cmd) {
        std::string action;
        std::vector<ivy_value> args;
        try {
            parse_command(cmd,action,args);
            ivy.__lock();

                if (action == "system.get_value") {
                    check_arity(args,1,action);
                    __ivy_out << "= " << ivy.ext__system__get_value(_arg<int>(args,0,0)) << std::endl;
                }
                else
    
                if (action == "system.req_append_new_entry") {
                    check_arity(args,1,action);
                    __ivy_out << "= " << ivy.ext__system__req_append_new_entry(_arg<__strlit>(args,0,0)) << std::endl;
                }
                else
    
            {
                std::cerr << "undefined action: " << action << std::endl;
            }
            ivy.__unlock();
        }
        catch (syntax_error& err) {
            std::cerr << "line " << lineno << ":" << err.pos << ": syntax error" << std::endl;
        }
        catch (out_of_bounds &err) {
            std::cerr << "line " << lineno << ":" << err.pos << ": " << err.txt << " bad value" << std::endl;
        }
        catch (bad_arity &err) {
            std::cerr << "action " << err.action << " takes " << err.num  << " input parameters" << std::endl;
        }
        if (isatty(fdes()))
            __ivy_out << "> "; __ivy_out.flush();
        lineno++;
    }
};



int main(int argc, char **argv){
        int test_iters = 100;

    int runs = 1;
    int seed = 1;
    int sleep_ms = 10;
    int final_ms = 0; 
    std::vector<char *> pargs; // positional args
    pargs.push_back(argv[0]);
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        size_t p = arg.find('=');
        if (p == std::string::npos)
            pargs.push_back(argv[i]);
        else {
            std::string param = arg.substr(0,p);
            std::string value = arg.substr(p+1);
            if (param == "out") {
                __ivy_out.open(value.c_str());
                if (!__ivy_out) {
                    std::cerr << "cannot open to write: " << value << std::endl;
                    return 1;
                }
            }
            else if (param == "iters") {
                test_iters = atoi(value.c_str());
            }
            else if (param == "runs") {
                runs = atoi(value.c_str());
            }
            else if (param == "seed") {
                seed = atoi(value.c_str());
            }
            else if (param == "delay") {
                sleep_ms = atoi(value.c_str());
            }
            else if (param == "wait") {
                final_ms = atoi(value.c_str());
            }
            else if (param == "modelfile") {
                __ivy_modelfile.open(value.c_str());
                if (!__ivy_modelfile) {
                    std::cerr << "cannot open to write: " << value << std::endl;
                    return 1;
                }
            }
            else {
                std::cerr << "unknown option: " << param << std::endl;
                return 1;
            }
        }
    }
    srand(seed);
    if (!__ivy_out.is_open())
        __ivy_out.basic_ios<char>::rdbuf(std::cout.rdbuf());
    argc = pargs.size();
    argv = &pargs[0];
    if (argc == 4){
        argc--;
        int fd = _open(argv[argc],0);
        if (fd < 0){
            std::cerr << "cannot open to read: " << argv[argc] << "\n";
            __ivy_exit(1);
        }
        _dup2(fd, 0);
    }
    if (argc != 3){
        std::cerr << "usage: raft node__size n\n";
        __ivy_exit(1);
    }
    std::vector<std::string> args;
    std::vector<ivy_value> arg_values(2);
    for(int i = 1; i < argc;i++){args.push_back(argv[i]);}
    int p__node__size;
    try {
        int pos = 0;
        arg_values[0] = parse_value(args[0],pos);
        p__node__size =  _arg<raft::node>(arg_values,0,0);
    }
    catch(out_of_bounds &) {
        std::cerr << "parameter node__size out of bounds\n";
        __ivy_exit(1);
    }
    catch(syntax_error &) {
        std::cerr << "syntax error in command argument\n";
        __ivy_exit(1);
    }
    int p__n;
    try {
        int pos = 0;
        arg_values[1] = parse_value(args[1],pos);
        p__n =  _arg<raft::node>(arg_values,1,0);
    }
    catch(out_of_bounds &) {
        std::cerr << "parameter n out of bounds\n";
        __ivy_exit(1);
    }
    catch(syntax_error &) {
        std::cerr << "syntax error in command argument\n";
        __ivy_exit(1);
    }

#ifdef _WIN32
    // Boilerplate from windows docs

    {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
        wVersionRequested = MAKEWORD(2, 2);

        err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0) {
            /* Tell the user that we could not find a usable */
            /* Winsock DLL.                                  */
            printf("WSAStartup failed with error: %d\n", err);
            return 1;
        }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            /* Tell the user that we could not find a usable */
            /* WinSock DLL.                                  */
            printf("Could not find a usable version of Winsock.dll\n");
            WSACleanup();
            return 1;
        }
    }
#endif
    raft_repl ivy(p__node__size,p__n);


    ivy.__unlock();

    cmd_reader *cr = new cmd_reader(ivy);

    // The main thread runs the console reader

    while (!cr->eof())
        cr->read();
    return 0;

    return 0;
}
