EXTRA_DIST =		\
	Doxyfile		\
	source/conf.py	\
	source/index.rst

if ENABLE_DOCS

doxygen/index.xml: Doxyfile
	mkdir -p doxygen && cd doxygen && \
	$(DOXYGEN) $(abs_srcdir)/Doxyfile

build/index.html: doxygen/index.xml
	@if test "$(abs_srcdir)" != "$(abs_builddir)"; then \
		rm -rf source; \
		cp -r $(srcdir)/source source; \
	fi
	$(SPHINX_BUILD) source/ build/

all: build/index.html

clean-local:
	-rm -fr doxygen;
	-rm -fr build;

endif
