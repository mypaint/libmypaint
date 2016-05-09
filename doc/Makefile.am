if ENABLE_DOCS

doxygen/index.xml: Doxyfile
	mkdir -p doxygen && cd doxygen && \
	$(DOXYGEN) ../Doxyfile

build/index.html: doxygen/index.xml
	$(SPHINX_BUILD) source/ build/

all: build/index.html

clean-local:
	-rm -fr doxygen;
	-rm -fr build;

endif
