
all: profile

profile: stdafx.cpp stdafx.h testTau.cpp
	cl -GX -DPROFILING_ON -o profile stdafx.cpp testTau.cpp -I..\..\..\..\..\include /link ..\..\..\..\..\win32\lib\tau-profile-static-mt.lib

clean: 
	del *.exe *.ncb *.obj *.bak profile.* *.trc *.edf *.gz *.vpt *.slog2
