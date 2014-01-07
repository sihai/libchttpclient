dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(asynchttpclient, 1.0)
AC_PROG_CC

AC_CANONICAL_SYSTEM

case $target in
  i*86-*-*)
    have_x86=true
    ;;
  x86_64-*-*)
    have_x86=true
    have_x86_64=true
    ;;
  amd64-*-*)
    have_x86=true
    have_x86_64=true
    ;;
esac

dnl Initialize automake
AM_INIT_AUTOMAKE

dnl this allows us specify individual liking flags for each target
AM_PROG_CC_C_O 

dnl Initialize Libtool
LT_INIT

dnl Check if Libtool is present
dnl Libtool is used for building share libraries 
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(curl, curl_easy_perform)

AC_CONFIG_FILES(Makefile
				test_case/Makefile
                libchttpclient/Makefile
                include/Makefile)
AC_OUTPUT