#include "Timer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <queue>
#include <optional>
#include <memory>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

using boost::multiprecision::cpp_int;

using namespace std;

int64_t collatzLen( cpp_int x )
{
   for ( int64_t i = 0; ; i++ )
   {
      if ( x == 1 )
         return i;
      if ( (x & 1) == 0 )
         x = x / 2;
      else
         x = x * 3 + 1;
   }
}

uint64_t bitLen( const cpp_int& x ) 
{
   return boost::multiprecision::msb( x ) + 1;
}

double log2( const cpp_int& x )
{
   int64_t len = bitLen( x );
   int64_t shift = std::max<int64_t>( 0, len - 200 );
   cpp_int xx = x >> shift;
   return shift + log2( static_cast<double>(x >> shift) );
}

int64_t powInt( int64_t b, int64_t p )
{
   if ( p == 0 )
      return 1;
   if ( p == 1 )
      return b;
   int64_t sq = powInt( b, p / 2 );
   return p & 1 ? sq * sq * b : sq * sq;
}

void initDividesTable( uint64_t M, const vector<uint8_t>& table0, vector<uint8_t>& table1 )
{
   //cout << "M = " << M << endl;
   //uint64_t M = 3;
   uint64_t mod = M * 6;
   for ( uint64_t i = 0; i < M * 3; i++ ) if ( i % 3 != 0 )
   {
      uint64_t x = i * 2;

      uint8_t bestSteps = numeric_limits<uint8_t>::max();
      for ( uint8_t numMulsBeforeDivide = 1; numMulsBeforeDivide < bestSteps; numMulsBeforeDivide++ )
      {
         bool canDivide = x % 18 == 4 || x % 18 == 16;
         if ( canDivide )
            bestSteps = min<uint8_t>( bestSteps, numMulsBeforeDivide + table0[(x - 1) / 3 * 2 % (M * 2) / 2] );

         x = x * 2;
      }
      
      table1[i] = bestSteps;
      //cout << i*2 << ": " << (int) bestSteps << endl;
   }
}

vector<uint8_t> createDividesTable( int numDivides )
{
   vector<uint8_t> table0 = { 0xFF, 0, 0 }; // # mul2s needed to reach 0 divides
   uint64_t M = 3;
   for ( int i = 0; i < numDivides; M *= 3, i++ )
   {
      cout << "createDividesTable i=" << i << endl;
      vector<uint8_t> table1( M * 3 );
      initDividesTable( M, table0, table1 );
      table0.swap( table1 );
   }
   return table0;
}

class LookaheadTable
{
public:
   static constexpr int NUM_DIVIDES = 19;
public:
   LookaheadTable()
   {
      m_table = createDividesTable( NUM_DIVIDES );
      cout << "m_table is " << m_table.size() / (1 << 20) << "MB" << endl;
   }

   //int leastMul2s( const cpp_int& num ) // returns fewest number of mul2s to achieve `NUM_DIVIDES` divides
   //{
   //   static uint64_t MOD_POW3 = 6 * powInt( 3, NUM_DIVIDES );
   //   uint64_t x = static_cast<uint64_t>(num % MOD_POW3);
   //   return m_table[x >> 1];
   //}
   int leastMul2s( uint64_t num ) // returns fewest number of mul2s to achieve `NUM_DIVIDES` divides
   {
      static uint64_t MOD_POW3 = 6 * powInt( 3, NUM_DIVIDES );
      uint64_t x = num % MOD_POW3;
      return m_table[x >> 1];
   }


private:
   vector<uint8_t> m_table;
};

LookaheadTable g_lookahead;

//int main()
//{
//   // backward step
//   // x -> 2x        always an option
//   // x -> (x-1)/3*2 when x%18 is 4 or 16
//   
//   //vector<uint8_t> table0 = { 0xFF, 0, 0 }; // # mul2s needed to reach 0 divides
//   //vector<uint8_t> table1( 9 );             // # mul2s needed to reach 1 divide
//   //vector<uint8_t> table2( 27 );            // # mul2s needed to reach 2 divides
//
//   //initDividesTable( 3, table0, table1 );
//   //initDividesTable( 9, table1, table2 );
//
//   vector<uint8_t> table2b = createDividesTable( 15 );
//   return 0;
//}


class BigInt3
{
public:
   static constexpr uint64_t BASE = 8105110306037952534ull; // 2*3^39   
   static constexpr int N = 6;
public:
   BigInt3()
   {
   }
   BigInt3( cpp_int x )
   {
      for ( int i = 0; i < N; i++ )
      {
         m[i] = static_cast<uint64_t>( x % BASE );
         x /= BASE;
      }
      if ( x != 0 )
         throw runtime_error( "BigInt::BigInt(): number too big" );
   }
   cpp_int value() const
   {
      cpp_int ret = 0;
      for ( int i = N - 1; i >= 0; i-- )
         ret = ret * BASE + m[i];
      return ret;
   }
   void doMul2()
   {
      for ( int i = N - 1; i >= 0; i-- )
      {
         m[i] <<= 1;
         uint64_t carry = m[i] >= BASE ? 1 : 0;
         if ( carry ) { m[i] -= BASE; m[i + 1]++; }
      }
   }
   void doDiv3Mul2()
   {
      int64_t remainder = 0;
      for ( int i = N - 1; i >= 0; i-- )
      {
         lldiv_t d = lldiv( m[i], 3 );
         m[i] = d.quot + remainder;
         remainder = d.rem * (BASE / 3);
      }
      doMul2();
   }
   bool canDiv() const
   {
      auto rem = m[0] % 18;
      return rem == 4 || rem == 16;
   }
   uint64_t leastSignificantBlock() const
   {
      return m[0];
   }

public:
   array<uint64_t, N> m;
};

