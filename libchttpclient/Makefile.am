#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
lib_LTLIBRARIES = libchttpclient.la

#######################################
# Build information for each library

# Sources for libchttpclient
libchttpclient_la_SOURCES = libchttpclient.c queue.c utils.c aes.c md5.c storage.c

# Linker options libTestProgram
libchttpclient_la_LDFLAGS = 

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
libchttpclient_la_CPPFLAGS = -I$(top_srcdir)/include

