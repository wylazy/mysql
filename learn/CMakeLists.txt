# Copyright (c) 2006, 2010, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

# This is the CMakeLists for InnoDB

INCLUDE(CheckFunctionExists)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckCSourceRuns)

ADD_DEFINITIONS(-DMYSQL_SERVER -DHAVE_EVENT_SCHEDULER)
ADD_DEFINITIONS(-DDISABLE_PSI_MUTEX
  -DDISABLE_PSI_RWLOCK
  -DDISABLE_PSI_COND
  -DDISABLE_PSI_FILE
  -DDISABLE_PSI_TABLE
  -DDISABLE_PSI_SOCKET
  -DDISABLE_PSI_STAGE
  -DDISABLE_PSI_STATEMENT
  -DDISABLE_PSI_IDLE
  -DDISABLE_PSI_STATEMENT_DIGEST
  )

# OS tests
IF(UNIX)
  IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    CHECK_INCLUDE_FILES (libaio.h HAVE_LIBAIO_H)
    CHECK_LIBRARY_EXISTS(aio io_queue_init "" HAVE_LIBAIO)
    ADD_DEFINITIONS("-DUNIV_LINUX -D_GNU_SOURCE=1")
    IF(HAVE_LIBAIO_H AND HAVE_LIBAIO)
      ADD_DEFINITIONS(-DLINUX_NATIVE_AIO=1)
      LINK_LIBRARIES(aio)
    ENDIF()
  ELSEIF(CMAKE_SYSTEM_NAME MATCHES "HP*")
    ADD_DEFINITIONS("-DUNIV_HPUX")
  ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "AIX")
    ADD_DEFINITIONS("-DUNIV_AIX")
  ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    ADD_DEFINITIONS("-DUNIV_SOLARIS")
  ENDIF()
ENDIF()

IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
# After: WL#5825 Using C++ Standard Library with MySQL code
#       we no longer use -fno-exceptions
#	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
ENDIF()

# Enable InnoDB's UNIV_DEBUG and UNIV_SYNC_DEBUG in debug builds
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUNIV_DEBUG -DUNIV_SYNC_DEBUG")

# Add -Wconversion if compiling with GCC
## As of Mar 15 2011 this flag causes 3573+ warnings. If you are reading this
## please fix them and enable the following code:
#IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")
#ENDIF()

CHECK_FUNCTION_EXISTS(sched_getcpu  HAVE_SCHED_GETCPU)

IF(NOT MSVC)
# either define HAVE_IB_GCC_ATOMIC_BUILTINS or not
IF(NOT CMAKE_CROSSCOMPILING)
  CHECK_C_SOURCE_RUNS(
  "
  int main()
  {
    long	x;
    long	y;
    long	res;

    x = 10;
    y = 123;
    res = __sync_bool_compare_and_swap(&x, x, y);
    if (!res || x != y) {
      return(1);
    }

    x = 10;
    y = 123;
    res = __sync_bool_compare_and_swap(&x, x + 1, y);
    if (res || x != 10) {
      return(1);
    }
    x = 10;
    y = 123;
    res = __sync_add_and_fetch(&x, y);
    if (res != 123 + 10 || x != 123 + 10) {
      return(1);
    }
    return(0);
  }"
  HAVE_IB_GCC_ATOMIC_BUILTINS
  )
  CHECK_C_SOURCE_RUNS(
  "
  int main()
  {
    long	res;
    char	c;

    c = 10;
    res = __sync_lock_test_and_set(&c, 123);
    if (res != 10 || c != 123) {
      return(1);
    }
    return(0);
  }"
  HAVE_IB_GCC_ATOMIC_BUILTINS_BYTE
  )
  CHECK_C_SOURCE_RUNS(
  "#include<stdint.h>
  int main()
  {
    int64_t	x,y,res;

    x = 10;
    y = 123;
    res = __sync_sub_and_fetch(&y, x);
    if (res != y || y != 113) {
      return(1);
    }
    res = __sync_add_and_fetch(&y, x);
    if (res != y || y != 123) {
      return(1);
    }
    return(0);
  }"
  HAVE_IB_GCC_ATOMIC_BUILTINS_64
  )
  CHECK_C_SOURCE_RUNS(
  "#include<stdint.h>
  int main()
  {
    __sync_synchronize();
    return(0);
  }"
  HAVE_IB_GCC_SYNC_SYNCHRONISE
  )
  CHECK_C_SOURCE_RUNS(
  "#include<stdint.h>
  int main()
  {
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    __atomic_thread_fence(__ATOMIC_RELEASE);
    return(0);
  }"
  HAVE_IB_GCC_ATOMIC_THREAD_FENCE
  )
ENDIF()

IF(HAVE_IB_GCC_ATOMIC_BUILTINS)
 ADD_DEFINITIONS(-DHAVE_IB_GCC_ATOMIC_BUILTINS=1)
ENDIF()

