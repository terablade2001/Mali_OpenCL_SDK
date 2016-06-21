#!/bin/bash
#export TOOLCHAIN_ROOT=/home/thomas/gcc-linaro-4.9-2015.05-x86_64_aarch64-linux-gnu
#export TOOLCHAIN_ROOT=/home/thomas/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf
#GCC_COMPILER_VERSION="4.9"

#opencl 1.1 for rk3288, T76x
export OPENCL_VER="1.1"
export OPENCL_INC="/home/thomas/openCL/include/${OPENCL_VER}"
#mali user space driver
export OPENCL_LIB="/home/thomas/openCL/rock-chips/rk3288/userspace/mali-t76x_r5p0-06rel0_linux_1+fbdev/fbdev"
export TOOLCHAIN_ROOT="/usr"

#export CMAKE_BUILD_TYPE="Debug"
export CMAKE_BUILD_TYPE="Release"
GCC_COMPILER_VERSION="4.8"

while [ $# -ge 1 ]; do
	case $1 in
	-ABI|-abi)
		echo "\$1=-abi"
		shift
		APP_ABI=$1
		shift
		;;
	-clean|-c|-C) #
		echo "\$1=-c,-C,-clean"
		clean_build=1
		shift
		;;
	-l|-L)
		echo "\$1=-l,-L"
		local_build=1
		;;
	--help|-h|-H)
		# The main case statement will give a usage message.
		echo "$0 -c|-clean -abi=[armeabi, armeabi-v7a, armv8-64,mips,mips64el, x86,x86_64]"
		exit 1
		break
		;;
	-*)
		echo "$0: unrecognized option $1" >&2
		exit 1
		;;
	*)
		break
		;;
	esac
done

case $(uname -s) in
  Darwin)
    CONFBUILD=i386-apple-darwin`uname -r`
    HOSTPLAT=darwin-x86
    CORE_COUNT=`sysctl -n hw.ncpu`
  ;;
  Linux)
    CONFBUILD=x86-unknown-linux
    HOSTPLAT=linux-`uname -m`
    CORE_COUNT=`grep processor /proc/cpuinfo | wc -l`
  ;;
CYGWIN*)
	CORE_COUNT=`grep processor /proc/cpuinfo | wc -l`
	;;
  *) echo $0: Unknown platform; exit
esac

echo APP_ABI=$APP_ABI
export APP_ABI
export ARM_ABI=${APP_ABI}

case "$APP_ABI" in
  armeabi)
    TARGPLAT=arm-linux-gnueabi
    TOOLCHAINS=arm-linux-gnueabi
    ARCH=arm

	#enable VFP only
	export FLOAT_ABI_SUFFIX="hf"
	export CFLAGS="-Os -pthread -mthumb -fdata-sections -Wa,--noexecstack -fsigned-char -Wno-psabi --sysroot=$SYS_ROOT"
	export CXXFLAGS="-pthread -mthumb -fdata-sections -Wa,--noexecstack -fsigned-char -Wno-psabi --sysroot=$SYS_ROOT"

  ;;
  armeabi-v7a*)
	export APP_ABI="armeabi-v7a"
    TARGPLAT=arm-linux-gnueabi
    TOOLCHAINS=arm-linux-gnueabi
    ARCH=arm

	export FLOAT_ABI_SUFFIX="hf"
	export CFLAGS="-Os -pthread -mthumb -fdata-sections -Wa,--noexecstack -fsigned-char -Wno-psabi --sysroot=$SYS_ROOT"
	export CXXFLAGS="-pthread -mthumb -fdata-sections -Wa,--noexecstack -fsigned-char -Wno-psabi --sysroot=$SYS_ROOT"

  ;;
  arm64-v8a)
    TARGPLAT=aarch64-linux-gnu
    TOOLCHAINS=aarch64-linux-gnu
    ARCH=arm64

	export CFLAGS="-Os -pthread -fdata-sections -Wa,--noexecstack -fsigned-char -Wno-psabi --sysroot=$SYS_ROOT"
	export CXXFLAGS="-pthread -fdata-sections -Wa,--noexecstack -fsigned-char -Wno-psabi --sysroot=$SYS_ROOT"

  ;;
  *) echo $0: Unknown target; exit
esac

