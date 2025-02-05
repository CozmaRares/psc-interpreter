STDVER = c++17
CXX = g++
CXXFLAGS = \
	-Wall -Wextra -Wcast-qual -Wshadow -Wconversion -Wpedantic -Werror \
	-fdiagnostics-color=always -fno-exceptions \
	-O2 -std=$(STDVER)

SRCDIR = src
SRCS = $(shell find $(SRCDIR) -name "*.cpp")

OUTDIR = bin

OBJDIR = $(OUTDIR)/obj
OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
OBJDIRS = $(sort $(dir $(OBJS)))

TARGET = $(OUTDIR)/program

MAKEJOBS = $(shell nproc)

all: build

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTDIR) $(OBJDIRS):
	mkdir -p $@

clean:
	rm $(OUTDIR) -r

lint: $(TARGET)
	cppcheck \
	--std=$(STDVER) --template=gcc -j $(MAKEJOBS) \
	--enable=all --report-progress --verbose --inconclusive --suppress=missingIncludeSystem --suppress=checkersReport \
	$(SRCDIR)

memcheck:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

build:
	make run -j$(MAKEJOBS)

.PHONY: all clean check run build