IF(HAVE_IB_GCC_ATOMIC_BUILTINS_BYTE)
 ADD_DEFINITIONS(-DHAVE_IB_GCC_ATOMIC_BUILTINS_BYTE=1)
ENDIF()

IF(HAVE_IB_GCC_ATOMIC_BUILTINS_64)
 ADD_DEFINITIONS(-DHAVE_IB_GCC_ATOMIC_BUILTINS_64=1)
ENDIF()

IF(HAVE_IB_GCC_SYNC_SYNCHRONISE)
 ADD_DEFINITIONS(-DHAVE_IB_GCC_SYNC_SYNCHRONISE=1)
ENDIF()

IF(HAVE_IB_GCC_ATOMIC_THREAD_FENCE)
 ADD_DEFINITIONS(-DHAVE_IB_GCC_ATOMIC_THREAD_FENCE=1)
ENDIF()

 # either define HAVE_IB_ATOMIC_PTHREAD_T_GCC or not
IF(NOT CMAKE_CROSSCOMPILING)
  CHECK_C_SOURCE_RUNS(
  "
  #include <pthread.h>
  #include <string.h>

  int main() {
    pthread_t       x1;
    pthread_t       x2;
    pthread_t       x3;

    memset(&x1, 0x0, sizeof(x1));
    memset(&x2, 0x0, sizeof(x2));
    memset(&x3, 0x0, sizeof(x3));

    __sync_bool_compare_and_swap(&x1, x2, x3);

    return(0);
  }"
  HAVE_IB_ATOMIC_PTHREAD_T_GCC)
ENDIF()
IF(HAVE_IB_ATOMIC_PTHREAD_T_GCC)
  ADD_DEFINITIONS(-DHAVE_IB_ATOMIC_PTHREAD_T_GCC=1)
ENDIF()

ENDIF(NOT MSVC)

CHECK_FUNCTION_EXISTS(asprintf  HAVE_ASPRINTF)
CHECK_FUNCTION_EXISTS(vasprintf  HAVE_VASPRINTF)

