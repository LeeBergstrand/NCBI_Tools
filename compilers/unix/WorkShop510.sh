#! /bin/sh
#############################################################################
# Setup the local working environment for the "configure" script
#   Compiler:    Sun C++ 5.10 (Studio 12 Update 1)
#   OS:          Solaris
#   Processors:  Sparc,  Intel
#
# $Revision: 164326 $  // by Denis Vakatov, NCBI (vakatov@ncbi.nlm.nih.gov)
#############################################################################


## Compiler location and attributes
WS_BIN="/netopt/studio12u1/prod/bin"
export WS_BIN

## Configure using generic script "WorkShop.sh"
${CONFIG_SHELL-/bin/sh} `dirname $0`/WorkShop.sh "$@"