#don't add sys_root in makefile. This generates the following error!!!
#${TOOLCHAIN_ROOT}/lib/gcc-cross/arm-linux-gnueabihf/4.8/../../../../arm-linux-gnueabihf/bin/ld: cannot find ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/lib/libc.so.6 inside ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/
#${TOOLCHAIN_ROOT}/lib/gcc-cross/arm-linux-gnueabihf/4.8/../../../../arm-linux-gnueabihf/bin/ld: cannot find ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/lib/libc_nonshared.a inside ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/
#${TOOLCHAIN_ROOT}/lib/gcc-cross/arm-linux-gnueabihf/4.8/../../../../arm-linux-gnueabihf/bin/ld: cannot find ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/lib/ld-linux-armhf.so.3 inside ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/
#collect2: error: ld returned 1 exit status

export SYS_ROOT="${TOOLCHAIN_ROOT}/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}/"
#export CC="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-gcc-${GCC_COMPILER_VERSION} --sysroot=$SYS_ROOT"
#export CXX="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-g++-${GCC_COMPILER_VERSION} --sysroot=$SYS_ROOT"
export CC="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-gcc-${GCC_COMPILER_VERSION}"
export CXX="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-g++-${GCC_COMPILER_VERSION}"
#export LD="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-ld"
export LD="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-gcc-${GCC_COMPILER_VERSION}"
#export LD="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-g++"
export AR="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-ar "
#!!!blis special, need ARCH to "archive"
#export ARCH=${AR}
export RANLIB="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-ranlib"
export STRIP="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-strip"
#export FORTRAN="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-gfortran --sysroot=$SYS_ROOT"
export FORTRAN="${TOOLCHAIN_ROOT}/bin/${TOOLCHAINS}${FLOAT_ABI_SUFFIX}-gfortran"

#cmake:FC
export FC=${FORTRAN}
#http://www.na-mic.org/svn/Slicer3-lib-mirrors/trunk/CMake/Modules/CMakeFortranInformation.cmake
#SET (CMAKE_Fortran_FLAGS "$ENV{FFLAGS}" CACHE STRING
#     "Flags for Fortran compiler.")
#IF (CMAKE_Fortran_FLAGS_INIT)
#    SET (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${CMAKE_Fortran_FLAGS_INIT}")
#ENDIF (CMAKE_Fortran_FLAGS_INIT)
export FFLAGS="-fPIC -pthread"
export image_scaling_SRC=`pwd`
export image_scaling_BUILD=`pwd`/build

if [ -d "$image_scaling_BUILD/$APP_ABI" ]; then
	if [ -n "$clean_build" ]; then
		rm -rf $image_scaling_BUILD/$APP_ABI/*
	fi
else
	mkdir -p $image_scaling_BUILD/$APP_ABI
fi
echo $image_scaling_BUILD/$APP_ABI

pushd ${image_scaling_BUILD}/$APP_ABI
export ARM_ABI=${APP_ABI}

case $APP_ABI in
	armeabi)
		cmake -DCMAKE_TOOLCHAIN_FILE=${image_scaling_SRC}/arm-gnueabi.toolchain.cmake \
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DAPP_ABI=${APP_ABI}  -DARM_ABI:STRING="${ARM_ABI}" \
		-DOPENCL_LIB=${OPENCL_LIB} -DOPENCL_INC=${OPENCL_INC} -DOPENCL_VER=${OPENCL_VER} \
		${image_scaling_SRC}
	;;
  armeabi-v7a*)
		cmake -DCMAKE_TOOLCHAIN_FILE=${image_scaling_SRC}/arm-gnueabi.toolchain.cmake \
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DAPP_ABI=${APP_ABI}  -DARM_ABI:STRING=${ARM_ABI} \
		-DOPENCL_LIB=${OPENCL_LIB} -DOPENCL_INC=${OPENCL_INC} -DOPENCL_VER=${OPENCL_VER} \
		${image_scaling_SRC}
	;;
  arm64-v8a)
		cmake -DCMAKE_TOOLCHAIN_FILE=${image_scaling_SRC}/arm-gnueabi.toolchain.cmake \
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DAPP_ABI=${APP_ABI}  -DARM_ABI:STRING=${ARM_ABI} \
		-DOPENCL_LIB=${OPENCL_LIB} -DOPENCL_INC=${OPENCL_INC} -DOPENCL_VER=${OPENCL_VER} \
		${image_scaling_SRC}
	;;
  *) echo $0: Unknown target; exit
esac

ret=$?
echo "ret=$ret"
if [ "$ret" != '0' ]; then
echo "$0 cmake error!!!!"
exit -1
fi

make -j${CORE_COUNT}
ret=$?
echo "ret=$ret"
if [ "$ret" != '0' ]; then
echo "$0 make error!!!!"
exit -1
fi

popd
exit 0
