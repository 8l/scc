
DIRS = lib cc1

all clean:
	for i in $(DIRS) ;\
	do \
		(cd $$i && $(MAKE) $@) ;\
	done
