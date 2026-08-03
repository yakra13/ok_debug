CXX=cl
LD=link

CFLAGS=/nologo /c /GS- /EHsc /std:c++20 /D_DEBUG /Zi /MTd
LDFLAGS=/DEBUG

COMMON=common
MOCK=$(COMMON)\base\mock.cpp
# MOCKSYS=$(COMMON)\base\mock_syscalls.cpp

DEBUG_OUT=build\debug\x64

all-debug: $(DEBUG_OUT)\example.exe

$(DEBUG_OUT):
	@if not exist "$(DEBUG_OUT)" mkdir "$(DEBUG_OUT)"

$(DEBUG_OUT)\example.exe: $(DEBUG_OUT)
	$(CXX) $(CFLAGS) /I$(COMMON) /Fo$(DEBUG_OUT)\ /Fd$(DEBUG_OUT)\ bofs\_Example\bof.cpp $(MOCK)

	$(LD) /DEBUG /PDB:$(DEBUG_OUT)\example.pdb /OUT:$@ $(DEBUG_OUT)\*.obj
# 	$(CXX) /DEBUG /OUT:$@ $(DEBUG_OUT)\*.obj