#! /bin/sh
# $Id: fcgi_sample.cgi 76742 2006-02-03 16:11:16Z vakatov $

# This script is a simple CGI to FastCGI redirector which forwards
# the CGI environment and request parameters to a FastCGI application
# for more information see http://www.fastcgi.com


/netopt/ncbi_tools/fcgi-current/bin/cgi-fcgi -bind -connect localhost:5000
