#
# "Makefile.am" for "libupnp/upnp"
#
# Copyright (C) 2020 Marcelo Roberto Jimenez
#

EXTRA_DIST = api_changes

docs:
	doxygen

html-local: docs

pdf-local: docs
	$(MAKE) -C doxygen/latex
	mv doxygen/latex/refman.pdf doxygen/latex/libupnp-reference.pdf

clean-local:
	-rm -rf doxygen
