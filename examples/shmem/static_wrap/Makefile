RM = /bin/rm -rf

CC = oshcc
CFLAGS = -Wall

LD = tau_cc.sh -optLinkOnly
LDFLAGS =

OBJ = rotput.o

LIBS =

ROTPUT = rotput

##############################################

.PHONY: all clean profile trace

all: $(ROTPUT)

profile: $(ROTPUT)
	oshrun -np 4 ./$(ROTPUT)
	pprof

trace: $(ROTPUT)
	TAU_TRACE=1 oshrun -np 4 ./$(ROTPUT)
	tau_treemerge.pl
	tau2slog2 tau.trc tau.edf -o tau.slog2
	rm *.trc *.edf

$(ROTPUT): $(OBJ)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ) $(ROTPUT)
	$(RM) *.trc *.edf profile* MULT* *.slog2
