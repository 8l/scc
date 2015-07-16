
DIRS = lib cc1 cc2

all clean:
	for i in $(DIRS) ;\
	do \
		(cd $$i && $(MAKE) $@) ;\
	done

.POSIX:
