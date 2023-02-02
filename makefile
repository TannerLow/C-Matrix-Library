python_cmd := python # can be python3 in some cases
output_file := 
delete := 
override CFLAGS := -Wall $(CFLAGS)

test_base_cmd = $(MAKE) -s CFLAGS="$(test_adds)"
base_cmd = gcc $(wildcard *.c) $(CFLAGS) -o $(output_file)
debug_cmd  = $(base_cmd) -DDEBUG
release_cmd = $(base_cmd) -DNDEBUG


ifeq ($(OS), Windows_NT)
	delete := del /f
	output_file := main.exe
else
	delete := rm -f
	output_file := main.out
endif


debug:
	$(python_cmd) line_counter.py
	@echo $(debug_cmd)
	@$(debug_cmd)

release:
	$(python_cmd) line_counter.py
	@echo $(release_cmd)
	@$(release_cmd)

test_debug: 
	@$(test_base_cmd) debug

test_release: 
	@$(test_base_cmd) release

clean:
	$(delete) main.exe