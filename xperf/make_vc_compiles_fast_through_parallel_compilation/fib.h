#ifndef FIB_INCLUDE_H
#define	FIB_INCLUDE_H

#ifdef USE_CONST_EXPR

// These constants can be tweaked to give different
// compile times. With VC++ 2013 they give compile
// times ranging from ~1.0 s to ~40 s. FibNMedium
// should probably be used for most cases.

const int FibNFast = 28; // ~1.0 s
const int FibNMedium = 29; // ~1.6 s
const int FibNSlow = 30; // ~2.3 s
const int FibNVerySlow = 31; // ~3.8 s

constexpr int const_fib(int n)
{
	return n <= 2 ? 1 : const_fib(n - 1) + const_fib(n - 2);
}

#define CALC_FIB1 const_fib(FibNMedium)
#define CALC_FIB2 const_fib(FibNMedium)
#define CALC_FIB3 const_fib(FibNMedium)
#define CALC_FIB4 const_fib(FibNMedium)
#define CALC_FIB5 const_fib(FibNMedium)
#define CALC_FIB6 const_fib(FibNMedium)

#else
// These constants can be tweaked to give different
// compile times. With VC++ 2013 they give compile
// times ranging from ~1.0 s to ~13 s. FibNMedium
// should probably be used for most cases.

const int FibNFast = 17;
const int FibNMedium = 18;
const int FibNSlow = 19;
const int FibNVerySlow = 20;

// This is a template metaprogramming Fibonacci template with
// anti-optimization measures. TreePos is a number
// whose only purpose is to create unique types
// for each invocation of FibSlow_t, even when 'N'
// is unchanged. When going down the -2 side of the
// tree (the top branch in my diagram) a bit is set,
// and the bit that is set depends on 'N' so this
// should guarantee that all types are different.
template <int TreePos, int N>
struct FibSlow_t {
	enum {
		value = FibSlow_t<TreePos, N - 1>::value +
		FibSlow_t<TreePos + (1 << N), N - 2>::value,
	};
};

// Explicitly specialized for N==2
template <int TreePos>
struct FibSlow_t<TreePos, 2> {
	enum { value = 1 };
};

// Explicitly specialized for N==1
template <int TreePos>
struct FibSlow_t<TreePos, 1> {
	enum { value = 1 };
};

// The different values for the initial TreePos value are to ensure
// that the different invocations don't share types and thus don't
// accidentally compile quickly.
#define CALC_FIB1 FibSlow_t<0, FibNMedium>::value
#define CALC_FIB2 FibSlow_t<1<<25, FibNMedium>::value
#define CALC_FIB3 FibSlow_t<2<<25, FibNMedium>::value
#define CALC_FIB4 FibSlow_t<3<<25, FibNMedium>::value
#define CALC_FIB5 FibSlow_t<4<<25, FibNMedium>::value
#define CALC_FIB6 FibSlow_t<5<<25, FibNMedium>::value

#endif

#endif
