# $Id: Makefile.sdbapi.mk 181100 2010-01-15 15:30:39Z ivanovp $

SDBAPI_LIB  = sdbapi ncbi_xdbapi_ftds $(FTDS_LIB) dbapi dbapi_driver $(XCONNEXT)
SDBAPI_LIBS = $(FTDS_LIBS)
