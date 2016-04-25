if ENABLE_DOCS

doxygen/index.xml: Doxyfile
	cd doxygen && \
	$(DOXYGEN) ../Doxyfile

build/index.html: doxygen/index.xml
	$(SPHINX_BUILD) source/ build/

all: build/index.html

endif