class SegmentedInt
{
public:
   SegmentedInt() {}
   SegmentedInt( const cpp_int& x )
   {
      m_pow2 = 1;
      m_pow3 = 200;
      cpp_int M = pow( cpp_int( 3 ), m_pow3 ) * pow( cpp_int( 2 ), m_pow2 );
      m_x1 = std::make_shared<cpp_int>( x / M );
      m_x0 = BigInt3( x % M );
      //m_x0c = x % M;
      //assert( m_x0.value() == m_x0c );
   }
   void doMul2()
   {
      SegmentedInt ret;
      m_pow2 = m_pow2 + 1;
      //assert( m_x0.value() == m_x0c );
      m_x0.doMul2();
      //m_x0c *= 2;
      //assert( m_x0.value() == m_x0c );
   }
   void doDiv3Mul2()
   {
      SegmentedInt ret;
      m_pow3--;
      m_pow2++;
      //assert( m_x0.value() == m_x0c );
      m_x0.doDiv3Mul2();
      //m_x0c = m_x0c / 3 * 2;
      //assert( m_x0.value() == m_x0c );
      if ( m_pow3 < 20 )
         *this = SegmentedInt( value() );
   }
   bool canDiv() const
   {
      if ( m_pow2 < 1 || m_pow3 < 2 )
         throw std::runtime_error( "SegmentedInt: m_x1 not divisible by 18" );
      return m_x0.canDiv();
      //return m_x0c % 18 == 4 || m_x0c % 18 == 16;
   }
   cpp_int value() const
   {
      return *m_x1 * (pow( cpp_int( 3 ), m_pow3 ) * pow( cpp_int( 2 ), m_pow2 )) + m_x0.value();
      //return *m_x1 * (pow( cpp_int( 3 ), m_pow3 ) * pow( cpp_int( 2 ), m_pow2 )) + m_x0c;
   }
   uint64_t leastSignificantBlock() const { return m_x0.leastSignificantBlock(); }
   //uint64_t leastSignificantBlock() const { return static_cast<uint64_t>( m_x0c % BigInt3::BASE ); }

private:
   // actual value = m_x1 * 2^m_pow2 * 3^m_pow3 + m_x0
   shared_ptr<cpp_int> m_x1;
   int m_pow2 = 0;
   int m_pow3 = 0;
   //cpp_int m_x0c;
   BigInt3 m_x0;
};


class MyInt
{
public:
   MyInt( const SegmentedInt& x, double log2_x ) : x( x ), m_log2_x( log2_x )
   {
      m_future_mul2s = g_lookahead.leastMul2s( x.leastSignificantBlock() );
   }
   static MyInt from( const cpp_int& x )
   {
      return MyInt( x, log2( x ) );
   }
   double relativeSize() const
   {
      return m_log2_x + m_future_mul2s;
   }
   bool operator<( const MyInt& rhs ) const
   {
      return relativeSize() < rhs.relativeSize();
   }
   MyInt& doMul2()
   {
      x.doMul2();
      m_log2_x += 1;
      m_future_mul2s = g_lookahead.leastMul2s( x.leastSignificantBlock() );
      return *this;
   }
   MyInt& doDiv3Mul2()
   {
      constexpr double LOG2_2_3 = -0.5849625007211561814; // log2( 2./3 )
      x.doDiv3Mul2();
      m_log2_x += LOG2_2_3;
      m_future_mul2s = g_lookahead.leastMul2s( x.leastSignificantBlock() );
      return *this;
   }
   bool canDiv() const
   {
      return x.canDiv();
   }
   cpp_int value() const { return x.value(); }

public:
   SegmentedInt x;
   double m_log2_x;
   int m_future_mul2s;
};

template<typename T>
T takeLast( priority_queue<T>& q )
{
   while ( q.size() > 1 )
      q.pop();
   return q.top();
}

int main()
{
   //{
   //   cpp_int val = pow( cpp_int( 11 ), 20 );
   //   SegmentedInt x( val );
   //   cout << val << endl;
   //   cout << x.value() << endl;
   //}

   Timer t;

   typedef MyInt T;

   //constexpr int64_t MAX_Q_SIZE = 5'000;
   //constexpr int64_t MAX_Q_SIZE = 50; // 5 -> Time = 13.7988  50 -> 106.689
   constexpr int64_t MAX_Q_SIZE = 100'000;
   priority_queue<T> q0; // queue to consume
   priority_queue<T> q1; // next queue
   priority_queue<T> q2; // next next queue
   auto push = [&]( priority_queue<T>& q, const T& val ) { q.push( val ); if ( q.size() > MAX_Q_SIZE ) q.pop(); };

   push( q0, MyInt::from( 8 ) );
   //push( q0, T::from( pow( cpp_int( 11 ), 1000 ) ) );   

   for ( int i = 3; i < 1'000'000; i++ )
   {
      if ( i % 1000 == 0 )
      {
         cout << i << "..." << bitLen( q0.top().value() ) << "   t=" << t.elapsedTime() << endl;
      }
      while ( !q0.empty() )
      {
         const T& x = q0.top();

         if ( x.canDiv() )
            push( q2, MyInt( x ).doDiv3Mul2() );
         push( q1, MyInt( x ).doMul2() );

         q0.pop();
      }

      q0.swap( q1 );
      q1.swap( q2 );
   }

   cout << "Time = " << t.elapsedTime() << endl;

   MyInt result = takeLast( q0 );
   cout << bitLen( result.x.value() ) << ": " << result.x.value() << endl;
   cout << "collatzLen( result.x ) = " << collatzLen( result.x.value() ) << endl;

   return 0;
}