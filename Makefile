# -*-Mode: makefile;-*-
# Makefile fuer Waldmeister
# geschrieben von AJ
#
ifneq (,)
This makefile requires GNU Make.
endif

# nur Directorystruktur - Unten gibt's noch Zusatzregeln
LIBDIRS  = std fast prof power comp
ALLDIRS := $(addprefix lib/, $(LIBDIRS))


.PHONY: std fast prof power comp
comp: lib/comp
std: lib/std
fast: lib/fast
prof: lib/prof
power: lib/power

.PHONY: all $(ALLDIRS)
all: $(ALLDIRS)
$(ALLDIRS): architecture
	@echo "making in" $@
	$(MAKE) -C $@



# make depend updates all .depend files
# make depend is only necessary when lots of files (includes)
# have changed or added
.PHONY: depend
depend:
	for subdir in $(ALLDIRS) ; do \
		$(MAKE) -C $$subdir $@ || exit 1 ; \
	done


.PHONY: tags TAGS
tags TAGS:
	etags sources/*/*.c include/*.h


count:
	@wc include/*.h sources/*/*.c

.PHONY: def-status
def-status:
	egrep -n '#define .+_TEST' include/*.h sources/*/*.c
	egrep -n '#define .+_STATISTIK' include/*.h sources/*/*.c

%.i:
	@echo "Making $@ in lib/std..."
	$(MAKE) -C lib/std $@
	$(MAKE) -C lib/std

%.s:
	@echo "Making $@ in lib/std..."
	$(MAKE) -C lib/std $@
	$(MAKE) -C lib/std

.PHONY: architecture
architecture: .architecture
.architecture:
	$(SH) Scripts/findarch


.PHONY: clean
clean:
	for subdir in $(ALLDIRS) ; do \
		$(MAKE) -C $$subdir $@ || exit 1 ; \
	done

.PHONY: realclean
realclean: clean
	rm -f bin/WaldmeisterII* bin/cce*
	rm -f .architecture
	rm -f TAGS
