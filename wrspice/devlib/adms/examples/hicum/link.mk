
#######################################################################
# Makefile for public file retrieval.
# User Configurable.
#######################################################################
#######################################################################
# User configuration
#
# URL of the directory that contains the distribution files.  This MUST
# end with a directory separation character.
# Bad cert, breaks curl secure mode.
SRCDIR  = --insecure https://www.iee.et.tu-dresden.de/iee/eb/forsch/Hicum_PD/HicumQ/

# Name of the distribution file to download.
SRCFILE = hicumL2V2p4p0.va
#SRCFILE = hicumL2V2p34.va
#######################################################################

# Full URL to the file to download, or empth if none.
SRCPATH = $(SRCDIR)$(SRCFILE)

ifeq ($(strip $(SRCPATH)),)
# Nothing to download, local files only.  Supply dummy targets.

fetch:

clean_fetch:

else
# Something to download has been specified.

fetch: chkSOURCE SOURCE/$(SRCFILE)

chkSOURCE:
	@if [ ! -d SOURCE ]; then \
	    mkdir SOURCE; \
	fi

SOURCE/$(SRCFILE):
	curl -L -O --output-dir ./SOURCE $(SRCPATH); \
	if [ "$$?" == "0" ]; then \
	    set -- SOURCE/*.tar.gz; \
	    if [ -f "$$1" ]; then \
	        tar xzf $@ -C SOURCE; \
	    fi \
	else \
	    echo "Download failed for $(SRCFILE)"; \
	    rm SOURCE/$(SRCFILE); \
	    exit 1;\
	fi
	@if [ -d SOURCE/code ]; then \
	    cp -f SOURCE/code/*.va .; \
	    set -- SOURCE/code/*.include; \
	    if [ -f "$$1" ]; then \
	        cp -f SOURCE/code/*.include .; \
	    fi; \
	else \
	    cp -f SOURCE/*.va .; \
	    set -- SOURCE/*.include; \
	    if [ -f "$$1" ]; then \
	        cp -f SOURCE/*.include .; \
	    fi \
	fi

clean_fetch::
	-@rm -f *.include *.va *.txt *.pdf
	-@rm -rf SOURCE
endif

