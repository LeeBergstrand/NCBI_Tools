# $Id: Makefile.sra_macros.mk 16076 2013-05-19 02:07:57Z ucko $

SRA_INCLUDE = -I$(includedir)/../src/sra/sdk/interfaces \
              -I$(top_srcdir)/src/sra/sdk/interfaces \
              -I$(includedir)/../src/internal/sra/sdk/interfaces \
              -I$(top_srcdir)/src/internal/sra/sdk/interfaces
# For internal use; $(NO_STRICT_ALIASING) technically belongs in C(XX)FLAGS,
# but listing it here is more convenient and should be safe.
SRA_INTERNAL_CPPFLAGS = -D_GNU_SOURCE $(D_SFX:d=-D_DEBUGGING) \
    $(NCBI_PLATFORM_BITS:%=-D_ARCH_BITS=%) -DLIBPREFIX=lib \
    -DSHLIBEXT=$(DLL)$(dll_ext) $(NO_STRICT_ALIASING)

EXT_SCHEMA_MODULES = axf sraxf wgsxf vxf
#READONLY_SCHEMA_LIBS = $(EXT_SCHEMA_MODULES:%=%$(DLL))
READONLY_SCHEMA_LIBS = axf$(DLL) sraxf$(DLL) wgsxf$(DLL) vxf$(DLL)
#UPDATE_SCHEMA_LIBS = $(EXT_SCHEMA_MODULES:%=w%$(DLL))
UPDATE_SCHEMA_LIBS = waxf$(DLL) wsraxf$(DLL) wwgsxf$(DLL) wvxf$(DLL)
ALWAYS_STATIC_SCHEMA_LIBS = $(EXT_SCHEMA_MODULES:%=%$(FORCE_STATIC))

SRA_SDK_LIBS = vdb kdb vfs kurl srapath kfg krypto kfs kproc klib judy
SRAXF_LIBS = sraxf ksrch
SRA_LIBS = sradb sraschema $(SRA_SDK_LIBS)
BAM_LIBS = align-access ncbi-bam $(SRA_SDK_LIBS)
SRAREAD_LDEP = $(EXT_SCHEMA_MODULES) align-reader ksrch $(SRA_LIBS)
SRAREAD_LIBS = sraread $(SRAREAD_LDEP)

SRA_SDK_SYSLIBS = $(CURL_LIBS) $(BZ2_LIBS) $(Z_LIBS) $(DL_LIBS)