# Solaris atomics
IF(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
  CHECK_FUNCTION_EXISTS(atomic_cas_ulong  HAVE_ATOMIC_CAS_ULONG)
  CHECK_FUNCTION_EXISTS(atomic_cas_32 HAVE_ATOMIC_CAS_32)
  CHECK_FUNCTION_EXISTS(atomic_cas_64 HAVE_ATOMIC_CAS_64)
  CHECK_FUNCTION_EXISTS(atomic_add_long_nv HAVE_ATOMIC_ADD_LONG_NV)
  CHECK_FUNCTION_EXISTS(atomic_swap_uchar HAVE_ATOMIC_SWAP_UCHAR)
  IF(HAVE_ATOMIC_CAS_ULONG AND
     HAVE_ATOMIC_CAS_32 AND
     HAVE_ATOMIC_CAS_64 AND
     HAVE_ATOMIC_ADD_LONG_NV AND
     HAVE_ATOMIC_SWAP_UCHAR)
    SET(HAVE_IB_SOLARIS_ATOMICS 1)
  ENDIF()

  IF(HAVE_IB_SOLARIS_ATOMICS)
    ADD_DEFINITIONS(-DHAVE_IB_SOLARIS_ATOMICS=1)
  ENDIF()

  IF(NOT CMAKE_CROSSCOMPILING)
  # either define HAVE_IB_ATOMIC_PTHREAD_T_SOLARIS or not
  CHECK_C_SOURCE_COMPILES(
  "   #include <pthread.h>
      #include <string.h>

      int main(int argc, char** argv) {
        pthread_t       x1;
        pthread_t       x2;
        pthread_t       x3;

        memset(&x1, 0x0, sizeof(x1));
        memset(&x2, 0x0, sizeof(x2));
        memset(&x3, 0x0, sizeof(x3));

        if (sizeof(pthread_t) == 4) {

          atomic_cas_32(&x1, x2, x3);

        } else if (sizeof(pthread_t) == 8) {

          atomic_cas_64(&x1, x2, x3);

        } else {

          return(1);
        }

      return(0);
    }
  " HAVE_IB_ATOMIC_PTHREAD_T_SOLARIS)
  CHECK_C_SOURCE_COMPILES(
  "#include <mbarrier.h>
  int main() {
    __machine_r_barrier();
    __machine_w_barrier();
    return(0);
  }"
  HAVE_IB_MACHINE_BARRIER_SOLARIS)
  ENDIF()
  IF(HAVE_IB_ATOMIC_PTHREAD_T_SOLARIS)
    ADD_DEFINITIONS(-DHAVE_IB_ATOMIC_PTHREAD_T_SOLARIS=1)
  ENDIF()
  IF(HAVE_IB_MACHINE_BARRIER_SOLARIS)
    ADD_DEFINITIONS(-DHAVE_IB_MACHINE_BARRIER_SOLARIS=1)
  ENDIF()
ENDIF()


IF(UNIX)
# this is needed to know which one of atomic_cas_32() or atomic_cas_64()
# to use in the source
SET(CMAKE_EXTRA_INCLUDE_FILES pthread.h)
CHECK_TYPE_SIZE(pthread_t SIZEOF_PTHREAD_T)
SET(CMAKE_EXTRA_INCLUDE_FILES)
ENDIF()

IF(SIZEOF_PTHREAD_T)
  ADD_DEFINITIONS(-DSIZEOF_PTHREAD_T=${SIZEOF_PTHREAD_T})
ENDIF()

IF(MSVC)
  ADD_DEFINITIONS(-DHAVE_WINDOWS_ATOMICS)
  ADD_DEFINITIONS(-DHAVE_WINDOWS_MM_FENCE)
ENDIF()


# Include directories under innobase
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/learn/innobase/include
		    ${CMAKE_SOURCE_DIR}/learn/innobase/handler)

# Sun Studio bug with -xO2
IF(CMAKE_CXX_COMPILER_ID MATCHES "SunPro"
	AND CMAKE_CXX_FLAGS_RELEASE MATCHES "O2"
	AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	# Sun Studio 12 crashes with -xO2 flag, but not with higher optimization
	# -xO3
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/learn/innobase/rem/rem0rec.cc
    PROPERTIES COMPILE_FLAGS -xO3)
ENDIF()

# Removing compiler optimizations for innodb/mem/* files on 64-bit Windows
# due to 64-bit compiler error, See MySQL Bug #19424, #36366, #34297
IF (MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET_SOURCE_FILES_PROPERTIES(mem/mem0mem.cc mem/mem0pool.cc
				    PROPERTIES COMPILE_FLAGS -Od)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/sql
  ${CMAKE_SOURCE_DIR}/regex
  ${SSL_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIR})

SET(INNOBASE_SOURCES
  ${CMAKE_SOURCE_DIR}/learn/innobase/api/api0api.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/api/api0misc.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/btr/btr0btr.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/btr/btr0cur.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/btr/btr0pcur.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/btr/btr0sea.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0buddy.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0buf.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0dblwr.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0checksum.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0dump.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0flu.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0lru.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0rea.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/data/data0data.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/data/data0type.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0boot.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0crea.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0dict.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0load.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0mem.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0stats.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dict/dict0stats_bg.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/dyn/dyn0dyn.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/eval/eval0eval.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/eval/eval0proc.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fil/fil0fil.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fsp/fsp0fsp.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fut/fut0fut.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fut/fut0lst.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ha/ha0ha.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ha/ha0storage.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ha/hash0hash.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0fts.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0ast.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0blex.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0config.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0opt.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0pars.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0que.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0sql.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/fts/fts0tlex.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ibuf/ibuf0ibuf.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/lock/lock0iter.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/lock/lock0lock.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/lock/lock0wait.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/log/log0log.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/log/log0recv.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/mach/mach0data.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/mem/mem0mem.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/mem/mem0pool.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/mtr/mtr0log.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/mtr/mtr0mtr.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/os/os0file.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/os/os0proc.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/os/os0sync.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/os/os0thread.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/page/page0cur.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/page/page0page.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/page/page0zip.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/pars/lexyy.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/pars/pars0grm.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/pars/pars0opt.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/pars/pars0pars.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/pars/pars0sym.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/que/que0que.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/read/read0read.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/rem/rem0cmp.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/rem/rem0rec.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0ext.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0ftsort.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0import.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0ins.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0merge.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0mysql.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0log.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0purge.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0row.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0sel.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0uins.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0umod.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0undo.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0upd.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0quiesce.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/row/row0vers.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/srv/srv0conc.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/srv/srv0mon.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/srv/srv0srv.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/srv/srv0start.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/sync/sync0arr.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/sync/sync0rw.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/sync/sync0sync.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0i_s.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0purge.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0rec.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0roll.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0rseg.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0sys.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0trx.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/trx/trx0undo.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/usr/usr0sess.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0bh.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0byte.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0crc32.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0dbg.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0list.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0mem.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0rbt.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0rnd.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0ut.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0vec.cc
  ${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0wqueue.cc
  ${CMAKE_SOURCE_DIR}/learn/lib.cc
  )

#SET(INNOBASE_SOURCES
#	${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0ut.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0dbg.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/ut/ut0mem.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/buf/buf0lru.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/mem/mem0mem.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/sync/sync0sync.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/os/os0thread.cc
#	${CMAKE_SOURCE_DIR}/learn/innobase/os/os0sync.cc
#  )

SET(UT_SOURCES ut.cc)
SET(HASH_SOURCES hash.cc)


ADD_CONVENIENCE_LIBRARY(innodb_lib ${INNOBASE_SOURCES})

MYSQL_ADD_EXECUTABLE(ut ${UT_SOURCES})
MYSQL_ADD_EXECUTABLE(hash ${HASH_SOURCES})

TARGET_LINK_LIBRARIES(ut mysys innodb_lib strings)
TARGET_LINK_LIBRARIES(hash mysys innodb_lib strings)
