# Note that this script can accept some limited command-line arguments, run
# `julia build_tarballs.jl --help` to see a usage message.
using BinaryBuilder, Pkg

name = "VideoCaptureWrap"
version = v"0.7.1"

# copy LICENSE file
cp("LICENSE", joinpath("src", "LICENSE"), force=true)

# Collection of sources required to complete build
sources = [
    DirectorySource("src", target="projectname"),
]
# Bash recipe for building across all platforms
script = raw"""
# Override compiler ID to silence the horrible "No features found" cmake error
if [[ $target == *"apple-darwin"* ]]; then
  macos_extra_flags="-DCMAKE_CXX_COMPILER_ID=AppleClang -DCMAKE_CXX_COMPILER_VERSION=10.0.0 -DCMAKE_CXX_STANDARD_COMPUTED_DEFAULT=11"
fi

Julia_PREFIX=$prefix

mkdir build
cd build
cmake -DJulia_PREFIX=$Julia_PREFIX -DCMAKE_FIND_ROOT_PATH=$prefix -DJlCxx_DIR=$prefix/lib/cmake/JlCxx \
      -DCMAKE_INSTALL_PREFIX=$prefix -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
      $macos_extra_flags -DCMAKE_BUILD_TYPE=Release \
      ../projectname/
VERBOSE=ON cmake --build . --config Release --target install -- -j${nproc}
cd ..
install_license projectname/LICENSE
"""

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line
platforms = [
   Linux(:i686, libc=:glibc),
   Linux(:x86_64, libc=:glibc),
   Linux(:aarch64, libc=:glibc),
   Linux(:armv7l, libc=:glibc, call_abi=:eabihf),
   #Linux(:powerpc64le, libc=:glibc), <- OpenCVQt_jll does not provide for this platform
   #Linux(:i686, libc=:musl), <- OpenCV_jll does not provide for this platform
   #Linux(:x86_64, libc=:musl), <- fails
   Linux(:aarch64, libc=:musl),
   Linux(:armv7l, libc=:musl, call_abi=:eabihf),
   MacOS(:x86_64),
   #FreeBSD(:x86_64), <- fails,
   Windows(:i686),
   Windows(:x86_64),
] |> expand_cxxstring_abis

# The products that we will ensure are always built

products = [
    LibraryProduct("libvideocapture", :libvideocapture), # our application
]

# Dependencies that must be installed before this package can be built
dependencies = [
    Dependency(PackageSpec(name="libcxxwrap_julia_jll", rev="libcxxwrap_julia-v0.8.5+0")),
    Dependency(PackageSpec(; name="OpenCVQt_jll", uuid="2e4a2372-4a7c-5bb9-ae2d-b1938f154ce2", url="https://github.com/terasakisatoshi/OpenCVQt_jll.jl", rev="OpenCVQt-v0.7.1+0")),
    BuildDependency(PackageSpec(name="libjulia_jll", version=v"1.5.1")),
]

# Build the tarballs, and possibly a `build.jl` as well.
build_tarballs(ARGS, name, version, sources, script, platforms, products, dependencies; preferred_gcc_version = v"7")
