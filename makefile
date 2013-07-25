NAME=fathead~
CSYM=fathead_tilde

current: pd_darwin

DEST = ../darwin_bin
FINAL_DEST = /Applications/Pd-0.40-2.app/Contents/Resources/extra


# ----------------------- Mac OSX -----------------------

pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(LINUXINCLUDE)  -o $*.o -c $*.c 
	cc -bundle -undefined suppress  -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o ../$*.pd_darwin
	cp $*.pd_darwin $(DEST)
# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_* so_locations
install: 
	cp $(NAME).pd_darwin $(DEST)	
	cp $(NAME).pd_darwin $(FINAL_DEST)
