## chksmb Makefile ##

SHFILE = chksmb.sh
HDRFILE = chksmb.hdr
OFILE = chksmb

all:
	( cat $(HDRFILE); gzip < $(SHFILE) ) > $(OFILE)
	chmod 700 $(OFILE)

clean:
	rm -vf $(OFILE)
