if ENABLE_DOCS

build/index.html: Doxyfile ../mypaint*.h
	$(DOXYGEN) Doxyfile

all: build/index.html

clean-local:
	-rm -fr build;

endif
