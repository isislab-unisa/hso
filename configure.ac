#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([HSO], [1.0], [m.dauria31@gmail.com])
AC_CONFIG_SRCDIR([src/hso.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Checks for programs.
AC_PROG_CC([mpicc])
AC_PROG_INSTALL
# Checks for libraries.
have_pthread=no
AC_SEARCH_LIBS([pthread_create, pthread_join],[pthread],[have_pthread=yes])

#check zmq
was_libzmq_check_lib_detected=no
PKG_CHECK_MODULES([libzmq], [libzmq >= 0.0.0],
    [
    ],
    [
        AC_ARG_WITH([libzmq],
            [
                AS_HELP_STRING([--with-libzmq],
                [Specify libzmq prefix])
            ],
            [search_libzmq="yes"],
            [])

        libzmq_synthetic_cflags=""
        libzmq_synthetic_libs="-lzmq"

        if test "x$search_libzmq" = "xyes"; then
            if test -r "${with_libzmq}/include/zmq.h"; then
                libzmq_synthetic_cflags="-I${with_libzmq}/include"
                libzmq_synthetic_libs="-L${with_libzmq}/lib -lzmq"
            else
                AC_MSG_ERROR([${with_libzmq}/include/zmq.h not found. Please check libzmq prefix])
            fi
        fi

        AC_CHECK_LIB([libzmq], [zmq_init zmq_ctx_new zmq_send zmq_recv],
            [
                CFLAGS="${libzmq_synthetic_cflags} ${CFLAGS}"
                LDFLAGS="${libzmq_synthetic_libs} ${LDFLAGS}"
                LIBS="${libzmq_synthetic_libs} ${LIBS}"

                AC_SUBST([libzmq_CFLAGS],[${libzmq_synthetic_cflags}])
                AC_SUBST([libzmq_LIBS],[${libzmq_synthetic_libs}])
                was_libzmq_check_lib_detected=yes
            ],
            [AC_MSG_ERROR([cannot link with -lzmq, install libzmq.])])
    ])

if test "x$was_libzmq_check_lib_detected" = "xno"; then
    CFLAGS="${libzmq_CFLAGS} ${CFLAGS}"
    LIBS="${libzmq_LIBS} ${LIBS}"
fi


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

if test "x${have_pthread}" = xyes; then
	AC_CHECK_HEADERS([pthread.h],[],[have_pthread=no])
fi

if test "x${have_pthread}" = xno; then
	AC_MSG_WARN([Unable to find pthread on this system])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday strdup strndup strstr])

AC_CONFIG_FILES([Makefile
		src/Makefile])
AC_OUTPUT
