These files are associated with the blog post "Make VC++ Compiles Fast Through Parallel Compilation"
which can be found here:

https://randomascii.wordpress.com/2014/03/22/make-vc-compiles-fast-through-parallel-compilation/

This post uses an intentionally pathologically slow-to-build project to demonstrate how to
maximize build parallelism in VC++ builds.

This project is carefully constructed to compile slowly. The individual
source files calculate a Fibonacci number at compile time so that they
compile slowly, and careful techniques are used in order to minimize
compile parallelism. Three different project files are then used to
demonstrate how build parallelism can be maximized.

The template techniques used for slow compilation are discussed here:
http://randomascii.wordpress.com/2014/03/10/making-compiles-slow/

Comments on that blog post suggested the constexpr technique which turns
out to be work better because it doesn't create thousands of types and
therefore parallelizes better (because there is less information to
be written to the shared .pdb file).
