/*
Copyright (C)  2004 Artem Khodush

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution. 

3. The name of the author may not be used to endorse or promote products 
derived from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "exec-stream.h"

#include <vector>
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32

#include <windows.h>

void sleep( int seconds )
{
    Sleep( seconds*1000 );
}

#else

#include <unistd.h>

#endif

// class for collecting and printing test results

class test_results_t {
public:
    static void add_test( std::string const & name );
    static bool register_failure( std::string const & assertion, std::string const & file, int line );
    static int print( std::ostream & o );

    class error_t : public std::exception {
    public:
        error_t( std::string const & msg ) 
        : m_msg( msg )
        {
        }

        ~error_t() throw()
        {
        }
        
        virtual char const * what() const throw()
        {
            return m_msg.c_str();
        }
        
    private:
        std::string m_msg;
    };
    
private:
    struct failure_t {
        std::string assertion;
        std::string file;
        int line;
    };
    typedef std::vector< failure_t > failures_t;
    struct result_t {
        std::string test_name;
        failures_t failures;
    };
    typedef std::vector< result_t > results_t;
    
    static results_t m_results;
    static bool m_printed;
    
    struct force_print_t {
        ~force_print_t();
    };
    friend struct force_print_t;
    
    static force_print_t m_force_print;
};

test_results_t::results_t test_results_t::m_results;
bool test_results_t::m_printed;
test_results_t::force_print_t test_results_t::m_force_print;

void test_results_t::add_test( std::string const & name )
{
    results_t::iterator i=m_results.begin();
    while( i!=m_results.end() && i->test_name!=name ) {
        ++i;
    }
    if( i!=m_results.end() ) {
        throw error_t( "test_results_t::add_test: duplicate test name: "+name );
    }
    result_t & r=*m_results.insert( m_results.end(), result_t() );
    r.test_name=name;
}

bool test_results_t::register_failure( std::string const & assertion, std::string const & file, int line )
{
    if( m_results.empty() ) {
        throw error_t( "test_results_t::register_failure: called without add_test() first" );
    }
    failures_t & failures=m_results.back().failures;
    failure_t & failure=*failures.insert( failures.end(), failure_t() );
    failure.assertion=assertion;
    failure.file=file;
    failure.line=line;
    return true;
}

int test_results_t::print( std::ostream & o )
{
    int n_ok=0;
    int n_failed=0;
    for( results_t::iterator res_i=m_results.begin(); res_i!=m_results.end(); ++res_i ) {
        if( res_i->failures.empty() ) {
            ++n_ok;
        }else {
            o<<"FAILED TEST: "<<res_i->test_name<<"\n";
            for( failures_t::iterator fail_i=res_i->failures.begin(); fail_i!=res_i->failures.end(); ++fail_i ) {
                o<<"    "<<fail_i->assertion<<" [at file "<<fail_i->file<<" line "<<fail_i->line<<"]\n";
            }
            ++n_failed;
        }
    }
    o<<"\nOK: "<<n_ok<<" FAILED: "<<n_failed<<"\n";
    m_printed=true;
    return n_failed;
}

test_results_t::force_print_t::~force_print_t()
{
    if( !test_results_t::m_results.empty() && !test_results_t::m_printed ) {
        test_results_t::print( std::cerr );
        std::cerr<<"UNEXPECTED TESTSUITE TERMINATION\n";
        std::cerr<<"while running test '"<<m_results.back().test_name<<"'\n";
    }
}

#define TEST_NAME(n) test_results_t::add_test(n);
#define TEST(e) ((e) || test_results_t::register_failure( #e, __FILE__, __LINE__ ))

std::string read_all( std::istream & i )
{
    std::string one;
    std::string ret;
    while( std::getline( i, one ).good() ) {
        ret+=one;
        ret+="\n";
    }
    ret+=one;
    return ret;
}

std::string random_string( std::size_t size )
{
    std::string s;
    s.reserve( size );
    srand( 1 );
    while( s.size()<size ) {
        char c=rand()%CHAR_MAX;
        if( isprint( c ) && !isspace( c ) ) {
            s+=c;
        }
    }
    return s;
}

bool check_if_english_error_messages()
{
#ifdef _WIN32
    std::string s;
    LPVOID buf;
    if( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
        0, 
        1, 
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), 
        (LPTSTR) &buf,
        0,
        0 
        )==0 ) {
        return false;
    }else {
        s=(LPTSTR)buf;
        LocalFree( buf );
        return s.find( "Incorrect function" )!=std::string::npos;
    }
#else
    std::string s( strerror( 1 ) );
    return s.find( "Operation not permitted" )!=std::string::npos;
#endif
}

// each function below is run as a child process for some test

int hello()
{
    std::cout<<"hello";
    return 0;
}

int helloworld()
{
    std::cout<<"hello\n";
    std::cout<<"world";
    return 0;
}

int hello_o_world_e()
{
    std::cout<<"hello";
    std::cerr<<"world";
    return 0;
}

int with_space()
{
    std::cout<<"with space ok\n";
    return 0;
}

int write_after_pause()
{
    sleep( 10 );
    std::cout<<"after pause";
    return 0;
}

int read_after_pause()
{
    sleep( 10 );
    std::string s;
    std::cin>>s;
    if( s==random_string( 20000 ) ) {
        std::cerr<<"OK";
    }else {
        std::cerr<<"ERROR";
    }
    return 0;
}

int dont_read()
{
    return 0;
}

int dont_stop()
{
    std::string s=random_string( 100 )+"\n";
    while( true ) {
        std::cout<< s;
    }
    return 0;
}

int echo_size() {
    std::string s;
    while( std::getline( std::cin, s ).good() ) {
        std::cout<<s.size()<<std::endl;
    }
    return 0;
}

int echo()
{
    std::string s;
    while( std::getline( std::cin, s ).good() ) {
        std::cout<<s<<"\n";
    }
    return 0;
}

int echo_with_err()
{
    std::string s;
    std::cerr<<random_string( 500000 );
    while( std::getline( std::cin, s ).good() ) {
        std::cout<<s<<"\n";
    }
    return 0;
}

int echo_picky()
{
    std::string s;
    while( std::getline( std::cin, s ).good() ) {
        if( s=="STOP" ) {
            std::cerr<<"OK\n";
            return 0;
        }
        std::cout<<s<<"\n";
    }
    return 0;
}

int pathologic()
{
    std::string in_s=random_string( 1000 );
    int cnt=2500;
    for( int i=0; i<cnt; ++i ) {
        std::string t=in_s+"\n";
        fputs( t.c_str(), stdout );
    }
    std::string out_s;
    int n=0;

    char buf[1002];
    
    while( fgets( buf, sizeof( buf ), stdin )!=0 ) {
        int len=strlen( buf );

        if( len>0 && buf[len-1]=='\n' ) {
            --len;
        }
        out_s.assign( buf, len );
        if( out_s!=in_s ) {
            fputs( "ERROR", stderr );
            return 0;
        }
        ++n;
    }
    if( n==1200 ) {
        fputs( "OK", stderr );
    }else {
        fputs( "ERROR", stderr );
    }
    return 0;
}

int long_out_line()
{
    std::cout<<random_string( 2000000 );
    return 0;
}

int exit_code()
{
    std::string s;
    std::stringstream ss;
    std::cin>>s;
    ss.str( s );
    int n;
    ss>>n;
    return n;
}

// selection of child functions
        
typedef int(*child_func_t)();

child_func_t find_child_func( std::string const & name ) 
{
    typedef std::map< std::string, child_func_t > child_funcs_t;
    static child_funcs_t funcs;
    if( funcs.size()==0 ) {
        funcs["hello"]=hello;
        funcs["helloworld"]=helloworld;
        funcs["hello-o-world-e"]=hello_o_world_e;
        funcs["with space"]=with_space;
        funcs["write-after-pause"]=write_after_pause;
        funcs["read-after-pause"]=read_after_pause;
        funcs["dont-read"]=dont_read;
        funcs["dont-stop"]=dont_stop;
        funcs["echo-size"]=echo_size;
        funcs["echo"]=echo;
        funcs["echo-with-err"]=echo_with_err;
        funcs["echo-picky"]=echo_picky;
        funcs["pathologic"]=pathologic;
        funcs["long-out-line"]=long_out_line;
        funcs["exit-code"]=exit_code;
    }
    child_funcs_t::iterator i=funcs.find( name );
    return i==funcs.end() ? 0 : i->second;
}

void pathologic_one( exec_stream_t & exec_stream, bool expect_ok=true )
{
    std::string in_s=random_string( 1000 );
    int cnt=1200;
    for( int i=0; i<cnt; ++i ) {
        exec_stream.in()<<in_s<<"\n";
    }
    exec_stream.close_in();
    std::string out_s;
    int n=0;
    while( std::getline( exec_stream.out(), out_s ).good() ) {
        if( out_s!=in_s ) {
            TEST( 0=="out_s!=in_s" );
            break;
        }
        ++n;
    }
    TEST( n==2500 );
    exec_stream.err()>>out_s;
    if( expect_ok ) {
        TEST( out_s=="OK" );
    }else {
        TEST( out_s=="ERROR" );
    }
}

int main( int argc, char ** argv ) 
{
    if( argc>=3 ) {
        if( std::string( argv[1] )=="child" ) {
            if( child_func_t func=find_child_func( argv[2] ) ) {
                return func();
            }
        }else if( std::string( argv[1] )=="args" ) {
            for( int i=2; i<argc; ++i ) {
                std::cout << argv[i] << "\n";
            }
        }
        return 0;
    }

    bool skip_long_tests=false;
    if( argc==2 && std::string( argv[1] )=="skip-long-tests" ) {
        skip_long_tests=true;
    }


    // do not test for specific error message text if it is not in english
    bool english_error_messages=check_if_english_error_messages();
    
    std::string program="./exec-stream-test";
    int n_failed=42;

    // the tests begin here
    
    try {

        {
            TEST_NAME( "hello" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child hello" );
            TEST( read_all( exec_stream.out() )=="hello" );
            TEST( read_all( exec_stream.err() )=="" );
            TEST( exec_stream.close() );
        }
        
        {
            TEST_NAME( "helloworld" );
            exec_stream_t exec_stream( program, "child helloworld" );
            TEST( read_all( exec_stream.out() )=="hello\nworld" );
            TEST( read_all( exec_stream.err() )=="" );
        }
        
        {
            TEST_NAME( "helloworld binary read" );
            exec_stream_t exec_stream;
            exec_stream.set_binary_mode( exec_stream_t::s_out );
            exec_stream.start( program, "child helloworld" );
#ifdef _WIN32
            TEST( read_all( exec_stream.out() )=="hello\r\nworld" );
#else
            TEST( read_all( exec_stream.out() )=="hello\nworld" );
#endif
            TEST( read_all( exec_stream.err() )=="" );
        }
        
        {
            TEST_NAME( "hello->out world->err" );
            exec_stream_t exec_stream( program, "child hello-o-world-e" );
            TEST( read_all( exec_stream.out() )=="hello" );
            TEST( read_all( exec_stream.err() )=="world" );
        }
        
        {
            TEST_NAME( "hello->out world->err read reversed" );
            exec_stream_t exec_stream( program, "child hello-o-world-e" );
            TEST( read_all( exec_stream.err() )=="world" );
            TEST( read_all( exec_stream.out() )=="hello" );
        }

        {
            TEST_NAME( "hello 5 times" );
            exec_stream_t exec_stream;

            exec_stream.start( program, "child helloworld" );
            TEST( read_all( exec_stream.out() )=="hello\nworld" );
            TEST( read_all( exec_stream.err() )=="" );

            exec_stream.start( program, "child helloworld" );
            // do not read, leave all in buffers

            exec_stream.start( program, "child hello-o-world-e" );
            
            TEST( read_all( exec_stream.out() )=="hello" );
            TEST( read_all( exec_stream.err() )=="world" );

            exec_stream.start( program, "child helloworld" );
            // do not read all, leave something in buffers
            std::string s;
            std::getline( exec_stream.out(), s );

            TEST( s=="hello" );

            exec_stream.start( program, "child hello-o-world-e" );
            
            TEST( read_all( exec_stream.out() )=="hello" );
            TEST( read_all( exec_stream.err() )=="world" );

            TEST( exec_stream.close() ); 
        }

        {
            TEST_NAME( "with space" );
            std::vector< std::string > args;
            args.push_back( "child" );
            args.push_back( "with space" );
            exec_stream_t exec_stream( program, args.begin(), args.end() );
            std::string s;
            std::getline( exec_stream.out(), s );
            TEST( s=="with space ok" );
            TEST( exec_stream.close() );
        }

        {
            TEST_NAME( "child write after pause" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child write-after-pause" );
            try {
                std::string s;
                std::getline( exec_stream.out(), s );
                TEST( 0=="unreached" );
            }catch( std::exception const & e ) {
                std::string m=e.what();
                if( english_error_messages ) {
                    TEST( m.find( "timeout" )!=std::string::npos || m.find( "timed out" )!=std::string::npos  );
                }
            }
            TEST( !exec_stream.close() );
            exec_stream.kill();
            
            exec_stream.set_wait_timeout( exec_stream_t::s_out, 12000 );
            exec_stream.start( program, "child write-after-pause" );
            std::string s;
            std::getline( exec_stream.out(), s );
            TEST( s=="after pause" );
            
            TEST( exec_stream.close() );
        }

        {
            TEST_NAME( "child read after pause" );
            exec_stream_t exec_stream;
            exec_stream.set_wait_timeout( exec_stream_t::s_err, 15000 );
            std::string in_s=random_string( 20000 )+"\n";
            
            exec_stream.start( program, "child read-after-pause" );
            exec_stream.in()<<in_s;
            std::string s;
            exec_stream.err()>>s;
            TEST( s=="OK" );
            TEST( exec_stream.close() );

            exec_stream.set_buffer_limit( exec_stream_t::s_in, 1000 );
            exec_stream.start( program, "child read-after-pause" );
            try {
                exec_stream.in()<<in_s;
                TEST( 0=="unreachable" );
            }catch( std::exception const & e ) {
                std::string m=e.what();
                if( english_error_messages ) {
                    TEST( m.find( "timeout" )!=std::string::npos || m.find( "timed out" )!=std::string::npos  );
                }
            }
            TEST( !exec_stream.close() );
            exec_stream.kill();
            
            exec_stream.set_wait_timeout( exec_stream_t::s_in, 15000 );
            exec_stream.start( program, "child read-after-pause" );
            exec_stream.in()<<in_s;
            exec_stream.err()>>s;
            TEST( s=="OK" );
            TEST( exec_stream.close() );
        }
        
        {
            TEST_NAME( "dont read" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child dont-read" );
            try {
                exec_stream.in()<<random_string( 20000 );
                exec_stream.close();
                TEST( 0=="unreachable" );
            }catch( std::exception const & e ) {
                std::string m=e.what();
                if( english_error_messages ) {
                    TEST( m.find( "Broken pipe" )!=std::string::npos 
                        || m.find( "pipe is being closed" )!=std::string::npos 
                        || m.find( "pipe has been ended" )!=std::string::npos );
                }
            }
        }

        {
            TEST_NAME( "dont stop" );
            exec_stream_t exec_stream;
            
            // on slow machines when child hogs CPU, this may take a while
            exec_stream.set_wait_timeout( exec_stream_t::s_child, 5000 );
            
            exec_stream.start( program, "child dont-stop" );
            exec_stream.in()<<random_string( 20000 );
            TEST( !exec_stream.close() );
            exec_stream.kill();

            exec_stream.start( program, "child hello" );
            TEST( read_all( exec_stream.out() )=="hello" );
            TEST( read_all( exec_stream.err() )=="" );
            TEST( exec_stream.close() );
        }
        
        {
            TEST_NAME( "echo-size" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child echo-size" );
            exec_stream.in()<<"1234\n";
            std::string out;
            exec_stream.out()>>out;
            TEST( out=="4" );
            exec_stream.in()<<"\n";
            exec_stream.out()>>out;
            TEST( out=="0" );
            TEST( exec_stream.close() );
        }

        {
            TEST_NAME( "echo" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child echo" );
            int sizes[]={ 4096-1, 4096+1, 4096+1, 4096-1, 4096*2, 3 };
            int cnt=sizeof( sizes )/sizeof( sizes[0] );
            for( int i=0; i<cnt; ++i ) {
                std::string in_s=random_string( sizes[i] );
                exec_stream.in()<<in_s<<"\n";
                std::string out_s;
                TEST( std::getline( exec_stream.out(), out_s ).good() );
                TEST( in_s==out_s );
            }
            TEST( exec_stream.close() );
        }

        {
            TEST_NAME( "echo with err" );
            exec_stream_t exec_stream;
            exec_stream.set_wait_timeout( exec_stream_t::s_out, 15000 );
            exec_stream.start( program, "child echo-with-err" );
            int sizes[]={ 4096-1, 4096+1, 4096+1, 4096-1, 4096*2, 3 };
            int cnt=sizeof( sizes )/sizeof( sizes[0] );
            for( int i=0; i<cnt; ++i ) {
                std::string in_s=random_string( sizes[i] );
                exec_stream.in()<<in_s<<"\n";
                std::string out_s;
                TEST( std::getline( exec_stream.out(), out_s ).good() );
                TEST( in_s==out_s );
            }
            exec_stream.close_in();
            std::string err;
            err.reserve( 500000 );
            exec_stream.err()>>err;
            TEST( err==random_string( 500000 ) );
            TEST( exec_stream.close() );
        }

        {
            TEST_NAME( "echo-picky" );
            exec_stream_t exec_stream;
            
            // disable exceptions while reading/writing
            exec_stream.in().exceptions( std::ios_base::goodbit );
            exec_stream.out().exceptions( std::ios_base::goodbit );
            exec_stream.err().exceptions( std::ios_base::goodbit );
            
            exec_stream.start( program, "child echo-picky" );
            std::string in_s;
            std::string out_s;
            
            in_s=random_string( 4096 );
            exec_stream.in()<<in_s<<"\n";
            TEST( std::getline( exec_stream.out(), out_s ).good() );
            TEST( in_s==out_s );
            
            in_s="ZZZZZ";
            exec_stream.in()<<in_s<<"\n";
            TEST( std::getline( exec_stream.out(), out_s ).good() );
            TEST( in_s==out_s );
            
            in_s="STOP";
            exec_stream.in()<<in_s<<"\n";
            TEST( !std::getline( exec_stream.out(), out_s ).good() );
            TEST( out_s=="" );
            
            TEST( std::getline( exec_stream.err(), out_s ).good() );
            TEST( out_s=="OK" );
        }
        
        {
            TEST_NAME( "echo-picky with exceptions" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child echo-picky" );
            std::string in_s;
            std::string out_s;
            
            in_s=random_string( 4096 );
            exec_stream.in()<<in_s<<"\n";
            TEST( std::getline( exec_stream.out(), out_s ).good() );
            TEST( in_s==out_s );
            
            in_s="ZZZZZ";
            exec_stream.in()<<in_s<<"\n";
            TEST( std::getline( exec_stream.out(), out_s ).good() );
            TEST( in_s==out_s );
            
            in_s="STOP";
            exec_stream.in()<<in_s<<"\n";
            TEST( !std::getline( exec_stream.out(), out_s ).good() );
            TEST( out_s=="" );
            
            TEST( std::getline( exec_stream.err(), out_s ).good() );
            TEST( out_s=="OK" );
        }

        {
            TEST_NAME( "echo-picky short" );
            exec_stream_t exec_stream;
            exec_stream.start( program, "child echo-picky" );
            std::string in_s;
            std::string out_s;
            
            in_s="STOP";
            exec_stream.in()<<in_s<<"\n";
            
            TEST( std::getline( exec_stream.err(), out_s ).good() );
            TEST( out_s=="OK" );
        }

        {
            TEST_NAME( "pathologic" );
            exec_stream_t exec_stream;
            // disable exceptions while reading/writing
            exec_stream.in().exceptions( std::ios_base::goodbit );
            exec_stream.out().exceptions( std::ios_base::goodbit );
            exec_stream.err().exceptions( std::ios_base::goodbit );
            // on slow machines this test may take quite a while
            exec_stream.set_wait_timeout( exec_stream_t::s_in|exec_stream_t::s_out|exec_stream_t::s_err, 60000 );
            
            exec_stream.start( program, "child pathologic" );
            pathologic_one( exec_stream );
            TEST( exec_stream.close() );
            
            exec_stream.set_buffer_limit( exec_stream_t::s_in, 500 );
            exec_stream.start( program, "child pathologic" );
            pathologic_one( exec_stream );
            TEST( exec_stream.close() );
        }

        if( !skip_long_tests ) {
        
            {
                TEST_NAME( "pathologic with exceptions" );
                exec_stream_t exec_stream;

                exec_stream.start( program, "child pathologic" );
                pathologic_one( exec_stream );
                TEST( exec_stream.close() );

                exec_stream.set_buffer_limit( exec_stream_t::s_in, 5000 );
                exec_stream.start( program, "child pathologic" );
                pathologic_one( exec_stream );
                TEST( exec_stream.close() );
            }

            {
                TEST_NAME( "pathologic two" );
                exec_stream_t exec_stream;
                // disable exceptions while reading/writing
                exec_stream.in().exceptions( std::ios_base::goodbit );
                exec_stream.out().exceptions( std::ios_base::goodbit );
                exec_stream.err().exceptions( std::ios_base::goodbit );

                exec_stream.set_buffer_limit( exec_stream_t::s_out, 5000 );
                exec_stream.start( program, "child pathologic" );
                pathologic_one( exec_stream );
                TEST( exec_stream.close() );
                exec_stream.set_buffer_limit( exec_stream_t::s_in|exec_stream_t::s_out, 5000 );
                exec_stream.start( program, "child pathologic" );
                pathologic_one( exec_stream, false );
                TEST( exec_stream.close() );
            }

            {
                TEST_NAME( "pathologic two with exceptions" );
                exec_stream_t exec_stream;

                exec_stream.set_buffer_limit( exec_stream_t::s_out, 5000 );
                exec_stream.start( program, "child pathologic" );
                pathologic_one( exec_stream );
                TEST( exec_stream.close() );
                exec_stream.set_buffer_limit( exec_stream_t::s_in|exec_stream_t::s_out, 5000 );
                exec_stream.start( program, "child pathologic" );
                try {
                    pathologic_one( exec_stream );
                    TEST( 0=="unreachable" );
                }catch( std::exception const & e ) {
                    std::string m=e.what();
                    if( english_error_messages ) {
                        TEST( m.find( "timeout" )!=std::string::npos || m.find( "timed out" )!=std::string::npos  );
                    }
                }
                TEST( exec_stream.close() );
            }
            
            {
                TEST_NAME( "long out line" );
                exec_stream_t exec_stream;
                exec_stream.set_wait_timeout( exec_stream_t::s_out, 100000 );
                exec_stream.start( program, "child long-out-line" );
                std::string s;
                s.reserve( 2000000 );
                exec_stream.out()>>s;
                TEST( s==random_string( 2000000 ) );
                TEST( exec_stream.close() );
            }

        }

        {
            TEST_NAME( "exit code" );
            exec_stream_t exec_stream;
            
            exec_stream.start( program, "child exit-code" );
            exec_stream.in()<<"2\n";
            TEST( exec_stream.close() );
            TEST( exec_stream.exit_code()==2 );

            exec_stream.start( program, "child exit-code" );
            exec_stream.in()<<"42\n";
            TEST( exec_stream.close() );
            TEST( exec_stream.exit_code()==42 );
        }

        {
            TEST_NAME( "args" );
            exec_stream_t exec_stream;
            std::string s;
            
            exec_stream.start( program, "args \"one two\" three" );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="one two" );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="three" );
            TEST( !std::getline( exec_stream.out(), s ).good() );
            
            exec_stream.start( program, "args -e \"print \\\"hello world\\\";\"" );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="-e" );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="print \"hello world\";" );
            TEST( !std::getline( exec_stream.out(), s ).good() );

            char const * args[]={ "args", "one two", "three" };
            exec_stream.start( program, &args[0], &args[3] );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="one two" );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="three" );
            TEST( !std::getline( exec_stream.out(), s ).good() );
            
            std::vector< std::string > args2;
            args2.push_back( "args" );
            args2.push_back( "-e" );
            args2.push_back( "print \"hello world\";" );
            exec_stream.start( program, args2.begin(), args2.end() );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="-e" );
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="print \"hello world\";" );
            TEST( !std::getline( exec_stream.out(), s ).good() );
        }

        {
            TEST_NAME( "kinky args" );
            exec_stream_t exec_stream( program, "args", "zzzz" );
            std::string s;
            TEST( std::getline( exec_stream.out(), s ).good() );
            TEST( s=="zzzz" );
            TEST( !std::getline( exec_stream.out(), s ).good() );
        }
        
        {
            TEST_NAME( "run unexistent program" );
            std::string prog="don't know what";
            try {
                exec_stream_t exec_stream;
                exec_stream.start( prog, "" );
                TEST( 0=="unreachable" );
            }catch( exec_stream_t::error_t & e ) {
                std::string msg=e.what();
                TEST( msg.find( prog )!=std::string::npos );
            }catch(...) {
                TEST( 0=="unexpected" );
            }
        }

        n_failed=test_results_t::print( std::cout );

    }catch( std::exception const & e ) {
        std::cerr<<"exception:"<<e.what()<<"\n";
    }catch( ... ) {
        std::cerr<<"unknown exception\n";
    }
        
    return n_failed;
}
